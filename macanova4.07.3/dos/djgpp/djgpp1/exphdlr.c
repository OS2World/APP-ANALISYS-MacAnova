/* This is file EXPHDLR.C */
/*
** Copyright (C) 1993 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
**
** This file is distributed under the terms listed in the document
** "copying.dj", available from DJ Delorie at the address above.
** A copy of "copying.dj" should accompany this file; if not, a copy
** should be available from where this file was obtained.  This file
** may not be distributed without a verbatim copy of "copying.dj".
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/* Modified for VCPI Implement by Y.Shibata Aug 5th 1991 */
/* Modified for DPMI Implement by H.Tsubakimoto */
/* Merged DPMI with V1.09+ C. Sandmann sandmann@clio.rice.edu */
/* History:66,55 */

/* Modified by G. W. Oehlert for use in MacAnova 9-9-94 */

#include <process.h>
#include <stdio.h>
#include <dos.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dir.h>
#include <ctype.h>
#include <io.h>

#include "gotypes.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "utils.h"
#include "paging.h"
#include "npx.h"
#include "mono.h"
#include "vcpi.h"
#include "graphics.h"
#include "dpmi.h"
#include "extdebug.h"
#include "vcpi.h"
#include "ustat.h"
#include "dpmisim.h"
#include "dalloc.h"
#include "valloc.h"
#include "control.h"

extern void do_faulting_finish_message(int);

void segfault(word32 v)
{
  if (!using_external_debugger || (tss_ptr == &ed_tss))
  {
    fprintf(stderr, "Segmentation violation in pointer 0x%08lx at %x:%lx\n", v-ARENA, tss_ptr->tss_cs, tss_ptr->tss_eip);
    do_faulting_finish_message(0);
  }
  tss_ptr->tss_irqn = 14; /* make it a segfault */
  tss_ptr->tss_cr2 = v;
}

#define CHECK_SEGFAULT(p) { \
  if (!page_is_valid(p)) \
  { \
    segfault(p); \
    return 1; \
  } \
}

extern int debug_mode;

extern unsigned int cdecl _openfd[];
extern word32 far *graphics_pt;
extern int ctrl_break_hit;

extern int was_user_int;
extern word16 vcpi_installed;           /* VCPI Installed flag */
word16 new_pic;                         /* current IRQ0 Vector */
char transfer_buffer[4096];             /* must be near ptr for small model */
word32 transfer_linear;

int in_hardware_interrupt = 0;

word8 old_master_lo=0x08;
word8 hard_master_lo=0x08, hard_master_hi=0x0f;
word8 hard_slave_lo=0x70,  hard_slave_hi=0x77;

word32 user_dta;
static struct REGPACK r;
static int in_graphics_mode=0;
int ctrl_c_causes_break=0; /* GWO */

static int i_10(void), i_21(void), i_31(void), i_33(void), generic_handler(void), i_21_44(void);
static int turbo_assist(void);

static word32 flmerge(word32 rf, word32 tf)
{
  return (rf & 0xcff) | (tf & 0xfffff300L);
}

void static set_controller(int v)
{
/*  disable();  */
  outportb(0x20, 0x11);
  outportb(0x21, v);
  outportb(0x21, 4);
  outportb(0x21, 1);
/*  enable();  */
}

static char cntrls_initted = 0;
extern char vector_78h, vector_79h;

int find_empty_pic(void)
{
  static word8 try[] = { 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xf8, 0x68, 0x78 };
  int i, j;
  for (i=0; i<sizeof(try); i++)
  {
    char far * far * vec = (char far * far *)(0L+try[i]*4L);
    for (j=1; j<8; j++)
    {
      if (vec[j] != vec[0])
	goto not_empty;
    }
/*    printf("Empty = %d\n", try[i]); */
    return try[i];
    not_empty:;
  }
  return 0x78;
}

extern int  far _ev_kbinter;            /* keyboard interrupt flag */
extern void interrupt (* far _ev_oldkbint)(void);
extern void interrupt _ev_keybdint(void);

word32 saved_interrupt_table[256];

void init_controllers(void)
{
  if(cntrls_initted) return;
  cntrls_initted = 1;

  movedata(0, 0, _DS, FP_OFF(saved_interrupt_table), 256*4);

  disable();

  if (vcpi_installed)
  {
    old_master_lo = vcpi_get_pic();
    hard_slave_lo = vcpi_get_secpic();
/*    printf("VCPI pics were m=%d s=%d\n", old_master_lo, hard_slave_lo); */
    hard_slave_hi = hard_slave_lo + 7;
  }
  else if(!use_DPMI)
  {
    old_master_lo = 0x08;
    hard_slave_lo = 0x70;
    hard_slave_hi = 0x77;
  }

  _ev_kbinter = 1;
  _ev_oldkbint = getvect(8+1);     /* this is *NOT* old_master_lo - required */
  setvect(8+1,_ev_keybdint);       /* so DV/X will work (it redirects) */

  if(use_DPMI)
  {
    enable();
    return;
  }

  if (old_master_lo == 0x08)
  {
    hard_master_lo = find_empty_pic();
    if (vcpi_installed)
      vcpi_set_pics(hard_master_lo, hard_slave_lo);
    set_controller(hard_master_lo);
    movedata(0, 0x08*4, 0, hard_master_lo*4, 0x08*4);
  }
  else
  {
    hard_master_lo = old_master_lo;
  }
  hard_master_hi = hard_master_lo + 7;

  enable();
  vector_78h = hard_master_lo;
  vector_79h = hard_master_lo + 1;
}

void uninit_controllers(void)
{
  if(!cntrls_initted) return;
  cntrls_initted = 0;
  disable();

  movedata(_DS, FP_OFF(saved_interrupt_table), 0, 0, 256*4);

  if (old_master_lo == 0x08 && !use_DPMI)
  {
    if (vcpi_installed)
      vcpi_set_pics(0x08, hard_slave_lo);
    set_controller(0x08);
  }
  setvect(8+1,_ev_oldkbint);  /* not old_master_lo for XDV */
  enable();
}

static DPMIaddress exceptions[17];
static DPMIaddress interrupt10;
static DPMIaddress interrupt21;
static DPMIaddress interrupt33;
static DPMIaddress interrupt75;

void setDPMIvector(void)
{
  int i;
  for (i = 0; i < sizeof(exceptions) / sizeof(*exceptions); ++i) {
    DPMIehandler(i, &exceptions[i]);
    DPMIchangeException(i, &exceptions[i]);
  }
  DPMIhandler(0x10, &interrupt10);
  DPMIchangeInterrupt(0x10, &interrupt10);
  DPMIhandler(0x21, &interrupt21);
  DPMIchangeInterrupt(0x21, &interrupt21);
  DPMIhandler(0x33, &interrupt33);
  DPMIchangeInterrupt(0x33, &interrupt33);
  DPMIhandlerNPX(&interrupt75);
  DPMIchangeInterrupt(0x75, &interrupt75);
}

void restoreDPMIvector(void)
{
  int i;
  for (i = 0; i < sizeof(exceptions) / sizeof(*exceptions); ++i)
    DPMIchangeException(i, &exceptions[i]);
  DPMIchangeInterrupt(0x10, &interrupt10);
  DPMIchangeInterrupt(0x21, &interrupt21);
  DPMIchangeInterrupt(0x33, &interrupt33);
  DPMIchangeInterrupt(0x75, &interrupt75);
}

void tss2reg(struct REGPACK *r)
{
  r->r_ax = (word16)(tss_ptr->tss_eax);
  r->r_bx = (word16)(tss_ptr->tss_ebx);
  r->r_cx = (word16)(tss_ptr->tss_ecx);
  r->r_dx = (word16)(tss_ptr->tss_edx);
  r->r_si = (word16)(tss_ptr->tss_esi);
  r->r_di = (word16)(tss_ptr->tss_edi);
  r->r_flags = (word16)(tss_ptr->tss_eflags);
  r->r_ds = r->r_es = _DS;
}

void reg2tss(struct REGPACK *r)
{
  tss_ptr->tss_eax = r->r_ax;
  tss_ptr->tss_ebx = r->r_bx;
  tss_ptr->tss_ecx = r->r_cx;
  tss_ptr->tss_edx = r->r_dx;
  tss_ptr->tss_esi = r->r_si;
  tss_ptr->tss_edi = r->r_di;
  tss_ptr->tss_eflags = flmerge(r->r_flags, tss_ptr->tss_eflags);
}

extern int ctrl_c_flag;

int double_fault(void)
{
  fprintf(stderr, "double fault!\n");
  exit(1);
  return 1;
}

int check_nonpresent_387(void )
{
  if (!using_external_debugger)
  {
    fprintf(stderr, "Fatal!  Application attempted to use not-present 80387!\n");
    fprintf(stderr, "Floating point opcode at virtual address 0x%08lx\n", tss_ptr->tss_eip);
  }
  return 1;
}

int unsupported_int()
{
  if (!debug_mode)
    fprintf(stderr, "Unsupported INT 0x%02x\n", tss_ptr->tss_irqn);
  return 1;
}

#define U unsupported_int
typedef int (*FUNC)(void);
static FUNC exception_handler_list[] = {
  U, U, U, U, U, U, U,
  /* 07 */ check_nonpresent_387,
  /* 08 */ double_fault,
  U, U, U, U, U,
  /* 0e */ page_in,
  U,
  /* 10 */ i_10,                /* Video Interrupt */
  /* 11 */ generic_handler,     /* Equipment detection */
  /* 12 */ generic_handler,     /* Get memory size (outdated) */
  U,
  /* 14 */ generic_handler,     /* Serial communication */
  /* 15 */ generic_handler,     /* Lots of strange things */
  /* 16 */ generic_handler,     /* Keyboard */
  /* 17 */ generic_handler,     /* Parallel printer */
  U, U,
  /* 1a */ generic_handler,     /* Get/set system time */
  U, U, U, U, U, U,
  /* 21 */ i_21,                /* DOS Services */
  U, U, U, U, U, U, U, U, U, U, U, U, U, U, U,
  /* 31 */ i_31,                /* DPMIsim */
  U,
  /* 33 */ i_33                 /* Mouse */
};
#undef U
#define NUM_EXCEPTIONS  (sizeof(exception_handler_list)/sizeof(exception_handler_list[0]))

exception_handler(void)
{
  int i;
  if (topline_info)
  {
    char buf[20];
    if (tss_ptr->tss_irqn == 14)
      sprintf(buf, "0x%08lx", tss_ptr->tss_cr2 - ARENA);
    else
      sprintf(buf, "0x%08lx", tss_ptr->tss_eip);
    for (i=0; buf[i]; i++)
      poke(screen_seg, i*2+80, buf[i] | 0x0600);
  }
  i = tss_ptr->tss_irqn;
/*  printf("i=%#02x\n", i); */
  if (((i >= hard_slave_lo)  && (i <= hard_slave_hi)
       && (i != hard_slave_lo + 5))
      || ((i >= hard_master_lo) && (i <= hard_master_hi)))
  {
    in_hardware_interrupt = 1;
    intr(i, &r);
    in_hardware_interrupt = 0;
    if (ctrl_break_hit)
      return 1;
    if (i == hard_master_lo + 1)
    {
      if (ctrl_c_causes_break)
      {
	r.r_ax = 0x0100;
	intr(0x16, &r);
	if (!(r.r_flags & 0x40) && (r.r_ax == 0x2e03))
	{
	  _AH = 0;
	  geninterrupt(0x16);
	  ctrl_c_flag = 1;  
	}
      }
    }
    if (ctrl_c_flag)
    {
      ctrl_c_flag = 0;
      if (ctrl_c_causes_break)
	return 1;
    }
    return 0;
  }
  if (i < NUM_EXCEPTIONS)
    return (exception_handler_list[i])();
  else
    return 1;
}

void retrieve_string(word32 v, char *transfer_buffer, char tchar)
{
  int i;
  if (!use_DPMI) {
#if 0
    for (i=0; i<4096; i++)
    {
      c = peek8(v);
      v++;
      transfer_buffer[i] = c;
      if (c == tchar)
	break;
    }
#else
    memscan32(v, transfer_buffer, tchar);
#endif
    return;
  } else {
    i = Pmemscan(tss_ptr->tss_ds, v, tchar, 4096);
    if (i == 0) i = 4096;
    Pmemget(tss_ptr->tss_ds, v, transfer_buffer, i);
    return;
  }
}

generic_handler(void)
{
  tss2reg(&r);
  intr(tss_ptr->tss_irqn, &r);
  reg2tss(&r);
  return 0;
}

i_10(void)
{
  word32 v;
  word16 i, j;

/* CB changes */
/* OLD:
 * switch((word16)(tss_ptr->tss_eax) & 0xFF00) {
 *   case 0xFD00:
 *     graphics_pageflip();
 *     return 0;
 *   case 0xFE00:
 *     graphics_inquiry();
 *     return 0;
 *   case 0xFF00:
 *     graphics_mode((word16)(tss_ptr->tss_eax) & 0xff);
 *     in_graphics_mode = (peekb(0x40, 0x49) > 7);
 *     return 0;
 * }
 */
  if(((word16)tss_ptr->tss_eax & 0xff00) >= gr_assist_func_start) {
    graphics_assist();
    in_graphics_mode = (peekb(0x40, 0x49) > 7);
    return 0;
  }
/* end CB changes */
  tss2reg(&r);

  i = (word16)tss_ptr->tss_eax; /* int10 function 0x11 subfunctions 0 & 0x10 */
  if(i==0x1100 || i==0x1110)
  { /* user-defined text characters */
    v = tss_ptr->tss_edx + ARENA; /* bh*cx bytes starting at (ds:dx) */
    CHECK_SEGFAULT(v);
    j = ((tss_ptr->tss_ebx >> 8) & 0xff) * ((word16)tss_ptr->tss_ecx);
    memget(v, transfer_buffer, j);
    r.r_dx = FP_OFF(transfer_buffer);
    r.r_ds = _DS;
  }

  intr(0x10, &r);
  reg2tss(&r);
  tss_ptr->tss_ebp = r.r_es * 16L + r.r_bp + 0xe0000000L;
  return 0;
}

#include "eventque.h"

#define  MSDRAW_STACK  128              /* stack size for mouse callback */

static word32  mousedraw_func32;        /* 32-bit mouse cursor drawing function */
static word32  mousedraw_contaddr;      /* jump to this address after mouse draw */
static char    mousedraw_active;        /* set while drawing mouse cursor */
EventQueue    *event_queue = NULL;      /* event queue */
typedef int far (*FFUNC)(void);
static FFUNC   mousedraw_callback = 0;  /* DPMI real mode callback to prot */

static void mousedraw_hook(void)
{
  disable();
  if(!mousedraw_active)
  {
    mousedraw_active = 1;
    if(use_DPMI)
    {
      mousedraw_callback();
      mousedraw_active = 0;
    }
    else
    {
      mousedraw_contaddr = a_tss.tss_eip;
      a_tss.tss_eip = mousedraw_func32;
    }
  }
  enable();
}

i_33(void)
{
  void (*msdrawfun)(void);
  int  queuesize;

  if(tss_ptr->tss_eax == 0x00ff) {
    if(event_queue != NULL) {
      EventQueueDeInit();
      event_queue = NULL;
    }
    if((queuesize = (int)tss_ptr->tss_ebx) > 0) {
      mousedraw_func32 = tss_ptr->tss_ecx;
      mousedraw_active = 0;
      msdrawfun = (mousedraw_func32 != 0L) ? mousedraw_hook : NULL;
      if(use_DPMI) {
	if(tss_ptr->tss_edx != 0x12345678L) return(0);  /* make sure V1.03 or more */
	mousedraw_callback = (FFUNC)mousedraw_func32;
      }
      event_queue = EventQueueInit(queuesize, MSDRAW_STACK, msdrawfun);
      if(event_queue != NULL) {
	tss_ptr->tss_ebx =
	  (((word32)FP_SEG(event_queue)) << 4) +
	  ((word32)FP_OFF(event_queue)) +
	  0xe0000000L;
	tss_ptr->tss_ecx =
	  (((word32)FP_SEG(&mousedraw_contaddr)) << 4) +
	  ((word32)FP_OFF(&mousedraw_contaddr)) +
	  0xe0000000L;
	tss_ptr->tss_edx =
	  (((word32)FP_SEG(&mousedraw_active)) << 4) +
	  ((word32)FP_OFF(&mousedraw_active)) +
	  0xe0000000L;
      }
      else tss_ptr->tss_ebx = 0L;
    }
    tss_ptr->tss_eax = 0x0ff0;              /* acknowledge event handling */
    return(0);
  }
  if (*((unsigned far *)0x000000CEL) == 0)
    return 0;
  r.r_ax = (word16)(tss_ptr->tss_eax);
  r.r_bx = (word16)(tss_ptr->tss_ebx);
  r.r_cx = (word16)(tss_ptr->tss_ecx);
  r.r_dx = (word16)(tss_ptr->tss_edx);
  intr(0x33, &r);
  tss_ptr->tss_eax = r.r_ax;
  tss_ptr->tss_ebx = r.r_bx;
  tss_ptr->tss_ecx = r.r_cx;
  tss_ptr->tss_edx = r.r_dx;
  return 0;
}

/*1.07 TSS last_tss; */

i_21(void)
{
  word32 v, trans_total, countleft;
  int i, c, ah, tchar, trans_count;
  char *cp;
  tss2reg(&r);
  ah = ((word16)(tss_ptr->tss_eax) >> 8) & 0xff;

  if (ah & 0x80) switch (ah)
  {
    case 0xfe:
      return external_debugger_handler();
    case 0xff:
      return turbo_assist();
    default:
      return 1;
  }
  else switch (ah)
  {
    case 1: /* read with echo */
    case 2: /* con output */
    case 3: /* aux input */
    case 4: /* aux output */
    case 5: /* prn output */
    case 6: /* direct con i/o */
    case 7: /* direct con input */
    case 8: /* kbd read */
    case 0x0b: /* kbd status */
    case 0x0d: /* reset disk */
    case 0x0e: /* select disk */
    case 0x18: /* return al=0 */
    case 0x19: /* get disk */
    case 0x1d: /* return al=0 */
    case 0x1e: /* return al=0 */
    case 0x20: /* return al=0 */
    case 0x2a: /* get date */
    case 0x2b: /* set date */
    case 0x2c: /* get time */
    case 0x2d: /* set time */
    case 0x2e: /* (re)set verify flag */
    case 0x30: /* get version */
    case 0x36: /* get disk free space */
    case 0x37: /* get/set switch char */
    case 0x42: /* seek */
    case 0x4d: /* get return code */
    case 0x54: /* get verify flag */
    case 0x57: /* get/set file time stamp */
    case 0x58: /* get/set UMB link state */
    case 0x66: /* get/set global code page */
    case 0x67: /* set handle count */
    case 0x68: /* commit (flush and update directory) */
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 0x38: /* get country info */
      r.r_ds = _DS;
      r.r_dx = FP_OFF(transfer_buffer);
      intr(0x21, &r);
      if (r.r_flags & 1) 
      {
	tss_ptr->tss_eflags |= 1;
	tss_ptr->tss_eax = r.r_ax;
	return 0;
      }
      memput(tss_ptr->tss_edx + ARENA, transfer_buffer, 34);
      return 0;
    case 0x33: /* ^C checking and more */
      switch (r.r_ax & 0xff) 
      {
      case 0x01: /* set ^C */
      case 0x02: /* set extended ^C */
	ctrl_c_causes_break = r.r_dx & 0xff; 
	/* fall through */
      case 0x00: /* get ^C */
      case 0x05: /* get boot drive */
      case 0x06: /* get true dos version */
	intr(0x21, &r);
	reg2tss(&r);
	return 0;
      default:
	return 1;
      }
    case 0x3e: /* close */
      if (using_external_debugger && r.r_bx < 2)
	return 0;
      if (r.r_bx == 1)
	redir_1_mono = redir_1_2 = 0;
      if (r.r_bx == 2)
	redir_2_mono = redir_2_1 = 0;
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 9: /* print string */
    case 0x39: /* mkdir */
    case 0x3a: /* rmdir */
    case 0x3b: /* chdir */
    case 0x41: /* unlink (delete) */
    case 0x43: /* chmod */
      if (ah == 9)
	tchar = '$';
      else
	tchar = 0;
      v = tss_ptr->tss_edx + ARENA;
      CHECK_SEGFAULT(v);
      retrieve_string(v, transfer_buffer, tchar);
      r.r_dx = FP_OFF(transfer_buffer);
      r.r_ds = _DS;
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 0x3c: /* creat (rewrite) */
      v = tss_ptr->tss_edx + ARENA;
      CHECK_SEGFAULT(v);
      retrieve_string(v, transfer_buffer, 0);
      i = _creat(transfer_buffer, (int)(tss_ptr->tss_ecx));
      if (i < 0)
      {
	tss_ptr->tss_eax = errno;
	tss_ptr->tss_eflags |= 1;
      }
      else
      {
	tss_ptr->tss_eax = i;
	tss_ptr->tss_eflags &= ~1;
      }
      return 0;
    case 0x3d: /* open */
      v = tss_ptr->tss_edx + ARENA;
      CHECK_SEGFAULT(v)
      retrieve_string(v, transfer_buffer, 0);
      i = (word16)(tss_ptr->tss_eax) & 0xf0;
      if (tss_ptr->tss_eax & O_WRONLY) i &= 1;
      if (tss_ptr->tss_eax & O_RDWR) i &= 2;
      i = _open(transfer_buffer, i);
      if (i < 0)
      {
	tss_ptr->tss_eax = errno;
	tss_ptr->tss_eflags |= 1;
      }
      else
      {
	tss_ptr->tss_eax = i;
	tss_ptr->tss_eflags &= ~1;
      }
      return 0;
    case 0x1a: /* set dta */
      user_dta = tss_ptr->tss_edx;
      setdta((char far *)transfer_buffer);
      return 0;
    case 0x2f: /* get dta */
      tss_ptr->tss_ebx = user_dta;
      return 0;
    case 0x56: /* rename/move */
      v = tss_ptr->tss_edx + ARENA;
      CHECK_SEGFAULT(v)
      retrieve_string(v, transfer_buffer, 0);
      i = strlen(transfer_buffer) + 1;
      r.r_dx = FP_OFF(transfer_buffer);
      r.r_ds = _DS;
      v = tss_ptr->tss_edi + ARENA;
      retrieve_string(v, transfer_buffer+i, 0);
      r.r_di = FP_OFF(transfer_buffer)+i;
      r.r_es = _DS;
      intr(0x21, &r);
      tss_ptr->tss_eax = r.r_ax;
      tss_ptr->tss_eflags = flmerge(r.r_flags, tss_ptr->tss_eflags);
      return 0;
    case 0x3f: /* read */
      if (!tss_ptr->tss_edx) {
	fprintf(stderr, "This image has a buggy read.s module.  Run DPMIFIX on it and try again.\n");
	return 1;
      }
      if (tss_ptr->tss_edx == transfer_linear)
      {
	i = read(r.r_bx, transfer_buffer, r.r_cx);
	if (i<0)
	{
	  tss_ptr->tss_eflags |= 1; /* carry */
	  tss_ptr->tss_eax = _doserrno;
	}
	else
	{
	  tss_ptr->tss_eflags &= ~1;
	  tss_ptr->tss_eax = i;
	}
	return 0;
      }
      trans_total = 0;
      countleft = tss_ptr->tss_ecx;
      v = tss_ptr->tss_edx;
      CHECK_SEGFAULT(v+ARENA)
      while (countleft > 0)
      {
	trans_count = (word16)((countleft <= 4096) ? countleft : 4096);
	i = read(r.r_bx, transfer_buffer, trans_count);
	if (i < 0)
	{
	  tss_ptr->tss_eflags |= 1; /* carry */
	  tss_ptr->tss_eax = _doserrno;
	  return 0;
	}
	memput(v+ARENA, transfer_buffer, i);
	trans_total += i;
	v += i;
	countleft -= i;
	if (isatty(r.r_bx) && (i<trans_count))
	  break; /* they're line buffered */
	if (i == 0)
	  break;
      }
      tss_ptr->tss_eax = trans_total;
      tss_ptr->tss_eflags &= ~1;
      return 0;
    case 0x40: /* write */
      if (tss_ptr->tss_edx == transfer_linear)
      {
	if ((r.r_bx == 1) && redir_1_mono)
	  i = mono_write(transfer_buffer, r.r_cx);
	else if ((r.r_bx == 2) && redir_2_mono)
	  i = mono_write(transfer_buffer, r.r_cx);
	else
	{
	  int fd = r.r_bx;
	  if ((r.r_bx == 2) && redir_2_1)
	    fd = 1;
	  else if ((r.r_bx == 1) && redir_1_2)
	    fd = 2;
	  if (r.r_cx == 0) /* for ftruncate */
	    i = _write(fd, transfer_buffer, r.r_cx);
	  else
	    i = write(fd, transfer_buffer, r.r_cx);
	}
	if (i<0)
	{
	  tss_ptr->tss_eflags |= 1; /* carry */
	  tss_ptr->tss_eax = _doserrno;
	}
	else
	{
	  tss_ptr->tss_eflags &= ~1;
	  tss_ptr->tss_eax = i;
	}
	return 0;
      }
      trans_total = 0;
      countleft = tss_ptr->tss_ecx;
      if (countleft == 0)
      {
	r.r_ax = 0x4000;
	r.r_cx = 0;
	intr(0x21,&r);
	tss_ptr->tss_eax = 0;
	tss_ptr->tss_eflags &= ~1;
	return 0;
      }
      v = tss_ptr->tss_edx;
      CHECK_SEGFAULT(v+ARENA);
      r.r_dx = (int)transfer_buffer;
      while (countleft > 0)
      {
	trans_count = (word16)((countleft <= 4096) ? countleft : 4096);
	memget(v+ARENA, transfer_buffer, trans_count);
	if ((r.r_bx == 1) && redir_1_mono)
	  i = mono_write(transfer_buffer, trans_count);
	else if ((r.r_bx == 2) && redir_2_mono)
	  i = mono_write(transfer_buffer, trans_count);
	else
	{
	  int fd = r.r_bx;
	  if ((r.r_bx == 2) && redir_2_1)
	    fd = 1;
	  else if ((r.r_bx == 1) && redir_1_2)
	    fd = 2;
	  i = write(fd, transfer_buffer, trans_count);
	  if (in_graphics_mode && (fd < 3))
	  {
	    word32 far *p = graphics_pt;
	    for (c = 0; c < 256; c++)
	      *p++ &= ~PT_P;
	  }
	}
	if (i<0) /* carry */
	{
	  tss_ptr->tss_eflags |= 1; /* carry */
	  tss_ptr->tss_eax = _doserrno;
	  return 0;
	}
	trans_total += i;
	v += i;
	countleft -= i;
	if (i < trans_count)
	  break;
      }
      tss_ptr->tss_eax = trans_total;
      tss_ptr->tss_eflags &= ~1;
      return 0;
    case 0x44: /* ioctl */
      return i_21_44();
    case 0x45: /* dup */
      i = r.r_bx;
      intr(0x21, &r);
      if (!(r.r_flags & 1))
	_openfd[r.r_ax] = i;
      reg2tss(&r);
      return 0;
    case 0x46: /* dup2 */
      i = r.r_bx;
      c = r.r_cx;
      intr(0x21, &r);
      if (!(r.r_flags & 1))
	_openfd[c] = i;
      reg2tss(&r);
      return 0;
    case 0x4e: /* find first */
      CHECK_SEGFAULT(user_dta+ARENA);
      v = tss_ptr->tss_edx + ARENA;
      CHECK_SEGFAULT(v);
      retrieve_string(v, transfer_buffer+43, 0);
      r.r_dx = FP_OFF(transfer_buffer+43);
      r.r_ds = _DS;
      intr(0x21, &r);
      reg2tss(&r);
      for (i=20; i>=0; i--)
	transfer_buffer[i+28] = transfer_buffer[i+26];
      transfer_buffer[32+13] = 0; /* asciiz termination */
      memput(user_dta+ARENA, transfer_buffer, 48);
      return 0;
    case 0x4f: /* find next */
      CHECK_SEGFAULT(user_dta+ARENA);
      memget(user_dta+ARENA, transfer_buffer, 48);
      for (i=0; i<=20; i++)
	transfer_buffer[i+26] = transfer_buffer[i+28];
      intr(0x21, &r);
      reg2tss(&r);
      for (i=20; i>=0; i--)
	transfer_buffer[i+28] = transfer_buffer[i+26];
      transfer_buffer[32+13] = 0; /* asciiz termination */
      memput(user_dta+ARENA, transfer_buffer, 48);
      return 0;
    case 0x47: /* getwd */
      getcurdir((int)(tss_ptr->tss_edx & 0xff), transfer_buffer);
      for (cp=transfer_buffer; *cp; cp++)
      {
	if (*cp == '\\') *cp = '/';
	*cp = tolower(*cp);
      }
      memput(tss_ptr->tss_esi+ARENA, transfer_buffer, strlen(transfer_buffer)+1);
      tss_ptr->tss_eax = (unsigned)r.r_ax;
      tss_ptr->tss_eflags &= ~1;
      return 0;
    case 0x4a: /* sbrk/brk -- NOT ORGINAL MEANING */
      if (tss_ptr->tss_eax & 0xff)
	tss_ptr->tss_eax = paging_sbrk(tss_ptr->tss_ebx);
      else
	tss_ptr->tss_eax = paging_brk(tss_ptr->tss_ebx);
      return 0;
    case 0x4c: /* exit */
      if (using_external_debugger)
	return 1;
      else
	exit((word8)(tss_ptr->tss_eax));
    default:
      return 1;
  }
}

static int reg2gate(word32 r)
{
  int g = (int)(r & 0xff);
  if (g >= 0x08 && g <= 0x0f)
    g = g - 0x08 + hard_master_lo;
  else if (g >= 0x70 && g <= 0x77)
    g = g - 0x70 + hard_slave_lo;
  return g;
}

static int dpmisim_is_exec = 0;

int i_31(void)
{
  int gate, i;
  word16 far *fptr;
  word16 dpmisim_spare_stack[128];
  union REGS r;
  struct SREGS s;
  switch ((word16)(tss_ptr->tss_eax))
  {
    case 0x0100:
      r.h.ah = 0x48;
      r.x.bx = (word16)tss_ptr->tss_ebx;
      int86(0x21, &r, &r);
      if ((r.x.flags & 1) && ((word16)tss_ptr->tss_ebx != 0xffff))
      {
	valloc_shrink_rmem(((word16)tss_ptr->tss_ebx)/256 + 1);
	r.h.ah = 0x48;
	r.x.bx = (word16)tss_ptr->tss_ebx;
	int86(0x21, &r, &r);
/*        if (r.x.flags & 1)
	  printf("biggest after shrinking is %04x\n", r.x.bx); */
      }
      tss_ptr->tss_eflags &= ~1;
      tss_ptr->tss_eflags |= r.x.flags & 1;
      tss_ptr->tss_edx =
      tss_ptr->tss_eax = r.x.ax;
      tss_ptr->tss_ebx = r.x.bx;
/*      printf("allocated %x, %d %x\n", r.x.ax, r.x.flags & 1, r.x.bx); */
      return 0;

    case 0x0101:
      r.h.ah = 0x49;
      r.x.bx = (word16)tss_ptr->tss_edx;
      int86(0x21, &r, &r);
/*      printf("released %x, %d %x\n", (word16)tss_ptr->tss_edx, r.x.flags & 1, r.x.ax); */
      return 0;

    case 0x0102:
      r.h.ah = 0x4a;
      r.x.bx = (word16)tss_ptr->tss_ebx;
      s.es = (word16)tss_ptr->tss_edx;
      int86x(0x21, &r, &r, &s);
      tss_ptr->tss_eflags &= ~1;
      tss_ptr->tss_eflags |= r.x.flags & 1;
      tss_ptr->tss_eax = r.x.ax;
      tss_ptr->tss_ebx = r.x.bx;
      return 0;

    case 0x0200:
      gate = (word8)tss_ptr->tss_ebx;
      tss_ptr->tss_ecx = peek(0, gate*4+2);
      tss_ptr->tss_edx = peek(0, gate*4);
      tss_ptr->tss_eflags &= ~1;
      return 0;

    case 0x0201:
      gate = (word8)tss_ptr->tss_ebx;
      disable();
      poke(0, gate*4+2, tss_ptr->tss_ecx);
      poke(0, gate*4, tss_ptr->tss_edx);
      enable();
      tss_ptr->tss_eflags &= ~1;
      return 0;

    case 0x0204:
      gate = reg2gate(tss_ptr->tss_ebx);
      tss_ptr->tss_ecx = idt[gate].selector;
      tss_ptr->tss_edx = idt[gate].offset0 | (idt[gate].offset1 << 16);
      tss_ptr->tss_eflags &= ~1;
      return 0;

    case 0x0205:
      gate = reg2gate(tss_ptr->tss_ebx);
      idt[gate].selector = (word16)(tss_ptr->tss_ecx);
      idt[gate].offset0 = (word16)(tss_ptr->tss_edx);
      idt[gate].offset1 = (word16)(tss_ptr->tss_edx >> 16);
      idt[gate].stype = 0x8e00;
      tss_ptr->tss_eflags &= ~1;
      return 0;

    case 0x0300:
    case 0x0301:
    case 0x0302:
      CHECK_SEGFAULT(tss_ptr->tss_edi + ARENA);
      if (tss_ptr->tss_ecx)
      {
	tss_ptr->tss_eax = 0x8021;
	tss_ptr->tss_eflags |= 1;
	return 0;
      }
      memget(tss_ptr->tss_edi + ARENA, dpmisim_regs, 50);

      if (dpmisim_regs[24] == 0)
      {
	dpmisim_regs[24] = _SS;
	dpmisim_regs[23] = (word16)(dpmisim_spare_stack) + sizeof(dpmisim_spare_stack);
      }
      if ((word16)tss_ptr->tss_eax != 0x0301)
      {
	dpmisim_regs[23] -= 2;  /* fake pushing flags on stack */
	fptr = MK_FP(dpmisim_regs[24], dpmisim_regs[23]);
	*fptr = dpmisim_regs[16];
      }

      if ((word16)tss_ptr->tss_eax == 0x0300)
      {
	dpmisim_regs[21] = peek(0, (word8)tss_ptr->tss_ebx * 4);
	dpmisim_regs[22] = peek(0, (word8)tss_ptr->tss_ebx * 4 + 2);
      }

      if (dpmisim_is_exec)
      {
	word32 our_interrupt_table[256];
	page_out_everything();
	movedata(0, 0, FP_SEG(our_interrupt_table), FP_OFF(our_interrupt_table), 256*4);
	uninit_controllers();
	dpmisim();
	init_controllers();
	movedata(FP_SEG(our_interrupt_table), FP_OFF(our_interrupt_table), 0, 0, 256*4);
       page_in_everything();
      }
      else
	dpmisim();

      memput(tss_ptr->tss_edi + ARENA, dpmisim_regs, 50);
      tss_ptr->tss_eflags &= ~1;
      return 0;

    case 0x0303:
      CHECK_SEGFAULT(tss_ptr->tss_edi + ARENA);
      for (i=0; i<16; i++)
	if (dpmisim_rmcb[i].cb_address == 0)
	  break;
      if (i == 16)
      {
	tss_ptr->tss_eflags |= 1;
	tss_ptr->tss_eax = 0x8015;
	return 0;
      }
      dpmisim_rmcb[i].cb_address = tss_ptr->tss_esi;
      dpmisim_rmcb[i].reg_ptr = tss_ptr->tss_edi;
      tss_ptr->tss_eflags &= ~1;
      tss_ptr->tss_ecx = _CS;
      tss_ptr->tss_edx = (word16)dpmisim_rmcb0 + i * ((word16)dpmisim_rmcb1 - (word16)dpmisim_rmcb0);
      return 0;

    case 0x0304:
      if ((word16)tss_ptr->tss_ecx == _CS)
	for (i=0; i<16; i++)
	  if ((word16)tss_ptr->tss_edx == (word16)dpmisim_rmcb0 + i * ((word16)dpmisim_rmcb1 - (word16)dpmisim_rmcb0))
	  {
	    dpmisim_rmcb[i].cb_address = 0;
	    tss_ptr->tss_eflags &= ~1;
	    return 0;
	  }
      tss_ptr->tss_eflags |= 1;
      tss_ptr->tss_eax = 0x8024;
      return 0;
      
    case 0x0500:
      CHECK_SEGFAULT(tss_ptr->tss_edi + ARENA);
      memset(transfer_buffer, 0xff, 48);
      ((word32 *)transfer_buffer)[8] = dalloc_max_size();
      ((word32 *)transfer_buffer)[4] =
      ((word32 *)transfer_buffer)[6] = valloc_max_size();
      ((word32 *)transfer_buffer)[2] =
      ((word32 *)transfer_buffer)[5] = valloc_max_size() - valloc_used();
      ((word32 *)transfer_buffer)[1] = 
	((word32 *)transfer_buffer)[5] + ((word32 *)transfer_buffer)[8] - dalloc_used();
      ((word32 *)transfer_buffer)[0] = ((word32 *)transfer_buffer)[1] * 4096L;
      memput(tss_ptr->tss_edi + ARENA, transfer_buffer, 48);
      tss_ptr->tss_eflags &= ~1;
      return 0;

    default: /* mark as unsupported */
      tss_ptr->tss_eflags |= 1;
      tss_ptr->tss_eax = 0x8001;
      return 0;
  }
}

struct time32 {
  word32 secs;
  word32 usecs;
};

struct tz32 {
  word32 offset;
  word32 dst;
};

struct  stat32 {
	short st_dev;
	short st_ino;
	short st_mode;
	short st_nlink;
	short st_uid;
	short st_gid;
	short st_rdev;
	short st_align_for_word32;
	long  st_size;
	long  st_atime;
	long  st_mtime;
	long  st_ctime;
	long  st_blksize;
};

static int dev_count=1;

turbo_assist(void)
{
  word32 p1, p2, r;
  struct time32 time32;
  struct tz32 tz32;
  struct stat32 statbuf32;
  struct stat statbuf;
  int i;
  char buf[128], *bp;
  word32 our_interrupt_table[256];

  p1 = tss_ptr->tss_ebx;
  p2 = tss_ptr->tss_ecx;
  switch ((word8)(tss_ptr->tss_eax))
  {
    case 1: /* creat */
      retrieve_string(p1+ARENA, buf, 0);
      r = creat(buf, S_IREAD | S_IWRITE);
      break;

    case 2: /* open */
      retrieve_string(p1+ARENA, buf, 0);
      r = open(buf, (int)p2, S_IREAD | S_IWRITE);
      break;

    case 3: /* fstat */
      memset(&statbuf, 0, sizeof(statbuf));
      r = fstat((int)p1, &statbuf);
      statbuf32.st_dev = dev_count++;
      statbuf32.st_ino = statbuf.st_ino;
      statbuf32.st_mode = statbuf.st_mode;
      statbuf32.st_nlink = statbuf.st_nlink;
      statbuf32.st_uid = 42;
      statbuf32.st_gid = 42;
      statbuf32.st_rdev = statbuf.st_rdev;
      statbuf32.st_size = statbuf.st_size;
      statbuf32.st_atime = statbuf.st_atime;
      statbuf32.st_mtime = statbuf.st_mtime;
      statbuf32.st_ctime = statbuf.st_ctime;
      statbuf32.st_blksize = 512;
      memput(p2+ARENA, &statbuf32, sizeof(statbuf32));
      break;

    case 4: /* gettimeofday */
      if (p2)
      {
	CHECK_SEGFAULT(p2+ARENA);
	tz32.offset = timezone;
	tz32.dst = daylight;
	memput(p2+ARENA, &tz32, sizeof(tz32));
      }
      if (p1)
      {
	int dh;
	CHECK_SEGFAULT(p1+ARENA);
	time((long *)&(time32.secs));
	_AH = 0x2c;
	geninterrupt(0x21);
	dh = _DH;
	time32.usecs = _DL * 10000L;
	if (time32.secs % 60 != dh)
	  time32.secs++;
	memput(p1+ARENA, &time32, sizeof(time32));
      }
      r = 0;
      break;

    case 5: /* settimeofday */
      if (p2)
      {
	CHECK_SEGFAULT(p2+ARENA);
	memget(p2+ARENA, &tz32, sizeof(tz32));
	timezone = tz32.offset;
	daylight = (int)tz32.dst;
      }
      if (p1)
      {
	CHECK_SEGFAULT(p1+ARENA);
	memget(p1+ARENA, &time32, sizeof(time32));
	stime((long *)&(time32.secs));
      }
      r = 0;
      break;

    case 6: /* stat */
      memset(&statbuf, 0, sizeof(statbuf));
      retrieve_string(p1+ARENA, transfer_buffer, 0);
      r = unixlike_stat(transfer_buffer, &statbuf);
      statbuf32.st_dev = dev_count++;
      statbuf32.st_ino = statbuf.st_ino;
      statbuf32.st_mode = statbuf.st_mode;
      statbuf32.st_nlink = statbuf.st_nlink;
      statbuf32.st_uid = statbuf.st_uid;
      statbuf32.st_gid = statbuf.st_gid;
      statbuf32.st_rdev = statbuf.st_rdev;
      statbuf32.st_size = statbuf.st_size;
      statbuf32.st_atime = statbuf.st_atime;
      statbuf32.st_mtime = statbuf.st_mtime;
      statbuf32.st_ctime = statbuf.st_ctime;
      statbuf32.st_blksize = 512;
      memput(p2+ARENA, &statbuf32, sizeof(statbuf32));
      break;

    case 7: /* system */
      retrieve_string(p1+ARENA, transfer_buffer, 0);
      page_out_everything();
      movedata(0, 0, FP_SEG(our_interrupt_table), FP_OFF(our_interrupt_table), 256*4);
      uninit_controllers();
      sscanf(transfer_buffer, "%s%n", buf, &i);
      if (strpbrk(transfer_buffer, "<>|") == NULL)
	r = spawnlp(P_WAIT, buf, buf, transfer_buffer+i, 0);
      else
	r = -1;
      bp = buf+strlen(buf)-4;
      if (stricmp(bp, ".exe") && stricmp(bp, ".com") && (r & 0x80000000L))
	r = system(transfer_buffer);
      init_controllers();
      movedata(FP_SEG(our_interrupt_table), FP_OFF(our_interrupt_table), 0, 0, 256*4);
      page_in_everything();
      break;

    case 8: /* _setmode() */
      _BX=(int)p1;
      _AX=0x4400;
      geninterrupt(0x21);
      i = _DX;
      if (p2 & O_BINARY)
	i |= 0x20;
      else
	i &= ~0x20;
      _BX=(int)p1;
      _DX = i;
      _AX=0x4401;
      geninterrupt(0x21);
      r = setmode((int)p1, (int)p2);
      break;

    case 9: /* chmod */
      retrieve_string(p1+ARENA, buf, 0);
      r = chmod(buf, (int)p2);
      break;

    case 10: /* DPMI exec */
      dpmisim_is_exec = 1;
      tss_ptr->tss_eax = 0x0300;
      i_31();
      dpmisim_is_exec = 0;
      return 0;

    default:
      return 1;
  }
  tss_ptr->tss_eflags &= ~1;
  if (r == -1)
  {
    tss_ptr->tss_eflags |= 1;
    tss_ptr->tss_eax = errno;
    return 0;
  }
  tss_ptr->tss_eax = r;
  return 0;
}

i_21_44(void)
{
  switch ((word8)(tss_ptr->tss_eax))
  {
    case 0x00:
    case 0x01:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0e:
    case 0x0f:
      intr(0x21, &r);
      tss_ptr->tss_edx = r.r_dx;
      tss_ptr->tss_eax = r.r_ax;
      tss_ptr->tss_eflags = flmerge(r.r_flags, tss_ptr->tss_eflags);
      return 0;
    default:
      return 1;
  }
}

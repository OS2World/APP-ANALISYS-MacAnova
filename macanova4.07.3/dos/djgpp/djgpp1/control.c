/* This is file CONTROL.C */
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
/* Merged DPMI and V1.09 C. Sandmann sandmann@clio.rice.edu */

/* Modified for MacAnova by G. W. Oehlert, 9-9-94 */

#include <dos.h>
#include <dir.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

#include "gotypes.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "valloc.h"
#include "utils.h"
#include "graphics.h"
#include "mono.h"
#include "vcpi.h"
#include "paging.h"
#include "dpmi.h"
#include "extdebug.h"
#include "exphdlr.h"
#include "dalloc.h"
#include "vcpi.h"
#include "mswitch.h"
#include "xms.h"
#include "npx.h"
#include "stubinfo.h"
#include "proginfo.h"
#include "control.h"
#include "eventque.h"

/* stamp.c will find this and replace it with the correct timestamp
   but leave the TIMETIME key alone */
static char build_time[] = "TIMETIMExxx xxx xx xx:xx:xx xxxx";

StubInfo stub_info = {
  STUB_INFO_MAGIC,
  sizeof(StubInfo),
  "go32",
  {
    GO32_RELEASE_NUMBER,
    GO32_RELEASE_TYPE_C,
    GO32_MINOR_VERSION,
    GO32_MAJOR_VERSION
  },
  262144L,
  0L,
  "",
  1,
  0L
};

PROGINFO prog_info;

int topline_info = 0;
int show_memory_info = 0;

extern transfer_buffer[];
extern word32 transfer_linear;

extern int ctrl_c_flag, in_hardware_interrupt;

GDT_S gdt[g_num];
IDT idt[256];

TSS *tss_ptr;
int debug_mode = 0;
int self_contained;
long header_offset = 0;
int use_ansi=0;
int use_mono=0;
int redir_1_mono=0;
int redir_2_mono=0;
int redir_2_1=0;
int redir_1_2=0;

static int initial_argc;
static int old_video_mode;
static int globbing=-1;
static int old_mask1, old_mask2;

int16 ems_handle=0;             /*  Get EMS Handle  */
word16 vcpi_installed = 0;      /*  VCPI Installed Flag  */
char use_DPMI = 0;
extern word32 DPMI_STACK;
word32 ARENA = 0x10000000L;

int ctrl_break_hit = 0;

extern word32 far *pd;

extern void far ivec0(void), ivec1(void);
extern void ivec7(void), ivec75(void);
extern void interrupt_common(void), page_fault(void);
extern void v74_handler(void), v78_handler(void), v79_handler(void);

void interrupt (*old_ctrlbrk)();

void set_command_line(char **argv, char **envv);

int exitDPMI = 0;

char *exception_names[] = {
  "Division by Zero",
  "Debug",
  0,
  "Breakpoint",
  "Overflow",
  "Bounds Check",
  "Invalid Opcode",
  "Coprocessor not available",
  "Double Fault",
  "Coprocessor overrun",
  "Invalid TSS",
  "Segment Not Present",
  "Stack Fault",
  "General Protection Fault",
  "Page fault",
  0,
  "Coprocessor Error",
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "Unsupported DOS request"
};
#define EXCEPTION_COUNT (sizeof(exception_names)/sizeof(exception_names[0]))

unsigned int cdecl _openfd[255] = {
  0x6001,0x7002,0x6002,0xa004,0xa002,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,
  0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff
};

void bump_file_limit(void)
{
  if (((((int)_osmajor)<<8)|_osminor) < 0x0303)
    return;
  _AH = 0x67;
  _BX = 255;
  geninterrupt(0x21);
}

void initialize_printer(void)
{
  _BX=4;
  _AH=0x44;
  _AL=0x01;
  _DX=0xa0;
  geninterrupt(0x21);
  setmode(4, O_BINARY);
}

void fillgdt(int sel, word32 limit, word32 base, word8 type, int G)
{
  GDT_S *g;
  g = gdt+sel;
  if (G & 2)
    limit = limit >> 12;
  g->lim0 = limit & 0xffff;
  g->lim1 = (limit>>16) & 0x0f;
  g->base0 = base & 0xffff;
  g->base1 = (base>>16) & 0xff;
  g->base2 = (base>>24) & 0xff;
  g->stype = type;
  g->lim1 |= G * 0x40;
}

void setup_tss(TSS *t, void (*eip)())
{
  memset(t, 0, sizeof(TSS));
  t->tss_cs = g_rcode*8;
  t->tss_eip = (long)FP_OFF(eip);
  t->tss_ss = g_rdata*8;
  t->tss_esp = (long)FP_OFF(t->tss_stack);
  t->tss_ds = g_rdata*8;
  t->tss_es = g_rdata*8;
  t->tss_fs = g_rdata*8;
  t->tss_eflags = 0x0200;
  t->tss_iomap = 0xffff; /* no map */
}

void abort(void);

void my_abort()
{
  static int recursing = 0;
  if (recursing)
    _exit(1);
  recursing = 1;
  printf("Abort!\n");
  exit(1);
}

void exit_func(void)
{
  int i;
  dalloc_uninit();
  EventQueueDeInit();
  uninit_controllers();
  if(!use_DPMI) {
    valloc_uninit();
    if ((ems_handle)&&(ems_handle != -1))
      ems_free(ems_handle);     /*  Deallocated EMS Page    */
    if (vcpi_installed)
      vcpi_flush();             /*  Deallocated VCPI Pages  */
  } else {
    DPMIprotectedMode();
    clearDPMIstate();
    restoreDPMIvector();
    uninitDPMI(exitDPMI);
  }
  if (topline_info)
    for (i=0; i<80; i++)
      poke(screen_seg, i*2, 0x0720);
  outportb(0x21, old_mask1);
  outportb(0xa1, old_mask2);
  setvect(0x1b, old_ctrlbrk);
}

void show_call_frame(void)
{
  word32 vbp,vbp_new, tos;
  int max=0;

  if (tss_ptr == &ed_tss)
    tos = 0xb0000000L;
  else
    tos = 0x90000000L;
  vbp = tss_ptr->tss_ebp;
  fprintf(stderr,"Call frame traceback EIPs:\n  0x%08lx\n",tss_ptr->tss_eip);
  if (vbp == 0) return;
  do {
    vbp_new = peek32(vbp+ARENA);
    if (vbp_new == 0) break;
    fprintf(stderr,"  0x%08lx\n",peek32(vbp+ARENA+4));  /* EIP */
    vbp = vbp_new;
    if (++max == 10)
      break;
  } while ((vbp >=tss_ptr->tss_esp) && (vbp < tos));
  /* 0x9000000 = areas[A_stack].last_addr for DPMI, but stack can be moved */
}

void do_faulting_finish_message(int print_reason, int exit_code)
{
  if (peekb(0x40, 0x49) != old_video_mode)
  {
    _AX = old_video_mode;
    geninterrupt(0x10);
  }
  if (tss_ptr->tss_irqn == hard_master_lo+1)
  {
    fprintf(stderr, "Ctrl-%s Hit!  Stopped at address %lx\n",
      ctrl_break_hit ? "Break" : "C", tss_ptr->tss_eip);
/*
    if (ctrl_break_hit)
      show_call_frame();
*/  /* GWO commented out */
  }
  else
  {
    if (print_reason)
    {
      char *en = (tss_ptr->tss_irqn >= EXCEPTION_COUNT) ? 0 : exception_names[tss_ptr->tss_irqn];
      if (tss_ptr->tss_irqn == hard_slave_lo + 5)
	en = "Floating Point exception";
      if (en == 0)
	fprintf(stderr, "Exception %d (0x%02x) at eip=%lx\n",
		tss_ptr->tss_irqn, tss_ptr->tss_irqn, tss_ptr->tss_eip);
      else
	fprintf(stderr, "%s at eip=%lx\n", en, tss_ptr->tss_eip);
    }
    fprintf(stderr, "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
	   tss_ptr->tss_eax,
	   tss_ptr->tss_ebx,
	   tss_ptr->tss_ecx,
	   tss_ptr->tss_edx,
	   tss_ptr->tss_esi,
	   tss_ptr->tss_edi);
    fprintf(stderr, "ebp=%08lx esp=%08lx cs=%x ds=%x es=%x fs=%x gs=%x ss=%x cr2=%08x\n",
	   tss_ptr->tss_ebp,
	   tss_ptr->tss_esp,
	   tss_ptr->tss_cs,
	   tss_ptr->tss_ds,
	   tss_ptr->tss_es,
	   tss_ptr->tss_fs,
	   tss_ptr->tss_gs,
	   tss_ptr->tss_ss,
	   tss_ptr->tss_cr2);
    /* show_call_frame(); */ /* GWO commented out */
  }
  exit(exit_code);
}

int ctrl_c_flag = 0;

void interrupt ctrl_break_func(void)
{
  ctrl_break_hit = 1;
  tss_ptr->tss_irqn = hard_master_lo + 1;
  if (!in_hardware_interrupt)
    old_ctrlbrk();
}


int ctrlbrk_func(void)
{
  tss_ptr->tss_irqn = hard_master_lo + 1;
  /* do_faulting_finish_message(1,3); GWO commented out */
  /* return 0; GWO commented out */
  return(1); /* GWO added */
}

char *char2rtype(word8 r, word8 relnum)
{
  static char buf[20];
  char *fmt = "";
  switch (r)
  {
    case 'a': fmt = ".alpha%d"; break;
    case 'b': fmt = ".beta%d"; break;
    case 'f': fmt = ""; break;
    case 'm': fmt = ".maint%d"; break;
    case 's': fmt = ".special%d"; break;
    default : fmt = ".unknown%d"; break;
  }
  sprintf(buf, fmt, relnum);
  return buf;
}

void usage(char *s)
{
  word32 lowest_ver = GO32_LOWEST_COMPATIBLE_VERSION;
  word8 *l = (word8 *)(&lowest_ver);
  use_mono = 0;
  fprintf(stderr, "Lowest version I can run is %d.%02d%s\n",
	  l[3], l[2], char2rtype(l[1], l[0]));
  if (initial_argc > 1)
    fprintf(stderr, "Invalid go32 usage running %s\n", s);
  fprintf(stderr, "go32.exe usage: go32 [-d {debugger}] [{program} [{options} . . . ]]\n");
  fprintf(stderr, "go32.exe build time was %s\n", build_time+8);
  show_memory_info = 1;
  if (use_DPMI)
  {
    DPMImaxmeminfo inf;
    DPMIprotectedMode();
    DPMImaxmem(&inf);
    DPMIrealMode();
    if (inf.physical_mem == -1 || inf.swap_mem == -1)
      fprintf(stderr, "DPMI virtual memory available: %ld Kb\n", inf.largest_block / 1024L);
    else
    {
      fprintf(stderr, "DPMI memory available: %ld Kb\n", inf.physical_mem * 4L);
      fprintf(stderr, "Swap space available: %ld Kb\n", inf.swap_mem * 4L);
    }
  }
  else
  {
    valloc_init();
    dalloc_init();
  }
  exit(1);
}

static void copyright(void)
{
  fprintf(stderr, "go32 version %d.%d%s Copyright (C) 1993 DJ Delorie\n",
	  GO32_MAJOR_VERSION,
	  GO32_MINOR_VERSION,
	  char2rtype(GO32_RELEASE_TYPE_C, GO32_RELEASE_NUMBER));
}

int have_80387;
int use_xms=0;
static word32 push32(void *ptr, int len);

void setup_idt_task(int which, int tss)
{
  idt[which].selector = tss*8;
  idt[which].stype = 0x8500;
  idt[which].offset0 = 0;
  idt[which].offset1 = 0;
}

static void getlongargs(int *ac, char ***av)
{
  char *_argcs = getenv("_argc");
  int _argc, i;
  char tmp[10];
  char **_argv;

  if (_argcs == 0)
    return;
  if (*ac > 1)
    return;

  _argc = atoi(_argcs);
  _argv = (char **)malloc((_argc+1)*sizeof(char*));
  for (i = 1; i<_argc; i++)
  {
    sprintf(tmp, "_argv%d", i);
    _argv[i] = getenv(tmp);
  }
  _argv[0] = (*av)[0];
  _argv[i] = 0;
  *av = _argv;
  *ac = _argc;
  putenv("_argc=");
}

void main(int argc, char **argv, char **envp)
{
  int i, n, emu_installed=0;
  struct stat stbuf;
  char *cp, *path, *emu_fn=0;
  unsigned short header[3];
  char *external_debugger = 0;
  char *running_fname;
  DPMIinfo info;
  int disable_dpmi = 0;
  char *argv0_to_pass;
  char *argv0_to_run;
  int search_for_stubinfo = 0;

  initial_argc = argc;

  fprintf(stderr, "");
  fflush(stderr);
  fprintf(stdout, "");
  fflush(stdout);

  old_mask1 = inportb(0x21);
  old_mask2 = inportb(0xa1);

  /* make calls to abort() really call my_abort() */
  i = _CS;
  n = (int)(abort);
  pokeb(i, n, 0xe9);
  poke(i, n+1, (int)(my_abort) - (n + 3));

  cp = getenv("GO32");
  path = 0;
  if (cp)
    while (1)
    {
      char sw[100];
      char val[100];
      if (sscanf(cp, "%s%n", sw, &i) < 1)
	break;
      cp += i;
      if (stricmp(sw, "ansi") == 0)
	use_ansi = 1;
      else if (stricmp(sw, "topline") == 0)
	topline_info = 1;
      else if (stricmp(sw, "meminfo") == 0)
	show_memory_info = 1;
      else if (stricmp(sw, "mono") == 0)
	use_mono = 1;
      else if (stricmp(sw, "2r1") == 0)
	redir_2_1 = 1;
      else if (stricmp(sw, "1r2") == 0)
	redir_1_2 = 1;
      else if (stricmp(sw, "2rm") == 0)
	redir_2_mono = 1;
      else if (stricmp(sw, "1rm") == 0)
	redir_1_mono = 1;
      else if (stricmp(sw, "glob") == 0)
	globbing = 1;
      else if (stricmp(sw, "noglob") == 0)
	globbing = 0;
      else if (stricmp(sw, "nodpmi") == 0)
	disable_dpmi = 1;
      else
      {
	val[0] = 0;
	sscanf(cp, "%s%n", val, &i);
	cp += i;
	if (val[0] == 0)
	  break;
      }
      if (stricmp(sw, "driver") == 0)
      {
	if (path) free(path);
	path = strdup(val);
      }
      else if (stricmp(sw, "dpmistack") == 0)
	DPMI_STACK = (atol(val) + 0xfffL) & ~0xfffL;
      else if (stricmp(sw, "tw") == 0)
	gr_def_tw = atoi(val);
      else if (stricmp(sw, "th") == 0)
	gr_def_th = atoi(val);
      else if (stricmp(sw, "gw") == 0)
	gr_def_gw = atoi(val);
      else if (stricmp(sw, "gh") == 0)
	gr_def_gh = atoi(val);
      else if (stricmp(sw, "nc") == 0)
	gr_def_numcolor = atoi(val);
      else if (stricmp(sw, "emu") == 0)
      {
	if (emu_fn) free(emu_fn);
	emu_fn = strdup(val);
      }
    }

  if (peekb(0x40,0x49) == 7)
    screen_seg = 0xb000;
  if (!topline_info)
    use_mono = 0;
  if (use_mono)
    screen_seg = 0xb000;

  if (!disable_dpmi)
  {
    if (initDPMI(&info))
    {
      hard_master_lo = info.PIC.master;
      hard_slave_lo = info.PIC.slave;
      setDPMIvector();
      DPMIrealMode();
      use_DPMI = 1;
      ARENA = 0;
    }
  }
  if (!use_DPMI)
  {
    if (xms_installed())
      use_xms = 1;
  }

  bump_file_limit();
  old_video_mode = peekb(0x40, 0x49);

  if (strcmp(argv[1], "!proxy") == 0)
  {
    int oseg, optr, i;
    int far *oargv;
    char far *oargve;

    if (argc > 6)
    {
      word32 this_version;
      static word8 tvp[4] = { GO32_RELEASE_NUMBER, GO32_RELEASE_TYPE_C, GO32_MINOR_VERSION, GO32_MAJOR_VERSION };
      int stubinfo_ofs, stubinfo_seg;
      int si_len = sizeof(StubInfo);
      StubInfo far *stubinfo_ptr;

      sscanf(argv[5], "%x", &stubinfo_seg);
      sscanf(argv[6], "%x", &stubinfo_ofs);
      stubinfo_ptr = (StubInfo far *)MK_FP(stubinfo_seg, stubinfo_ofs);
      if (si_len > (int)stubinfo_ptr->struct_length)
	si_len = (int)stubinfo_ptr->struct_length;
      movedata(stubinfo_seg, stubinfo_ofs, FP_SEG(&stub_info), FP_OFF(&stub_info), si_len);

      if (DPMI_STACK < stub_info.min_stack)
	DPMI_STACK = stub_info.min_stack;

      this_version = *(word32 *)tvp;

      if (stub_info.required_go32_version[1] == 's')
	if (required_go32_version_w(stub_info) != this_version)
	{
	  copyright();
	  fprintf(stderr, "Error: Special version (%d.%d.special%d) of go32 required to run this program.\n",
	    stub_info.required_go32_version[3],
	    stub_info.required_go32_version[2],
	    stub_info.required_go32_version[0]);
	  exit(1);
	}
      if (required_go32_version_w(stub_info) > this_version)
      {
	copyright();
	fprintf(stderr, "Error: This program requires a version of go32 (%d.%d%s) newer than this one.\n",
	  stub_info.required_go32_version[3],
	  stub_info.required_go32_version[2],
	  char2rtype(stub_info.required_go32_version[1], stub_info.required_go32_version[0]));
	exit(1);
      }
      if (required_go32_version_w(stub_info) < GO32_LOWEST_COMPATIBLE_VERSION)
      {
	copyright();
	fprintf(stderr, "Error: This program requires a version of go32 (%d.%d%s) older than this one.\n",
	  stub_info.required_go32_version[3],
	  stub_info.required_go32_version[2],
	  char2rtype(stub_info.required_go32_version[1], stub_info.required_go32_version[0]));
	exit(1);
      }
      
      search_for_stubinfo = 0;
    }

    sscanf(argv[2], "%x", &argc);
    sscanf(argv[3], "%x", &oseg);
    sscanf(argv[4], "%x", &optr);
    oargv = MK_FP(oseg, optr);
    argv = (char **)malloc(sizeof(char *) * (argc+1));
    for (i=0; i<argc+1; i++)
    {
      if (oargv[i] == 0)
      {
	argv[i] = 0;
	break;
      }
      oargve = MK_FP(oseg, oargv[i]);
      for (optr=0; oargve[optr]; optr++);
      argv[i] = (char *)malloc(optr+1);
      for (optr=0; oargve[optr]; optr++)
	argv[i][optr] = oargve[optr];
      argv[i][optr] = 0;
    }
  }

  if (globbing == -1)
    globbing = stub_info.enable_globbing;

  getlongargs(&argc, &argv);

  if (!use_DPMI)
  {
    ems_handle = emm_present();
    switch (cputype())
    {
      case 1:
	if ((ems_handle)&&(ems_handle != -1))
	  ems_free(ems_handle);
	fprintf(stderr, "CPU must be a 386 to run this program.\n");
	exit(1);
      case 2:
	if (ems_handle)
	  {
	  if ((vcpi_installed = vcpi_present()) != 0)
	    break;
	  else if (ems_handle != -1)
	    ems_free(ems_handle);
	  }
	fprintf(stderr, "CPU must be in REAL mode (not V86 mode) to run this program without VCPI.\n");
	fprintf(stderr, "(If you are using an EMS emulator, make sure that EMS isn't disabled)\n");
	exit(1);
    }
  }

  _fmode = O_BINARY;
  setup_graphics_driver(path);
  if (path) free(path);

  setbuf(stdin, 0);
  atexit((atexit_t)exit_func);
  ctrlbrk(ctrlbrk_func);
  old_ctrlbrk = getvect(0x1b);
  setvect(0x1b, ctrl_break_func);
  n = (int)ivec1-(int)ivec0;
  for (i=0; i<256; i++)
  {
    idt[i].selector = g_altc*8;
    idt[i].stype = 0x8e00;
    idt[i].offset0 = (int)FP_OFF(ivec0) + n*i;
    idt[i].offset1 = 0;
  }
  setup_idt_task(14, g_ptss);

  cp = getenv("387");
  if (cp)
    if (tolower(cp[0]) == 'n')
      have_80387 = 0;
    else if (tolower(cp[0]) == 'y')
      have_80387 = 1;
    else
      have_80387 = detect_80387();
  else
    have_80387 = detect_80387();
  if (have_80387)
  {
    idt[7].selector = g_rcode*8;
    idt[7].offset0 = (int)ivec7;
    idt[7].offset1 = 0;
    idt[0x75].selector = g_rcode*8;
    idt[0x75].offset0 = (int)ivec75;
    idt[0x75].offset1 = 0;
  }

  if (cp && (tolower(cp[0]) == 'q'))
    if (have_80387)
      printf("An 80387 has been detected.\n");
    else
      printf("No 80387 has been detected.\n");

  fillgdt(g_zero, 0, 0, 0, 0);
  fillgdt(g_gdt, sizeof(gdt), ptr2linear(gdt), 0x92, 0);
  fillgdt(g_idt, sizeof(idt), ptr2linear(idt), 0x92, 0);
  fillgdt(g_rcode, 0xffff, (word32)_CS*16L, 0x9a, 0);
  fillgdt(g_rdata, 0xffff, (word32)_DS*16L, 0x92, 0);
  fillgdt(g_core, 0xffffffffL, 0, 0x92, 3);
  fillgdt(g_acode, 0xefffffffL, 0x10000000L, 0x9a, 3);
  fillgdt(g_adata, 0xefffffffL, 0x10000000L, 0x92, 3);
  fillgdt(g_ctss, sizeof(TSS), ptr2linear(&c_tss), 0x89, 1);
  fillgdt(g_atss, sizeof(TSS), ptr2linear(&a_tss), 0x89, 1);
  fillgdt(g_ptss, sizeof(TSS), ptr2linear(&p_tss), 0x89, 1);
  fillgdt(g_itss, sizeof(TSS), ptr2linear(&i_tss), 0x89, 1);
  fillgdt(g_rtss, sizeof(TSS), ptr2linear(&r_tss), 0x89, 1);
  fillgdt(g_rc32, 0xffff, (word32)_CS*16L, 0x9a, 3);
/* CB change: this is done in "graphics.c" now */
/* here this selector is initted such that we can set up a dummy paging */
/* function until the first graphics mode set */
/* also set up the selector for the BIOS data segment */
/* OLD:
 *  fillgdt(g_grdr, 0xffff, (word32)gr_paging_segment*16L, 0x9a, 0);
 */
  fillgdt(g_BIOSdata, 0xffff, (word32)0x400, 0x92, 0);
/* end CB change */
  fillgdt(g_v74, sizeof(TSS), ptr2linear(&v74_tss), 0x89, 1);
  fillgdt(g_v78, sizeof(TSS), ptr2linear(&v78_tss), 0x89, 1);
  fillgdt(g_v79, sizeof(TSS), ptr2linear(&v79_tss), 0x89, 1);
  fillgdt(g_altc, 0xffff, ((word32)FP_SEG(ivec0))*16L, 0x9a, 0);
  fillgdt(g_edcs, 0x0fffffffL, 0xa0000000L, 0x9a, 3);
  fillgdt(g_edds, 0x0fffffffL, 0xa0000000L, 0x92, 3);

  setup_tss(&c_tss, go_real_mode);
  setup_tss(&a_tss, go_real_mode);
  setup_tss(&o_tss, go_real_mode);
  setup_tss(&f_tss, go_real_mode);
  setup_tss(&ed_tss, go_real_mode);
  setup_tss(&r_tss, go_real_mode);
  setup_tss(&i_tss, interrupt_common);
  setup_tss(&p_tss, page_fault);
  setup_tss(&v74_tss, v74_handler);
  setup_tss(&v78_tss, v78_handler);
  setup_tss(&v79_tss, v79_handler);
  tss_ptr = &a_tss;

  a_tss.tss_edx = ptr2linear(&prog_info);
  prog_info.size_of_this_structure_in_bytes = sizeof(PROGINFO);
  prog_info.pid = 42;
  transfer_linear =
  prog_info.linear_address_of_transfer_buffer = ptr2linear(transfer_buffer) + 0xe0000000L;
  prog_info.size_of_transfer_buffer = 4096L;
  prog_info.linear_address_of_stub_info_structure = ptr2linear(&stub_info) + 0xe0000000L;
  prog_info.linear_address_of_original_psp = (word32)_psp*16L + 0xe0000000L;
  prog_info.run_mode = _GO32_RUN_MODE_UNDEF;
  prog_info.run_mode_info = 0;

  core_selector =
  a_tss.tss_gs =
  prog_info.selector_for_linear_memory = (word16)(g_core * 8L);

/*  a_tss.tss_esi = 42; */ /* PID */
  if(use_DPMI)
  {
    a_tss.tss_edi = 0;
    prog_info.run_mode = _GO32_RUN_MODE_DPMI;
    prog_info.run_mode_info = (info.version.major << 8)
			    | ((info.version.minor/10) << 4)
			    | (info.version.minor%10);
  }
  else
    a_tss.tss_edi = ptr2linear(transfer_buffer) + 0xe0000000L;

  argv0_to_pass = argv[0];
  for (i=0; argv0_to_pass[i]; i++)
  {
    if (argv0_to_pass[i] == '\\')
      argv0_to_pass[i] = '/';
    argv0_to_pass[i] = tolower(argv0_to_pass[i]);
  }

  if (topline_info)
    for (i=0; i<80; i++)
      poke(screen_seg, i*2, 0x0720);

  if (stub_info.actual_file_to_run[0])
  {
    char *cp;
    argv0_to_run = (char *)malloc(strlen(argv0_to_pass + 13));
    strcpy(argv0_to_run, argv0_to_pass);
    for (cp = argv0_to_run + strlen(argv0_to_run) - 1; cp > argv0_to_run; cp--)
      if (cp[-1] == '/' || cp[-1] == '\\' || cp[-1] == ':')
	break;
    strcpy(cp, stub_info.actual_file_to_run);
    if (access(argv0_to_run, 0))
    {
      strcat(cp, ".exe");
      if (access(argv0_to_run, 0))
       cp[strlen(cp)-4] = 0; /* remove the .exe */
    }
  }
  else
  {
    argv0_to_run = argv0_to_pass;
  }

  if (emu_fn == NULL && !have_80387)
  {
    char *emu_try, *last_slash;

    /* If no emu_fn specified, and no 80387 is  present, check for an
     * "emu387" file in the  same directory as the extender.  If we find
     * one, we will use that as the 80387 emulator.  Otherwise,
     * no 80387 emulation.
     */

    /* Grab the directory containing the extender and append emu387. */
    last_slash = strrchr (argv0_to_run, '/');
    if (last_slash != NULL)
    {
      int path_len = last_slash - argv0_to_run + 1;     /* Length incl. '/' */
      emu_try = (char *) malloc (path_len + 7);
      strncpy (emu_try, argv0_to_run, path_len);
      strcpy (emu_try + path_len, "emu387");
    }
    else
      emu_try = strdup ("emu387");

    if (!stat (emu_try, &stbuf))
      emu_fn = emu_try;                 /* Found it. */
    else
      free (emu_try);                   /* Nope. */
  }


  self_contained = 0;
  n = open(argv0_to_run, O_RDONLY|O_BINARY);
  header[0] = 0;
  read(n, header, sizeof(header));
  if (header[0] == 0x5a4d)
  {
    word32 stub_offset;
    header_offset = (long)header[2]*512L;
    if (header[1])
      header_offset += (long)header[1] - 512L;
    lseek(n, header_offset - 4, 0);
    read(n, &stub_offset, 4);
/*    if (search_for_stubinfo)
      get_stubinfo(stub_offset); */
    header[0] = 0;
    read(n, header, sizeof(header));
    if (header[0] == 0x010b)
      self_contained = 1;
    if (header[0] == 0x014c)
      self_contained = 1;
  }
  close(n);

  if (self_contained)
  {
    paging_set_file(argv0_to_run);
    running_fname = argv0_to_run;
    emu_installed = emu_install(emu_fn);
    set_command_line(argv, envp);
  }
  else
  {
    header_offset = 0;
    for (cp=argv0_to_run; *cp; cp++)
    {
      if (*cp == '.')
	path = cp;
      if (*cp == '/' || *cp == '\\')
	path = 0;
    }
    if (path)
      *path = 0;
    if (stat(argv0_to_run, &stbuf)) /* not found */
    {
      copyright();
      if (strcmp(argv[1], "-d") == 0 && argc > 2)
      {
	debug_mode = 1;
	external_debugger = argv[2];
	argv += 2;
	argc -= 2;
      }
      if (argv[1] == 0)
	usage(argv0_to_pass);
      paging_set_file(argv[1]);
      running_fname = argv[1];
      emu_installed = emu_install(emu_fn);
      set_command_line(argv+1, envp);
    }
    else /* found */
    {
      paging_set_file(argv0_to_run);
      running_fname = argv0_to_run;
      emu_installed = emu_install(emu_fn);
      set_command_line(argv, envp);
    }
  }

  dalloc_init();
  init_controllers();

  setup_idt_task(0x74, g_v74);
  setup_idt_task(hard_master_lo, g_v78);
  setup_idt_task(hard_master_lo+1, g_v79);

  if(!use_DPMI)
    a_tss.tss_eax = (g_core*8L)<<16;
  a_tss.tss_eax |= (word32)hard_master_lo | (((word32)hard_slave_lo)<<8);

  prog_info.master_interrupt_controller_base = (word8)hard_master_lo;
  prog_info.slave_interrupt_controller_base = (word8)hard_slave_lo;

  if (external_debugger)
    load_external_debugger(external_debugger, running_fname, argv0_to_pass);

  if (emu_installed)
  {
    push32(&(a_tss.tss_eip), 4);
    a_tss.tss_eip = emu_start_ip();
  }
  go_til_stop();
  do_faulting_finish_message(1,1);
}

static word32 push32(void *ptr, int len)
{
  if ((a_tss.tss_esp & ~0xFFF) != ((a_tss.tss_esp-len) & ~0xFFF))
  {
    a_tss.tss_cr2 = a_tss.tss_esp - len + ARENA;
    page_in();
  }
  a_tss.tss_esp -= len;
  a_tss.tss_esp = a_tss.tss_esp & (~3);
  memput(a_tss.tss_esp+ARENA, ptr, len);
  return a_tss.tss_esp;
}

static int fscan_q(FILE *f, char *buf)
{
  char *ibuf = buf;
  int c, quote=-1, gotsome=0, addquote=0;
  while ((c = fgetc(f)) != EOF)
  {
    if (c == '\\')
    {
      char c2 = fgetc(f);
      if (! strchr("\"'`\\ \t\n\r", c2))
	*buf++ = c;
      *buf++ = c2;
      addquote = 0;
    }
    else if (c == quote)
    {
      quote = -1;
      if (c == '\'')
	addquote = 1;
    }
    else if (isspace(c) && (quote==-1))
    {
      if (gotsome)
      {
	if (addquote)
	  *buf++ = '\'';
	*buf = 0;
	return 1;
      }
      addquote = 0;
    }
    else
    {
      if ((quote == -1) && ((c == '"') || (c == '\'')))
      {
	quote = c;
	gotsome=1;
	if ((c == '\'') && (buf == ibuf))
	  *buf++ = c;
      }
      else
      {
	*buf++ = c;
	gotsome=1;
      }
      addquote = 0;
    }
  }
  return 0;
}

static void glob(char *buf, void (*func)(char *))
{
  if (globbing && strpbrk(buf, "*?"))
  {
    char *dire, *cp;
    struct ffblk ff;
    int done, upcase=0;
    done = findfirst(buf, &ff, FA_RDONLY|FA_DIREC|FA_ARCH);
    if (done)
      func(buf);
    else
    {
      char nbuf[180];
      strcpy(nbuf, buf);
      for (dire=cp=nbuf; *cp; cp++)
      {
	if (strchr("/\\:", *cp))
	  dire = cp + 1;
	if (isupper(*cp))
	  upcase = 1;
      }
      while (!done)
      {
	strcpy(dire, ff.ff_name);
	if (!upcase)
	  strlwr(dire);
	if (strcmp(dire, ".") && strcmp(dire, ".."))
	  func(nbuf);
	done = findnext(&ff);
      }
    }
  }
  else
    func(buf);
}

static void foreach_arg(char **argv, void (*func)(char *))
{
  int i;
  FILE *f;
  char buf[180];
  for (i=0; argv[i]; i++)
  {
    if (argv[i][0] == '@')
    {
      f = fopen(argv[i]+1, "rt");
      while (fscan_q(f, buf) == 1)
      {
	if (!strcmp(buf, "\032"))
	  continue;
	glob(buf, func);
      }
      fclose(f);
    }
    else
      glob(argv[i], func);
  }
}

static int num_actual_args;

static void just_incr(void)
{
  num_actual_args++;
}

static word32 *a;

void pusharg(char *ar)
{
  int s = strlen(ar);
  if ((ar[0] == '\'') && (ar[s-1] == '\''))
  {
    ar[s-1] = '\0';
    ar++;
  }
  a[num_actual_args] = push32(ar, s+1);
  num_actual_args++;
}

void set_command_line(char **argv, char **envv)
{
  unsigned envc;
  word32 *e, v, argp, envp;

  a_tss.tss_cr2 = a_tss.tss_esp + ARENA;
  page_in();

  num_actual_args = 0;
  foreach_arg(argv, (void (*)(char*))just_incr);

  for (envc=0; envv[envc]; envc++);
  e = (word32 *)malloc((envc+1)*sizeof(word32));
  if (e == 0)
  {
    fprintf(stderr, "Fatal! no memory to copy environment\n");
    exit(1);
  }
  for (envc=0; envv[envc]; envc++)
  {
    char *sp, *dp;
    int state = 0;
    sp = dp = envv[envc];
    while (*sp)
    {
      switch (state)
      {
	case 0: /* waiting for '=' */
	  if (!isspace(*sp))
	    *dp++ = *sp;
	  if (*sp == '=')
	    state = 1;
	  break;
	case 1: /* got '=' */
	  if (!isspace(*sp))
	  {
	    *dp++ = *sp;
	    state = 2;
	  }
	  break;
	case 2: /* in value */
	  *dp++ = *sp;
	  break;
      }
      sp++;
    }
    while (dp > envv[envc] && isspace(dp[-1]))
      *--dp = 0;
    v = push32(envv[envc], strlen(envv[envc])+1);
    e[envc] = v;
  }
  e[envc] = 0;

  a = (word32 *)malloc((num_actual_args+1)*sizeof(word32));
  if (a == 0)
  {
    fprintf(stderr, "Fatal! no memory to copy arguments\n");
    exit(1);
  }
  num_actual_args = 0;
  foreach_arg(argv, pusharg);
  a[num_actual_args] = 0;

  envp = push32(e, (envc+1)*sizeof(word32));
  argp = push32(a, (num_actual_args+1)*sizeof(word32));

  push32(&envp, sizeof(word32));
  push32(&argp, sizeof(word32));
  v = num_actual_args;
  push32(&v, sizeof(word32));
}

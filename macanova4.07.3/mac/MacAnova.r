/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1999 by Gary Oehlert and Christopher Bingham
*(C)* unless indicated otherwise
*(C)*
*(C)* You may give out copies of this software;  for conditions see the
*(C)* file COPYING included with this distribution
*(C)*
*(C)* This file is distributed WITHOUT ANY WARANTEE; without even
*(C)* the implied warantee of MERCHANTABILITY or FITNESS FOR
*(C)* A PARTICULAR PURPOSE
*(C)*
*/

/*
	970114 added history items to Edit menu
	970224 new Other Options dialog box and slightly modifed some
	       other Options dialog boxes
	970922 Slight modification of Edit menu
	980512 Updated version message; increased preferred size
*/

#include "macResource.h" /* resource ID defines */

#define diamondMark "\0x13"

resource 'SIZE' (-1, purgeable) {
	1,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	2560000,  /* preferred mem size	*/
	665600    /* minimum mem size 650K */
};

resource 'BNDL' (128) {
	'mat2',
	0,
	{	/* array TypeArray: 2 elements */
		/* [1] */
		'ICN#',
		{	/* array IDArray: 3 elements */
			/* [1] */
			0, 128,
			/* [2] */
			1, 129,
			/* [3] */
			2, 130,
		},
		/* [2] */
		'FREF',
		{	/* array IDArray: 4 elements */
			/* [1] */
			0, 128,
			/* [2] */
			1, 129,
			/* [3] */
			2, 130,
			/* [4] */
			3, 131
		}
	}
};

resource 'FREF' (128) {
	'APPL',
	0,
	""
};

resource 'FREF' (129) {
	'TEXT',
	1,
	""
};

resource 'FREF' (130) {
	'S000',
	2,
	""
};

resource 'FREF' (131) {
	'Sasc',
	2,
	""
};

/* Macanova icons */
resource 'ICN#' (128) { /* Application */
	{	/* array: 2 elements */
		/* [1] */
		$"FFFF FFFF 8000 0001 8000 0001 8000 0881"
		$"8000 0501 8000 0201 8000 0201 8000 0201"
		$"8000 1001 8000 3001 8000 5001 8000 9001"
		$"8001 1001 8002 1001 8004 1001 8008 1001"
		$"8010 1001 8020 1001 8040 1001 8080 1001"
		$"8100 1021 8200 1051 8400 5489 8800 3801"
		$"9300 1089 8480 0051 8480 0021 8480 0021"
		$"8300 0021 8000 0001 8000 0001 FFFF FFFF",
		/* [2] */
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
	}
};

resource 'ICN#' (129) { /* for saved output file */
	{	/* array: 2 elements */
		/* [1] */
		$"FFFF FFFC 8000 0004 81E0 0F04 8210 1084"
		$"8408 2044 8408 2044 8400 2004 8200 1004"
		$"8180 0C04 8060 0304 8010 0084 8008 0044"
		$"8008 0044 8008 0044 8408 2044 8210 1084"
		$"81E0 0F04 8000 0004 BFFF FFF4 8000 0004"
		$"8004 0E04 8004 1104 8004 2104 8004 2004"
		$"8074 2004 808C 7804 8104 2004 8104 2004"
		$"8104 2004 808C 2004 8074 2004 FFFF FFFC",
		/* [2] */
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
		$"FFFF FFFC FFFF FFFC FFFF FFFC FFFF FFFC"
	}
};

resource 'ICN#' (130) { /* for saved workspace */
	{	/* array: 2 elements */
		/* [1] */
		$"FFFF FFFF 8000 0001 8000 0001 9040 0001"
		$"8880 0001 8500 0001 8200 0001 8500 0001"
		$"8880 0001 9058 0001 8008 0001 8010 0001"
		$"8000 0001 8010 4001 8008 8001 8005 0001"
		$"8002 0001 8002 0001 8002 0001 8002 6001"
		$"8000 2001 8000 4001 8000 1F81 8000 0081"
		$"8000 0101 8000 0201 8000 0401 8000 0801"
		$"8000 1001 8000 1F81 8000 0001 FFFF FFFF",
		/* [2] */
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
		$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
	}
};


/* Leading "8" is 0x38 = 56 = length of rest of string */
data 'mat2' (0) {
	"8MacAnova 4.07, Copyright 1999 G. W. Oehlert & C. Bingham"
};

resource 'MENU' (FILEMENU) {
	2,
	textMenuProc,
	0x7FF2DCB7, /*0010 1101 1100 1011 0111  1,2,3,5,6,8,11,12,13,15, 16, 18 enabled */
	enabled,
	"File",
	{	/* array: 18 elements */
		/* [1] */
		"OpenÉ", noIcon, "O", noMark, plain,
		/* [2] */
		"Save Window", noIcon, "S", noMark, plain,
		/* [3] */
		"Save Window AsÉ", noIcon, noKey, noMark, plain,
		/* [4] */
		"-", noIcon, noKey, noMark, plain,
		/* [5] */
		"Page SetupÉ", noIcon, noKey, noMark, plain,
		/* [6] */
		"Print WindowÉ", noIcon, "P", noMark, plain,
		/* [7] */
		"-", noIcon, noKey, noMark, plain,
		/* [8] */
		"Interrupt", noIcon, "I", noMark, plain,
		/* [9] */
		"Go On", noIcon, noKey, noMark, plain,
		/* [10] */
		"-", noIcon, noKey, noMark, plain,
		/* [11] */
		"Restore WorkspaceÉ", noIcon, "R", noMark, plain,
		/* [12] */
		"Save Workspace", noIcon, "K", noMark, plain,
		/* [13] */
		"Save Workspace AsÉ", noIcon, noKey, noMark, plain,
		/* [14] */
		"-", noIcon, noKey, noMark, plain,
		/* [15] */
		"Open Batch File...", noIcon, "¿", noMark, plain,
		/* [16] */
		"Spool Output to FileÉ", noIcon, "§", noMark, plain,
		/* [17] */
		"-", noIcon, noKey, noMark, plain,
		/* [18] */
		"Quit", noIcon, "Q", noMark, plain
	}
};

resource 'MENU' (EDITMENU) {
	3,
	textMenuProc,
	0x7FFFF8DC, /* 000 1101 1100  items 3, 4, 5, 7, 8 enabled */
	enabled,
	"Edit",
	{	/* array: 8 elements */
		/* [1] */
		"Undo", noIcon, "Z", noMark, plain,
		/* [2] */
		"-", noIcon, noKey, noMark, plain,
		/* [3] */
		"Cut", noIcon, "X", noMark, plain,
		/* [4] */
		"Copy", noIcon, "C", noMark, plain,
		/* [5] */
		"Paste", noIcon, "V", noMark, plain,
		/* [6] */
		"-", noIcon, noKey, noMark, plain,
		/* [7] */
		"Copy to End", noIcon, "/", noMark, plain,
		/* [8] */
		"Execute", noIcon, "\\", noMark, plain,
		/* [9] */
		"-", noIcon, noKey, noMark, plain,
		/* [10] */
		"Up History           F7", noIcon, noKey, noMark, plain,
		/* [11] */
		"Down History      F8", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (WINDOWMENU) {
	4,
	textMenuProc,
	0x7FFE0000, /* 00 0000 0000 0000 0000  no items enabled */
	enabled,
	"Windows",
	{	/* array: 22 elements */
		/* [1] */
		"Hide", noIcon, noKey, noMark, plain,
		/* [2] */
		"Close", noIcon, "W", noMark, plain,
		/* [3] */
		"New Window", noIcon, "N", noMark, plain,
		/* [4] */
		"-", noIcon, noKey, noMark, plain,
		/* [5] */
		"Graph 1", noIcon, "1", noMark, plain,
		/* [6] */
		"Graph 2", noIcon, "2", noMark, plain,
		/* [7] */
		"Graph 3", noIcon, "3", noMark, plain,
		/* [8] */
		"Graph 4", noIcon, "4", noMark, plain,
		/* [9] */
		"Graph 5", noIcon, "5", noMark, plain,
		/* [10] */
		"Graph 6", noIcon, "6", noMark, plain,
		/* [11] */
		"Graph 7", noIcon, "7", noMark, plain,
		/* [12] */
		"Graph 8", noIcon, "8", noMark, plain,
		/* [13] */
		"Panel 1-4", noIcon, "G", noMark, plain,
		/* [14] */
		"Panel 5-8", noIcon, noKey, noMark, plain,
		/* [15] */
		"-", noIcon, noKey, noMark, plain,
		/* [16] */
		"Untitled-1", noIcon, "M", check, plain,
		/* [17] */
		"-", noIcon, noKey, noMark, plain,
		/* [18] */
		"Scroll To Top", noIcon, "T", noMark, plain,
		/* [19] */
		"Go To End", noIcon, "E", noMark, plain,
		/* [20] */
		"Go To Prompt", noIcon, "A", noMark, plain,
		/* [21] */
		"Page Up", noIcon, "U", noMark, plain,
		/* [22] */
		"Page Down", noIcon, "D", noMark, plain
	}
};

resource 'MENU' (COMMANDMENU) {
	5,
	textMenuProc,
	0x7FFFFFFD, /* 11 1111 1101 */
	enabled,
	"Command",
	{	/* array: 10 elements */
		/* [1] */
		"Edit CommandsÉ", noIcon, noKey, noMark, plain,
		/* [2] */
		"-", noIcon, noKey, noMark, plain,
		/* [3] */
		"listbrief(real:T,char:T,logic:T)", noIcon, "L", diamondMark, plain,
		/* [4] */
		"list(real:T,char:T,logic:T)", noIcon, noKey, noMark, plain,
		/* [5] */
		"regress(\"\")", noIcon, noKey, noMark, plain,
		/* [6] */
		"anova(\"\")", noIcon, noKey, noMark, plain,
		/* [7] */
		"resvsrankits()", noIcon, noKey, diamondMark, plain,
		/* [8] */
		"resvsyhat()", noIcon, noKey, diamondMark, plain,
		/* [9] */
		"resvsindex()", noIcon, noKey, diamondMark, plain,
		/* [10] */
		"describe()", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (OPTIONSMENU, "Options") {
	OPTIONSMENU,
	textMenuProc,
	allEnabled,
	enabled,
	"Options",
	{	/* array: 7 elements */
		/* [1] */
		"Significant Digits", noIcon, noKey, noMark, plain,
		/* [2] */
		"Output Formats", noIcon, noKey, noMark, plain,
		/* [3] */
		"Random # Seeds", noIcon, noKey, noMark, plain,
		/* [4] */
		"Angle Units", noIcon, noKey, noMark, plain,
		/* [5] */
		"GLM Options", noIcon, noKey, noMark, plain,
		/* [6] */
		"Batch Options", noIcon, noKey, noMark, plain,
		/* [7] */
		"Other Options", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (FONTMENU, "FontSizes") {
	FONTMENU,
	textMenuProc,
	allEnabled,
	enabled,
	"Font",
	{	/* array: 2 elements */
		/* [1] */
		"Size", noIcon, hierarchicalMenu, FONTSIZEMENUchar, plain,
		/* [2] */
		"-", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (FONTSIZEMENU, "FontSizes") {
	FONTSIZEMENU,
	textMenuProc,
	allEnabled,
	enabled,
	"Size",
	{	/* array: 0 elements */
		/* [0] */
	}
};

resource 'WIND' (COMMANDWINDOW) {
	{40, 2, 330, 506},
	zoomDocProc,
	invisible,
	goAway,
	0x0,
	/*"MacAnova Command Window"*/
	"Untitled"
};

resource 'WIND' (GRAPHWINDOW) {
	{45, 12, 335, 495},
	zoomDocProc,
	invisible,
	goAway,
	0x0,
	"Graph ?"
};

resource 'WIND' (BATCHMODE) {
	{24, 4, 50, 30},
	plainDBox,
	invisible,
	noGoAway,
	0x0,
	""
};

/* Start of copy of MacAnova.Dlog.r */

resource 'DLOG' (EDITCOMMANDS, "Edit Commands") {
	{52, 28, 302, 490},
	dBoxProc,
	invisible,
	goAway,
	0x0,
	EDITCOMMANDS,
	"Edit Commands"
};

resource 'DLOG' (CONSOLEDLOG, "ConsoleInput") {
	{74, 24, 300, 491},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	CONSOLEDLOG,
	"Console Input"
};

resource 'DLOG' (ABOUTMACANOVA, "AboutMacAnova", purgeable) {
	{70, 16, 288, 494},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	ABOUTMACANOVA,
	"About MacAnova"
};

resource 'DLOG' (BATCHMODE, "BatchMode") {
	{114, 140, 184, 406},
	noGrowDocProc,
	invisible,
	noGoAway,
	0x0,
	BATCHMODE,
	"MacAnova Non-Interactive Mode"
};

resource 'DLOG' (TWOCHOICEDLOG, "Two choices") {
	{108, 87, 223, 429},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	TWOCHOICEDLOG,
	""
};

resource 'DLOG' (FORMATSDLOG, "Output formats") {
	{86, 46, 291, 466},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	FORMATSDLOG,
	""
};

resource 'DLOG' (SEEDSDLOG, "Seeds") {
	{90, 122, 217, 375},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	SEEDSDLOG,
	""
};

resource 'DLOG' (ANGLESDLOG, "Angles") {
	{112, 120, 230, 360},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	ANGLESDLOG,
	""
};

resource 'DLOG' (BOPTIONSDLOG, "Batch options") {
	{86, 116, 200, 370},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	BOPTIONSDLOG,
	""
};

resource 'DLOG' (NSIGDLOG, "Significant Digits") {
	{84, 118, 162, 366},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	NSIGDLOG,
	""
};

resource 'DLOG' (GLMOPTIONSDLOG, "GLM Options") {
	{74, 114, 193, 393},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	GLMOPTIONSDLOG,
	""
};

resource 'DLOG' (OTHEROPTIONSDLOG, "Other Options") {
	{92, 150, 342, 493},
	dBoxProc,
	invisible,
	noGoAway,
	0x0,
	OTHEROPTIONSDLOG,
	""
};

resource 'DITL' (MYALERT, "MyAlert", purgeable) {
	{	/* array DITLarray: 2 elements */
		/* [1] */
		{82, 178, 101, 268},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{4, 5, 71, 437},
		StaticText {
			disabled,
			"^0"
		}
	}
};

resource 'DITL' (SAVEALERT, "SaveAlert", purgeable) {
	{	/* array DITLarray: 4 elements */
		/* [1] */
		{55, 288, 75, 348},
		Button {
			enabled,
			"Save"
		},
		/* [2] */
		{55, 215, 75, 275},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{55, 72, 75, 156},
		Button {
			enabled,
			"DonÕt Save"
		},
		/* [4] */
		{10, 75, 42, 348},
		StaticText {
			disabled,
			"^0"
		}
	}
};

resource 'DITL' (CONSOLEDLOG, "Console Input") {
	{	/* array DITLarray: 5 elements */
		/* [1] */
		{193, 296, 211, 364},
		Button {
			enabled,
			"Next Line"
		},
		/* [2] */
		{194, 118, 212, 188},
		Button {
			enabled,
			"Done"
		},
		/* [3] */
		{0, 0, 5, 6},
		UserItem {
			disabled
		},
		/* [4] */
		{31, 18, 175, 445},
		EditText {
			disabled,
			""
		},
		/* [5] */
		{7, 18, 26, 445},
		UserItem {
			disabled
		}
	}
};

resource 'DITL' (ABOUTMACANOVA, "About Macanova") {
	{	/* array DITLarray: 10 elements */
		/* [1] */
		{-100, -100, -80, -50},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{2, 2, 26, 26},
		UserItem {
			enabled
		},
		/* [3] */
		{42, 34, 58, 444},
		UserItem {
			enabled
		},
		/* [4] */
		{78, 34, 94, 444},
		UserItem {
			enabled
		},
		/* [5] */
		{96, 34, 112, 444},
		UserItem {
			enabled
		},
		/* [6] */
		{114, 34, 130, 444},
		UserItem {
			enabled
		},
		/* [7] */
		{132, 34, 148, 444},
		UserItem {
			enabled
		},
		/* [8] */
		{150, 34, 166, 444},
		UserItem {
			enabled
		},
		/* [9] */
		{168, 34, 184, 444},
		UserItem {
			enabled
		},
		/* [10] */
		{0, 0, 0, 0},
		UserItem {
			disabled
		}
	}
};

resource 'DITL' (BATCHMODE, "Batchmode") {
	{	/* array DITLarray: 1 elements */
		/* [1] */
		{24, 88, 45, 167},
		Button {
			enabled,
			"Interrupt"
		}
	}
};

resource 'DITL' (TWOCHOICEDLOG, "Two choice") {
	{	/* array DITLarray: 4 elements */
		/* [1] */
		{74, 202, 94, 292},
		Button {
			enabled,
			"Button 1"
		},
		/* [2] */
		{74, 50, 94, 140},
		Button {
			enabled,
			"Button 2"
		},
		/* [3] */
		{0, 0, 0, 0},
		UserItem {
			disabled
		},
		/* [4] */
		{14, 19, 54, 323},
		StaticText {
			disabled,
			"^0"
		}
	}
};

resource 'DITL' (NSIGDLOG, "Significant digits") {
	{	/* array DITLarray: 5 elements */
		/* [1] */
		{45, 161, 65, 219},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{45, 34, 65, 92},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{1, 5, 16, 22},
		UserItem {
			disabled
		},
		/* [4] */
		{15, 17, 31, 189},
		StaticText {
			disabled,
			"Default Significant Digits"
		},
		/* [5] */
		{15, 201, 31, 229},
		EditText {
			disabled,
			"M"
		}
	}
};

resource 'DITL' (FORMATSDLOG, "Output Formats") {
	{	/* array DITLarray: 17 elements */
		/* [1] */
		{170, 291, 190, 376},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{170, 48, 190, 133},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{170, 166, 190, 251},
		Button {
			enabled,
			"Set Default"
		},
		/* [4] */
		{3, 395, 17, 416},
		UserItem {
			disabled
		},
		/* [5] */
		{41, 11, 57, 111},
		StaticText {
			disabled,
			"Which Format:"
		},
		/* [6] */
		{32, 118, 49, 349},
		RadioButton {
			enabled,
			"Write and matwrite (ÔwformatÕ)"
		},
		/* [7] */
		{53, 118, 70, 316},
		RadioButton {
			enabled,
			"Other output (ÔformatÕ)"
		},
		/* [8] */
		{90, 11, 107, 64},
		StaticText {
			enabled,
			"Type:"
		},
		/* [9] */
		{80, 65, 96, 218},
		RadioButton {
			enabled,
			"Floating point (ÔgÕ)"
		},
		/* [10] */
		{104, 65, 120, 204},
		RadioButton {
			enabled,
			"Fixed point (ÔfÕ)"
		},
		/* [11] */
		{80, 233, 96, 280},
		StaticText {
			disabled,
			"Width:"
		},
		/* [12] */
		{80, 377, 96, 409},
		EditText {
			disabled,
			"MM"
		},
		/* [13] */
		{104, 233, 120, 372},
		StaticText {
			disabled,
			"Number of Decimals:"
		},
		/* [14] */
		{104, 377, 120, 409},
		EditText {
			disabled,
			"NN"
		},
		/* [15] */
		{134, 65, 150, 219},
		StaticText {
			disabled,
			"Print Missing Values As"
		},
		/* [16] */
		{134, 233, 150, 374},
		EditText {
			disabled,
			"MISSING"
		},
		/* [17] */
		{9, 148, 25, 273},
		StaticText {
			disabled,
			"Set Output Format"
		}
	}
};

resource 'DITL' (SEEDSDLOG, "Seeds") {
	{	/* array DITLarray: 8 elements */
		/* [1] */
		{94, 175, 114, 233},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{94, 21, 114, 79},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{7, 231, 22, 247},
		UserItem {
			disabled
		},
		/* [4] */
		{39, 17, 55, 67},
		StaticText {
			disabled,
			"Seed 1"
		},
		/* [5] */
		{40, 144, 56, 237},
		EditText {
			disabled,
			"1234567890"
		},
		/* [6] */
		{65, 17, 81, 67},
		StaticText {
			disabled,
			"Seed 2"
		},
		/* [7] */
		{66, 144, 82, 237},
		EditText {
			disabled,
			"1234567890"
		},
		/* [8] */
		{11, 38, 27, 214},
		StaticText {
			disabled,
			"Set Random Number Seeds"
		}
	}
};

resource 'DITL' (ANGLESDLOG, "Angles") {
	{	/* array DITLarray: 7 elements */
		/* [1] */
		{87, 158, 107, 216},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{87, 26, 107, 84},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{8, 12, 25, 27},
		UserItem {
			disabled
		},
		/* [4] */
		{38, 14, 54, 155},
		StaticText {
			disabled,
			"Set Units For Angles"
		},
		/* [5] */
		{16, 160, 34, 234},
		RadioButton {
			enabled,
			"Degrees"
		},
		/* [6] */
		{37, 160, 55, 224},
		RadioButton {
			enabled,
			"Cycles"
		},
		/* [7] */
		{58, 160, 76, 234},
		RadioButton {
			enabled,
			"Radians"
		}
	}
};

resource 'DITL' (BOPTIONSDLOG, "Batch Options") {
	{	/* array DITLarray: 8 elements */
		/* [1] */
		{81, 178, 101, 236},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{81, 17, 101, 75},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{1, 230, 15, 246},
		UserItem {
			disabled
		},
		/* [4] */
		{29, 168, 47, 225},
		RadioButton {
			enabled,
			"Echo"
		},
		/* [5] */
		{53, 168, 70, 241},
		RadioButton {
			enabled,
			"No Echo"
		},
		/* [6] */
		{43, 18, 59, 93},
		StaticText {
			disabled,
			"Error Limit:"
		},
		/* [7] */
		{43, 102, 59, 133},
		EditText {
			disabled,
			"MM"
		},
		/* [8] */
		{9, 47, 25, 200},
		StaticText {
			disabled,
			"Set Options For batch()"
		}
	}
};

resource 'DITL' (GLMOPTIONSDLOG, "GLM Options") {
	{	/* array DITLarray: 10 elements */
		/* [1] */
		{85, 185, 105, 243},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{85, 33, 105, 91},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{93, 5, 110, 20},
		UserItem {
			disabled
		},
		/* [4] */
		{35, 28, 51, 146},
		StaticText {
			disabled,
			"Print P-Values"
		},
		/* [5] */
		{35, 161, 51, 211},
		RadioButton {
			enabled,
			"Yes"
		},
		/* [6] */
		{35, 216, 51, 266},
		RadioButton {
			enabled,
			"No"
		},
		/* [7] */
		{59, 28, 75, 146},
		StaticText {
			disabled,
			"Print F-Statistics"
		},
		/* [8] */
		{59, 161, 75, 211},
		RadioButton {
			enabled,
			"Yes"
		},
		/* [9] */
		{59, 216, 75, 266},
		RadioButton {
			enabled,
			"No"
		},
		/* [10] */
		{11, 10, 28, 274},
		StaticText {
			disabled,
			"Set Regression, ANOVA and GLM Options"
		}
	}
};

resource 'DITL' (OTHEROPTIONSDLOG, "Other Options") {
	{	/* array DITLarray: 22 elements */
		/* [1] */
		{218, 239, 238, 297},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{218, 43, 238, 101},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{20, 10, 37, 25},
		UserItem {
			disabled
		},
		/* [4] */
		{37, 25, 53, 216},
		StaticText {
			disabled,
			"Always Make ÒDumbÓ Plots"
		},
		/* [5] */
		{37, 224, 53, 274},
		RadioButton {
			enabled,
			"Yes"
		},
		/* [6] */
		{37, 279, 53, 329},
		RadioButton {
			enabled,
			"No"
		},
		/* [7] */
		{62, 25, 78, 216},
		StaticText {
			disabled,
			"Print WARNING Messages"
		},
		/* [8] */
		{62, 224, 78, 274},
		RadioButton {
			enabled,
			"Yes"
		},
		/* [9] */
		{62, 279, 78, 329},
		RadioButton {
			enabled,
			"No"
		},
		/* [10] */
		{87, 25, 103, 216},
		StaticText {
			disabled,
			"restore() Deletes Variables"
		},
		/* [11] */
		{87, 224, 103, 274},
		RadioButton {
			enabled,
			"Yes"
		},
		/* [12] */
		{87, 279, 103, 329},
		RadioButton {
			enabled,
			"No"
		},
		/* [13] */
		{112, 25, 128, 216},
		StaticText {
			disabled,
			"Scroll Back After Commands"
		},
		/* [14] */
		{112, 224, 128, 274},
		RadioButton {
			enabled,
			"Yes"
		},
		/* [15] */
		{112, 279, 128, 329},
		RadioButton {
			enabled,
			"No"
		},
		/* [16] */
		{137, 25, 153, 177},
		StaticText {
			disabled,
			"Prompt"
		},
		/* [17] */
		{137, 188, 153, 313},
		EditText {
			disabled,
			"Cmd> "
		},
		/* [18] */
		{162, 25, 178, 177},
		StaticText {
			disabled,
			"Lines To Remember"
		},
		/* [19] */
		{162, 188, 178, 313},
		EditText {
			disabled,
			"50"
		},
		/* [20] */
		{187, 25, 203, 177},
		StaticText {
			disabled,
			"Limit For while() Loops"
		},
		/* [21] */
		{187, 188, 203, 313},
		EditText {
			disabled,
			"1000"
		},
		/* [22] */
		{13, 112, 32, 232},
		StaticText {
			enabled,
			"Set Other Options"
		}
	}
};

resource 'DITL' (EDITCOMMANDS, "Edit Commands") {
	{	/* array DITLarray: 21 elements */
		/* [1] */
		{228, 270, 246, 340},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{228, 112, 246, 182},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{0, 0, 0, 0},
		UserItem {
			disabled
		},
		/* [4] */
		{27, 12, 42, 425},
		EditText {
			disabled,
			"Command 1"
		},
		/* [5] */
		{52, 12, 67, 425},
		EditText {
			disabled,
			"Command 2"
		},
		/* [6] */
		{77, 12, 92, 425},
		EditText {
			disabled,
			"Command 3"
		},
		/* [7] */
		{102, 12, 117, 425},
		EditText {
			disabled,
			"Command 4"
		},
		/* [8] */
		{127, 12, 142, 425},
		EditText {
			disabled,
			"Command 5"
		},
		/* [9] */
		{152, 12, 167, 425},
		EditText {
			disabled,
			"Command 6"
		},
		/* [10] */
		{177, 12, 192, 425},
		EditText {
			disabled,
			"Command 7"
		},
		/* [11] */
		{202, 12, 217, 425},
		EditText {
			disabled,
			"Command 8"
		},
		/* [12] */
		{27, 435, 42, 450},
		CheckBox {
			enabled,
			""
		},
		/* [13] */
		{52, 435, 67, 450},
		CheckBox {
			enabled,
			""
		},
		/* [14] */
		{77, 435, 92, 450},
		CheckBox {
			enabled,
			""
		},
		/* [15] */
		{102, 435, 117, 450},
		CheckBox {
			enabled,
			""
		},
		/* [16] */
		{127, 435, 142, 450},
		CheckBox {
			enabled,
			""
		},
		/* [17] */
		{152, 435, 167, 450},
		CheckBox {
			enabled,
			""
		},
		/* [18] */
		{177, 435, 192, 450},
		CheckBox {
			enabled,
			""
		},
		/* [19] */
		{202, 435, 217, 450},
		CheckBox {
			enabled,
			""
		},
		/* [20] */
		{3, 12, 18, 425},
		UserItem {
			disabled
		},
		/* [21] */
		{3, 430, 18, 459},
		UserItem {
			disabled
		}
	}
};

resource 'ALRT' (MYALERT, "MyAlert", purgeable) {
	{64, 36, 178, 480},
	MYALERT,
	{	/* array: 4 elements */
		/* [1] */
		OK, visible, sound1,
		/* [2] */
		OK, visible, sound1,
		/* [3] */
		OK, visible, sound1,
		/* [4] */
		OK, visible, sound1
	}
};

resource 'ALRT' (SAVEALERT, "SaveAlert", purgeable) {
	{94, 80, 183, 438},
	SAVEALERT,
	{	/* array: 4 elements */
		/* [1] */
		OK, visible, sound1,
		/* [2] */
		OK, visible, sound1,
		/* [3] */
		OK, visible, sound1,
		/* [4] */
		OK, visible, sound1
	}
};


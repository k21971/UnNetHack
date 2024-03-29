/* Copyright (C) 2001 by Alex Kompel <shurikk@pacbell.net> */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MHNethackMessages_H
#define MHNethackMessages_H

/* nethack messages */
#define WM_MSNH_COMMAND  (WM_APP+1)

#define MSNH_MSG_ADDWND			100
#define MSNH_MSG_PUTSTR			101
#define MSNH_MSG_PRINT_GLYPH	102
#define MSNH_MSG_CLEAR_WINDOW	103
#define MSNH_MSG_CLIPAROUND		104
#define MSNH_MSG_STARTMENU		105
#define MSNH_MSG_ADDMENU		106
#define MSNH_MSG_CURSOR			107
#define MSNH_MSG_ENDMENU		108
#define MSNH_MSG_DIED			109
#define MSNH_MSG_CARET			110

typedef struct mswin_nhmsg_add_wnd {
  winid		  wid;
} MSNHMsgAddWnd, *PMSNHMsgAddWnd;

typedef struct mswin_nhmsg_putstr {
  int		  attr;
  const char* text;
  boolean	  append;
} MSNHMsgPutstr, *PMSNHMsgPutstr;

typedef struct mswin_nhmsg_print_glyph {
  coordxy x;
  coordxy y;
  int glyph;
} MSNHMsgPrintGlyph, *PMSNHMsgPrintGlyph;

typedef struct mswin_nhmsg_cliparound {
  int			x;
  int			y;
} MSNHMsgClipAround, *PMSNHMsgClipAround;

typedef struct mswin_nhmsg_add_menu {
	int				glyph;
	const ANY_P*	identifier;
	CHAR_P			accelerator;
	CHAR_P			group_accel;
	int				attr;
	const char *	str;
	BOOLEAN_P		presel;
} MSNHMsgAddMenu, *PMSNHMsgAddMenu;

typedef struct mswin_nhmsg_cursor {
  int			x;
  int			y;
} MSNHMsgCursor, *PMSNHMsgCursor;

typedef struct mswin_nhmsg_end_menu {
	const char* text;
} MSNHMsgEndMenu, *PMSNHMsgEndMenu;

#endif

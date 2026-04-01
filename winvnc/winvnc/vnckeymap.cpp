// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncKeymap.cpp

// This code originally just mapped between X keysyms and local Windows
// virtual keycodes. Now it actually does the local-end simulation of
// key presses, to keep this messy code on one place!
#pragma warning(disable : 4786)
#include <stdlib.h>
#include "vnckeymap.h"
#include <rdr/types.h>
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_CURRENCY
#define XK_GREEK
#define XK_TECHNICAL
#define XK_XKB_KEYS
#define XK_KOREAN

//	[v1.0.2-jp1 fix] IOport's patch (Define XK_KATAKANA)
#define XK_KATAKANA

#include "keysymdef.h"
#include "uvncUiAccess.h"
#include "cadthread.h"
//	[v1.0.2-jp1 fix] IOport's patch (Include "ime.h" for IME control)
#include <ime.h>
#include <map>
#include <vector>
#include "SettingsManager.h"

// Mapping of X keysyms to windows VK codes. Ordering here is the same as
// keysymdef.h to make checking easier
rdr::U32 keysymDead=0;

struct keymap_t {
  rdr::U32 keysym;
  rdr::U8 vk;
  bool extended;
};

std::vector<unsigned char> deadChars;

static keymap_t keymap[] = {

  // TTY functions

  { XK_BackSpace,        VK_BACK, 0 },
  { XK_Tab,              VK_TAB, 0 },
  { XK_Clear,            VK_CLEAR, 0 },
  { XK_Return,           VK_RETURN, 0 },
  { XK_Pause,            VK_PAUSE, 0 },
  { XK_Escape,           VK_ESCAPE, 0 },
  { XK_Delete,           VK_DELETE, 1 },

  // Japanese stuff - almost certainly wrong...
  //	[v1.0.2-jp1 fix] IOport's patch (Correct definition of Japanese key)
  //{ XK_Kanji,            VK_KANJI, 0 },
  //{ XK_Kana_Shift,       VK_KANA, 0 },
  // Japanese key
  { XK_Kanji,            VK_KANJI, 0 },				/* 0x19: Kanji, Kanji convert */
  { XK_Muhenkan,         VK_NONCONVERT, 0 },		/* 0x1d: Cancel Conversion */
  { XK_Romaji,           VK_DBE_ROMAN, 0 },			/* 0xf5: to Romaji */
  { XK_Hiragana,         VK_DBE_HIRAGANA, 0 },		/* 0xf2: to Hiragana */
  { XK_Katakana,         VK_DBE_KATAKANA, 0 },		/* 0xf1: to Katakana */
  { XK_Zenkaku,          VK_DBE_SBCSCHAR, 0 },		/* 0xf3: to Zenkaku */
  { XK_Hankaku,          VK_DBE_DBCSCHAR, 0 },		/* 0xf4: to Hankaku */
  { XK_Eisu_toggle,      VK_DBE_ALPHANUMERIC, 0 },	/* 0xf0: Alphanumeric toggle */
  { XK_Mae_Koho,         VK_CONVERT, 0 },			/* 0x1c: Previous Candidate */

  // Cursor control & motion

  { XK_Home,             VK_HOME, 1 },
  { XK_Left,             VK_LEFT, 1 },
  { XK_Up,               VK_UP, 1 },
  { XK_Right,            VK_RIGHT, 1 },
  { XK_Down,             VK_DOWN, 1 },
  { XK_Page_Up,          VK_PRIOR, 1 },
  { XK_Page_Down,        VK_NEXT, 1 },
  { XK_End,              VK_END, 1 },

  // Misc functions

  { XK_Select,           VK_SELECT, 0 },
  { XK_Print,            VK_SNAPSHOT, 0 },
  { XK_Execute,          VK_EXECUTE, 0 },
  { XK_Insert,           VK_INSERT, 1 },
  { XK_Help,             VK_HELP, 0 },
  { XK_Break,            VK_CANCEL, 1 },

  // Keypad Functions, keypad numbers

  { XK_KP_Space,         VK_SPACE, 0 },
  { XK_KP_Tab,           VK_TAB, 0 },
  { XK_KP_Enter,         VK_RETURN, 1 },
  { XK_KP_F1,            VK_F1, 0 },
  { XK_KP_F2,            VK_F2, 0 },
  { XK_KP_F3,            VK_F3, 0 },
  { XK_KP_F4,            VK_F4, 0 },
  { XK_KP_Home,          VK_HOME, 0 },
  { XK_KP_Left,          VK_LEFT, 0 },
  { XK_KP_Up,            VK_UP, 0 },
  { XK_KP_Right,         VK_RIGHT, 0 },
  { XK_KP_Down,          VK_DOWN, 0 },
  { XK_KP_End,           VK_END, 0 },
  { XK_KP_Page_Up,       VK_PRIOR, 0 },
  { XK_KP_Page_Down,     VK_NEXT, 0 },
  { XK_KP_Begin,         VK_CLEAR, 0 },
  { XK_KP_Insert,        VK_INSERT, 0 },
  { XK_KP_Delete,        VK_DELETE, 0 },
  // XXX XK_KP_Equal should map in the same way as ascii '='
  { XK_KP_Multiply,      VK_MULTIPLY, 0 },
  { XK_KP_Add,           VK_ADD, 0 },
  { XK_KP_Separator,     VK_SEPARATOR, 0 },
  { XK_KP_Subtract,      VK_SUBTRACT, 0 },
  { XK_KP_Decimal,       VK_DECIMAL, 0 },
  { XK_KP_Divide,        VK_DIVIDE, 1 },

  { XK_KP_0,             VK_NUMPAD0, 0 },
  { XK_KP_1,             VK_NUMPAD1, 0 },
  { XK_KP_2,             VK_NUMPAD2, 0 },
  { XK_KP_3,             VK_NUMPAD3, 0 },
  { XK_KP_4,             VK_NUMPAD4, 0 },
  { XK_KP_5,             VK_NUMPAD5, 0 },
  { XK_KP_6,             VK_NUMPAD6, 0 },
  { XK_KP_7,             VK_NUMPAD7, 0 },
  { XK_KP_8,             VK_NUMPAD8, 0 },
  { XK_KP_9,             VK_NUMPAD9, 0 },

  // Auxilliary Functions

  { XK_F1,               VK_F1, 0 },
  { XK_F2,               VK_F2, 0 },
  { XK_F3,               VK_F3, 0 },
  { XK_F4,               VK_F4, 0 },
  { XK_F5,               VK_F5, 0 },
  { XK_F6,               VK_F6, 0 },
  { XK_F7,               VK_F7, 0 },
  { XK_F8,               VK_F8, 0 },
  { XK_F9,               VK_F9, 0 },
  { XK_F10,              VK_F10, 0 },
  { XK_F11,              VK_F11, 0 },
  { XK_F12,              VK_F12, 0 },
  { XK_F13,              VK_F13, 0 },
  { XK_F14,              VK_F14, 0 },
  { XK_F15,              VK_F15, 0 },
  { XK_F16,              VK_F16, 0 },
  { XK_F17,              VK_F17, 0 },
  { XK_F18,              VK_F18, 0 },
  { XK_F19,              VK_F19, 0 },
  { XK_F20,              VK_F20, 0 },
  { XK_F21,              VK_F21, 0 },
  { XK_F22,              VK_F22, 0 },
  { XK_F23,              VK_F23, 0 },
  { XK_F24,              VK_F24, 0 },

    // Modifiers
    
  { XK_Shift_L,          VK_SHIFT, 0 },
  { XK_Shift_R,          VK_RSHIFT, 0 },
  { XK_Control_L,        VK_CONTROL, 0 },
  { XK_Control_R,        VK_CONTROL, 1 },
  { XK_Alt_L,            VK_MENU, 0 },
  { XK_Alt_R,            VK_RMENU, 1 },

// Left & Right Windows keys & Windows Menu Key

  { XK_Super_L,			VK_LWIN, 0 }, 
  { XK_Super_R,			VK_RWIN, 0 }, 
  { XK_Menu,			VK_APPS, 0 }, 

  // Adding Hangul Key
  { XK_Hangul,			VK_HANGEUL, 0 },
};

struct latin1ToDeadChars_t {
  rdr::U8 latin1Char;
  rdr::U8 deadChar;
  rdr::U8 baseChar;
  int a,b,c;
};

latin1ToDeadChars_t latin1ToDeadChars[] = {

  { XK_Agrave, XK_grave, XK_A },
  { XK_Egrave, XK_grave, XK_E },
  { XK_Igrave, XK_grave, XK_I },
  { XK_Ograve, XK_grave, XK_O },
  { XK_Ugrave, XK_grave, XK_U },
  { XK_agrave, XK_grave, XK_a },
  { XK_egrave, XK_grave, XK_e },
  { XK_igrave, XK_grave, XK_i },
  { XK_ograve, XK_grave, XK_o},
  { XK_ugrave, XK_grave, XK_u },

  { XK_Aacute, XK_acute, XK_A },
  { XK_Eacute, XK_acute, XK_E },
  { XK_Iacute, XK_acute, XK_I },
  { XK_Oacute, XK_acute, XK_O },
  { XK_Uacute, XK_acute, XK_U },
  { XK_Yacute, XK_acute, XK_Y },
  { XK_aacute, XK_acute, XK_a },
  { XK_eacute, XK_acute, XK_e },
  { XK_iacute, XK_acute, XK_i },
  { XK_oacute, XK_acute, XK_o},
  { XK_uacute, XK_acute, XK_u },
  { XK_yacute, XK_acute, XK_y },

  { XK_Acircumflex, XK_asciicircum, XK_A },
  { XK_Ecircumflex, XK_asciicircum, XK_E },
  { XK_Icircumflex, XK_asciicircum, XK_I },
  { XK_Ocircumflex, XK_asciicircum, XK_O },
  { XK_Ucircumflex, XK_asciicircum, XK_U },
  { XK_acircumflex, XK_asciicircum, XK_a },
  { XK_ecircumflex, XK_asciicircum, XK_e },
  { XK_icircumflex, XK_asciicircum, XK_i },
  { XK_ocircumflex, XK_asciicircum, XK_o},
  { XK_ucircumflex, XK_asciicircum, XK_u },

  { XK_Adiaeresis, XK_diaeresis, XK_A },
  { XK_Ediaeresis, XK_diaeresis, XK_E },
  { XK_Idiaeresis, XK_diaeresis, XK_I },
  { XK_Odiaeresis, XK_diaeresis, XK_O },
  { XK_Udiaeresis, XK_diaeresis, XK_U },
  { XK_adiaeresis, XK_diaeresis, XK_a },
  { XK_ediaeresis, XK_diaeresis, XK_e },
  { XK_idiaeresis, XK_diaeresis, XK_i },
  { XK_odiaeresis, XK_diaeresis, XK_o},
  { XK_udiaeresis, XK_diaeresis, XK_u },
  { XK_ydiaeresis, XK_diaeresis, XK_y },

  { XK_Aring, XK_degree, XK_A },
  { XK_aring, XK_degree, XK_a },

  { XK_Ccedilla, XK_cedilla, XK_C },
  { XK_ccedilla, XK_cedilla, XK_c },

  { XK_Atilde, XK_asciitilde, XK_A },
  { XK_Ntilde, XK_asciitilde, XK_N },
  { XK_Otilde, XK_asciitilde, XK_O },
  { XK_atilde, XK_asciitilde, XK_a },
  { XK_ntilde, XK_asciitilde, XK_n },
  { XK_otilde, XK_asciitilde, XK_o },
};

rdr::U8 latin1DeadChars[] = {
  XK_grave, XK_acute, XK_asciicircum, XK_diaeresis, XK_degree, XK_cedilla,
  XK_asciitilde
};

#define NoSymbol 0xfff
rdr::U16 ascii_to_x[256] = {
	NoSymbol,	NoSymbol,	NoSymbol,	XK_KP_Enter,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_Delete,	XK_Tab,		XK_Linefeed,	NoSymbol,
	NoSymbol,	XK_Return,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	XK_Escape,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	XK_space,	XK_exclam,	XK_quotedbl,	XK_numbersign,
	XK_dollar,	XK_percent,	XK_ampersand,	XK_apostrophe,
	XK_parenleft,	XK_parenright,	XK_asterisk,	XK_plus,
	XK_comma,	XK_minus,	XK_period,	XK_slash,
	XK_0,		XK_1,		XK_2,		XK_3,
	XK_4,		XK_5,		XK_6,		XK_7,
	XK_8,		XK_9,		XK_colon,	XK_semicolon,
	XK_less,	XK_equal,	XK_greater,	XK_question,
	XK_at,		XK_A,		XK_B,		XK_C,
	XK_D,		XK_E,		XK_F,		XK_G,
	XK_H,		XK_I,		XK_J,		XK_K,
	XK_L,		XK_M,		XK_N,		XK_O,
	XK_P,		XK_Q,		XK_R,		XK_S,
	XK_T,		XK_U,		XK_V,		XK_W,
	XK_X,		XK_Y,		XK_Z,		XK_bracketleft,
	XK_backslash,	XK_bracketright,XK_asciicircum,	XK_underscore,
	XK_grave,	XK_a,		XK_b,		XK_c,
	XK_d,		XK_e,		XK_f,		XK_g,
	XK_h,		XK_i,		XK_j,		XK_k,
	XK_l,		XK_m,		XK_n,		XK_o,
	XK_p,		XK_q,		XK_r,		XK_s,
	XK_t,		XK_u,		XK_v,		XK_w,
	XK_x,		XK_y,		XK_z,		XK_braceleft,
	XK_bar,		XK_braceright,	XK_asciitilde,	XK_BackSpace,
// 128
	XK_Ccedilla,	XK_udiaeresis,	XK_eacute,	XK_acircumflex,
	XK_adiaeresis,	XK_agrave,	XK_aring,	XK_ccedilla,
	XK_ecircumflex,	XK_ediaeresis,	XK_egrave,	XK_idiaeresis,
	XK_icircumflex,	XK_igrave,	XK_Adiaeresis,	XK_Aring,
	XK_Eacute,	XK_ae,		XK_AE,		XK_ocircumflex,
	XK_odiaeresis,	XK_ograve,	XK_ntilde,	XK_ugrave,
	XK_ydiaeresis,	XK_Odiaeresis,	XK_Udiaeresis,	XK_cent,
	XK_sterling,	XK_yen,		XK_paragraph,	XK_section,
// 160
	XK_aacute,	XK_degree,	XK_cent,	XK_sterling,
	XK_ntilde,	XK_Ntilde,	XK_paragraph,	XK_Greek_BETA,
	XK_questiondown,XK_hyphen,	XK_notsign,	XK_onehalf,
	XK_onequarter,	XK_exclamdown,	XK_guillemotleft,XK_guillemotright,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
// 192
	XK_questiondown,XK_exclamdown,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	XK_Agrave,
	NoSymbol,	NoSymbol,	XK_AE,		XK_ae,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
// 224
	XK_Greek_alpha,	XK_ssharp,	XK_Greek_GAMMA,	XK_Greek_pi,
	XK_Greek_SIGMA,	XK_Greek_sigma,	XK_mu,	        XK_Greek_tau,
	XK_Greek_PHI,	XK_Greek_THETA,	XK_Greek_OMEGA,	XK_Greek_delta,
	XK_infinity,	XK_Ooblique,	XK_Greek_epsilon, XK_intersection,
	XK_identical,	XK_plusminus,	XK_greaterthanequal, XK_lessthanequal,
	XK_topintegral,	XK_botintegral,	XK_division,	XK_similarequal,
	XK_degree,	NoSymbol,	NoSymbol,	XK_radical,
	XK_Greek_eta,	XK_twosuperior,	XK_periodcentered, NoSymbol,
  };




// doKeyboardEvent wraps the system keybd_event function and attempts to find
// the appropriate scancode corresponding to the supplied virtual keycode.

inline void doKeyboardEvent(BYTE vkCode, DWORD flags) {
  keybd_uni_event(vkCode, MapVirtualKey(vkCode, 0), flags, 0);
}

// KeyStateModifier is a class which helps simplify generating a "fake" press
// or release of shift, ctrl, alt, etc. An instance of the class is created
// for every key which may need to be pressed or released. Then either press()
// or release() may be called to make sure that the corresponding key is in the
// right state. The destructor of the class automatically reverts to the
// previous state.

class KeyStateModifier {
public:
  KeyStateModifier(int vkCode_, int flags_=0)
    : vkCode(vkCode_), flags(flags_), pressed(false), released(false)
  {}
  void press() {
    if (!(GetAsyncKeyState(vkCode) & 0x8000)) {
      doKeyboardEvent(vkCode, flags);
      vnclog.Print(LL_INTINFO, "fake %d down\n", vkCode);
      pressed = true;
    }
  }
  void release() {
    if (GetAsyncKeyState(vkCode) & 0x8000) {
      doKeyboardEvent(vkCode, flags | KEYEVENTF_KEYUP);
      vnclog.Print(LL_INTINFO, "fake %d up\n", vkCode);
      released = true;
    }
  }
  ~KeyStateModifier() {
    if (pressed) {
      doKeyboardEvent(vkCode, flags | KEYEVENTF_KEYUP);
      vnclog.Print(LL_INTINFO, "fake %d up\n", vkCode);
    } else if (released) {
      doKeyboardEvent(vkCode, flags);
      vnclog.Print(LL_INTINFO, "fake %d down\n", vkCode);
    }
  }
  int vkCode;
  int flags;
  bool pressed;
  bool released;
};

// Keymapper - a single instance of this class is used to generate Windows key
// events.
void doKeyEventWithModifiers(BYTE vkCode, BYTE modifierState, bool down)
{
  KeyStateModifier ctrl(VK_CONTROL);
  KeyStateModifier alt(VK_MENU);
  KeyStateModifier shift(VK_SHIFT);

  if (down) {
    if (modifierState & 2) ctrl.press();
    if (modifierState & 4) alt.press();
    if (modifierState & 1) {
      shift.press(); 
    } else {
      shift.release();
    }
  }
  doKeyboardEvent(vkCode, down ? 0 : KEYEVENTF_KEYUP);
}

// X11 keysym to Unicode conversion tables
// Identical to viewer KeyMap.h - sourced from X.Org/XFree86 keysym tables
static unsigned short const ks_to_ucs_1a1_1ff[] = {
            0x0104, 0x02d8, 0x0141, 0x0000, 0x013d, 0x015a, 0x0000, /* 0x01a1-0x01a7 */
    0x0000, 0x0160, 0x015e, 0x0164, 0x0179, 0x0000, 0x017d, 0x017b, /* 0x01a8-0x01af */
    0x0000, 0x0105, 0x02db, 0x0142, 0x0000, 0x013e, 0x015b, 0x02c7, /* 0x01b0-0x01b7 */
    0x0000, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, /* 0x01b8-0x01bf */
    0x0154, 0x0000, 0x0000, 0x0102, 0x0000, 0x0139, 0x0106, 0x0000, /* 0x01c0-0x01c7 */
    0x010c, 0x0000, 0x0118, 0x0000, 0x011a, 0x0000, 0x0000, 0x010e, /* 0x01c8-0x01cf */
    0x0110, 0x0143, 0x0147, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, /* 0x01d0-0x01d7 */
    0x0158, 0x016e, 0x0000, 0x0170, 0x0000, 0x0000, 0x0162, 0x0000, /* 0x01d8-0x01df */
    0x0155, 0x0000, 0x0000, 0x0103, 0x0000, 0x013a, 0x0107, 0x0000, /* 0x01e0-0x01e7 */
    0x010d, 0x0000, 0x0119, 0x0000, 0x011b, 0x0000, 0x0000, 0x010f, /* 0x01e8-0x01ef */
    0x0111, 0x0144, 0x0148, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, /* 0x01f0-0x01f7 */
    0x0159, 0x016f, 0x0000, 0x0171, 0x0000, 0x0000, 0x0163, 0x02d9  /* 0x01f8-0x01ff */
};
static unsigned short const ks_to_ucs_2a1_2fe[] = {
            0x0126, 0x0000, 0x0000, 0x0000, 0x0000, 0x0124, 0x0000, /* 0x02a1-0x02a7 */
    0x0000, 0x0130, 0x0000, 0x011e, 0x0134, 0x0000, 0x0000, 0x0000, /* 0x02a8-0x02af */
    0x0000, 0x0127, 0x0000, 0x0000, 0x0000, 0x0000, 0x0125, 0x0000, /* 0x02b0-0x02b7 */
    0x0000, 0x0131, 0x0000, 0x011f, 0x0135, 0x0000, 0x0000, 0x0000, /* 0x02b8-0x02bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010a, 0x0108, 0x0000, /* 0x02c0-0x02c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02c8-0x02cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0120, 0x0000, 0x0000, /* 0x02d0-0x02d7 */
    0x011c, 0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x015c, 0x0000, /* 0x02d8-0x02df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010b, 0x0109, 0x0000, /* 0x02e0-0x02e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02e8-0x02ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0121, 0x0000, 0x0000, /* 0x02f0-0x02f7 */
    0x011d, 0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x015d          /* 0x02f8-0x02fe */
};
static unsigned short const ks_to_ucs_3a2_3fe[] = {
                    0x0138, 0x0156, 0x0000, 0x0128, 0x013b, 0x0000, /* 0x03a2-0x03a7 */
    0x0000, 0x0000, 0x0112, 0x0122, 0x0166, 0x0000, 0x0000, 0x0000, /* 0x03a8-0x03af */
    0x0000, 0x0000, 0x0000, 0x0157, 0x0000, 0x0129, 0x013c, 0x0000, /* 0x03b0-0x03b7 */
    0x0000, 0x0000, 0x0113, 0x0123, 0x0167, 0x014a, 0x0000, 0x014b, /* 0x03b8-0x03bf */
    0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012e, /* 0x03c0-0x03c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0116, 0x0000, 0x0000, 0x012a, /* 0x03c8-0x03cf */
    0x0000, 0x0145, 0x014c, 0x0136, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03d0-0x03d7 */
    0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0168, 0x016a, 0x0000, /* 0x03d8-0x03df */
    0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012f, /* 0x03e0-0x03e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0117, 0x0000, 0x0000, 0x012b, /* 0x03e8-0x03ef */
    0x0000, 0x0146, 0x014d, 0x0137, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03f0-0x03f7 */
    0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0169, 0x016b          /* 0x03f8-0x03fe */
};
static unsigned short const ks_to_ucs_4a1_4df[] = {
            0x3002, 0x3008, 0x3009, 0x3001, 0x30fb, 0x30f2, 0x30a1, /* 0x04a1-0x04a7 */
    0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3, /* 0x04a8-0x04af */
    0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad, /* 0x04b0-0x04b7 */
    0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd, /* 0x04b8-0x04bf */
    0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc, /* 0x04c0-0x04c7 */
    0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, /* 0x04c8-0x04cf */
    0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, /* 0x04d0-0x04d7 */
    0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c  /* 0x04d8-0x04df */
};
static unsigned short const ks_to_ucs_590_5fe[] = {
    0x06f0, 0x06f1, 0x06f2, 0x06f3, 0x06f4, 0x06f5, 0x06f6, 0x06f7, /* 0x0590-0x0597 */
    0x06f8, 0x06f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x0598-0x059f */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x066a, 0x0670, 0x0679, /* 0x05a0-0x05a7 */
    0x067e, 0x0686, 0x0688, 0x0691, 0x060c, 0x0000, 0x06d4, 0x0000, /* 0x05a8-0x05af */
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, /* 0x05b0-0x05b7 */
    0x0668, 0x0669, 0x0000, 0x061b, 0x0000, 0x0000, 0x0000, 0x061f, /* 0x05b8-0x05bf */
    0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, /* 0x05c0-0x05c7 */
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, /* 0x05c8-0x05cf */
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, /* 0x05d0-0x05d7 */
    0x0638, 0x0639, 0x063a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x05d8-0x05df */
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, /* 0x05e0-0x05e7 */
    0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, /* 0x05e8-0x05ef */
    0x0650, 0x0651, 0x0652, 0x0653, 0x0654, 0x0655, 0x0698, 0x06a4, /* 0x05f0-0x05f7 */
    0x06a9, 0x06af, 0x06ba, 0x06be, 0x06cc, 0x06d2, 0x06c1          /* 0x05f8-0x05fe */
};
static unsigned short const ks_to_ucs_680_6ff[] = {
    0x0492, 0x0496, 0x049a, 0x049c, 0x04a2, 0x04ae, 0x04b0, 0x04b2, /* 0x0680-0x0687 */
    0x04b6, 0x04b8, 0x04ba, 0x0000, 0x04d8, 0x04e2, 0x04e8, 0x04ee, /* 0x0688-0x068f */
    0x0493, 0x0497, 0x049b, 0x049d, 0x04a3, 0x04af, 0x04b1, 0x04b3, /* 0x0690-0x0697 */
    0x04b7, 0x04b9, 0x04bb, 0x0000, 0x04d9, 0x04e3, 0x04e9, 0x04ef, /* 0x0698-0x069f */
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, /* 0x06a0-0x06a7 */
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0491, 0x045e, 0x045f, /* 0x06a8-0x06af */
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, /* 0x06b0-0x06b7 */
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0490, 0x040e, 0x040f, /* 0x06b8-0x06bf */
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, /* 0x06c0-0x06c7 */
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, /* 0x06c8-0x06cf */
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, /* 0x06d0-0x06d7 */
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a, /* 0x06d8-0x06df */
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, /* 0x06e0-0x06e7 */
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, /* 0x06e8-0x06ef */
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, /* 0x06f0-0x06f7 */
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a  /* 0x06f8-0x06ff */
};
static unsigned short const ks_to_ucs_7a1_7f9[] = {
            0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c, /* 0x07a1-0x07a7 */
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015, /* 0x07a8-0x07af */
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc, /* 0x07b0-0x07b7 */
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07b8-0x07bf */
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, /* 0x07c0-0x07c7 */
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, /* 0x07c8-0x07cf */
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7, /* 0x07d0-0x07d7 */
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07d8-0x07df */
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, /* 0x07e0-0x07e7 */
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, /* 0x07e8-0x07ef */
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, /* 0x07f0-0x07f7 */
    0x03c8, 0x03c9                                                   /* 0x07f8-0x07f9 */
};
static unsigned short const ks_to_ucs_8a4_8fe[] = {
                                    0x2320, 0x2321, 0x0000, 0x231c, /* 0x08a4-0x08a7 */
    0x231d, 0x231e, 0x231f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08a8-0x08af */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08b0-0x08b7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222b, /* 0x08b8-0x08bf */
    0x2234, 0x0000, 0x221e, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000, /* 0x08c0-0x08c7 */
    0x2245, 0x2246, 0x0000, 0x0000, 0x0000, 0x0000, 0x22a2, 0x0000, /* 0x08c8-0x08cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221a, 0x0000, /* 0x08d0-0x08d7 */
    0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228, /* 0x08d8-0x08df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e0-0x08e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e8-0x08ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000, /* 0x08f0-0x08f7 */
    0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193          /* 0x08f8-0x08fe */
};
static unsigned short const ks_to_ucs_9df_9f8[] = {
                                                            0x2422, /* 0x09df       */
    0x2666, 0x25a6, 0x2409, 0x240c, 0x240d, 0x240a, 0x0000, 0x0000, /* 0x09e0-0x09e7 */
    0x240a, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x2500, /* 0x09e8-0x09ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x251c, 0x2524, 0x2534, 0x252c, /* 0x09f0-0x09f7 */
    0x2502                                                           /* 0x09f8       */
};
static unsigned short const ks_to_ucs_aa1_afe[] = {
            0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009, /* 0x0aa1-0x0aa7 */
    0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025, /* 0x0aa8-0x0aaf */
    0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a, /* 0x0ab0-0x0ab7 */
    0x2105, 0x0000, 0x0000, 0x2012, 0x2039, 0x2024, 0x203a, 0x0000, /* 0x0ab8-0x0abf */
    0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000, /* 0x0ac0-0x0ac7 */
    0x0000, 0x2122, 0x2120, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25ad, /* 0x0ac8-0x0acf */
    0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033, /* 0x0ad0-0x0ad7 */
    0x0000, 0x271d, 0x0000, 0x220e, 0x25c2, 0x2023, 0x25cf, 0x25ac, /* 0x0ad8-0x0adf */
    0x25e6, 0x25ab, 0x25ae, 0x25b5, 0x25bf, 0x2606, 0x2022, 0x25aa, /* 0x0ae0-0x0ae7 */
    0x25b4, 0x25be, 0x261a, 0x261b, 0x2663, 0x2666, 0x2665, 0x0000, /* 0x0ae8-0x0aef */
    0x2720, 0x2020, 0x2021, 0x2713, 0x2612, 0x266f, 0x266d, 0x2642, /* 0x0af0-0x0af7 */
    0x2640, 0x2121, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e          /* 0x0af8-0x0afe */
};
static unsigned short const ks_to_ucs_cdf_cfa[] = {
                                                            0x2017, /* 0x0cdf       */
    0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, /* 0x0ce0-0x0ce7 */
    0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, /* 0x0ce8-0x0cef */
    0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, /* 0x0cf0-0x0cf7 */
    0x05e8, 0x05e9, 0x05ea                                           /* 0x0cf8-0x0cfa */
};
static unsigned short const ks_to_ucs_da1_df9[] = {
            0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, /* 0x0da1-0x0da7 */
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, /* 0x0da8-0x0daf */
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, /* 0x0db0-0x0db7 */
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, /* 0x0db8-0x0dbf */
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, /* 0x0dc0-0x0dc7 */
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, /* 0x0dc8-0x0dcf */
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, /* 0x0dd0-0x0dd7 */
    0x0e38, 0x0e39, 0x0e3a, 0x0000, 0x0000, 0x0000, 0x0e3e, 0x0e3f, /* 0x0dd8-0x0ddf */
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, /* 0x0de0-0x0de7 */
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0000, 0x0000, /* 0x0de8-0x0def */
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, /* 0x0df0-0x0df7 */
    0x0e58, 0x0e59                                                   /* 0x0df8-0x0df9 */
};
static unsigned short const ks_to_ucs_ea0_eff[] = {
    0x0000, 0x1101, 0x1101, 0x11aa, 0x1102, 0x11ac, 0x11ad, 0x1103, /* 0x0ea0-0x0ea7 */
    0x1104, 0x1105, 0x11b0, 0x11b1, 0x11b2, 0x11b3, 0x11b4, 0x11b5, /* 0x0ea8-0x0eaf */
    0x11b6, 0x1106, 0x1107, 0x1108, 0x11b9, 0x1109, 0x110a, 0x110b, /* 0x0eb0-0x0eb7 */
    0x110c, 0x110d, 0x110e, 0x110f, 0x1110, 0x1111, 0x1112, 0x1161, /* 0x0eb8-0x0ebf */
    0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167, 0x1168, 0x1169, /* 0x0ec0-0x0ec7 */
    0x116a, 0x116b, 0x116c, 0x116d, 0x116e, 0x116f, 0x1170, 0x1171, /* 0x0ec8-0x0ecf */
    0x1172, 0x1173, 0x1174, 0x1175, 0x11a8, 0x11a9, 0x11aa, 0x11ab, /* 0x0ed0-0x0ed7 */
    0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3, /* 0x0ed8-0x0edf */
    0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb, /* 0x0ee0-0x0ee7 */
    0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x0000, /* 0x0ee8-0x0eef */
    0x0000, 0x0000, 0x1140, 0x0000, 0x0000, 0x1159, 0x119e, 0x0000, /* 0x0ef0-0x0ef7 */
    0x11eb, 0x0000, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9  /* 0x0ef8-0x0eff */
};
static unsigned short const ks_to_ucs_12a1_12fe[] = {
            0x1e02, 0x1e03, 0x0000, 0x0000, 0x0000, 0x1e0a, 0x0000, /* 0x12a1-0x12a7 */
    0x1e80, 0x0000, 0x1e82, 0x1e0b, 0x1ef2, 0x0000, 0x0000, 0x0000, /* 0x12a8-0x12af */
    0x1e1e, 0x1e1f, 0x0000, 0x0000, 0x1e40, 0x1e41, 0x0000, 0x1e56, /* 0x12b0-0x12b7 */
    0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61, /* 0x12b8-0x12bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c0-0x12c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c8-0x12cf */
    0x0174, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6a, /* 0x12d0-0x12d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0176, 0x0000, /* 0x12d8-0x12df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e0-0x12e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e8-0x12ef */
    0x0175, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6b, /* 0x12f0-0x12f7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0177          /* 0x12f8-0x12fe */
};
static unsigned short const ks_to_ucs_13bc_13be[] = {
    0x0152, 0x0153, 0x0178                                           /* 0x13bc-0x13be */
};
static unsigned short const ks_to_ucs_14a1_14ff[] = {
            0x2741, 0x00a7, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab, /* 0x14a1-0x14a7 */
    0x2014, 0x002e, 0x055d, 0x002c, 0x2013, 0x058a, 0x2026, 0x055c, /* 0x14a8-0x14af */
    0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, /* 0x14b0-0x14b7 */
    0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, /* 0x14b8-0x14bf */
    0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b, /* 0x14c0-0x14c7 */
    0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f, /* 0x14c8-0x14cf */
    0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, /* 0x14d0-0x14d7 */
    0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, /* 0x14d8-0x14df */
    0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b, /* 0x14e0-0x14e7 */
    0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f, /* 0x14e8-0x14ef */
    0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583, /* 0x14f0-0x14f7 */
    0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x2019, 0x0027  /* 0x14f8-0x14ff */
};
static unsigned short const ks_to_ucs_15d0_15f6[] = {
    0x10d0, 0x10d1, 0x10d2, 0x10d3, 0x10d4, 0x10d5, 0x10d6, 0x10d7, /* 0x15d0-0x15d7 */
    0x10d8, 0x10d9, 0x10da, 0x10db, 0x10dc, 0x10dd, 0x10de, 0x10df, /* 0x15d8-0x15df */
    0x10e0, 0x10e1, 0x10e2, 0x10e3, 0x10e4, 0x10e5, 0x10e6, 0x10e7, /* 0x15e0-0x15e7 */
    0x10e8, 0x10e9, 0x10ea, 0x10eb, 0x10ec, 0x10ed, 0x10ee, 0x10ef, /* 0x15e8-0x15ef */
    0x10f0, 0x10f1, 0x10f2, 0x10f3, 0x10f4, 0x10f5, 0x10f6          /* 0x15f0-0x15f6 */
};
static unsigned short const ks_to_ucs_16a0_16f6[] = {
    0x0000, 0x0000, 0xf0a2, 0x1e8a, 0x0000, 0xf0a5, 0x012c, 0xf0a7, /* 0x16a0-0x16a7 */
    0xf0a8, 0x01b5, 0x01e6, 0x0000, 0x0000, 0x0000, 0x0000, 0x019f, /* 0x16a8-0x16af */
    0x0000, 0x0000, 0xf0b2, 0x1e8b, 0x01d1, 0xf0b5, 0x012d, 0xf0b7, /* 0x16b0-0x16b7 */
    0xf0b8, 0x01b6, 0x01e7, 0x0000, 0x0000, 0x01d2, 0x0000, 0x0275, /* 0x16b8-0x16bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x018f, 0x0000, /* 0x16c0-0x16c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16c8-0x16cf */
    0x0000, 0x1e36, 0xf0d2, 0xf0d3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d0-0x16d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d8-0x16df */
    0x0000, 0x1e37, 0xf0e2, 0xf0e3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e0-0x16e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e8-0x16ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0259          /* 0x16f0-0x16f6 */
};
static unsigned short const ks_to_ucs_1e9f_1eff[] = {
                                                            0x0303, /* 0x1e9f       */
    0x1ea0, 0x1ea1, 0x1ea2, 0x1ea3, 0x1ea4, 0x1ea5, 0x1ea6, 0x1ea7, /* 0x1ea0-0x1ea7 */
    0x1ea8, 0x1ea9, 0x1eaa, 0x1eab, 0x1eac, 0x1ead, 0x1eae, 0x1eaf, /* 0x1ea8-0x1eaf */
    0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3, 0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7, /* 0x1eb0-0x1eb7 */
    0x1eb8, 0x1eb9, 0x1eba, 0x1ebb, 0x1ebc, 0x1ebd, 0x1ebe, 0x1ebf, /* 0x1eb8-0x1ebf */
    0x1ec0, 0x1ec1, 0x1ec2, 0x1ec3, 0x1ec4, 0x1ec5, 0x1ec6, 0x1ec7, /* 0x1ec0-0x1ec7 */
    0x1ec8, 0x1ec9, 0x1eca, 0x1ecb, 0x1ecc, 0x1ecd, 0x1ece, 0x1ecf, /* 0x1ec8-0x1ecf */
    0x1ed0, 0x1ed1, 0x1ed2, 0x1ed3, 0x1ed4, 0x1ed5, 0x1ed6, 0x1ed7, /* 0x1ed0-0x1ed7 */
    0x1ed8, 0x1ed9, 0x1eda, 0x1edb, 0x1edc, 0x1edd, 0x1ede, 0x1edf, /* 0x1ed8-0x1edf */
    0x1ee0, 0x1ee1, 0x1ee2, 0x1ee3, 0x1ee4, 0x1ee5, 0x1ee6, 0x1ee7, /* 0x1ee0-0x1ee7 */
    0x1ee8, 0x1ee9, 0x1eea, 0x1eeb, 0x1eec, 0x1eed, 0x1eee, 0x1eef, /* 0x1ee8-0x1eef */
    0x1ef0, 0x1ef1, 0x0300, 0x0301, 0x1ef4, 0x1ef5, 0x1ef6, 0x1ef7, /* 0x1ef0-0x1ef7 */
    0x1ef8, 0x1ef9, 0x01a0, 0x01a1, 0x01af, 0x01b0, 0x0309, 0x0323  /* 0x1ef8-0x1eff */
};
static unsigned short const ks_to_ucs_20a0_20ac[] = {
    0x20a0, 0x20a1, 0x20a2, 0x20a3, 0x20a4, 0x20a5, 0x20a6, 0x20a7, /* 0x20a0-0x20a7 */
    0x20a8, 0x20a9, 0x20aa, 0x20ab, 0x20ac                           /* 0x20a8-0x20ac */
};

// Convert an X11 keysym to its Unicode codepoint.
// Returns 0 if no mapping exists.
// Handles Unicode keysyms (0x01000000 | UCS), Latin-1, and all
// non-Latin script ranges via the tables above.
static WORD keysymToUnicode(rdr::U32 keysym)
{
    if (keysym & 0x01000000)           return (WORD)(keysym & 0x00FFFFFF);
    if (keysym >= 0x0020 && keysym <= 0x00ff) return (WORD)keysym;
    if (keysym >= 0x01a1 && keysym <= 0x01ff) return ks_to_ucs_1a1_1ff[keysym - 0x01a1];
    if (keysym >= 0x02a1 && keysym <= 0x02fe) return ks_to_ucs_2a1_2fe[keysym - 0x02a1];
    if (keysym >= 0x03a2 && keysym <= 0x03fe) return ks_to_ucs_3a2_3fe[keysym - 0x03a2];
    if (keysym >= 0x04a1 && keysym <= 0x04df) return ks_to_ucs_4a1_4df[keysym - 0x04a1];
    if (keysym >= 0x0590 && keysym <= 0x05fe) return ks_to_ucs_590_5fe[keysym - 0x0590];
    if (keysym >= 0x0680 && keysym <= 0x06ff) return ks_to_ucs_680_6ff[keysym - 0x0680];
    if (keysym >= 0x07a1 && keysym <= 0x07f9) return ks_to_ucs_7a1_7f9[keysym - 0x07a1];
    if (keysym >= 0x08a4 && keysym <= 0x08fe) return ks_to_ucs_8a4_8fe[keysym - 0x08a4];
    if (keysym >= 0x09df && keysym <= 0x09f8) return ks_to_ucs_9df_9f8[keysym - 0x09df];
    if (keysym >= 0x0aa1 && keysym <= 0x0afe) return ks_to_ucs_aa1_afe[keysym - 0x0aa1];
    if (keysym >= 0x0cdf && keysym <= 0x0cfa) return ks_to_ucs_cdf_cfa[keysym - 0x0cdf];
    if (keysym >= 0x0da1 && keysym <= 0x0df9) return ks_to_ucs_da1_df9[keysym - 0x0da1];
    if (keysym >= 0x0ea0 && keysym <= 0x0eff) return ks_to_ucs_ea0_eff[keysym - 0x0ea0];
    if (keysym >= 0x12a1 && keysym <= 0x12fe) return ks_to_ucs_12a1_12fe[keysym - 0x12a1];
    if (keysym >= 0x13bc && keysym <= 0x13be) return ks_to_ucs_13bc_13be[keysym - 0x13bc];
    if (keysym >= 0x14a1 && keysym <= 0x14ff) return ks_to_ucs_14a1_14ff[keysym - 0x14a1];
    if (keysym >= 0x15d0 && keysym <= 0x15f6) return ks_to_ucs_15d0_15f6[keysym - 0x15d0];
    if (keysym >= 0x16a0 && keysym <= 0x16f6) return ks_to_ucs_16a0_16f6[keysym - 0x16a0];
    if (keysym >= 0x1e9f && keysym <= 0x1eff) return ks_to_ucs_1e9f_1eff[keysym - 0x1e9f];
    if (keysym >= 0x20a0 && keysym <= 0x20ac) return ks_to_ucs_20a0_20ac[keysym - 0x20a0];
    return 0;
}

class Keymapper {

public:
  Keymapper()
  {
    for (int i = 0; i < sizeof(keymap) / sizeof(keymap_t); i++) {
      vkMap[keymap[i].keysym] = keymap[i].vk;
      extendedMap[keymap[i].keysym] = keymap[i].extended;
    }

	// Find dead characters for the current keyboard layout
  // XXX how could we handle the keyboard layout changing?
  BYTE keystate[256];
  memset(keystate, 0, 256);
  for (int j = 0; j < sizeof(latin1DeadChars); j++) {
    SHORT s = VkKeyScan(latin1DeadChars[j]);
    if (s != -1) {
      BYTE vkCode = LOBYTE(s);
      BYTE modifierState = HIBYTE(s);
      keystate[VK_SHIFT] = (modifierState & 1) ? 0x80 : 0;
      keystate[VK_CONTROL] = (modifierState & 2) ? 0x80 : 0;
      keystate[VK_MENU] = (modifierState & 4) ? 0x80 : 0;
      rdr::U8 chars[2];
      int nchars = ToAscii(vkCode, 0, keystate, (WORD*)&chars, 0);
      if (nchars < 0) {
        vnclog.Print(LL_INTWARN, "Found dead key 0x%x '%c'",
                   latin1DeadChars[j], latin1DeadChars[j]);
        deadChars.push_back(latin1DeadChars[j]);
        ToAscii(vkCode, 0, keystate, (WORD*)&chars, 0);
      }
    }
  }

  }


  

  




  void keyEvent(rdr::U32 keysym, bool down, bool jap, bool unicode)
  {
	  vnclog.Print(LL_INTWARN, " keysym 0x%x",keysym);
	if (keysym>=XK_dead_grave && keysym <=XK_dead_belowdot)// && down)
	{
		keysymDead=keysym;
		vnclog.Print(LL_INTWARN, " ************** DEAD KEY");
		//we have a dead key
		//Record dead key
		return;
	}

    if ((keysym >= 32 && keysym <= 126) ||
        (keysym >= 160 && keysym <= 255))
    {
	if (keysymDead!=0 && down)
	{
		vnclog.Print(LL_INTWARN, " Compose dead 0x%x 0x%x",keysymDead,keysym);
		switch (keysymDead)
		{
		case XK_dead_grave:
			switch(keysym)
			{
			case XK_A: keysym=XK_Agrave;break;
			case XK_E: keysym=XK_Egrave;break;
			case XK_I: keysym=XK_Igrave;break;
			case XK_O: keysym=XK_Ograve;break;
			case XK_U: keysym=XK_Ugrave;break;
			case XK_a: keysym=XK_agrave;break;
			case XK_e: keysym=XK_egrave;break;
			case XK_i: keysym=XK_igrave;break;
			case XK_o: keysym=XK_ograve;break;
			case XK_u: keysym=XK_ugrave;break;
			}
		case XK_dead_acute:
			switch(keysym)
			{
			case XK_A: keysym=XK_Aacute;break;
			case XK_E: keysym=XK_Eacute;break;
			case XK_I: keysym=XK_Iacute;break;
			case XK_O: keysym=XK_Oacute;break;
			case XK_U: keysym=XK_Uacute;break;
			case XK_a: keysym=XK_aacute;break;
			case XK_e: keysym=XK_eacute;break;
			case XK_i: keysym=XK_iacute;break;
			case XK_o: keysym=XK_oacute;break;
			case XK_u: keysym=XK_uacute;break;
			case XK_y: keysym=XK_yacute;break;
			case XK_Y: keysym=XK_Yacute;break;

			}
		case XK_dead_circumflex:
			switch(keysym)
			{
			case XK_A: keysym=XK_Acircumflex;break;
			case XK_E: keysym=XK_Ecircumflex;break;
			case XK_I: keysym=XK_Icircumflex;break;
			case XK_O: keysym=XK_Ocircumflex;break;
			case XK_U: keysym=XK_Ucircumflex;break;
			case XK_a: keysym=XK_acircumflex;break;
			case XK_e: keysym=XK_ecircumflex;break;
			case XK_i: keysym=XK_icircumflex;break;
			case XK_o: keysym=XK_ocircumflex;break;
			case XK_u: keysym=XK_ucircumflex;break;
			}
		case XK_dead_tilde:
			switch(keysym)
			{
			case XK_A : keysym=XK_Ntilde;break;
			case XK_O : keysym=XK_Otilde;break;
			case XK_a : keysym=XK_atilde;break;
			case XK_n : keysym=XK_ntilde;break;
			case XK_o : keysym=XK_otilde;break;
			}

		case XK_dead_diaeresis:
			switch(keysym)
			{
			case XK_A: keysym=XK_Adiaeresis;break;
			case XK_E: keysym=XK_Ediaeresis;break;
			case XK_I: keysym=XK_Idiaeresis;break;
			case XK_O: keysym=XK_Odiaeresis;break;
			case XK_U: keysym=XK_Udiaeresis;break;
			case XK_a: keysym=XK_adiaeresis;break;
			case XK_e: keysym=XK_ediaeresis;break;
			case XK_i: keysym=XK_idiaeresis;break;
			case XK_o: keysym=XK_odiaeresis;break;
			case XK_u: keysym=XK_udiaeresis;break;
			case XK_y: keysym=XK_ydiaeresis;break;

			}
		case XK_dead_cedilla:
			switch(keysym)
			{
			case XK_C: keysym=XK_Ccedilla;break;
			case XK_c: keysym=XK_ccedilla;break;
			}
		}
		keysymDead=0;
		vnclog.Print(LL_INTWARN, " Composed 0x%x",keysym);

	}
      // ordinary Latin-1 character

      SHORT s = VkKeyScan(keysym);

      //	[v1.0.2-jp1 fix] yak!'s patch
	  // This break Other keyboards, we need an easy way of fixing this
	  if (jap)
	  {
		  if (keysym==XK_kana_WO) {
			s = 0x0130;
		  } else if (keysym==XK_backslash) {
			s = 0x00e2;
		  } else if (keysym==XK_yen) {
			s = 0x00dc;
		  }
	  }

	  vnclog.Print(LL_INTWARN, " SHORT s %i",s);

	 if (s == -1)
	 {
		 
      if (down) {
		  vnclog.Print(LL_INTWARN, "down");
        // not a single keypress - try synthesizing dead chars.
			{
			  vnclog.Print(LL_INTWARN, " Found key");
			  //Lookup ascii representation
			  int ascii=0;
#if 0
              // 11 Dec 2008 jdp disabled since the viewer is sending unicode now
			  for (ascii=0;ascii<256;ascii++)
			  {
				  if (keysym==ascii_to_x[ascii]) break;
			  }
#endif
              ascii = keysym;
			  if (ascii <= 255)
			  {

			  rdr::U8 a0=ascii/100;
			  ascii=ascii%100;
			  rdr::U8 a1=ascii/10;
			  ascii=ascii%10;
			  rdr::U8 a2=ascii;

              KeyStateModifier shift(VK_SHIFT);
              KeyStateModifier lshift(VK_LSHIFT);
              KeyStateModifier rshift(VK_RSHIFT);

              lshift.release();
              rshift.release();			  

              vnclog.Print(LL_INTWARN, " Simulating ALT+%d%d%d\n", a0, a1 ,a2);

			  keybd_uni_event(VK_MENU,MapVirtualKey( VK_MENU, 0 ), 0 ,0);
              /**
                Pressing the Alt+NNN combinations without leading zero (for example, Alt+20, Alt+130, Alt+221) 
                will insert characters from the Extended ASCII (or MS DOS ASCII, or OEM) table. The character 
                glyphs contained by this table depend on the language of Windows. See the table below for the 
                list of characters that can be inserted through the Alt+NNN combinations (without leading zero)
                in English Windows.

                Pressing the Alt+0NNN combinations will insert the ANSI characters corresponding to the activate 
                keyboard layout. Please see Windows Character Map utility (charmap.exe) for the possible Alt+0NNN
                combinations.

                Finally, the Alt+00NNN combinations (two leading zeros) will insert Unicode characters. The Unicode 
                codes of characters are displayed in Charmap.

              **/
              // jdp 11 December 2008 - Need the leading 0! 
			  keybd_uni_event(VK_NUMPAD0,    MapVirtualKey(VK_NUMPAD0,    0), 0, 0);
			  keybd_uni_event(VK_NUMPAD0,    MapVirtualKey(VK_NUMPAD0,    0),KEYEVENTF_KEYUP,0);
			  keybd_uni_event(VK_NUMPAD0+a0, MapVirtualKey(VK_NUMPAD0+a0, 0), 0, 0);
			  keybd_uni_event(VK_NUMPAD0+a0, MapVirtualKey(VK_NUMPAD0+a0, 0),KEYEVENTF_KEYUP,0);
			  keybd_uni_event(VK_NUMPAD0+a1, MapVirtualKey(VK_NUMPAD0+a1, 0),0,0);
			  keybd_uni_event(VK_NUMPAD0+a1, MapVirtualKey(VK_NUMPAD0+a1, 0),KEYEVENTF_KEYUP, 0);
			  keybd_uni_event(VK_NUMPAD0+a2, MapVirtualKey(VK_NUMPAD0+a2, 0) ,0, 0);
			  keybd_uni_event(VK_NUMPAD0+a2, MapVirtualKey(VK_NUMPAD0+a2, 0),KEYEVENTF_KEYUP, 0);
			  keybd_uni_event(VK_MENU, MapVirtualKey( VK_MENU, 0 ),KEYEVENTF_KEYUP, 0);
			  return;
			  }
        }
        vnclog.Print(LL_INTWARN, "ignoring unrecognised Latin-1 keysym 0x%x",keysym);
      }
      return;
    }

      BYTE vkCode = LOBYTE(s);

      // 18 March 2008 jdp
      // Correct the keymask shift state to cope with the capslock state
      BOOL capslockOn = (GetKeyState(VK_CAPITAL) & 1) != 0;

      BYTE modifierState = HIBYTE(s);
      modifierState = capslockOn ? modifierState ^ 1 : modifierState;
      KeyStateModifier ctrl(VK_CONTROL);
      KeyStateModifier alt(VK_MENU);
      KeyStateModifier shift(VK_SHIFT);
      KeyStateModifier lshift(VK_LSHIFT);
      KeyStateModifier rshift(VK_RSHIFT);

	  if (down) {
		  if (modifierState & 2) ctrl.press();
		  if (modifierState & 4) alt.press();
		  if (modifierState & 1) {
			  shift.press();
		  }
		  else {
			  // [v1.0.2-jp1 fix] Even if "SHIFT + SPACE" are pressed, "SHIFT" is valid
			  if (vkCode == 0x20) {
			  }
			  else {
				  lshift.release();
				  rshift.release();
			  }
		  }
	  } 

      vnclog.Print(LL_INTINFO,
                   "latin-1 key: keysym %d(0x%x) vkCode 0x%x down %d capslockOn %d\n",
                   keysym, keysym, vkCode, down, capslockOn);

      doKeyboardEvent(vkCode, down ? 0 : KEYEVENTF_KEYUP);

    } else {

      // see if it's a recognised keyboard key, otherwise ignore it

    if (vkMap.find(keysym) == vkMap.end()) {
		if (unicode) {
			if (keysym == 65509)
				return;
			
			WORD unicodeChar = keysymToUnicode(keysym);
			if (unicodeChar == 0) {
				vnclog.Print(LL_INTINFO, "Unhandled keysym 0x%x\n", keysym);
				return;
			}
			
			INPUT inputs[1];
			if (down) {
				inputs[0].type= INPUT_KEYBOARD;
				inputs[0].ki.wVk = 0;
				inputs[0].ki.wScan = unicodeChar;
				inputs[0].ki.time = 0;
				inputs[0].ki.dwExtraInfo = NULL;
				inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
				SendInput(1, inputs, sizeof(INPUT));
			} else {
				inputs[0].type= INPUT_KEYBOARD;
				inputs[0].ki.wVk = 0;
				inputs[0].ki.wScan = unicodeChar;
				inputs[0].ki.time = 0;
				inputs[0].ki.dwExtraInfo = NULL;
				inputs[0].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
				SendInput(1, inputs, sizeof(INPUT));
			}
		}
        return;
      }
      BYTE vkCode = vkMap[keysym];
      DWORD flags = 0;
      if (extendedMap[keysym]) flags |= KEYEVENTF_EXTENDEDKEY;
      if (!down) flags |= KEYEVENTF_KEYUP;

//      vnclog.Print(LL_INTINFO,
  //                "keyboard key: keysym %d(0x%x) vkCode 0x%x ext %d down %d\n",
    //               keysym, keysym, vkCode, extendedMap[keysym], down);

      if (down && (vkCode == VK_DELETE) &&
          ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) &&
          ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0))
      {
		vnclog.Print(LL_INTINFO,
                 "CAD\n");
		// If running under Windows Vista and started from Session0 in Application mode
		if (settings->RunningFromExternalService())
		{
			      vnclog.Print(LL_INTINFO,
                 "Windows Vista and runnning as system -> CAD\n");

				// Try to run the special Windows Vista cad.exe file...
				HANDLE ThreadHandle2;
				DWORD dwTId;
				ThreadHandle2 = CreateThread(NULL, 0, vncCad::Cadthread, NULL, 0, &dwTId);
				CloseHandle(ThreadHandle2);
		}
		else
		{
			vnclog.Print(LL_INTINFO,
                 "Windows Vista and runnning as user -> Taskmgr\n");
			WinExec("taskmgr.exe", SW_SHOWNORMAL);
		}		

        return;
      }
	  else if (down && (vkCode == VK_HANGEUL))
	  {
		  HWND hWnd = GetForegroundWindow();
		  
		  HWND hIME_Main = ImmGetDefaultIMEWnd(hWnd);
		  LRESULT MainStatus = ::SendMessage(hIME_Main, WM_IME_CONTROL, 5, 0);
		  
		  HWND hChildWnd = GetWindow(hWnd, GW_CHILD);
		  long childCount = 0L;

		  while (IsWindow(hChildWnd))
		  {
			  HWND hIME = ImmGetDefaultIMEWnd(hChildWnd);
			  if (MainStatus == 1)
			  {
				  ::SendMessage(hIME, WM_IME_CONTROL, 2, 0);
				  ::SendMessage(hIME, WM_IME_CONTROL, 1, 0);
				  ::SendMessage(hIME, WM_IME_CONTROL, 3, 0);
				  ::SendMessage(hIME, WM_IME_CONTROL, 4, 0);
				  ::SendMessage(hIME, WM_IME_CONTROL, 6, 0);
			  }
			  else if (MainStatus == 0)
			  {
				  ::SendMessage(hIME, WM_IME_CONTROL, 2, 1);
				  ::SendMessage(hIME, WM_IME_CONTROL, 1, 1);
				  ::SendMessage(hIME, WM_IME_CONTROL, 3, 1);
				  ::SendMessage(hIME, WM_IME_CONTROL, 4, 1);
				  ::SendMessage(hIME, WM_IME_CONTROL, 6, 1);
			  }

			  childCount++;
			  hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);
		  }
		  return;
	  }
      doKeyboardEvent(vkCode, flags);
    }
  }

private:
  std::map<rdr::U32,rdr::U8> vkMap;
  std::map<rdr::U32,bool> extendedMap;
} key_mapper;

void vncKeymap::keyEvent(CARD32 keysym, bool down,bool jap, bool unicode)
{
  key_mapper.keyEvent(keysym, down,jap, unicode);
}



void
SetShiftState(BYTE key, BOOL down)
{
	BOOL keystate = (GetAsyncKeyState(key) & 0x8000) != 0;

	// This routine sets the specified key to the desired value (up or down)
	if ((keystate && down) || ((!keystate) && (!down)))
		return;

	vnclog.Print(LL_INTINFO,
		VNCLOG("setshiftstate %d - (%s->%s)\n"),
		key, keystate ? "down" : "up",
		down ? "down" : "up");

	// Now send a key event to set the key to the new value
	doKeyboardEvent(key, down ? 0 : KEYEVENTF_KEYUP);
	keystate = (GetAsyncKeyState(key) & 0x8000) != 0;

	vnclog.Print(LL_INTINFO,
		VNCLOG("new state %d (%s)\n"),
		key, keystate ? "down" : "up");
}

void
vncKeymap::ClearShiftKeys()
{
	// On NT, clear both sets of keys
	// LEFT
	SetShiftState(VK_LSHIFT, FALSE);
	SetShiftState(VK_LCONTROL, FALSE);
	SetShiftState(VK_LMENU, FALSE);
	// RIGHT
	SetShiftState(VK_RSHIFT, FALSE);
	SetShiftState(VK_RCONTROL, FALSE);
	SetShiftState(VK_RMENU, FALSE);

}

/* $TOG: keysymdef.h /main/28 1998/05/22 16:18:01 kaleb $ */

/***********************************************************
Copyright 1987, 1994, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#define XK_VoidSymbol		0xFFFFFF	/* void symbol */

#ifdef XK_MISCELLANY
/*
 * TTY Functions, cleverly chosen to map to ascii, for convenience of
 * programming, but could have been arbitrary (at the cost of lookup
 * tables in client code.
 */

#define XK_BackSpace		0xFF08	/* back space, back char */
#define XK_Tab			0xFF09
#define XK_Linefeed		0xFF0A	/* Linefeed, LF */
#define XK_Clear		0xFF0B
#define XK_Return		0xFF0D	/* Return, enter */
#define XK_Pause		0xFF13	/* Pause, hold */
#define XK_Scroll_Lock		0xFF14
#define XK_Sys_Req		0xFF15
#define XK_Escape		0xFF1B
#define XK_Delete		0xFFFF	/* Delete, rubout */



/* International & multi-key character composition */

#define XK_Multi_key		0xFF20  /* Multi-key character compose */
#define XK_Codeinput		0xFF37
#define XK_SingleCandidate	0xFF3C
#define XK_MultipleCandidate	0xFF3D
#define XK_PreviousCandidate	0xFF3E

/* Japanese keyboard support */

#define XK_Kanji		0xFF21	/* Kanji, Kanji convert */
#define XK_Muhenkan		0xFF22  /* Cancel Conversion */
#define XK_Henkan_Mode		0xFF23  /* Start/Stop Conversion */
#define XK_Henkan		0xFF23  /* Alias for Henkan_Mode */
#define XK_Romaji		0xFF24  /* to Romaji */
#define XK_Hiragana		0xFF25  /* to Hiragana */
#define XK_Katakana		0xFF26  /* to Katakana */
#define XK_Hiragana_Katakana	0xFF27  /* Hiragana/Katakana toggle */
#define XK_Zenkaku		0xFF28  /* to Zenkaku */
#define XK_Hankaku		0xFF29  /* to Hankaku */
#define XK_Zenkaku_Hankaku	0xFF2A  /* Zenkaku/Hankaku toggle */
#define XK_Touroku		0xFF2B  /* Add to Dictionary */
#define XK_Massyo		0xFF2C  /* Delete from Dictionary */
#define XK_Kana_Lock		0xFF2D  /* Kana Lock */
#define XK_Kana_Shift		0xFF2E  /* Kana Shift */
#define XK_Eisu_Shift		0xFF2F  /* Alphanumeric Shift */
#define XK_Eisu_toggle		0xFF30  /* Alphanumeric toggle */
#define XK_Kanji_Bangou		0xFF37  /* Codeinput */
#define XK_Zen_Koho		0xFF3D	/* Multiple/All Candidate(s) */
#define XK_Mae_Koho		0xFF3E	/* Previous Candidate */

/* 0xFF31 thru 0xFF3F are under XK_KOREAN */

/* Cursor control & motion */

#define XK_Home			0xFF50
#define XK_Left			0xFF51	/* Move left, left arrow */
#define XK_Up			0xFF52	/* Move up, up arrow */
#define XK_Right		0xFF53	/* Move right, right arrow */
#define XK_Down			0xFF54	/* Move down, down arrow */
#define XK_Prior		0xFF55	/* Prior, previous */
#define XK_Page_Up		0xFF55
#define XK_Next			0xFF56	/* Next */
#define XK_Page_Down		0xFF56
#define XK_End			0xFF57	/* EOL */
#define XK_Begin		0xFF58	/* BOL */


/* Misc Functions */

#define XK_Select		0xFF60	/* Select, mark */
#define XK_Print		0xFF61
#define XK_Execute		0xFF62	/* Execute, run, do */
#define XK_Insert		0xFF63	/* Insert, insert here */
#define XK_Undo			0xFF65	/* Undo, oops */
#define XK_Redo			0xFF66	/* redo, again */
#define XK_Menu			0xFF67
#define XK_Find			0xFF68	/* Find, search */
#define XK_Cancel		0xFF69	/* Cancel, stop, abort, exit */
#define XK_Help			0xFF6A	/* Help */
#define XK_Break		0xFF6B
#define XK_Mode_switch		0xFF7E	/* Character set switch */
#define XK_script_switch        0xFF7E  /* Alias for mode_switch */
#define XK_Num_Lock		0xFF7F

/* Keypad Functions, keypad numbers cleverly chosen to map to ascii */

#define XK_KP_Space		0xFF80	/* space */
#define XK_KP_Tab		0xFF89
#define XK_KP_Enter		0xFF8D	/* enter */
#define XK_KP_F1		0xFF91	/* PF1, KP_A, ... */
#define XK_KP_F2		0xFF92
#define XK_KP_F3		0xFF93
#define XK_KP_F4		0xFF94
#define XK_KP_Home		0xFF95
#define XK_KP_Left		0xFF96
#define XK_KP_Up		0xFF97
#define XK_KP_Right		0xFF98
#define XK_KP_Down		0xFF99
#define XK_KP_Prior		0xFF9A
#define XK_KP_Page_Up		0xFF9A
#define XK_KP_Next		0xFF9B
#define XK_KP_Page_Down		0xFF9B
#define XK_KP_End		0xFF9C
#define XK_KP_Begin		0xFF9D
#define XK_KP_Insert		0xFF9E
#define XK_KP_Delete		0xFF9F
#define XK_KP_Equal		0xFFBD	/* equals */
#define XK_KP_Multiply		0xFFAA
#define XK_KP_Add		0xFFAB
#define XK_KP_Separator		0xFFAC	/* separator, often comma */
#define XK_KP_Subtract		0xFFAD
#define XK_KP_Decimal		0xFFAE
#define XK_KP_Divide		0xFFAF

#define XK_KP_0			0xFFB0
#define XK_KP_1			0xFFB1
#define XK_KP_2			0xFFB2
#define XK_KP_3			0xFFB3
#define XK_KP_4			0xFFB4
#define XK_KP_5			0xFFB5
#define XK_KP_6			0xFFB6
#define XK_KP_7			0xFFB7
#define XK_KP_8			0xFFB8
#define XK_KP_9			0xFFB9



/*
 * Auxilliary Functions; note the duplicate definitions for left and right
 * function keys;  Sun keyboards and a few other manufactures have such
 * function key groups on the left and/or right sides of the keyboard.
 * We've not found a keyboard with more than 35 function keys total.
 */

#define XK_F1			0xFFBE
#define XK_F2			0xFFBF
#define XK_F3			0xFFC0
#define XK_F4			0xFFC1
#define XK_F5			0xFFC2
#define XK_F6			0xFFC3
#define XK_F7			0xFFC4
#define XK_F8			0xFFC5
#define XK_F9			0xFFC6
#define XK_F10			0xFFC7
#define XK_F11			0xFFC8
#define XK_L1			0xFFC8
#define XK_F12			0xFFC9
#define XK_L2			0xFFC9
#define XK_F13			0xFFCA
#define XK_L3			0xFFCA
#define XK_F14			0xFFCB
#define XK_L4			0xFFCB
#define XK_F15			0xFFCC
#define XK_L5			0xFFCC
#define XK_F16			0xFFCD
#define XK_L6			0xFFCD
#define XK_F17			0xFFCE
#define XK_L7			0xFFCE
#define XK_F18			0xFFCF
#define XK_L8			0xFFCF
#define XK_F19			0xFFD0
#define XK_L9			0xFFD0
#define XK_F20			0xFFD1
#define XK_L10			0xFFD1
#define XK_F21			0xFFD2
#define XK_R1			0xFFD2
#define XK_F22			0xFFD3
#define XK_R2			0xFFD3
#define XK_F23			0xFFD4
#define XK_R3			0xFFD4
#define XK_F24			0xFFD5
#define XK_R4			0xFFD5
#define XK_F25			0xFFD6
#define XK_R5			0xFFD6
#define XK_F26			0xFFD7
#define XK_R6			0xFFD7
#define XK_F27			0xFFD8
#define XK_R7			0xFFD8
#define XK_F28			0xFFD9
#define XK_R8			0xFFD9
#define XK_F29			0xFFDA
#define XK_R9			0xFFDA
#define XK_F30			0xFFDB
#define XK_R10			0xFFDB
#define XK_F31			0xFFDC
#define XK_R11			0xFFDC
#define XK_F32			0xFFDD
#define XK_R12			0xFFDD
#define XK_F33			0xFFDE
#define XK_R13			0xFFDE
#define XK_F34			0xFFDF
#define XK_R14			0xFFDF
#define XK_F35			0xFFE0
#define XK_R15			0xFFE0

/* Modifiers */

#define XK_Shift_L		0xFFE1	/* Left shift */
#define XK_Shift_R		0xFFE2	/* Right shift */
#define XK_Control_L		0xFFE3	/* Left control */
#define XK_Control_R		0xFFE4	/* Right control */
#define XK_Caps_Lock		0xFFE5	/* Caps lock */
#define XK_Shift_Lock		0xFFE6	/* Shift lock */

#define XK_Meta_L		0xFFE7	/* Left meta */
#define XK_Meta_R		0xFFE8	/* Right meta */
#define XK_Alt_L		0xFFE9	/* Left alt */
#define XK_Alt_R		0xFFEA	/* Right alt */
#define XK_Super_L		0xFFEB	/* Left super */
#define XK_Super_R		0xFFEC	/* Right super */
#define XK_Hyper_L		0xFFED	/* Left hyper */
#define XK_Hyper_R		0xFFEE	/* Right hyper */
#endif /* XK_MISCELLANY */

/*
 * ISO 9995 Function and Modifier Keys
 * Byte 3 = 0xFE
 */

#ifdef XK_XKB_KEYS
#define	XK_ISO_Lock					0xFE01
#define	XK_ISO_Level2_Latch				0xFE02
#define	XK_ISO_Level3_Shift				0xFE03
#define	XK_ISO_Level3_Latch				0xFE04
#define	XK_ISO_Level3_Lock				0xFE05
#define	XK_ISO_Group_Shift		0xFF7E	/* Alias for mode_switch */
#define	XK_ISO_Group_Latch				0xFE06
#define	XK_ISO_Group_Lock				0xFE07
#define	XK_ISO_Next_Group				0xFE08
#define	XK_ISO_Next_Group_Lock				0xFE09
#define	XK_ISO_Prev_Group				0xFE0A
#define	XK_ISO_Prev_Group_Lock				0xFE0B
#define	XK_ISO_First_Group				0xFE0C
#define	XK_ISO_First_Group_Lock				0xFE0D
#define	XK_ISO_Last_Group				0xFE0E
#define	XK_ISO_Last_Group_Lock				0xFE0F

#define	XK_ISO_Left_Tab					0xFE20
#define	XK_ISO_Move_Line_Up				0xFE21
#define	XK_ISO_Move_Line_Down				0xFE22
#define	XK_ISO_Partial_Line_Up				0xFE23
#define	XK_ISO_Partial_Line_Down			0xFE24
#define	XK_ISO_Partial_Space_Left			0xFE25
#define	XK_ISO_Partial_Space_Right			0xFE26
#define	XK_ISO_Set_Margin_Left				0xFE27
#define	XK_ISO_Set_Margin_Right				0xFE28
#define	XK_ISO_Release_Margin_Left			0xFE29
#define	XK_ISO_Release_Margin_Right			0xFE2A
#define	XK_ISO_Release_Both_Margins			0xFE2B
#define	XK_ISO_Fast_Cursor_Left				0xFE2C
#define	XK_ISO_Fast_Cursor_Right			0xFE2D
#define	XK_ISO_Fast_Cursor_Up				0xFE2E
#define	XK_ISO_Fast_Cursor_Down				0xFE2F
#define	XK_ISO_Continuous_Underline			0xFE30
#define	XK_ISO_Discontinuous_Underline			0xFE31
#define	XK_ISO_Emphasize				0xFE32
#define	XK_ISO_Center_Object				0xFE33
#define	XK_ISO_Enter					0xFE34

#define	XK_dead_grave					0xFE50
#define	XK_dead_acute					0xFE51
#define	XK_dead_circumflex				0xFE52
#define	XK_dead_tilde					0xFE53
#define	XK_dead_macron					0xFE54
#define	XK_dead_breve					0xFE55
#define	XK_dead_abovedot				0xFE56
#define	XK_dead_diaeresis				0xFE57
#define	XK_dead_abovering				0xFE58
#define	XK_dead_doubleacute				0xFE59
#define	XK_dead_caron					0xFE5A
#define	XK_dead_cedilla					0xFE5B
#define	XK_dead_ogonek					0xFE5C
#define	XK_dead_iota					0xFE5D
#define	XK_dead_voiced_sound				0xFE5E
#define	XK_dead_semivoiced_sound			0xFE5F
#define	XK_dead_belowdot				0xFE60

#define	XK_First_Virtual_Screen				0xFED0
#define	XK_Prev_Virtual_Screen				0xFED1
#define	XK_Next_Virtual_Screen				0xFED2
#define	XK_Last_Virtual_Screen				0xFED4
#define	XK_Terminate_Server				0xFED5

#define	XK_AccessX_Enable				0xFE70
#define	XK_AccessX_Feedback_Enable			0xFE71
#define	XK_RepeatKeys_Enable				0xFE72
#define	XK_SlowKeys_Enable				0xFE73
#define	XK_BounceKeys_Enable				0xFE74
#define	XK_StickyKeys_Enable				0xFE75
#define	XK_MouseKeys_Enable				0xFE76
#define	XK_MouseKeys_Accel_Enable			0xFE77
#define	XK_Overlay1_Enable				0xFE78
#define	XK_Overlay2_Enable				0xFE79
#define	XK_AudibleBell_Enable				0xFE7A

#define	XK_Pointer_Left					0xFEE0
#define	XK_Pointer_Right				0xFEE1
#define	XK_Pointer_Up					0xFEE2
#define	XK_Pointer_Down					0xFEE3
#define	XK_Pointer_UpLeft				0xFEE4
#define	XK_Pointer_UpRight				0xFEE5
#define	XK_Pointer_DownLeft				0xFEE6
#define	XK_Pointer_DownRight				0xFEE7
#define	XK_Pointer_Button_Dflt				0xFEE8
#define	XK_Pointer_Button1				0xFEE9
#define	XK_Pointer_Button2				0xFEEA
#define	XK_Pointer_Button3				0xFEEB
#define	XK_Pointer_Button4				0xFEEC
#define	XK_Pointer_Button5				0xFEED
#define	XK_Pointer_DblClick_Dflt			0xFEEE
#define	XK_Pointer_DblClick1				0xFEEF
#define	XK_Pointer_DblClick2				0xFEF0
#define	XK_Pointer_DblClick3				0xFEF1
#define	XK_Pointer_DblClick4				0xFEF2
#define	XK_Pointer_DblClick5				0xFEF3
#define	XK_Pointer_Drag_Dflt				0xFEF4
#define	XK_Pointer_Drag1				0xFEF5
#define	XK_Pointer_Drag2				0xFEF6
#define	XK_Pointer_Drag3				0xFEF7
#define	XK_Pointer_Drag4				0xFEF8
#define	XK_Pointer_Drag5				0xFEFD

#define	XK_Pointer_EnableKeys				0xFEF9
#define	XK_Pointer_Accelerate				0xFEFA
#define	XK_Pointer_DfltBtnNext				0xFEFB
#define	XK_Pointer_DfltBtnPrev				0xFEFC

#endif


/*
 *  Latin 1
 *  Byte 3 = 0
 */
#ifdef XK_LATIN1
#define XK_space               0x020
#define XK_exclam              0x021
#define XK_quotedbl            0x022
#define XK_numbersign          0x023
#define XK_dollar              0x024
#define XK_percent             0x025
#define XK_ampersand           0x026
#define XK_apostrophe          0x027
#define XK_quoteright          0x027	/* deprecated */
#define XK_parenleft           0x028
#define XK_parenright          0x029
#define XK_asterisk            0x02a
#define XK_plus                0x02b
#define XK_comma               0x02c
#define XK_minus               0x02d
#define XK_period              0x02e
#define XK_slash               0x02f
#define XK_0                   0x030
#define XK_1                   0x031
#define XK_2                   0x032
#define XK_3                   0x033
#define XK_4                   0x034
#define XK_5                   0x035
#define XK_6                   0x036
#define XK_7                   0x037
#define XK_8                   0x038
#define XK_9                   0x039
#define XK_colon               0x03a
#define XK_semicolon           0x03b
#define XK_less                0x03c
#define XK_equal               0x03d
#define XK_greater             0x03e
#define XK_question            0x03f
#define XK_at                  0x040
#define XK_A                   0x041
#define XK_B                   0x042
#define XK_C                   0x043
#define XK_D                   0x044
#define XK_E                   0x045
#define XK_F                   0x046
#define XK_G                   0x047
#define XK_H                   0x048
#define XK_I                   0x049
#define XK_J                   0x04a
#define XK_K                   0x04b
#define XK_L                   0x04c
#define XK_M                   0x04d
#define XK_N                   0x04e
#define XK_O                   0x04f
#define XK_P                   0x050
#define XK_Q                   0x051
#define XK_R                   0x052
#define XK_S                   0x053
#define XK_T                   0x054
#define XK_U                   0x055
#define XK_V                   0x056
#define XK_W                   0x057
#define XK_X                   0x058
#define XK_Y                   0x059
#define XK_Z                   0x05a
#define XK_bracketleft         0x05b
#define XK_backslash           0x05c
#define XK_bracketright        0x05d
#define XK_asciicircum         0x05e
#define XK_underscore          0x05f
#define XK_grave               0x060
#define XK_quoteleft           0x060	/* deprecated */
#define XK_a                   0x061
#define XK_b                   0x062
#define XK_c                   0x063
#define XK_d                   0x064
#define XK_e                   0x065
#define XK_f                   0x066
#define XK_g                   0x067
#define XK_h                   0x068
#define XK_i                   0x069
#define XK_j                   0x06a
#define XK_k                   0x06b
#define XK_l                   0x06c
#define XK_m                   0x06d
#define XK_n                   0x06e
#define XK_o                   0x06f
#define XK_p                   0x070
#define XK_q                   0x071
#define XK_r                   0x072
#define XK_s                   0x073
#define XK_t                   0x074
#define XK_u                   0x075
#define XK_v                   0x076
#define XK_w                   0x077
#define XK_x                   0x078
#define XK_y                   0x079
#define XK_z                   0x07a
#define XK_braceleft           0x07b
#define XK_bar                 0x07c
#define XK_braceright          0x07d
#define XK_asciitilde          0x07e

#define XK_nobreakspace        0x0a0
#define XK_exclamdown          0x0a1
#define XK_cent        	       0x0a2
#define XK_sterling            0x0a3
#define XK_currency            0x0a4
#define XK_yen                 0x0a5
#define XK_brokenbar           0x0a6
#define XK_section             0x0a7
#define XK_diaeresis           0x0a8
#define XK_copyright           0x0a9
#define XK_ordfeminine         0x0aa
#define XK_guillemotleft       0x0ab	/* left angle quotation mark */
#define XK_notsign             0x0ac
#define XK_hyphen              0x0ad
#define XK_registered          0x0ae
#define XK_macron              0x0af
#define XK_degree              0x0b0
#define XK_plusminus           0x0b1
#define XK_twosuperior         0x0b2
#define XK_threesuperior       0x0b3
#define XK_acute               0x0b4
#define XK_mu                  0x0b5
#define XK_paragraph           0x0b6
#define XK_periodcentered      0x0b7
#define XK_cedilla             0x0b8
#define XK_onesuperior         0x0b9
#define XK_masculine           0x0ba
#define XK_guillemotright      0x0bb	/* right angle quotation mark */
#define XK_onequarter          0x0bc
#define XK_onehalf             0x0bd
#define XK_threequarters       0x0be
#define XK_questiondown        0x0bf
#define XK_Agrave              0x0c0
#define XK_Aacute              0x0c1
#define XK_Acircumflex         0x0c2
#define XK_Atilde              0x0c3
#define XK_Adiaeresis          0x0c4
#define XK_Aring               0x0c5
#define XK_AE                  0x0c6
#define XK_Ccedilla            0x0c7
#define XK_Egrave              0x0c8
#define XK_Eacute              0x0c9
#define XK_Ecircumflex         0x0ca
#define XK_Ediaeresis          0x0cb
#define XK_Igrave              0x0cc
#define XK_Iacute              0x0cd
#define XK_Icircumflex         0x0ce
#define XK_Idiaeresis          0x0cf
#define XK_ETH                 0x0d0
#define XK_Eth                 0x0d0	/* deprecated */
#define XK_Ntilde              0x0d1
#define XK_Ograve              0x0d2
#define XK_Oacute              0x0d3
#define XK_Ocircumflex         0x0d4
#define XK_Otilde              0x0d5
#define XK_Odiaeresis          0x0d6
#define XK_multiply            0x0d7
#define XK_Ooblique            0x0d8
#define XK_Ugrave              0x0d9
#define XK_Uacute              0x0da
#define XK_Ucircumflex         0x0db
#define XK_Udiaeresis          0x0dc
#define XK_Yacute              0x0dd
#define XK_THORN               0x0de
#define XK_Thorn               0x0de	/* deprecated */
#define XK_ssharp              0x0df
#define XK_agrave              0x0e0
#define XK_aacute              0x0e1
#define XK_acircumflex         0x0e2
#define XK_atilde              0x0e3
#define XK_adiaeresis          0x0e4
#define XK_aring               0x0e5
#define XK_ae                  0x0e6
#define XK_ccedilla            0x0e7
#define XK_egrave              0x0e8
#define XK_eacute              0x0e9
#define XK_ecircumflex         0x0ea
#define XK_ediaeresis          0x0eb
#define XK_igrave              0x0ec
#define XK_iacute              0x0ed
#define XK_icircumflex         0x0ee
#define XK_idiaeresis          0x0ef
#define XK_eth                 0x0f0
#define XK_ntilde              0x0f1
#define XK_ograve              0x0f2
#define XK_oacute              0x0f3
#define XK_ocircumflex         0x0f4
#define XK_otilde              0x0f5
#define XK_odiaeresis          0x0f6
#define XK_division            0x0f7
#define XK_oslash              0x0f8
#define XK_ugrave              0x0f9
#define XK_uacute              0x0fa
#define XK_ucircumflex         0x0fb
#define XK_udiaeresis          0x0fc
#define XK_yacute              0x0fd
#define XK_thorn               0x0fe
#define XK_ydiaeresis          0x0ff
#endif /* XK_LATIN1 */

/*
 *   Latin 2
 *   Byte 3 = 1
 */

#ifdef XK_LATIN2
#define XK_Aogonek             0x1a1
#define XK_breve               0x1a2
#define XK_Lstroke             0x1a3
#define XK_Lcaron              0x1a5
#define XK_Sacute              0x1a6
#define XK_Scaron              0x1a9
#define XK_Scedilla            0x1aa
#define XK_Tcaron              0x1ab
#define XK_Zacute              0x1ac
#define XK_Zcaron              0x1ae
#define XK_Zabovedot           0x1af
#define XK_aogonek             0x1b1
#define XK_ogonek              0x1b2
#define XK_lstroke             0x1b3
#define XK_lcaron              0x1b5
#define XK_sacute              0x1b6
#define XK_caron               0x1b7
#define XK_scaron              0x1b9
#define XK_scedilla            0x1ba
#define XK_tcaron              0x1bb
#define XK_zacute              0x1bc
#define XK_doubleacute         0x1bd
#define XK_zcaron              0x1be
#define XK_zabovedot           0x1bf
#define XK_Racute              0x1c0
#define XK_Abreve              0x1c3
#define XK_Lacute              0x1c5
#define XK_Cacute              0x1c6
#define XK_Ccaron              0x1c8
#define XK_Eogonek             0x1ca
#define XK_Ecaron              0x1cc
#define XK_Dcaron              0x1cf
#define XK_Dstroke             0x1d0
#define XK_Nacute              0x1d1
#define XK_Ncaron              0x1d2
#define XK_Odoubleacute        0x1d5
#define XK_Rcaron              0x1d8
#define XK_Uring               0x1d9
#define XK_Udoubleacute        0x1db
#define XK_Tcedilla            0x1de
#define XK_racute              0x1e0
#define XK_abreve              0x1e3
#define XK_lacute              0x1e5
#define XK_cacute              0x1e6
#define XK_ccaron              0x1e8
#define XK_eogonek             0x1ea
#define XK_ecaron              0x1ec
#define XK_dcaron              0x1ef
#define XK_dstroke             0x1f0
#define XK_nacute              0x1f1
#define XK_ncaron              0x1f2
#define XK_odoubleacute        0x1f5
#define XK_udoubleacute        0x1fb
#define XK_rcaron              0x1f8
#define XK_uring               0x1f9
#define XK_tcedilla            0x1fe
#define XK_abovedot            0x1ff
#endif /* XK_LATIN2 */

/*
 *   Latin 3
 *   Byte 3 = 2
 */

#ifdef XK_LATIN3
#define XK_Hstroke             0x2a1
#define XK_Hcircumflex         0x2a6
#define XK_Iabovedot           0x2a9
#define XK_Gbreve              0x2ab
#define XK_Jcircumflex         0x2ac
#define XK_hstroke             0x2b1
#define XK_hcircumflex         0x2b6
#define XK_idotless            0x2b9
#define XK_gbreve              0x2bb
#define XK_jcircumflex         0x2bc
#define XK_Cabovedot           0x2c5
#define XK_Ccircumflex         0x2c6
#define XK_Gabovedot           0x2d5
#define XK_Gcircumflex         0x2d8
#define XK_Ubreve              0x2dd
#define XK_Scircumflex         0x2de
#define XK_cabovedot           0x2e5
#define XK_ccircumflex         0x2e6
#define XK_gabovedot           0x2f5
#define XK_gcircumflex         0x2f8
#define XK_ubreve              0x2fd
#define XK_scircumflex         0x2fe
#endif /* XK_LATIN3 */


/*
 *   Latin 4
 *   Byte 3 = 3
 */

#ifdef XK_LATIN4
#define XK_kra                 0x3a2
#define XK_kappa               0x3a2	/* deprecated */
#define XK_Rcedilla            0x3a3
#define XK_Itilde              0x3a5
#define XK_Lcedilla            0x3a6
#define XK_Emacron             0x3aa
#define XK_Gcedilla            0x3ab
#define XK_Tslash              0x3ac
#define XK_rcedilla            0x3b3
#define XK_itilde              0x3b5
#define XK_lcedilla            0x3b6
#define XK_emacron             0x3ba
#define XK_gcedilla            0x3bb
#define XK_tslash              0x3bc
#define XK_ENG                 0x3bd
#define XK_eng                 0x3bf
#define XK_Amacron             0x3c0
#define XK_Iogonek             0x3c7
#define XK_Eabovedot           0x3cc
#define XK_Imacron             0x3cf
#define XK_Ncedilla            0x3d1
#define XK_Omacron             0x3d2
#define XK_Kcedilla            0x3d3
#define XK_Uogonek             0x3d9
#define XK_Utilde              0x3dd
#define XK_Umacron             0x3de
#define XK_amacron             0x3e0
#define XK_iogonek             0x3e7
#define XK_eabovedot           0x3ec
#define XK_imacron             0x3ef
#define XK_ncedilla            0x3f1
#define XK_omacron             0x3f2
#define XK_kcedilla            0x3f3
#define XK_uogonek             0x3f9
#define XK_utilde              0x3fd
#define XK_umacron             0x3fe
#endif /* XK_LATIN4 */

/*
 * Latin-9 (a.k.a. Latin-0)
 * Byte 3 = 19
 */

#ifdef XK_LATIN9
#define XK_OE                  0x13bc
#define XK_oe                  0x13bd
#define XK_Ydiaeresis          0x13be
#endif /* XK_LATIN9 */

/*
 * Katakana
 * Byte 3 = 4
 */



/*
 * Greek
 * Byte 3 = 7
 */
#define XK_Greek_OMEGA                                 0x7d9
#define XK_Greek_alpha                                 0x7e
#define XK_Greek_BETA                                  0x7c2
#define XK_Greek_GAMMA                                 0x7c3
#define XK_Greek_SIGMA                                 0x7d2
#define XK_Greek_delta                                 0x7e4
#define XK_Greek_THETA                                 0x7c8
#define XK_Greek_PHI                                   0x7d6
#define XK_Greek_pi                                    0x7f0
#define XK_Greek_sigma                                 0x7f2
#define XK_Greek_eta                                   0x7e7
#define XK_Greek_epsilon                               0x7e5
#define XK_Greek_tau                                   0x7f4


#define XK_topintegral                                 0x8a4
#define XK_botintegral                                 0x8a5
#define XK_similarequal                                0x8c9
#define XK_identical                                   0x8cf
#define XK_radical                                     0x8d6
#define XK_infinity                                    0x8c2
#define XK_intersection                                0x8dc
#define XK_greaterthanequal                            0x8be
#define XK_lessthanequal                               0x8bc

/*
 *  Special
 *  Byte 3 = 9
 */

#ifdef XK_SPECIAL
#define XK_blank                                       0x9df
#define XK_soliddiamond                                0x9e0
#define XK_checkerboard                                0x9e1
#define XK_ht                                          0x9e2
#define XK_ff                                          0x9e3
#define XK_cr                                          0x9e4
#define XK_lf                                          0x9e5
#define XK_nl                                          0x9e8
#define XK_vt                                          0x9e9
#define XK_lowrightcorner                              0x9ea
#define XK_uprightcorner                               0x9eb
#define XK_upleftcorner                                0x9ec
#define XK_lowleftcorner                               0x9ed
#define XK_crossinglines                               0x9ee
#define XK_horizlinescan1                              0x9ef
#define XK_horizlinescan3                              0x9f0
#define XK_horizlinescan5                              0x9f1
#define XK_horizlinescan7                              0x9f2
#define XK_horizlinescan9                              0x9f3
#define XK_leftt                                       0x9f4
#define XK_rightt                                      0x9f5
#define XK_bott                                        0x9f6
#define XK_topt                                        0x9f7
#define XK_vertbar                                     0x9f8
#endif /* XK_SPECIAL */

/*
 *  Publishing
 *  Byte 3 = a
 */

#ifdef XK_CURRENCY
#define XK_EcuSign					0x20a0
#define XK_ColonSign					0x20a1
#define XK_CruzeiroSign					0x20a2
#define XK_FFrancSign					0x20a3
#define XK_LiraSign					0x20a4
#define XK_MillSign					0x20a5
#define XK_NairaSign					0x20a6
#define XK_PesetaSign					0x20a7
#define XK_RupeeSign					0x20a8
#define XK_WonSign					0x20a9
#define XK_NewSheqelSign				0x20aa
#define XK_DongSign					0x20ab
#define XK_EuroSign					0x20ac
#endif

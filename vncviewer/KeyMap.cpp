//
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the VNC system is not available from the place
//  whence you received this file, check http://www.uvnc.com or
//  contact the authors on vnc@uk.research.att.com for information on obtaining
//  it.
//

#include "stdhdrs.h"
#include "vncviewer.h"
#include "KeyMap.h"


// ModifierKeyReleaser is a class which helps simplify generating a "fake" release
//   of modifier key (shift, ctrl, alt, etc.) to server. An instance of the class is
//   created for every modifier key which may need to be released.
// Then either release() may be called to send key release event for KeySym that is
//   associated with that modifier key.
// The destructor of the class automatically send the key press event for the released
//   KeySym if release() was called during the existance of the instance.
class ModifierKeyReleaser {
  public:
    ModifierKeyReleaser(ClientConnection* clientCon_, BYTE virtKey, bool extended)
      : clientCon(clientCon_), extVkey(virtKey + (extended ? 256 : 0)), keysym(0),
        modKeyName(_T('\0'))
    {
        switch (extVkey) {
            case VK_CONTROL :
            case VK_LCONTROL :
                modKeyName = _T("Left Ctrl ");
                break;
            case (VK_CONTROL + 256) :
            case VK_RCONTROL :
                modKeyName = _T("Right Ctrl ");
                break;
            case VK_MENU :
            case VK_LMENU :
                modKeyName = _T("Left Alt ");
                break;
            case (VK_MENU + 256) :
            case VK_RMENU :
                modKeyName = _T("Right Alt ");
                break;
        }
    }
    void release(CARD32* downKeysym) {
        if (downKeysym[extVkey] != XK_VoidSymbol) {
            keysym = downKeysym[extVkey];
           vnclog.Print(5, _T("  Fake release %s(%sVirtKey 0x%02x): "), modKeyName,
                      ((extVkey & 256) ? _T("extended ") : _T("")), (extVkey & 255));
            clientCon->SendKeyEvent(keysym, false);
        }
    }
    ~ModifierKeyReleaser() {
        if (keysym) {
           vnclog.Print(5, _T("Fake press %s(%sVirtKey 0x%02x)\n"), modKeyName,
                      ((extVkey & 256) ? _T("extended ") : _T("")), (extVkey & 255));
            clientCon->SendKeyEvent(keysym, true);
        }
    }
    ClientConnection *clientCon;
    UINT extVkey;
    CARD32 keysym;
    TCHAR *modKeyName;
};


KeyMap::KeyMap()
  : storedDeadChar(0), sendDeadKey(true), useUnicodeKeysym(true)
{
    BuildUCS2KS_Map();
}

void KeyMap::SetKeyMapOption1(bool sendDeadKeyOpt)
{
    sendDeadKey = sendDeadKeyOpt;
    if (sendDeadKeyOpt) storedDeadChar = 0;
}

void KeyMap::SetKeyMapOption2(bool useUnicodeKeysymOpt)
{
    useUnicodeKeysym = useUnicodeKeysymOpt;
}

void KeyMap::Reset()
{
   vnclog.Print(8, _T("Reset keyboard for first use\n"));
    storedDeadChar = 0;
    GetKeyboardState(KBKeysState);
    FlushDeadKey(KBKeysState);

    for (int i = 0; i < (sizeof(downKeysym) / sizeof(CARD32)); i++) {
        downKeysym[i] = XK_VoidSymbol;
    }

    for (int ii = 0; ii < (sizeof(downUnicode) / sizeof(WCHAR)); ii++) {
        downUnicode[ii] = NULL;
    }
}

// When losing focus the client should send key release to server for all sent key presses
void KeyMap::ReleaseAllKeys(ClientConnection* clientCon)
{
    for (int extVkey = 0; extVkey < (sizeof(downKeysym) / sizeof(CARD32)); extVkey++) {
        if (downKeysym[extVkey] != XK_VoidSymbol) {
           vnclog.Print(7, _T("\nReleasing %sVirtKey 0x%02x ->")
                         _T(" Send KeySym 0x%08x (release)\n"),
                      ((extVkey & 256) ? _T("extended ") : _T("")),
                      (extVkey & 255), downKeysym[extVkey]);
            clientCon->SendKeyEvent(downKeysym[extVkey], false);
            downKeysym[extVkey] = XK_VoidSymbol;
        }
    }
}

void KeyMap::FlushDeadKey(BYTE* keyState)
{
    // Flush out the stored dead key 'state' for next usage
    // To flush out the stored dead key 'state' we can not have any Alt key pressed
    keyState[VK_MENU] = keyState[VK_LMENU] = keyState[VK_RMENU] = 0;
    int ret = ToUnicode(VK_SPACE, 0, keyState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);
   vnclog.Print(8, _T("Flush dead key gives: "));

    if (ret < 0) {
       vnclog.Print(8, _T("dead key: 0x%04x (%c)\n"), *ucsChar, *ucsChar);
        FlushDeadKey(keyState);
    } else {
        for (int i=0; i < ret; i++) {
           vnclog.Print(8, _T("%d character(s): 0x%04x (%c)\n"),
                      ret, *(ucsChar+i), *(ucsChar+i));
        }
    }
}

void KeyMap::StoreModifier(BYTE* modifierState, BYTE* keyState)
{
    if (keyState[VK_SHIFT] & 0x80) *modifierState |= 1;
    if (keyState[VK_CONTROL] & 0x80) *modifierState |= 2;
    if (keyState[VK_MENU] & 0x80) *modifierState |= 4;
}

void KeyMap::SetModifier(BYTE modifierState, BYTE* keyState)
{
    if (modifierState & 1) {
        if ((keyState[VK_SHIFT] & 0x80) == 0) {
            keyState[VK_SHIFT] = keyState[VK_LSHIFT] = 0x80;
            keyState[VK_RSHIFT] = 0;
           vnclog.Print(8, _T("[Emulating Shift press] "));
        }
    } else {
        if ((keyState[VK_SHIFT] & 0x80) != 0) {
            keyState[VK_SHIFT] = keyState[VK_LSHIFT] = keyState[VK_RSHIFT] = 0;
           vnclog.Print(8, _T("[Emulating Shift release] "));
        }
    }
    if (modifierState & 2) {
        if ((keyState[VK_CONTROL] & 0x80) == 0) {
            keyState[VK_CONTROL] = keyState[VK_LCONTROL] = 0x80;
            keyState[VK_RCONTROL] = 0;
           vnclog.Print(8, _T("[Emulating Control press] "));
        }
    } else {
        if ((keyState[VK_CONTROL] & 0x80) != 0) {
            keyState[VK_CONTROL] = keyState[VK_LCONTROL] = keyState[VK_RCONTROL] = 0;
           vnclog.Print(8, _T("[Emulating Control release] "));
        }
    }
    if (modifierState & 4) {
        if ((keyState[VK_MENU] & 0x80) == 0) {
            keyState[VK_MENU] = keyState[VK_LMENU] = 0x80;
            keyState[VK_RMENU] = 0;
           vnclog.Print(8, _T("[Emulating Alt press] "));
        }
    } else {
        if ((keyState[VK_MENU] & 0x80) != 0) {
            keyState[VK_MENU] = keyState[VK_LMENU] = keyState[VK_RMENU] = 0;
           vnclog.Print(8, _T("[Emulating Alt release] "));
        }
    }
}

void KeyMap::BuildUCS2KS_Map(void)
{
    int     i;
    CARD32  j;
    WCHAR   ucs;

    // Building map for KeySym Latin-2, 3, 4, 8 & 9 and UCS Latin Extended-A & B
    for (i = 0, j = 0x01a1; i < (sizeof(keysym_to_unicode_1a1_1ff) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR) keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }
    for (i = 0, j = 0x02a1; i < (sizeof(keysym_to_unicode_2a1_2fe) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR) keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }
    for (i = 0, j = 0x03a2; i < (sizeof(keysym_to_unicode_3a2_3fe) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR) keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }
    for (i = 0, j = 0x12a1; i < (sizeof(keysym_to_unicode_12a1_12fe) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR) keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }
    for (i = 0, j = 0x13bc; i < (sizeof(keysym_to_unicode_13bc_13be) / sizeof(short)); i++, j++) {
        ksLatinExtMap[j] = ucs = (WCHAR) keysym_to_unicode_1a1_1ff[i];
        if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Katakana and UCS Katakana, CJK Symbols and Punctuation
    for (i = 0, j = 0x04a1; i < (sizeof(keysym_to_unicode_4a1_4df) / sizeof(short)); i++, j++) {
        ksKatakanaMap[j] = ucs = (WCHAR) keysym_to_unicode_4a1_4df[i];
        if (IsUCSKatakana(ucs))
            ucsKatakanaMap[ucs] = j;
          else if (IsUCSCJKSym_Punc(ucs))
            ucsCJKSym_PuncMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Arabic and UCS Arabic
    for (i = 0, j = 0x0590; i < (sizeof(keysym_to_unicode_590_5fe) / sizeof(short)); i++, j++) {
        ksArabicMap[j] = ucs = (WCHAR) keysym_to_unicode_590_5fe[i];
        if (IsUCSArabic(ucs))
            ucsArabicMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Cyrillic and UCS Cyrillic
    for (i = 0, j = 0x0680; i < (sizeof(keysym_to_unicode_680_6ff) / sizeof(short)); i++, j++) {
        ksCyrillicMap[j] = ucs = (WCHAR) keysym_to_unicode_680_6ff[i];
        if (IsUCSCyrillic(ucs))
            ucsCyrillicMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Greek and UCS Greek and Coptic
    for (i = 0, j = 0x07a1; i < (sizeof(keysym_to_unicode_7a1_7f9) / sizeof(short)); i++, j++) {
        ksGreekMap[j] = ucs = (WCHAR) keysym_to_unicode_7a1_7f9[i];
        if (IsUCSGreek_Coptic(ucs))
            ucsGreek_CopticMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Technical and UCS Others
    for (i = 0, j = 0x08a4; i < (sizeof(keysym_to_unicode_8a4_8fe) / sizeof(short)); i++, j++) {
        ksTechnicalMap[j] = ucs = (WCHAR) keysym_to_unicode_8a4_8fe[i];
        if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Special and UCS Others
    for (i = 0, j = 0x09df; i < (sizeof(keysym_to_unicode_9df_9f8) / sizeof(short)); i++, j++) {
        ksSpecialMap[j] = ucs = (WCHAR) keysym_to_unicode_aa1_afe[i];
        if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Publishing and UCS Others
    for (i = 0, j = 0x0aa1; i < (sizeof(keysym_to_unicode_aa1_afe) / sizeof(short)); i++, j++) {
        ksPublishingMap[j] = ucs = (WCHAR) keysym_to_unicode_aa1_afe[i];
        if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Hebrew and UCS Hebrew, General Punctuation
    for (i = 0, j = 0x0cdf; i < (sizeof(keysym_to_unicode_cdf_cfa) / sizeof(short)); i++, j++) {
        ksHebrewMap[j] = ucs = (WCHAR) keysym_to_unicode_cdf_cfa[i];
        if (IsUCSHebrew(ucs))
            ucsHebrewMap[ucs] = j;
          else if (IsUCSGenPunc(ucs))
            ucsGenPuncMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Thai and UCS Thai
    for (i = 0, j = 0x0da1; i < (sizeof(keysym_to_unicode_da1_df9) / sizeof(short)); i++, j++) {
        ksThaiMap[j] = ucs = (WCHAR) keysym_to_unicode_da1_df9[i];
        if (IsUCSThai(ucs))
            ucsThaiMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Korean and UCS Hangul Jamo
    for (i = 0, j = 0x0ea0; i < (sizeof(keysym_to_unicode_ea0_eff) / sizeof(short)); i++, j++) {
        ksKoreanMap[j] = ucs = (WCHAR) keysym_to_unicode_ea0_eff[i];
        if (IsUCSHangulJamo(ucs))
            ucsHangulJamoMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Armenian and UCS Armenian, General Punctuation
    for (i = 0, j = 0x14a1; i < (sizeof(keysym_to_unicode_14a1_14ff) / sizeof(short)); i++, j++) {
        ksArmenianMap[j] = ucs = (WCHAR) keysym_to_unicode_14a1_14ff[i];
        if (IsUCSArmenian(ucs))
            ucsArmenianMap[ucs] = j;
          else if (IsUCSGenPunc(ucs))
            ucsGenPuncMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Georgian and UCS Georgian
    for (i = 0, j = 0x15d0; i < (sizeof(keysym_to_unicode_15d0_15f6) / sizeof(short)); i++, j++) {
        ksGeorgianMap[j] = ucs = (WCHAR) keysym_to_unicode_15d0_15f6[i];
        if (IsUCSGeorgian(ucs))
            ucsGeorgianMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Azeri and UCS Others
    for (i = 0, j = 0x16a0; i < (sizeof(keysym_to_unicode_16a0_16f6) / sizeof(short)); i++, j++) {
        ksAzeriMap[j] = ucs = (WCHAR) keysym_to_unicode_16a0_16f6[i];
        if (IsUCSLatinExtAdd(ucs))
            ucsLatinExtAddMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Vietnamese and UCS Latin Extended Additional, Latin Extended-A & B
    for (i = 0, j = 0x1e9f; i < (sizeof(keysym_to_unicode_1e9f_1eff) / sizeof(short)); i++, j++) {
        ksVietnameseMap[j] = ucs = (WCHAR) keysym_to_unicode_1e9f_1eff[i];
        if (IsUCSLatinExtAdd(ucs))
            ucsLatinExtAddMap[ucs] = j;
          else if (IsUCSLatinExtAB(ucs))
            ucsLatinExtABMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }

    // Building map for KeySym Currency and UCS Currency Symbols
    for (i = 0, j = 0x20a0; i < (sizeof(keysym_to_unicode_20a0_20ac) / sizeof(short)); i++, j++) {
        ksCurrencyMap[j] = ucs = (WCHAR) keysym_to_unicode_20a0_20ac[i];
        if (IsUCSCurrSym(ucs))
            ucsCurrSymMap[ucs] = j;
          else if (ucs && (ucsOthersMap.find(ucs) != ucsOthersMap.end()))
            ucsOthersMap[ucs] = j;
    }
}

CARD32 KeyMap::UCS2X(WCHAR UnicodeChar)
{
    CARD32  XChar = XK_VoidSymbol;
    bool    isUnicodeKeysym = false;

    if (IsUCSPrintableLatin1(UnicodeChar)) {
        // If the key is printable Latin-1 character
        // Latin-1 in UCS and X KeySym are exactly the same code
       vnclog.Print(8, _T("  0x%04x (Latin-1 '%c'): "), UnicodeChar, UnicodeChar);
        XChar = (CARD32) (UnicodeChar & 0xffff);
    } else if (UnicodeChar < 255) {
        // Do not send non printable Latin-1 character
       vnclog.Print(8, _T("  0x%04x (Non printable Latin-1): "), UnicodeChar);
    } else if (IsUCSLatinExtAB(UnicodeChar)) {
        // Latin Extended-A or B
       vnclog.Print(8, _T("  0x%04x (Latin Extended-A or B '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsLatinExtABMap.find(UnicodeChar) != ucsLatinExtABMap.end()) {
            XChar = ucsLatinExtABMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsLatinExtABMap\n"));
        }
    } else if (IsUCSGreek_Coptic(UnicodeChar)) {
        // Greek and Coptic
       vnclog.Print(8, _T("  0x%04x (Greek and Coptic '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsGreek_CopticMap.find(UnicodeChar) != ucsGreek_CopticMap.end()) {
            XChar = ucsGreek_CopticMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsGreek_CopticMap\n"));
        }
    } else if (IsUCSCyrillic(UnicodeChar)) {
        // Cyrillic
       vnclog.Print(8, _T("  0x%04x (Cyrillic '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsCyrillicMap.find(UnicodeChar) != ucsCyrillicMap.end()) {
            XChar = ucsCyrillicMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsCyrillicMap\n"));
        }
    } else if (IsUCSArmenian(UnicodeChar)) {
        // Armenian
       vnclog.Print(8, _T("  0x%04x (Armenian '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsArmenianMap.find(UnicodeChar) != ucsArmenianMap.end()) {
            XChar = ucsArmenianMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsCyrillicMap\n"));
        }
    } else if (IsUCSHebrew(UnicodeChar)) {
        // Hebrew
       vnclog.Print(8, _T("  0x%04x (Hebrew '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsHebrewMap.find(UnicodeChar) != ucsHebrewMap.end()) {
            XChar = ucsHebrewMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsHebrewMap\n"));
        }
    } else if (IsUCSArabic(UnicodeChar)) {
        // Arabic
       vnclog.Print(8, _T("  0x%04x (Arabic '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsArabicMap.find(UnicodeChar) != ucsArabicMap.end()) {
            XChar = ucsArabicMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsArabicMap\n"));
        }
    } else if (IsUCSThai(UnicodeChar)) {
        // Thai
       vnclog.Print(8, _T("  0x%04x (Thai '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsThaiMap.find(UnicodeChar) != ucsThaiMap.end()) {
            XChar = ucsThaiMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsThaiMap\n"));
        }
    } else if (IsUCSGeorgian(UnicodeChar)) {
        // Georgian
       vnclog.Print(8, _T("  0x%04x (Georgian '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsGeorgianMap.find(UnicodeChar) != ucsGeorgianMap.end()) {
            XChar = ucsGeorgianMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsGeorgianMap\n"));
        }
    } else if (IsUCSHangulJamo(UnicodeChar)) {
        // Hangul Jamo
       vnclog.Print(8, _T("  0x%04x (Hangul Jamo '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsHangulJamoMap.find(UnicodeChar) != ucsHangulJamoMap.end()) {
            XChar = ucsHangulJamoMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsHangulJamoMap\n"));
        }
    } else if (IsUCSLatinExtAdd(UnicodeChar)) {
        // Latin Extended Additional
       vnclog.Print(8, _T("  0x%04x (Latin Extended Additional '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsLatinExtAddMap.find(UnicodeChar) != ucsLatinExtAddMap.end()) {
            XChar = ucsLatinExtAddMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsLatinExtAddMap\n"));
        }
    } else if (IsUCSGenPunc(UnicodeChar)) {
        // General Punctuation
       vnclog.Print(8, _T("  0x%04x (General Punctuation '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsGenPuncMap.find(UnicodeChar) != ucsGenPuncMap.end()) {
            XChar = ucsGenPuncMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsGenPuncMap\n"));
        }
    } else if (IsUCSCurrSym(UnicodeChar)) {
        // Currency Symbols
       vnclog.Print(8, _T("  0x%04x (Currency Symbols '%c'): "), UnicodeChar);
        if (ucsCurrSymMap.find(UnicodeChar) != ucsCurrSymMap.end()) {
            XChar = ucsCurrSymMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsCurrSymMap\n"));
        }
    } else if (IsUCSCJKSym_Punc(UnicodeChar)) {
        // CJK Symbols and Punctuation
       vnclog.Print(8, _T("  0x%04x (CJK Symbols and Punctuation '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsCJKSym_PuncMap.find(UnicodeChar) != ucsCJKSym_PuncMap.end()) {
            XChar = ucsCJKSym_PuncMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsCJKSym_PuncMap\n"));
        }
    } else if (IsUCSKatakana(UnicodeChar)) {
        // Katakana
       vnclog.Print(8, _T("  0x%04x (Katakana '%c'): "), UnicodeChar, UnicodeChar);
        if (ucsKatakanaMap.find(UnicodeChar) != ucsKatakanaMap.end()) {
            XChar = ucsKatakanaMap[UnicodeChar];
        } else if (useUnicodeKeysym) {
            isUnicodeKeysym = true;
        } else {
           vnclog.Print(8, _T("  The character is missing in ucsKatakanaMap\n"));
        }
    } else if (ucsOthersMap.find(UnicodeChar) != ucsOthersMap.end()) {
        // Other Unicode Characters
       vnclog.Print(8, _T("  0x%04x (Other Unicode Characters '%c'): "), UnicodeChar, UnicodeChar);
        XChar = ucsOthersMap[UnicodeChar];
    } else if (useUnicodeKeysym) {
        isUnicodeKeysym = true;
    } else {
       vnclog.Print(8, _T("  0x%04x (Not supported yet)\n"), UnicodeChar);
    }

    if (isUnicodeKeysym) {
       vnclog.Print(8, _T("  0x%04x (Other Unicode Characters '%c'): "), UnicodeChar, UnicodeChar);
        // Send the undefined X KeySym by using X protocol convention for
        //   defining new KeySym (Unicode KeySym):
        //   KeySym = ((UCS & 0xffffff) | 0x01000000)
        // Since Windows only support UCS-16 so we can have this formula:
        XChar = (((CARD32) UnicodeChar) & 0xffff) | 0x01000000;
    }

    return XChar;
}
bool reset=false;
void KeyMap::PCtoX(BYTE virtKey, DWORD keyData, ClientConnection* clientCon)
{
    bool down = ((keyData & 0x80000000) == 0);
    bool extended = ((keyData & 0x1000000) != 0);
    bool repeated = ((keyData & 0xc0000000) == 0x40000000);
    UINT extVkey = virtKey + (extended ? 256 : 0);

	// exclude winkey when not scroll-lock
	if (virtKey==91 || virtKey==92) return;

   vnclog.Print(8, _T("\nPCtoX: %svirtKey 0x%02x%s%s, keyData 0x%08x\n"),
              (extended ? _T("extended ") : _T("")), virtKey,
              (repeated ? _T(" repeated") : _T("")),
              (down ? _T(" down") : _T(" up")), keyData);

    // If this is a key release then just send the associated sent KeySym when
    //   this key was pressed
    if (!down) {
     vnclog.Print(8, _T("Release the associated KeySym when this VirtKey was pressed\n"));

      if (downUnicode[extVkey]) {
         vnclog.Print(8, _T("  0x%04x (%c): "), downUnicode[extVkey], downUnicode[extVkey]);
          downUnicode[extVkey] = NULL;
      } else {
         vnclog.Print(8, _T("  Control character: "));
      }

      releaseKey(clientCon, extVkey);
     vnclog.Print(8, _T("\n"));
	 GetKeyboardState(KBKeysState);
    if (!((KBKeysState[VK_MENU] & 0x80) && (KBKeysState[VK_CONTROL] & 0x80)))
	{
	 if (storedDeadChar && reset) {
 	 reset=false;
	 keybd_event(VK_SPACE, 0, 0, 0);
	 keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
	 }
	}
      return;
    }

    // We try to look it up in our key table
    // Look up the desired code in the keyMap table try to find the exact match according to
    //   the extended flag, then try the opposite of the extended flag
    CARD32 foundXCode = XK_VoidSymbol;
    bool exactMatched = false;
   vnclog.Print(8, _T("Looking in key table "));

    for (UINT i = 0; i < (sizeof(keyMap) / sizeof(vncKeyMapping_t)); i++) {
        if (keyMap[i].WinCode == virtKey) {
            foundXCode = keyMap[i].XCode;
            if (extended == keyMap[i].extVK) {
                exactMatched = true;
                break;
            }
        }
    }

    if (foundXCode != XK_VoidSymbol) {
       vnclog.Print(8, _T("-> keyMap gives (from %s extended flag) KeySym %u (0x%08x)\n"),
                  (exactMatched ? _T("matched") : _T("opposite")),
                  foundXCode, foundXCode);
        pressKey(clientCon, extVkey, foundXCode);
       vnclog.Print(8, _T("\n"));
        return;
    } else {
       vnclog.Print(8, _T("-> not in special keyMap\n"));
    }

    // Under CE, we're not so concerned about this bit because we handle a WM_CHAR message later
#ifndef UNDER_CE
    GetKeyboardState(KBKeysState);

    ModifierKeyReleaser lctrl(clientCon, VK_CONTROL, 0);
    ModifierKeyReleaser lalt(clientCon, VK_MENU, 0);
    ModifierKeyReleaser ralt(clientCon, VK_MENU, 1);

    if ((KBKeysState[VK_MENU] & 0x80) && (KBKeysState[VK_CONTROL] & 0x80)) {
        // This is a Ctrl-Alt (AltGr) key on international keyboards (= LCtrl-RAlt)
        // Ex. Ctrl-Alt-Q gives '@' on German keyboards
       vnclog.Print(8, _T("Ctrl-Alt pressed:\n"));

        // We must release Control and Alt (AltGr) if they were both pressed, so the character
        //   is seen without them by the VNC server
        // We don't release the Right Control; this allows German users
        //   to use it for doing Ctrl-AltGr-x, e.g. Ctl-@, etc
        lctrl.release(downKeysym);
        lalt.release(downKeysym);
        ralt.release(downKeysym);
    } else {
        // This is not a Ctrl-Alt (AltGr) key
       vnclog.Print(8, _T("Ctrl-Alt not pressed, fake release any Ctrl key\n"));

        // There are no KeySym corresponding to control characters, e.g. Ctrl-F
        // The server has already known whether the Ctrl key is pressed from the previouse key event
        // So we are interested in the key that would be there if the Ctrl key were not pressed
        KBKeysState[VK_CONTROL] = KBKeysState[VK_LCONTROL] = KBKeysState[VK_RCONTROL] = 0;
    }

	int ret;
    if (storedDeadChar) {
        SHORT virtDeadKey;
        BYTE prevModifierState = 0;

       vnclog.Print(8, _T("[Storing base character modifier(s)]\n"));
        StoreModifier(&prevModifierState, KBKeysState);
        virtDeadKey = VkKeyScanW(storedDeadChar);

       vnclog.Print(8, _T("[A dead key was stored, restoring the dead key state:")
                     _T(" 0x%02x (%c) using virtDeadKey 0x%02x] "),
                  storedDeadChar, storedDeadChar, virtDeadKey);
        SetModifier(HIBYTE(virtDeadKey), KBKeysState);
       vnclog.Print(8, _T("\n"));
        ToUnicode((virtDeadKey & 0xff), 0, KBKeysState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);

       vnclog.Print(8, _T("[Restoring base character modifier(s)] "));
        SetModifier(prevModifierState, KBKeysState);
       vnclog.Print(8, _T("\n"));

        storedDeadChar = 0;
		ret = ToUnicode(virtKey, 0, KBKeysState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);
    }

    else ret = ToUnicode(virtKey, 0, KBKeysState, ucsChar, (sizeof(ucsChar) / sizeof(WCHAR)), 0);
	if (ucsChar[0]==8364)
	{
		//euro
//		return;
	}
    if (ret < 0 || ret==2) {
        //  It is a dead key
       vnclog.Print(8, _T("ToUnicode returns dead key: 0x%02x (%c) "), *ucsChar, *ucsChar);

        if (sendDeadKey) {
            // We try to look it up in our dead key table
            // Look up the desired code in the deadKeyMap table
            foundXCode = XK_VoidSymbol;

            for (UINT i = 0; i < (sizeof(deadKeyMap) / sizeof(vncDeadKeyMapping_t)); i++) {
                if (deadKeyMap[i].deadKeyChar == *ucsChar) {
                    foundXCode = deadKeyMap[i].XCode;
                    break;
                }
            }

            if (foundXCode != XK_VoidSymbol) {
               vnclog.Print(8, _T("-> deadKeyMap gives KeySym %u (0x%08x)\n"),
                          foundXCode, foundXCode);
                pressKey(clientCon, extVkey, foundXCode);
            } else {
               vnclog.Print(8, _T("-> not in deadKeyMap\n"));
            }
        } else {
            storedDeadChar = *ucsChar;
			reset=true;
           vnclog.Print(8, _T("-> Store the dead key state, wait for next key-stroke\n"));
        }

        FlushDeadKey(KBKeysState);
    } else if (ret > 0) {
       vnclog.Print(8, _T("ToUnicode returns %d character(s):\n"), ret);

        for (int i = 0; i < ret; i++) {
            CARD32 xChar = UCS2X(*(ucsChar+i));
            if (xChar != XK_VoidSymbol) {
                downUnicode[extVkey] = *(ucsChar+i);
                pressKey(clientCon, extVkey, xChar);

            }
        }
    } else {
       vnclog.Print(8, _T("No character is generated by this key event\n"));
    }
#endif

   vnclog.Print(8, _T("\n"));
};

// This will send key release event to server if the VK key was previously pressed.
// The same KeySym that was sent when this VK key was pressed will be sent.
void KeyMap::releaseKey(ClientConnection* clientCon, UINT extVkey)
{
    if (downKeysym[extVkey] != XK_VoidSymbol) {
       vnclog.Print(4, _T("Send KeySym 0x%08x (release)\n"), downKeysym[extVkey]);
        clientCon->SendKeyEvent(downKeysym[extVkey], false);
        downKeysym[extVkey] = XK_VoidSymbol;
    } else {
       vnclog.Print(4, _T("No associated KeySym\n"));
    }
}

// This will send key press event to server if the VK key was not previously pressed.
// The sent KeySym will be recorded so in can be retrieved later when this VK key is
//   released.
// However it is possible that the VK key has been pressed by other KeySym, then
//   we need to release the old KeySym first.
// One example is in a repeated key press event (auto repeat), the modifiers state
//   may differ from the previous key press event. If this is the case then it may
//   generate different character(s). Since the  server may have different keyboard
//   layout, this character(s) may be mapped to different KeyCap (ScanCode).
// Other example is if a single key press generates several characterss
void KeyMap::pressKey(ClientConnection* clientCon, UINT extVkey, CARD32 keysym)
{
    if (downKeysym[extVkey] != XK_VoidSymbol) {
       vnclog.Print(4, _T("The %sVirtKey 0x%02x has been pressed -> Release previous")
                     _T(" associated KeySym 0x%08x\n"),
                  ((extVkey & 256) ? _T("extended ") : _T("")),
                  (extVkey & 255), downKeysym[extVkey]);
        clientCon->SendKeyEvent(downKeysym[extVkey], false);
        downKeysym[extVkey] = XK_VoidSymbol;
       vnclog.Print(4, _T("  New KeySym: "));
    }

   vnclog.Print(4, _T("Send KeySym 0x%08x (press)\n"), keysym);
    clientCon->SendKeyEvent(keysym, true);
    downKeysym[extVkey] = keysym;
}

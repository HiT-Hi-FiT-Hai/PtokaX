/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2010  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "SettingPageMyINFO.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageMyINFO::SettingPageMyINFO() {
    bUpdateNoTagMessage = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageMyINFO::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case CB_NO_TAG_ACTION:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32NoTagAction = (uint32_t)::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_NO_TAG_MESSAGE], ui32NoTagAction != 0 ? TRUE : FALSE);
                    ::EnableWindow(hWndPageItems[EDT_NO_TAG_REDIRECT], ui32NoTagAction == 2 ? TRUE : FALSE);
                    ::EnableWindow(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], ui32NoTagAction == 0 ? TRUE : FALSE);
                }

                break;
            case EDT_NO_TAG_MESSAGE:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
            case EDT_NO_TAG_REDIRECT:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
            case EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 999);

                    return 0;
                }

                break;
            case CB_ORIGINAL_MYINFO_ACTION:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    BOOL bEnable = ((uint32_t)::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_GETCURSEL, 0, 0) == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[BTN_REMOVE_DESCRIPTION], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_REMOVE_TAG], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_REMOVE_CONNECTION], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_REMOVE_EMAIL], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_MODE_TO_MYINFO], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_MODE_TO_DESCRIPTION], bEnable);
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::Save() {
    if(bCreated == false) {
        return;
    }

    SettingManager->SetBool(SETBOOL_ACCEPT_UNKNOWN_TAG, ::SendMessage(hWndPageItems[BTN_ACCEPT_UNKNOWN], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    char buf[257];
    int iLen = ::GetWindowText(hWndPageItems[EDT_NO_TAG_MESSAGE], buf, 257);

    if((int16_t)::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0) != SettingManager->iShorts[SETSHORT_NO_TAG_OPTION] ||
        strcmp(buf, SettingManager->sTexts[SETTXT_NO_TAG_MSG]) != NULL) {
        bUpdateNoTagMessage = true;
    }

    SettingManager->SetText(SETTXT_NO_TAG_MSG, buf, iLen);

    SettingManager->SetShort(SETSHORT_NO_TAG_OPTION, (int16_t)::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_GETCURSEL, 0, 0));

    iLen = ::GetWindowText(hWndPageItems[EDT_NO_TAG_REDIRECT], buf, 257);

    if(bUpdateNoTagMessage == false && ((SettingManager->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS]) != NULL))) {
        bUpdateNoTagMessage = true;
    }

    SettingManager->SetText(SETTXT_NO_TAG_REDIR_ADDRESS, buf, iLen);

    SettingManager->SetBool(SETBOOL_REPORT_SUSPICIOUS_TAG, ::SendMessage(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MYINFO_DELAY, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_FULL_MYINFO_OPTION, (int16_t)::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_GETCURSEL, 0, 0));

    SettingManager->SetBool(SETBOOL_STRIP_DESCRIPTION, ::SendMessage(hWndPageItems[BTN_REMOVE_DESCRIPTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_STRIP_TAG, ::SendMessage(hWndPageItems[BTN_REMOVE_TAG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_STRIP_CONNECTION, ::SendMessage(hWndPageItems[BTN_REMOVE_CONNECTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_STRIP_EMAIL, ::SendMessage(hWndPageItems[BTN_REMOVE_EMAIL], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_MODE_TO_MYINFO, ::SendMessage(hWndPageItems[BTN_MODE_TO_MYINFO], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_MODE_TO_DESCRIPTION, ::SendMessage(hWndPageItems[BTN_MODE_TO_DESCRIPTION], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & bUpdatedNoTagMessage,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
    bUpdatedNoTagMessage = bUpdateNoTagMessage;
}

//------------------------------------------------------------------------------
bool SettingPageMyINFO::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_DESCRIPTION_TAG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_DESCRIPTION_TAG], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 184,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_ACCEPT_UNKNOWN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT_UNKNOWN_TAG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 17, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ACCEPT_UNKNOWN], BM_SETCHECK, (SettingManager->bBools[SETBOOL_ACCEPT_UNKNOWN_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_NO_TAG_ACTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NO_TAG_ACTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 34, 437, 126,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_NO_TAG_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        13, 49, 421, 21, m_hWnd, (HMENU)CB_NO_TAG_ACTION, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_ACCEPT]);
    ::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_REJECT]);
    ::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_REDIR]);
    ::SendMessage(hWndPageItems[CB_NO_TAG_ACTION], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_NO_TAG_OPTION], 0);

    hWndPageItems[GB_NO_TAG_MESSAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 73, 427, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_NO_TAG_MESSAGE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_NO_TAG_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 18, 88, 411, 18, m_hWnd, (HMENU)EDT_NO_TAG_MESSAGE, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NO_TAG_MESSAGE], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[GB_NO_TAG_REDIRECT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 114, 427, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_NO_TAG_REDIRECT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_NO_TAG_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | ES_AUTOHSCROLL, 18, 129, 411, 18, m_hWnd, (HMENU)EDT_NO_TAG_REDIRECT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NO_TAG_REDIRECT], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_NO_TAG_REDIRECT], LanguageManager->sTexts[LAN_REDIRECT_HINT]);

    hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REPORT_BAD_TAGS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 165, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REPORT_SUSPICIOUS_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_MYINFO_PROCESSING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MYINFO_PROCESSING], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 187, 447, 201, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_MINUTES_BEFORE_ACCEPT_NEW_MYINFO] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MYINFO_DELAY],
        WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 206, 299, 16, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER |
        ES_AUTOHSCROLL | ES_RIGHT, 312, 202, 110, 20, m_hWnd, (HMENU)EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MINUTES_BEFORE_ACCEPT_NEW_MYINFO], 422, 202, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_MINUTES_BEFORE_ACCEPT_NEW_MYINFO],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MYINFO_DELAY], 0));

    hWndPageItems[LBL_ORIGINAL_MYINFO] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_ORIGINAL_MYINFO],
        WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 233, 136, 16, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_ORIGINAL_MYINFO_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        149, 228, 290, 21, m_hWnd, (HMENU)CB_ORIGINAL_MYINFO_ACTION, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_FULL_MYINFO_ALL]);
    ::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_FULL_MYINFO_PROFILE]);
    ::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_FULL_MYINFO_NONE]);
    ::SendMessage(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION], 0);
    AddToolTip(hWndPageItems[CB_ORIGINAL_MYINFO_ACTION], LanguageManager->sTexts[LAN_MYINFO_TO_HINT]);

    hWndPageItems[GB_MODIFIED_MYINFO_OPTIONS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_OPTIONS_FOR_MODIFIED_MYINFO], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, 252, 437, 131, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_REMOVE_DESCRIPTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REMOVE_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 266, 421, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REMOVE_DESCRIPTION], BM_SETCHECK, (SettingManager->bBools[SETBOOL_STRIP_DESCRIPTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_REMOVE_TAG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REMOVE_TAG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 285, 421, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REMOVE_TAG], BM_SETCHECK, (SettingManager->bBools[SETBOOL_STRIP_TAG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_REMOVE_CONNECTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REMOVE_CONNECTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 304, 421, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REMOVE_CONNECTION], BM_SETCHECK, (SettingManager->bBools[SETBOOL_STRIP_CONNECTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_REMOVE_EMAIL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REMOVE_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 323, 421, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REMOVE_EMAIL], BM_SETCHECK, (SettingManager->bBools[SETBOOL_STRIP_EMAIL] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_MODE_TO_MYINFO] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_MODE_TO_MYINFO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 342, 421, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_MODE_TO_MYINFO], BM_SETCHECK, (SettingManager->bBools[SETBOOL_MODE_TO_MYINFO] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_MODE_TO_DESCRIPTION] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_MODE_TO_DESCR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 361, 421, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_MODE_TO_DESCRIPTION], BM_SETCHECK, (SettingManager->bBools[SETBOOL_MODE_TO_DESCRIPTION] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[EDT_NO_TAG_MESSAGE], SettingManager->iShorts[SETSHORT_NO_TAG_OPTION] != 0 ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[EDT_NO_TAG_REDIRECT], SettingManager->iShorts[SETSHORT_NO_TAG_OPTION] == 2 ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[BTN_REPORT_SUSPICIOUS_TAG], SettingManager->iShorts[SETSHORT_NO_TAG_OPTION] == 0 ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[BTN_REMOVE_DESCRIPTION], SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[BTN_REMOVE_TAG], SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[BTN_REMOVE_CONNECTION], SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[BTN_REMOVE_EMAIL], SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[BTN_MODE_TO_MYINFO], SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[BTN_MODE_TO_DESCRIPTION], SettingManager->iShorts[SETSHORT_FULL_MYINFO_OPTION] == 0 ? FALSE : TRUE);

    ::SetWindowLongPtr(hWndPageItems[BTN_MODE_TO_DESCRIPTION], GWLP_USERDATA, (LONG_PTR)this);
    wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_MODE_TO_DESCRIPTION], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageMyINFO::GetPageName() {
    return LanguageManager->sTexts[LAN_MYINFO_PROCESSING];
}
//------------------------------------------------------------------------------

void SettingPageMyINFO::FocusLastItem() {
    ::SetFocus(hWndPageItems[BTN_MODE_TO_DESCRIPTION]);
}
//------------------------------------------------------------------------------

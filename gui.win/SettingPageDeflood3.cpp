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
#include "SettingPageDeflood3.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageDeflood3::SettingPageDeflood3() {
    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood3::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_SEARCH_MSGS:
            case EDT_SEARCH_SECS:
            case EDT_SEARCH_MSGS2:
            case EDT_SEARCH_SECS2:
            case EDT_SEARCH_INTERVAL_MSGS:
            case EDT_SEARCH_INTERVAL_SECS:
            case EDT_SAME_SEARCH_MSGS:
            case EDT_SAME_SEARCH_SECS:
            case EDT_GETNICKLIST_MSGS:
            case EDT_GETNICKLIST_MINS:
            case EDT_MYINFO_MSGS:
            case EDT_MYINFO_SECS:
            case EDT_MYINFO_MSGS2:
            case EDT_MYINFO_SECS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 999);

                    return 0;
                }

                break;
            case EDT_SR_MSGS:
            case EDT_SR_MSGS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 32767);

                    return 0;
                }

                break;
            case EDT_SR_SECS:
            case EDT_SR_SECS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 9999);

                    return 0;
                }

                break;
            case EDT_SR_TO_PASSIVE_LIMIT:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 0, 32767);

                    return 0;
                }

                break;
            case EDT_SR_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 8192);

                    return 0;
                }

                break;
            case EDT_MYINFO_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 64, 512);

                    return 0;
                }

                break;
            case EDT_RECONNECT_TIME:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 256);

                    return 0;
                }

                break;
            case CB_SEARCH:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SEARCH], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SEARCH_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SEARCH_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SEARCH2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SEARCH2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SEARCH_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SEARCH_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SEARCH_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SEARCH_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SEARCH_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SEARCH_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SAME_SEARCH:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SAME_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_SEARCH_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_SEARCH_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SAME_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_SEARCH_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_SEARCH_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SR:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SR], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SR_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SR_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SR_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SR_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SR_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SR_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SR2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SR2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SR_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SR_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SR_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SR_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SR_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SR_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_GETNICKLIST:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_GETNICKLIST_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_GETNICKLIST_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_GETNICKLIST_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_GETNICKLIST_MINS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_GETNICKLIST_MINS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_GETNICKLIST_MINUTES], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_MYINFO:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_MYINFO], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_MYINFO_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MYINFO_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MYINFO_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_MYINFO_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MYINFO_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MYINFO_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_MYINFO2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_MYINFO2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_MYINFO_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MYINFO_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MYINFO_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_MYINFO_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MYINFO_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MYINFO_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood3::Save() {
    if(bCreated == false) {
        return;
    }

    SettingManager->SetShort(SETSHORT_SEARCH_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SEARCH], CB_GETCURSEL, 0, 0));

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_SEARCH_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SEARCH_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SEARCH_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SEARCH_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_SEARCH2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SEARCH_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SEARCH_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SEARCH_INTERVAL_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_INTERVAL_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SEARCH_INTERVAL_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SAME_SEARCH_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SAME_SEARCH_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_SEARCH_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SAME_SEARCH_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_SEARCH_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SR_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SR], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SR_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SR_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SR_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SR_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SR_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_SR2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SR_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SR_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SR_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SR_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SR_TO_PASSIVE_LIMIT], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_PASIVE_SR, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SR_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_SR_LEN, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_GETNICKLIST_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_GETNICKLIST_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_GETNICKLIST_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_GETNICKLIST_MINS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_GETNICKLIST_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_MYINFO_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_MYINFO], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_MYINFO_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MYINFO_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MYINFO_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MYINFO_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_MYINFO_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_MYINFO2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_MYINFO_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MYINFO_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MYINFO_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MYINFO_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MYINFO_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_MYINFO_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_RECONNECT_TIME], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MIN_RECONN_TIME, LOWORD(lResult));
    }
}
//------------------------------------------------------------------------------

void SettingPageDeflood3::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
}
//------------------------------------------------------------------------------

bool SettingPageDeflood3::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_SEARCH] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SEARCH], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 97,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SEARCH] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 18, 180, 21, m_hWnd, (HMENU)CB_SEARCH, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SEARCH], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SEARCH_ACTION], 0);

    hWndPageItems[EDT_SEARCH_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 18, 30, 21, m_hWnd, (HMENU)EDT_SEARCH_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SEARCH_MSGS], 223, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SEARCH_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SEARCH_MESSAGES], 0));

    hWndPageItems[LBL_SEARCH_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 23, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SEARCH_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 18, 30, 21, m_hWnd, (HMENU)EDT_SEARCH_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SEARCH_SECS], 285, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SEARCH_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SEARCH_TIME], 0));

    hWndPageItems[LBL_SEARCH_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 23, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SEARCH2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 45, 180, 21, m_hWnd, (HMENU)CB_SEARCH2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SEARCH2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SEARCH_ACTION2], 0);

    hWndPageItems[EDT_SEARCH_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 45, 30, 21, m_hWnd, (HMENU)EDT_SEARCH_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_MSGS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SEARCH_MSGS2], 223, 45, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SEARCH_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SEARCH_MESSAGES2], 0));

    hWndPageItems[LBL_SEARCH_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 50, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SEARCH_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 45, 30, 21, m_hWnd, (HMENU)EDT_SEARCH_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SEARCH_SECS2], 285, 45, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SEARCH_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SEARCH_TIME2], 0));

    hWndPageItems[LBL_SEARCH_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 50, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_SEARCH_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT, 8, 76, 180, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SEARCH_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 72, 30, 20, m_hWnd, (HMENU)EDT_SEARCH_INTERVAL_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SEARCH_INTERVAL_MSGS], 223, 72, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SEARCH_INTERVAL_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SEARCH_INTERVAL_MESSAGES], 0));

    hWndPageItems[LBL_SEARCH_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 76, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SEARCH_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 72, 30, 20, m_hWnd, (HMENU)EDT_SEARCH_INTERVAL_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SEARCH_INTERVAL_SECS], 285, 72, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SEARCH_INTERVAL_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SEARCH_INTERVAL_TIME], 0));

    hWndPageItems[LBL_SEARCH_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 76, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SAME_SEARCH] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SAME_SEARCH], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 100, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SAME_SEARCH] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 115, 180, 21, m_hWnd, (HMENU)CB_SAME_SEARCH, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_SEARCH], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION], 0);

    hWndPageItems[EDT_SAME_SEARCH_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 115, 30, 21, m_hWnd, (HMENU)EDT_SAME_SEARCH_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_SEARCH_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_SEARCH_MSGS], 223, 115, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_SEARCH_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_SEARCH_MESSAGES], 0));

    hWndPageItems[LBL_SAME_SEARCH_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 120, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SAME_SEARCH_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 115, 30, 21, m_hWnd, (HMENU)EDT_SAME_SEARCH_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_SEARCH_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_SEARCH_SECS], 285, 115, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_SEARCH_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_SEARCH_TIME], 0));

    hWndPageItems[LBL_SAME_SEARCH_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 120, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SR], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 144, 447, 71,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SR] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 159, 180, 21, m_hWnd, (HMENU)CB_SR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SR], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SR], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SR_ACTION], 0);

    hWndPageItems[EDT_SR_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 159, 40, 21, m_hWnd, (HMENU)EDT_SR_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SR_MSGS], EM_SETLIMITTEXT, 5, 0);

    AddUpDown(hWndPageItems[UD_SR_MSGS], 233, 159, 17, 21, (LPARAM)MAKELONG(32767, 1), (WPARAM)hWndPageItems[EDT_SR_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SR_MESSAGES], 0));

    hWndPageItems[LBL_SR_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 255, 164, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SR_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        265, 159, 30, 21, m_hWnd, (HMENU)EDT_SR_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SR_SECS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_SR_SECS], 295, 159, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_SR_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SR_TIME], 0));

    hWndPageItems[LBL_SR_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 317, 164, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SR2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 186, 180, 21, m_hWnd, (HMENU)CB_SR2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SR2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SR_ACTION2], 0);

    hWndPageItems[EDT_SR_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 186, 40, 21, m_hWnd, (HMENU)EDT_SR_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SR_MSGS2], EM_SETLIMITTEXT, 5, 0);

    AddUpDown(hWndPageItems[UD_SR_MSGS2], 233, 186, 17, 21, (LPARAM)MAKELONG(32767, 1), (WPARAM)hWndPageItems[EDT_SR_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SR_MESSAGES2], 0));

    hWndPageItems[LBL_SR_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 255, 191, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SR_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        265, 186, 30, 21, m_hWnd, (HMENU)EDT_SR_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SR_SECS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_SR_SECS2], 295, 186, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_SR_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SR_TIME2], 0));

    hWndPageItems[LBL_SR_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 317, 191, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SR_TO_PASSIVE_LIMIT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PSR_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 215, 221, 43,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        82, 230, 40, 20, m_hWnd, (HMENU)EDT_SR_TO_PASSIVE_LIMIT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT], EM_SETLIMITTEXT, 5, 0);
    AddToolTip(hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_SR_TO_PASSIVE_LIMIT], 122, 230, 17, 20, (LPARAM)MAKELONG(32767, 0), (WPARAM)hWndPageItems[EDT_SR_TO_PASSIVE_LIMIT],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_PASIVE_SR], 0));

    hWndPageItems[GB_SR_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SR_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        226, 215, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_SR_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT, 234, 234, 143, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SR_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 230, 40, 20, m_hWnd, (HMENU)EDT_SR_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SR_LEN], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_SR_LEN], 422, 230, 17, 20, (LPARAM)MAKELONG(8192, 1), (WPARAM)hWndPageItems[EDT_SR_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_SR_LEN], 0));

    hWndPageItems[GB_GETNICKLIST] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_GETNICKLISTS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 258, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_GETNICKLIST] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 273, 180, 21, m_hWnd, (HMENU)CB_GETNICKLIST, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_GETNICKLIST], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION], 0);

    hWndPageItems[EDT_GETNICKLIST_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 273, 30, 21, m_hWnd, (HMENU)EDT_GETNICKLIST_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_GETNICKLIST_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_GETNICKLIST_MSGS], 223, 273, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_GETNICKLIST_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_GETNICKLIST_MESSAGES], 0));

    hWndPageItems[LBL_GETNICKLIST_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 278, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_GETNICKLIST_MINS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 273, 30, 21, m_hWnd, (HMENU)EDT_GETNICKLIST_MINS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_GETNICKLIST_MINS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_GETNICKLIST_MINS], 285, 273, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_GETNICKLIST_MINS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_GETNICKLIST_TIME], 0));

    hWndPageItems[LBL_GETNICKLIST_MINUTES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 278, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MYINFO] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MYINFOS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 302, 447, 71,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_MYINFO] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 317, 180, 21, m_hWnd, (HMENU)CB_MYINFO, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_MYINFO], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MYINFO_ACTION], 0);

    hWndPageItems[EDT_MYINFO_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 317, 30, 21, m_hWnd, (HMENU)EDT_MYINFO_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MYINFO_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MYINFO_MSGS], 223, 317, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MYINFO_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MYINFO_MESSAGES], 0));

    hWndPageItems[LBL_MYINFO_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 322, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MYINFO_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 317, 30, 21, m_hWnd, (HMENU)EDT_MYINFO_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MYINFO_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MYINFO_SECS], 285, 317, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MYINFO_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MYINFO_TIME], 0));

    hWndPageItems[LBL_MYINFO_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 322, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_MYINFO2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 344, 180, 21, m_hWnd, (HMENU)CB_MYINFO2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_MYINFO2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MYINFO_ACTION2], 0);

    hWndPageItems[EDT_MYINFO_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 344, 30, 21, m_hWnd, (HMENU)EDT_MYINFO_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MYINFO_MSGS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MYINFO_MSGS2], 223, 344, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MYINFO_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MYINFO_MESSAGES2], 0));

    hWndPageItems[LBL_MYINFO_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 349, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MYINFO_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 344, 30, 21, m_hWnd, (HMENU)EDT_MYINFO_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MYINFO_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MYINFO_SECS2], 285, 344, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MYINFO_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MYINFO_TIME2], 0));

    hWndPageItems[LBL_MYINFO_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 344, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MYINFO_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MYINFO_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 373, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_MYINFO_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 392, 143, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MYINFO_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        156, 388, 40, 20, m_hWnd, (HMENU)EDT_MYINFO_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MYINFO_LEN], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MYINFO_LEN], 196, 388, 17, 20, (LPARAM)MAKELONG(512, 64), (WPARAM)hWndPageItems[EDT_MYINFO_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_MYINFO_LEN], 0));

    hWndPageItems[GB_RECONNECT_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_RECONN_TIME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        226, 373, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_RECONNECT_TIME_MINIMAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MINIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT, 234, 392, 143, 69,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_RECONNECT_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        308, 388, 40, 20, m_hWnd, (HMENU)EDT_RECONNECT_TIME, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RECONNECT_TIME], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_RECONNECT_TIME], 348, 388, 17, 20, (LPARAM)MAKELONG(256, 1), (WPARAM)hWndPageItems[EDT_RECONNECT_TIME],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME], 0));

    hWndPageItems[LBL_RECONNECT_TIME_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 370, 392, 143, 69,
        m_hWnd, NULL, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", GetPageName(), MB_OK);
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[EDT_SEARCH_MSGS], SettingManager->iShorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SEARCH_MSGS], SettingManager->iShorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SEARCH_DIVIDER], SettingManager->iShorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SEARCH_SECS], SettingManager->iShorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SEARCH_SECS], SettingManager->iShorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SEARCH_SECONDS], SettingManager->iShorts[SETSHORT_SEARCH_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SEARCH_MSGS2], SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SEARCH_MSGS2], SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SEARCH_DIVIDER2], SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SEARCH_SECS2], SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SEARCH_SECS2], SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SEARCH_SECONDS2], SettingManager->iShorts[SETSHORT_SEARCH_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SAME_SEARCH_MSGS], SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_SEARCH_MSGS], SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_SEARCH_DIVIDER], SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SAME_SEARCH_SECS], SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_SEARCH_SECS], SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_SEARCH_SECONDS], SettingManager->iShorts[SETSHORT_SAME_SEARCH_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SR_MSGS], SettingManager->iShorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SR_MSGS], SettingManager->iShorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SR_DIVIDER], SettingManager->iShorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SR_SECS], SettingManager->iShorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SR_SECS], SettingManager->iShorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SR_SECONDS], SettingManager->iShorts[SETSHORT_SR_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SR_MSGS2], SettingManager->iShorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SR_MSGS2], SettingManager->iShorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SR_DIVIDER2], SettingManager->iShorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SR_SECS2], SettingManager->iShorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SR_SECS2], SettingManager->iShorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SR_SECONDS2], SettingManager->iShorts[SETSHORT_SR_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_GETNICKLIST_MSGS], SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_GETNICKLIST_MSGS], SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_GETNICKLIST_DIVIDER], SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_GETNICKLIST_MINS], SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_GETNICKLIST_MINS], SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_GETNICKLIST_MINUTES], SettingManager->iShorts[SETSHORT_GETNICKLIST_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_MYINFO_MSGS], SettingManager->iShorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MYINFO_MSGS], SettingManager->iShorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MYINFO_DIVIDER], SettingManager->iShorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_MYINFO_SECS], SettingManager->iShorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MYINFO_SECS], SettingManager->iShorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MYINFO_SECONDS], SettingManager->iShorts[SETSHORT_MYINFO_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_MYINFO_MSGS2], SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MYINFO_MSGS2], SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MYINFO_DIVIDER2], SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_MYINFO_SECS2], SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MYINFO_SECS2], SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MYINFO_SECONDS2], SettingManager->iShorts[SETSHORT_MYINFO_ACTION2] == 0 ? FALSE : TRUE);

    ::SetWindowLongPtr(hWndPageItems[EDT_RECONNECT_TIME], GWLP_USERDATA, (LONG_PTR)this);
    wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_RECONNECT_TIME], GWLP_WNDPROC, (LONG_PTR)EditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood3::GetPageName() {
    return LanguageManager->sTexts[LAN_MORE_MORE_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood3::FocusLastItem() {
    ::SetFocus(hWndPageItems[EDT_RECONNECT_TIME]);
}
//------------------------------------------------------------------------------

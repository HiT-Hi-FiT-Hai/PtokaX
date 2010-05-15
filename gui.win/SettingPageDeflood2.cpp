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
#include "SettingPageDeflood2.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageDeflood2::SettingPageDeflood2() {
    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_PM_MSGS:
            case EDT_PM_SECS:
            case EDT_PM_MSGS2:
            case EDT_PM_SECS2:
            case EDT_PM_INTERVAL_MSGS:
            case EDT_PM_INTERVAL_SECS:
            case EDT_SAME_PM_MSGS:
            case EDT_SAME_PM_SECS:
            case EDT_WARN_COUNT:
            case EDT_NEW_CONNS_FROM_SAME_IP_COUNT:
            case EDT_NEW_CONNS_FROM_SAME_IP_TIME:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 999);

                    return 0;
                }

                break;
            case EDT_SAME_MULTI_PM_MSGS:
            case EDT_SAME_MULTI_PM_LINES:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 2, 999);

                    return 0;
                }

                break;
            case EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 0, 999);

                    return 0;
                }

                break;
            case EDT_RECEIVED_DATA_KB:
            case EDT_RECEIVED_DATA_SECS:
            case EDT_RECEIVED_DATA_KB2:
            case EDT_RECEIVED_DATA_SECS2:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 9999);

                    return 0;
                }

                break;
            case EDT_MAX_USERS_FROM_SAME_IP:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 256);

                    return 0;
                }

                break;
            case CB_PM:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_PM], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_PM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_PM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_PM2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_PM2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_PM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_PM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_PM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_PM_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_PM_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_PM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SAME_PM:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_PM], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SAME_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_PM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SAME_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_PM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_PM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SAME_MULTI_PM:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_WITH], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_LINES], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_LINES], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_LINES], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_RECEIVED_DATA:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_KB], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_KB], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_RECEIVED_DATA2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_KB2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_KB2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_RECEIVED_DATA_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RECEIVED_DATA_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RECEIVED_DATA_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood2::Save() {
    if(bCreated == false) {
        return;
    }

    SettingManager->SetShort(SETSHORT_PM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_PM], CB_GETCURSEL, 0, 0));

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_PM_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_PM_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_PM_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_PM_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_PM2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_PM_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_PM_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_PM_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_PM_INTERVAL_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_INTERVAL_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_PM_INTERVAL_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SAME_PM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_PM], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SAME_PM_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_PM_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SAME_PM_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_PM_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SAME_MULTI_PM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_PM_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_MULTI_PM_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_PM_LINES], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_MULTI_PM_LINES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_PM_COUNT_TO_USER, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_MAX_DOWN_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_KB], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_DOWN_KB, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_DOWN_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_MAX_DOWN_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_KB2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_DOWN_KB2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_RECEIVED_DATA_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_DOWN_TIME2, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_DEFLOOD_WARNING_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_WARN_COUNT], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_DEFLOOD_WARNING_COUNT, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_COUNT], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_NEW_CONNECTIONS_COUNT, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_TIME], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_NEW_CONNECTIONS_TIME, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAX_USERS_FROM_SAME_IP], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_CONN_SAME_IP, LOWORD(lResult));
    }

    SettingManager->SetBool(SETBOOL_DEFLOOD_REPORT, ::SendMessage(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageDeflood2::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
}
//------------------------------------------------------------------------------

bool SettingPageDeflood2::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_PM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PRIVATE_MSGS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 97,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_PM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 18, 180, 21, m_hWnd, (HMENU)CB_PM, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_PM], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_PM_ACTION], 0);

    hWndPageItems[EDT_PM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 18, 30, 21, m_hWnd, (HMENU)EDT_PM_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_PM_MSGS], 223, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_PM_MESSAGES], 0));

    hWndPageItems[LBL_PM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 23, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 18, 30, 21, m_hWnd, (HMENU)EDT_PM_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_PM_SECS], 285, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_PM_TIME], 0));

    hWndPageItems[LBL_PM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 23, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_PM2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 45, 180, 21, m_hWnd, (HMENU)CB_PM2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_PM2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_PM_ACTION2], 0);

    hWndPageItems[EDT_PM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 45, 30, 21, m_hWnd, (HMENU)EDT_PM_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_MSGS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_PM_MSGS2], 223, 45, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_PM_MESSAGES2], 0));

    hWndPageItems[LBL_PM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 50, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 45, 30, 21, m_hWnd, (HMENU)EDT_PM_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_PM_SECS2], 285, 45, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_PM_TIME2], 0));

    hWndPageItems[LBL_PM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 50, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_PM_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT, 8, 76, 180, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 72, 30, 20, m_hWnd, (HMENU)EDT_PM_INTERVAL_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_PM_INTERVAL_MSGS], 223, 72, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_INTERVAL_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_PM_INTERVAL_MESSAGES], 0));

    hWndPageItems[LBL_PM_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 76, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 72, 30, 20, m_hWnd, (HMENU)EDT_PM_INTERVAL_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_PM_INTERVAL_SECS], 285, 72, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_PM_INTERVAL_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_PM_INTERVAL_TIME], 0));

    hWndPageItems[LBL_PM_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 76, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SAME_PM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SAME_PRIVATE_MSGS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 100, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SAME_PM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 115, 180, 21, m_hWnd, (HMENU)CB_SAME_PM, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_PM], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SAME_PM_ACTION], 0);

    hWndPageItems[EDT_SAME_PM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 115, 30, 21, m_hWnd, (HMENU)EDT_SAME_PM_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_PM_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_PM_MSGS], 223, 115, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_PM_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_PM_MESSAGES], 0));

    hWndPageItems[LBL_SAME_PM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 120, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SAME_PM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 115, 30, 21, m_hWnd, (HMENU)EDT_SAME_PM_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_PM_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_PM_SECS], 285, 115, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_PM_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_PM_TIME], 0));

    hWndPageItems[LBL_SAME_PM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 120, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SAME_MULTI_PM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SAME_MULTI_PRIVATE_MSGS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 144, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SAME_MULTI_PM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 159, 180, 21, m_hWnd, (HMENU)CB_SAME_MULTI_PM, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_PM], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION], 0);

    hWndPageItems[EDT_SAME_MULTI_PM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 159, 30, 21, m_hWnd, (HMENU)EDT_SAME_MULTI_PM_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_MULTI_PM_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_MULTI_PM_MSGS], 223, 159, 17, 21, (LPARAM)MAKELONG(999, 2), (WPARAM)hWndPageItems[EDT_SAME_MULTI_PM_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_MESSAGES], 0));

    hWndPageItems[LBL_SAME_MULTI_PM_WITH] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_WITH_LWR], WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 164, 30, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SAME_MULTI_PM_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        280, 159, 30, 21, m_hWnd, (HMENU)EDT_SAME_MULTI_PM_LINES, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_MULTI_PM_LINES], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_MULTI_PM_LINES], 310, 159, 17, 21, (LPARAM)MAKELONG(999, 2), (WPARAM)hWndPageItems[EDT_SAME_MULTI_PM_LINES],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_LINES], 0));

    hWndPageItems[LBL_SAME_MULTI_PM_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_LINES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 332, 164, 107, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PM_LIM_TO_URS_FRM_MULTI_NICKS],
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 188, 447, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAX_MSGS_TO_ONE_USER_PER_MIN],
        WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 207, 379, 16, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        392, 203, 30, 20, m_hWnd, (HMENU)EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS], 422, 203, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_PM_LIMIT_TO_ONE_USER_FROM_MULTIPLE_USERS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_PM_COUNT_TO_USER], 0));

    hWndPageItems[GB_RECEIVED_DATA] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_RECV_DATA_FROM_USER], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 231, 447, 71,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_RECEIVED_DATA] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 246, 180, 21, m_hWnd, (HMENU)CB_RECEIVED_DATA, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION], 0);

    hWndPageItems[EDT_RECEIVED_DATA_KB] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 246, 30, 21, m_hWnd, (HMENU)EDT_RECEIVED_DATA_KB, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_KB], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RECEIVED_DATA_KB], 223, 246, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_KB],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_DOWN_KB], 0));

    hWndPageItems[LBL_RECEIVED_DATA_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "kB /", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 251, 30, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_RECEIVED_DATA_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        280, 246, 30, 21, m_hWnd, (HMENU)EDT_RECEIVED_DATA_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_SECS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RECEIVED_DATA_SECS], 310, 246, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_DOWN_TIME], 0));

    hWndPageItems[LBL_RECEIVED_DATA_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 332, 251, 107, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_RECEIVED_DATA2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 273, 180, 21, m_hWnd, (HMENU)CB_RECEIVED_DATA2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_RECEIVED_DATA2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MAX_DOWN_ACTION2], 0);

    hWndPageItems[EDT_RECEIVED_DATA_KB2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 273, 30, 21, m_hWnd, (HMENU)EDT_RECEIVED_DATA_KB2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_KB2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RECEIVED_DATA_KB2], 223, 273, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_KB2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_DOWN_KB2], 0));

    hWndPageItems[LBL_RECEIVED_DATA_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "kB /", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 278, 30, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_RECEIVED_DATA_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        280, 273, 30, 21, m_hWnd, (HMENU)EDT_RECEIVED_DATA_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RECEIVED_DATA_SECS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RECEIVED_DATA_SECS2], 310, 273, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RECEIVED_DATA_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_DOWN_TIME2], 0));

    hWndPageItems[LBL_RECEIVED_DATA_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 332, 278, 107, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_WARN_SETTING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_WARN_SET], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 302, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_WARN_ACTION] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 317, 180, 21, m_hWnd, (HMENU)CB_RECEIVED_DATA2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_WARN_ACTION], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_DEFLOOD_WARNING_ACTION], 0);

    hWndPageItems[LBL_WARN_AFTER] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_AFTER], WS_CHILD | WS_VISIBLE | SS_CENTER, 193, 322, 40, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_WARN_COUNT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        238, 317, 30, 21, m_hWnd, (HMENU)EDT_WARN_COUNT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_WARN_COUNT], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_WARN_COUNT], 268, 317, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_WARN_COUNT],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_DEFLOOD_WARNING_COUNT], 0));

    hWndPageItems[LBL_WARN_WARNINGS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_WARNINGS], WS_CHILD | WS_VISIBLE | SS_CENTER, 290, 322, 149, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_NEW_CONNS_FROM_SAME_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NEW_CONNS_FRM_SINGLE_IP_LIM], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 346, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_COUNT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 361, 30, 20, m_hWnd, (HMENU)EDT_NEW_CONNS_FROM_SAME_IP_COUNT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_COUNT], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_COUNT], 38, 361, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_COUNT],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_NEW_CONNECTIONS_COUNT], 0));

    hWndPageItems[LBL_NEW_CONNS_FROM_SAME_IP_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 60, 365, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        70, 361, 30, 20, m_hWnd, (HMENU)EDT_NEW_CONNS_FROM_SAME_IP_TIME, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_TIME], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_NEW_CONNS_FROM_SAME_IP_TIME], 100, 361, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_NEW_CONNS_FROM_SAME_IP_TIME],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_NEW_CONNECTIONS_TIME], 0));

    hWndPageItems[LBL_NEW_CONNS_FROM_SAME_IP_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_CENTER, 122, 365, 91, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MAX_USERS_FROM_SAME_IP] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAX_USR_SAME_IP], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        226, 346, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_USERS_FROM_SAME_IP] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        318, 361, 30, 20, m_hWnd, (HMENU)EDT_MAX_USERS_FROM_SAME_IP, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_USERS_FROM_SAME_IP], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAX_USERS_FROM_SAME_IP], 348, 361, 17, 20, (LPARAM)MAKELONG(256, 1), (WPARAM)hWndPageItems[EDT_MAX_USERS_FROM_SAME_IP],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_CONN_SAME_IP], 0));

    hWndPageItems[BTN_REPORT_FLOOD_TO_OPS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REPORT_FLOOD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0, 394, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_DEFLOOD_REPORT] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", GetPageName(), MB_OK);
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[EDT_PM_MSGS], SettingManager->iShorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_PM_MSGS], SettingManager->iShorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_PM_DIVIDER], SettingManager->iShorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_PM_SECS], SettingManager->iShorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_PM_SECS], SettingManager->iShorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_PM_SECONDS], SettingManager->iShorts[SETSHORT_PM_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_PM_MSGS2], SettingManager->iShorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_PM_MSGS2], SettingManager->iShorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_PM_DIVIDER2], SettingManager->iShorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_PM_SECS2], SettingManager->iShorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_PM_SECS2], SettingManager->iShorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_PM_SECONDS2], SettingManager->iShorts[SETSHORT_PM_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SAME_PM_MSGS], SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_PM_MSGS], SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_PM_DIVIDER], SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SAME_PM_SECS], SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_PM_SECS], SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_PM_SECONDS], SettingManager->iShorts[SETSHORT_SAME_PM_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_MSGS], SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_MSGS], SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_WITH], SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_PM_LINES], SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_PM_LINES], SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_PM_LINES], SettingManager->iShorts[SETSHORT_SAME_MULTI_PM_ACTION] == 0 ? FALSE : TRUE);

    ::SetWindowLongPtr(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], GWLP_USERDATA, (LONG_PTR)this);
    wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood2::GetPageName() {
    return LanguageManager->sTexts[LAN_MORE_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood2::FocusLastItem() {
    ::SetFocus(hWndPageItems[BTN_REPORT_FLOOD_TO_OPS]);
}
//------------------------------------------------------------------------------

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
#include "SettingPageDeflood.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageDeflood::SettingPageDeflood() {
    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageDeflood::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_GLOBAL_MAIN_CHAT_MSGS:
            case EDT_GLOBAL_MAIN_CHAT_SECS:
            case EDT_GLOBAL_MAIN_CHAT_SECS2:
            case EDT_MAIN_CHAT_MSGS:
            case EDT_MAIN_CHAT_SECS:
            case EDT_MAIN_CHAT_MSGS2:
            case EDT_MAIN_CHAT_SECS2:
            case EDT_MAIN_CHAT_INTERVAL_MSGS:
            case EDT_MAIN_CHAT_INTERVAL_SECS:
            case EDT_SAME_MAIN_CHAT_MSGS:
            case EDT_SAME_MAIN_CHAT_SECS:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 999);

                    return 0;
                }

                break;
            case EDT_SAME_MULTI_MAIN_CHAT_MSGS:
            case EDT_SAME_MULTI_MAIN_CHAT_LINES:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 2, 999);

                    return 0;
                }

                break;
            case EDT_CTM_MSGS:
            case EDT_CTM_SECS:
            case EDT_CTM_MSGS2:
            case EDT_CTM_SECS2:
            case EDT_RCTM_MSGS:
            case EDT_RCTM_SECS:
            case EDT_RCTM_MSGS2:
            case EDT_RCTM_SECS2:
            case EDT_DEFLOOD_TEMP_BAN_TIME:
            case EDT_MAX_USERS_LOGINS:
                if(HIWORD(wParam) == EN_CHANGE) {
					MinMaxCheck((HWND)lParam, 1, 9999);

                    return 0;
                }

                break;
            case CB_GLOBAL_MAIN_CHAT:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_MAIN_CHAT:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_MAIN_CHAT2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SAME_MAIN_CHAT:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_SAME_MULTI_MAIN_CHAT:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_CTM:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_CTM], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_CTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_CTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_CTM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_CTM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_CTM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_CTM2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_CTM2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_CTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_CTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_CTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_CTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_CTM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_RCTM:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RCTM], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_RCTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RCTM_MSGS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_RCTM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RCTM_SECS], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
            case CB_RCTM2:
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    uint32_t ui32Action = (uint32_t)::SendMessage(hWndPageItems[CB_RCTM2], CB_GETCURSEL, 0, 0);
                    ::EnableWindow(hWndPageItems[EDT_RCTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RCTM_MSGS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[EDT_RCTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[UD_RCTM_SECS2], ui32Action == 0 ? FALSE : TRUE);
                    ::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS2], ui32Action == 0 ? FALSE : TRUE);
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageDeflood::Save() {
    if(bCreated == false) {
        return;
    }

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAIN_CHAT_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAIN_CHAT_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_MAIN_CHAT_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAIN_CHAT_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAIN_CHAT_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_INTERVAL_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_CHAT_INTERVAL_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_INTERVAL_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_CHAT_INTERVAL_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SAME_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_MAIN_CHAT_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_MAIN_CHAT_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_SAME_MULTI_MAIN_CHAT_LINES, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_CTM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_CTM], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_CTM_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_CTM_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_CTM_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_CTM_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_CTM_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_CTM2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_CTM_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_CTM_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_CTM_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_CTM_TIME2, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_RCTM_ACTION, (int16_t)::SendMessage(hWndPageItems[CB_RCTM], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_RCTM_MSGS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_RCTM_MESSAGES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_RCTM_SECS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_RCTM_TIME, LOWORD(lResult));
    }

    SettingManager->SetShort(SETSHORT_RCTM_ACTION2, (int16_t)::SendMessage(hWndPageItems[CB_RCTM2], CB_GETCURSEL, 0, 0));

    lResult = ::SendMessage(hWndPageItems[UD_RCTM_MSGS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_RCTM_MESSAGES2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_RCTM_SECS2], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_RCTM_TIME2, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_DEFLOOD_TEMP_BAN_TIME], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_DEFLOOD_TEMP_BAN_TIME, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAX_USERS_LOGINS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_SIMULTANEOUS_LOGINS, LOWORD(lResult));
    }
}
//------------------------------------------------------------------------------

void SettingPageDeflood::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
}
//------------------------------------------------------------------------------

bool SettingPageDeflood::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_GLOBAL_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_GLOBAL_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 18, 30, 21, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], 38, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES], 0));

    hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 60, 23, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        70, 18, 30, 21, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], 100, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME], 0));

    hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SEC_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 122, 23, 25, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_GLOBAL_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        152, 18, 175, 21, m_hWnd, (HMENU)CB_GLOBAL_MAIN_CHAT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_LOCK_CHAT]);
    ::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_ONLY_TO_OPS_WITH_IP]);
    ::SendMessage(hWndPageItems[CB_GLOBAL_MAIN_CHAT], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION], 0);

    hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_FOR], WS_CHILD | WS_VISIBLE | SS_CENTER, 332, 23, 25, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        362, 18, 30, 21, m_hWnd, (HMENU)EDT_GLOBAL_MAIN_CHAT_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], 392, 18, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT], 0));

    hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SEC_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 414, 23, 25, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 47, 447, 97,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 62, 180, 21, m_hWnd, (HMENU)CB_MAIN_CHAT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION], 0);

    hWndPageItems[EDT_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 62, 30, 21, m_hWnd, (HMENU)EDT_MAIN_CHAT_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_MSGS], 223, 62, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAIN_CHAT_MESSAGES], 0));

    hWndPageItems[LBL_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 67, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 62, 30, 21, m_hWnd, (HMENU)EDT_MAIN_CHAT_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_SECS], 285, 62, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAIN_CHAT_TIME], 0));

    hWndPageItems[LBL_MAIN_CHAT_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 67, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_MAIN_CHAT2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 89, 180, 21, m_hWnd, (HMENU)CB_MAIN_CHAT2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_MAIN_CHAT2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2], 0);

    hWndPageItems[EDT_MAIN_CHAT_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 89, 30, 21, m_hWnd, (HMENU)EDT_MAIN_CHAT_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_MSGS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_MSGS2], 223, 89, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAIN_CHAT_MESSAGES2], 0));

    hWndPageItems[LBL_MAIN_CHAT_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 94, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_CHAT_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 89, 30, 21, m_hWnd, (HMENU)EDT_MAIN_CHAT_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_SECS2], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_SECS2], 285, 89, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAIN_CHAT_TIME2], 0));

    hWndPageItems[LBL_MAIN_CHAT_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 94, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_MAIN_CHAT_INTERVAL] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_INTERVAL], WS_CHILD | WS_VISIBLE | SS_RIGHT, 8, 120, 180, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 116, 30, 20, m_hWnd, (HMENU)EDT_MAIN_CHAT_INTERVAL_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_INTERVAL_MSGS], 223, 116, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_INTERVAL_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_CHAT_INTERVAL_MESSAGES], 0));

    hWndPageItems[LBL_MAIN_CHAT_INTERVAL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 120, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 116, 30, 20, m_hWnd, (HMENU)EDT_MAIN_CHAT_INTERVAL_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_INTERVAL_SECS], 285, 116, 17, 20, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_INTERVAL_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_CHAT_INTERVAL_TIME], 0));

    hWndPageItems[LBL_MAIN_CHAT_INTERVAL_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 120, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SAME_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SAME_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 144, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SAME_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 159, 180, 21, m_hWnd, (HMENU)CB_SAME_MAIN_CHAT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_MAIN_CHAT], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION], 0);

    hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 159, 30, 21, m_hWnd, (HMENU)EDT_SAME_MAIN_CHAT_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], 223, 159, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_MESSAGES], 0));

    hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 164, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SAME_MAIN_CHAT_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 159, 30, 21, m_hWnd, (HMENU)EDT_SAME_MAIN_CHAT_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], 285, 159, 17, 21, (LPARAM)MAKELONG(999, 1), (WPARAM)hWndPageItems[EDT_SAME_MAIN_CHAT_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_TIME], 0));

    hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 164, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_SAME_MULTI_MAIN_CHAT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SAME_MULTI_MAIN_CHAT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 188, 447, 44,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_SAME_MULTI_MAIN_CHAT] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 203, 180, 21, m_hWnd, (HMENU)CB_SAME_MULTI_MAIN_CHAT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_SAME_MULTI_MAIN_CHAT], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION], 0);

    hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 203, 30, 21, m_hWnd, (HMENU)EDT_SAME_MULTI_MAIN_CHAT_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], 223, 203, 17, 21, (LPARAM)MAKELONG(999, 2), (WPARAM)hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES], 0));

    hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_WITH_LWR], WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 208, 30, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        280, 203, 30, 21, m_hWnd, (HMENU)EDT_SAME_MULTI_MAIN_CHAT_LINES, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], 310, 203, 17, 21, (LPARAM)MAKELONG(999, 2), (WPARAM)hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_LINES], 0));

    hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_LINES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 332, 208, 107, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_CTM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_CONNECTTOME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 232, 447, 71,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_CTM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 247, 180, 21, m_hWnd, (HMENU)CB_CTM, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_CTM], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_CTM_ACTION], 0);

    hWndPageItems[EDT_CTM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 247, 30, 21, m_hWnd, (HMENU)EDT_CTM_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_CTM_MSGS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_CTM_MSGS], 223, 247, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_CTM_MESSAGES], 0));

    hWndPageItems[LBL_CTM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 252, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_CTM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 247, 30, 21, m_hWnd, (HMENU)EDT_CTM_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_CTM_SECS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_CTM_SECS], 285, 247, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_CTM_TIME], 0));

    hWndPageItems[LBL_CTM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 252, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_CTM2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 274, 180, 21, m_hWnd, (HMENU)CB_CTM2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_CTM2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_CTM_ACTION2], 0);

    hWndPageItems[EDT_CTM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 274, 30, 21, m_hWnd, (HMENU)EDT_CTM_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_CTM_MSGS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_CTM_MSGS2], 223, 274, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_CTM_MESSAGES2], 0));

    hWndPageItems[LBL_CTM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 279, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_CTM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 274, 30, 21, m_hWnd, (HMENU)EDT_CTM_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_CTM_SECS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_CTM_SECS2], 285, 274, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_CTM_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_CTM_TIME2], 0));

    hWndPageItems[LBL_CTM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 279, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_RCTM] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REVCONNECTTOME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 303, 447, 71,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_RCTM] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 318, 180, 21, m_hWnd, (HMENU)CB_RCTM, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_RCTM], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_RCTM_ACTION], 0);

    hWndPageItems[EDT_RCTM_MSGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 318, 30, 21, m_hWnd, (HMENU)EDT_RCTM_MSGS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RCTM_MSGS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RCTM_MSGS], 223, 318, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RCTM_MSGS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_RCTM_MESSAGES], 0));

    hWndPageItems[LBL_RCTM_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 323, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_RCTM_SECS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 318, 30, 21, m_hWnd, (HMENU)EDT_RCTM_SECS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RCTM_SECS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RCTM_SECS], 285, 318, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RCTM_SECS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_RCTM_TIME], 0));

    hWndPageItems[LBL_RCTM_SECONDS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 323, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_RCTM2] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        8, 345, 180, 21, m_hWnd, (HMENU)CB_RCTM2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISABLED]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_IGNORE]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_WARN]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_DISCONNECT]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KICK]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TEMP_BAN]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PERM_BAN]);
    ::SendMessage(hWndPageItems[CB_RCTM2], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_RCTM_ACTION2], 0);

    hWndPageItems[EDT_RCTM_MSGS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        193, 345, 30, 21, m_hWnd, (HMENU)EDT_RCTM_MSGS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RCTM_MSGS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RCTM_MSGS2], 223, 345, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RCTM_MSGS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_RCTM_MESSAGES2], 0));

    hWndPageItems[LBL_RCTM_DIVIDER2] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 245, 350, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_RCTM_SECS2] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        255, 345, 30, 21, m_hWnd, (HMENU)EDT_RCTM_SECS2, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RCTM_SECS2], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_RCTM_SECS2], 285, 345, 17, 21, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_RCTM_SECS2],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_RCTM_TIME2], 0));

    hWndPageItems[LBL_RCTM_SECONDS2] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SECONDS_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 307, 350, 132, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_DEFLOOD_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_DEFLOOD_TEMP_BAN_TIME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 374, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 389, 30, 20, m_hWnd, (HMENU)EDT_DEFLOOD_TEMP_BAN_TIME, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_DEFLOOD_TEMP_BAN_TIME], 38, 389, 17, 20, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_DEFLOOD_TEMP_BAN_TIME],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0));

    hWndPageItems[LBL_DEFLOOD_TEMP_BAN_TIME_MINUTES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MINUTES_LWR], WS_CHILD | WS_VISIBLE | SS_LEFT, 60, 394, 153, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MAX_USERS_LOGINS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAX_USERS_LOGINS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        226, 374, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_USERS_LOGINS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        234, 389, 30, 20, m_hWnd, (HMENU)EDT_MAX_USERS_LOGINS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_USERS_LOGINS], EM_SETLIMITTEXT, 4, 0);

    AddUpDown(hWndPageItems[UD_MAX_USERS_LOGINS], 264, 389, 17, 20, (LPARAM)MAKELONG(9999, 1), (WPARAM)hWndPageItems[EDT_MAX_USERS_LOGINS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_SIMULTANEOUS_LOGINS], 0));

    hWndPageItems[LBL_MAX_USERS_LOGINS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_PER_10_SECONDS], WS_CHILD | WS_VISIBLE | SS_LEFT, 286, 394, 153, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", GetPageName(), MB_OK);
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_DIVIDER], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_FOR], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_GLOBAL_MAIN_CHAT_SECS2], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_GLOBAL_MAIN_CHAT_SECS2], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_GLOBAL_MAIN_CHAT_SEC2], SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_MSGS2], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_MSGS2], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_DIVIDER2], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_MAIN_CHAT_SECS2], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_MAIN_CHAT_SECS2], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_MAIN_CHAT_SECONDS2], SettingManager->iShorts[SETSHORT_MAIN_CHAT_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_DIVIDER], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SAME_MAIN_CHAT_SECS], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_MAIN_CHAT_SECS], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_MAIN_CHAT_SECONDS], SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_MSGS], SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_WITH], SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_SAME_MULTI_MAIN_CHAT_LINES], SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_SAME_MULTI_MAIN_CHAT_LINES], SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_SAME_MULTI_MAIN_CHAT_LINES], SettingManager->iShorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_CTM_MSGS], SettingManager->iShorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_CTM_MSGS], SettingManager->iShorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER], SettingManager->iShorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_CTM_SECS], SettingManager->iShorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_CTM_SECS], SettingManager->iShorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_CTM_SECONDS], SettingManager->iShorts[SETSHORT_CTM_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_CTM_MSGS2], SettingManager->iShorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_CTM_MSGS2], SettingManager->iShorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_CTM_DIVIDER2], SettingManager->iShorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_CTM_SECS2], SettingManager->iShorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_CTM_SECS2], SettingManager->iShorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_CTM_SECONDS2], SettingManager->iShorts[SETSHORT_CTM_ACTION2] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_RCTM_MSGS], SettingManager->iShorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_RCTM_MSGS], SettingManager->iShorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER], SettingManager->iShorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_RCTM_SECS], SettingManager->iShorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_RCTM_SECS], SettingManager->iShorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS], SettingManager->iShorts[SETSHORT_RCTM_ACTION] == 0 ? FALSE : TRUE);

    ::EnableWindow(hWndPageItems[EDT_RCTM_MSGS2], SettingManager->iShorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_RCTM_MSGS2], SettingManager->iShorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_RCTM_DIVIDER2], SettingManager->iShorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[EDT_RCTM_SECS2], SettingManager->iShorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[UD_RCTM_SECS2], SettingManager->iShorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);
    ::EnableWindow(hWndPageItems[LBL_RCTM_SECONDS2], SettingManager->iShorts[SETSHORT_RCTM_ACTION2] == 0 ? FALSE : TRUE);

    ::SetWindowLongPtr(hWndPageItems[EDT_MAX_USERS_LOGINS], GWLP_USERDATA, (LONG_PTR)this);
    wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_MAX_USERS_LOGINS], GWLP_WNDPROC, (LONG_PTR)EditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageDeflood::GetPageName() {
    return LanguageManager->sTexts[LAN_DEFLOOD];
}
//------------------------------------------------------------------------------

void SettingPageDeflood::FocusLastItem() {
    ::SetFocus(hWndPageItems[EDT_MAX_USERS_LOGINS]);
}
//------------------------------------------------------------------------------

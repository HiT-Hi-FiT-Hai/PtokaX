/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2011  Petr Kozelka, PPK at PtokaX dot org

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
#include "SettingPageRules.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageRules::SettingPageRules() {
    bUpdateNickLimitMessage = bUpdateMinShare = bUpdateMaxShare = bUpdateShareLimitMessage = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageRules::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_NICK_LEN_MSG:
            case EDT_NICK_LEN_REDIR_ADDR:
            case EDT_SHARE_MSG:
            case EDT_SHARE_REDIR_ADDR:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
            case EDT_MIN_NICK_LEN:
            case EDT_MAX_NICK_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 64);

                    uint16_t ui16Min = 0, ui16Max = 0;

                    LRESULT lResult = ::SendMessage(hWndPageItems[UD_MIN_NICK_LEN], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Min = LOWORD(lResult);
                    }

                    lResult = ::SendMessage(hWndPageItems[UD_MAX_NICK_LEN], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Max = LOWORD(lResult);
                    }

                    if(ui16Min == 0 && ui16Max == 0) {
                        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], FALSE);
                        ::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], FALSE);
                        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], FALSE);
                    } else {
                        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], TRUE);
                        ::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], TRUE);
                        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                    }

                    return 0;
                }

                break;
            case BTN_NICK_LEN_REDIR:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case EDT_MIN_SHARE:
            case EDT_MAX_SHARE:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 9999);

                    uint16_t ui16Min = 0, ui16Max = 0;

                    LRESULT lResult = ::SendMessage(hWndPageItems[UD_MIN_SHARE], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Min = LOWORD(lResult);
                    }

                    lResult = ::SendMessage(hWndPageItems[UD_MAX_SHARE], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Max = LOWORD(lResult);
                    }

                    if(ui16Min == 0 && ui16Max == 0) {
                        ::EnableWindow(hWndPageItems[EDT_SHARE_MSG], FALSE);
                        ::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], FALSE);
                        ::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], FALSE);
                    } else {
                        ::EnableWindow(hWndPageItems[EDT_SHARE_MSG], TRUE);
                        ::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], TRUE);
                        ::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                    }

                    return 0;
                }

                break;
            case BTN_SHARE_REDIR:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case UD_MAIN_CHAT_LEN:
            case UD_PM_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 32767);

                    return 0;
                }

                break;
            case UD_MAIN_CHAT_LINES:
            case UD_PM_LINES:
            case UD_SEARCH_MIN_LEN:
            case UD_SEARCH_MAX_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 999);

                    return 0;
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageRules::Save() {
    if(bCreated == false) {
        return;
    }

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_MIN_NICK_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MIN_NICK_LEN]) {
            bUpdateNickLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MIN_NICK_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAX_NICK_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MAX_NICK_LEN]) {
            bUpdateNickLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MAX_NICK_LEN, LOWORD(lResult));
    }

    char buf[257];
    int iLen = ::GetWindowText(hWndPageItems[EDT_NICK_LEN_MSG], buf, 257);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_NICK_LIMIT_MSG]) != NULL) {
        bUpdateNickLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_NICK_LIMIT_MSG, buf, iLen);

    bool bNickRedir = ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bNickRedir != SettingManager->bBools[SETBOOL_NICK_LIMIT_REDIR]) {
        bUpdateNickLimitMessage = true;
    }

    SettingManager->SetBool(SETBOOL_NICK_LIMIT_REDIR, bNickRedir);

    iLen = ::GetWindowText(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], buf, 257);

    if((SettingManager->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS]) != NULL)) {
        bUpdateNickLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_NICK_LIMIT_REDIR_ADDRESS, buf, iLen);

    lResult = ::SendMessage(hWndPageItems[UD_MIN_SHARE], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MIN_SHARE_LIMIT]) {
            bUpdateMinShare = true;
            bUpdateShareLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MIN_SHARE_LIMIT, LOWORD(lResult));
    }

    uint16_t ui16MinShareUnits = (int16_t)::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_GETCURSEL, 0, 0);

    if(ui16MinShareUnits != SettingManager->iShorts[SETSHORT_MIN_SHARE_UNITS]) {
        bUpdateMinShare = true;
        bUpdateShareLimitMessage = true;
    }

    SettingManager->SetShort(SETSHORT_MIN_SHARE_UNITS, ui16MinShareUnits);

    lResult = ::SendMessage(hWndPageItems[UD_MAX_SHARE], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MAX_SHARE_LIMIT]) {
            bUpdateMaxShare = true;
            bUpdateShareLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MAX_SHARE_LIMIT, LOWORD(lResult));
    }

    uint16_t ui16MaxShareUnits = (int16_t)::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_GETCURSEL, 0, 0);

    if(ui16MaxShareUnits != SettingManager->iShorts[SETSHORT_MAX_SHARE_UNITS]) {
        bUpdateMaxShare = true;
        bUpdateShareLimitMessage = true;
    }

    SettingManager->SetShort(SETSHORT_MAX_SHARE_UNITS, ui16MaxShareUnits);

    iLen = ::GetWindowText(hWndPageItems[EDT_SHARE_MSG], buf, 257);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_SHARE_LIMIT_MSG]) != NULL) {
        bUpdateShareLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_SHARE_LIMIT_MSG, buf, iLen);

    bool bShareRedir = ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bShareRedir != SettingManager->bBools[SETBOOL_SHARE_LIMIT_REDIR]) {
        bUpdateShareLimitMessage = true;
    }

    SettingManager->SetBool(SETBOOL_SHARE_LIMIT_REDIR, bShareRedir);

    iLen = ::GetWindowText(hWndPageItems[EDT_SHARE_REDIR_ADDR], buf, 257);

    if((SettingManager->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS]) != NULL)) {
        bUpdateShareLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_SHARE_LIMIT_REDIR_ADDRESS, buf, iLen);

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_CHAT_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_MAIN_CHAT_LINES], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_CHAT_LINES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_PM_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_PM_LINES], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_PM_LINES, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_MIN_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MIN_SEARCH_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SEARCH_MAX_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_SEARCH_LEN, LOWORD(lResult));
    }
}
//------------------------------------------------------------------------------

void SettingPageRules::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool &bUpdatedShareLimitMessage,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool &bUpdatedNickLimitMessage, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool &bUpdatedMinShare, bool &bUpdatedMaxShare) {
    bUpdatedNickLimitMessage = bUpdateNickLimitMessage;
    bUpdatedShareLimitMessage = bUpdateShareLimitMessage;
    bUpdatedMinShare = bUpdateMinShare;
    bUpdatedMaxShare = bUpdateMaxShare;
}

//------------------------------------------------------------------------------

bool SettingPageRules::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_NICK_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NICK_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 125,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MIN_NICK_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 18, 40, 20, m_hWnd, (HMENU)EDT_MIN_NICK_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MIN_NICK_LEN], EM_SETLIMITTEXT, 2, 0);
    AddToolTip(hWndPageItems[EDT_MIN_NICK_LEN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MIN_NICK_LEN], 48, 18, 17, 20, (LPARAM)MAKELONG(64, 0), (WPARAM)hWndPageItems[EDT_MIN_NICK_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MIN_NICK_LEN], 0));

    hWndPageItems[LBL_MIN_NICK_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MIN_LEN], WS_CHILD | WS_VISIBLE | SS_LEFT, 70, 21, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_MAX_NICK_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAX_LEN], WS_CHILD | WS_VISIBLE | SS_RIGHT, 226, 21, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_NICK_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 18, 40, 20, m_hWnd, (HMENU)EDT_MAX_NICK_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_NICK_LEN], EM_SETLIMITTEXT, 2, 0);
    AddToolTip(hWndPageItems[EDT_MAX_NICK_LEN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MAX_NICK_LEN], 422, 18, 17, 20, (LPARAM)MAKELONG(64, 0), (WPARAM)hWndPageItems[EDT_MAX_NICK_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_NICK_LEN], 0));

    hWndPageItems[GB_NICK_LEN_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 41, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_NICK_LEN_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_NICK_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, 56, 421, 18, m_hWnd, (HMENU)EDT_NICK_LEN_MSG, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NICK_LEN_MSG], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_NICK_LEN_MSG], LanguageManager->sTexts[LAN_NICK_LIMIT_MSG_HINT]);

    hWndPageItems[GB_NICK_LEN_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 82, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_NICK_LEN_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 98, 80, 16, m_hWnd, (HMENU)BTN_NICK_LEN_REDIR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_NICK_LEN_REDIR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_NICK_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[EDT_NICK_LEN_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 98, 97, 336, 18, m_hWnd, (HMENU)EDT_NICK_LEN_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], LanguageManager->sTexts[LAN_REDIRECT_HINT]);

    hWndPageItems[GB_SHARE_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SHARE_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 128, 447, 126,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MIN_SHARE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 143, 40, 21, m_hWnd, (HMENU)EDT_MIN_SHARE, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MIN_SHARE], EM_SETLIMITTEXT, 4, 0);
    AddToolTip(hWndPageItems[EDT_MIN_SHARE], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MIN_SHARE], 48, 143, 17, 21, (LPARAM)MAKELONG(9999, 0), (WPARAM)hWndPageItems[EDT_MIN_SHARE],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MIN_SHARE_LIMIT], 0));

    hWndPageItems[CB_MIN_SHARE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        70, 143, 50, 21, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_BYTES]);
    ::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KILO_BYTES]);
    ::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_MEGA_BYTES]);
    ::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_GIGA_BYTES]);
    ::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TERA_BYTES]);
    ::SendMessage(hWndPageItems[CB_MIN_SHARE], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MIN_SHARE_UNITS], 0);

    hWndPageItems[LBL_MIN_SHARE] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MIN_SHARE], WS_CHILD | WS_VISIBLE | SS_LEFT, 125, 146, 96, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_MAX_SHARE] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAX_SHARE], WS_CHILD | WS_VISIBLE | SS_RIGHT, 226, 146, 96, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_SHARE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        327, 143, 40, 21, m_hWnd, (HMENU)EDT_MAX_SHARE, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_SHARE], EM_SETLIMITTEXT, 4, 0);
    AddToolTip(hWndPageItems[EDT_MAX_SHARE], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MAX_SHARE], 367, 143, 17, 21, (LPARAM)MAKELONG(9999, 0), (WPARAM)hWndPageItems[EDT_MAX_SHARE],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_SHARE_LIMIT], 0));

    hWndPageItems[CB_MAX_SHARE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        389, 143, 50, 21, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_BYTES]);
    ::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_KILO_BYTES]);
    ::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_MEGA_BYTES]);
    ::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_GIGA_BYTES]);
    ::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_TERA_BYTES]);
    ::SendMessage(hWndPageItems[CB_MAX_SHARE], CB_SETCURSEL, SettingManager->iShorts[SETSHORT_MAX_SHARE_UNITS], 0);

    hWndPageItems[GB_SHARE_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 167, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SHARE_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_SHARE_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, 182, 421, 18, m_hWnd, (HMENU)EDT_SHARE_MSG, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SHARE_MSG], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_SHARE_MSG], LanguageManager->sTexts[LAN_SHARE_LIMIT_MSG_HINT]);

    hWndPageItems[GB_SHARE_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 208, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_SHARE_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 224, 80, 16, m_hWnd, (HMENU)BTN_SHARE_REDIR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SHARE_REDIR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_SHARE_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[EDT_SHARE_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 98, 223, 336, 18, m_hWnd, (HMENU)EDT_NICK_LEN_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SHARE_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_SHARE_REDIR_ADDR], LanguageManager->sTexts[LAN_REDIRECT_HINT]);

    hWndPageItems[GB_MAIN_CHAT_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAIN_CHAT_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 254, 447, 43,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_CHAT_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 269, 40, 20, m_hWnd, (HMENU)EDT_MAIN_CHAT_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_LEN], EM_SETLIMITTEXT, 5, 0);
    AddToolTip(hWndPageItems[EDT_MAIN_CHAT_LEN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_LEN], 48, 269, 17, 20, (LPARAM)MAKELONG(32767, 0), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_CHAT_LEN], 0));

    hWndPageItems[LBL_MAIN_CHAT_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_LENGTH], WS_CHILD | WS_VISIBLE | SS_LEFT, 70, 272, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_MAIN_CHAT_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_LINES], WS_CHILD | WS_VISIBLE | SS_RIGHT, 226, 272, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAIN_CHAT_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 269, 40, 20, m_hWnd, (HMENU)EDT_MAIN_CHAT_LINES, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAIN_CHAT_LINES], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_MAIN_CHAT_LINES], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MAIN_CHAT_LINES], 422, 269, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_MAIN_CHAT_LINES],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_CHAT_LINES], 0));

    hWndPageItems[GB_PM_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PM_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 297, 447, 43,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 312, 40, 20, m_hWnd, (HMENU)EDT_PM_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_LEN], EM_SETLIMITTEXT, 5, 0);
    AddToolTip(hWndPageItems[EDT_PM_LEN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_PM_LEN], 48, 312, 17, 20, (LPARAM)MAKELONG(32767, 0), (WPARAM)hWndPageItems[EDT_PM_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_PM_LEN], 0));

    hWndPageItems[LBL_PM_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_LENGTH], WS_CHILD | WS_VISIBLE | SS_LEFT, 70, 315, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_PM_LINES] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_LINES], WS_CHILD | WS_VISIBLE | SS_RIGHT, 226, 315, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PM_LINES] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 312, 40, 20, m_hWnd, (HMENU)EDT_PM_LINES, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PM_LINES], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_PM_LINES], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_PM_LINES], 422, 312, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_PM_LINES],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_PM_LINES], 0));

    hWndPageItems[GB_SEARCH_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SEARCH_LIMITS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 340, 447, 43,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SEARCH_MIN_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 355, 40, 20, m_hWnd, (HMENU)EDT_SEARCH_MIN_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_MIN_LEN], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_SEARCH_MIN_LEN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_SEARCH_MIN_LEN], 48, 355, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SEARCH_MIN_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MIN_SEARCH_LEN], 0));

    hWndPageItems[LBL_SEARCH_MIN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MINIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT, 70, 358, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_SEARCH_MAX] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_RIGHT, 226, 358, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SEARCH_MAX_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 355, 40, 20, m_hWnd, (HMENU)EDT_SEARCH_MAX_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SEARCH_MAX_LEN], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_SEARCH_MAX_LEN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_SEARCH_MAX_LEN], 422, 355, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SEARCH_MAX_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_SEARCH_LEN], 0));

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    if(SettingManager->iShorts[SETSHORT_MIN_SHARE_LIMIT] == 0 && SettingManager->iShorts[SETSHORT_MAX_SHARE_LIMIT] == 0) {
        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], FALSE);
        ::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], FALSE);
        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], FALSE);
    } else {
        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_MSG], TRUE);
        ::EnableWindow(hWndPageItems[BTN_NICK_LEN_REDIR], TRUE);
        ::EnableWindow(hWndPageItems[EDT_NICK_LEN_REDIR_ADDR], SettingManager->bBools[SETBOOL_NICK_LIMIT_REDIR] == true ? TRUE : FALSE);
    }

    if(SettingManager->iShorts[SETSHORT_MIN_NICK_LEN] == 0 && SettingManager->iShorts[SETSHORT_MAX_NICK_LEN] == 0) {
        ::EnableWindow(hWndPageItems[EDT_SHARE_MSG], FALSE);
        ::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], FALSE);
        ::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], FALSE);
    } else {
        ::EnableWindow(hWndPageItems[EDT_SHARE_MSG], TRUE);
        ::EnableWindow(hWndPageItems[BTN_SHARE_REDIR], TRUE);
        ::EnableWindow(hWndPageItems[EDT_SHARE_REDIR_ADDR], SettingManager->bBools[SETBOOL_SHARE_LIMIT_REDIR] == true ? TRUE : FALSE);
    }

    ::SetWindowLongPtr(hWndPageItems[EDT_SEARCH_MAX_LEN], GWLP_USERDATA, (LONG_PTR)this);
    wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_SEARCH_MAX_LEN], GWLP_WNDPROC, (LONG_PTR)EditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageRules::GetPageName() {
    return LanguageManager->sTexts[LAN_RULES];
}
//------------------------------------------------------------------------------

void SettingPageRules::FocusLastItem() {
    ::SetFocus(hWndPageItems[EDT_SEARCH_MAX_LEN]);
}
//------------------------------------------------------------------------------

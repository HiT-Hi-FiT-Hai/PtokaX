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
#include "SettingPageRules2.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageRules2::SettingPageRules2() {
    bUpdateSlotsLimitMessage = bUpdateHubSlotRatioMessage = bUpdateMaxHubsLimitMessage = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageRules2::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_SLOTS_MSG:
            case EDT_SLOTS_REDIR_ADDR:
            case EDT_HUBS_SLOTS_MSG:
            case EDT_HUBS_SLOTS_REDIR_ADDR:
            case EDT_HUBS_MSG:
            case EDT_HUBS_REDIR_ADDR:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
            case EDT_SLOTS_MIN:
            case EDT_SLOTS_MAX:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 999);

                    uint16_t ui16Min = 0, ui16Max = 0;

                    LRESULT lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MIN], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Min = LOWORD(lResult);
                    }

                    lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MAX], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Max = LOWORD(lResult);
                    }

                    if(ui16Min == 0 && ui16Max == 0) {
                        ::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], FALSE);
                        ::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], FALSE);
                        ::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], FALSE);
                    } else {
                        ::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], TRUE);
                        ::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], TRUE);
                        ::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                    }

                    return 0;
                }

                break;
            case BTN_SLOTS_REDIR:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case EDT_HUBS:
            case EDT_SLOTS:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 999);

                    uint16_t ui16Hubs = 0, ui16Slots = 0;

                    LRESULT lResult = ::SendMessage(hWndPageItems[UD_HUBS], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Hubs = LOWORD(lResult);
                    }

                    lResult = ::SendMessage(hWndPageItems[UD_SLOTS], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Slots = LOWORD(lResult);
                    }

                    if(ui16Hubs == 0 || ui16Slots == 0) {
                        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], FALSE);
                        ::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], FALSE);
                        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], FALSE);
                    } else {
                        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], TRUE);
                        ::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], TRUE);
                        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                    }

                    return 0;
                }

                break;
            case BTN_HUBS_SLOTS_REDIR:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case EDT_MAX_HUBS:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 999);

                    uint16_t ui16Hubs = 0;

                    LRESULT lResult = ::SendMessage(hWndPageItems[UD_MAX_HUBS], UDM_GETPOS, 0, 0);
                    if(HIWORD(lResult) == 0) {
                        ui16Hubs = LOWORD(lResult);
                    }

                    if(ui16Hubs == 0) {
                        ::EnableWindow(hWndPageItems[EDT_HUBS_MSG], FALSE);
                        ::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], FALSE);
                        ::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], FALSE);
                    } else {
                        ::EnableWindow(hWndPageItems[EDT_HUBS_MSG], TRUE);
                        ::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], TRUE);
                        ::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                    }

                    return 0;
                }

                break;
            case BTN_HUBS_REDIR:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }

                break;
            case EDT_CTM_LEN:
            case EDT_RCTM_LEN:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 1, 512);

                    return 0;
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageRules2::Save() {
    if(bCreated == false) {
        return;
    }

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MIN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MIN_SLOTS_LIMIT]) {
            bUpdateSlotsLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MIN_SLOTS_LIMIT, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SLOTS_MAX], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MAX_SLOTS_LIMIT]) {
            bUpdateSlotsLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MAX_SLOTS_LIMIT, LOWORD(lResult));
    }

    char buf[257];
    int iLen = ::GetWindowText(hWndPageItems[EDT_SLOTS_MSG], buf, 257);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_SLOTS_LIMIT_MSG]) != NULL) {
        bUpdateSlotsLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_SLOTS_LIMIT_MSG, buf, iLen);

    bool bSlotsRedir = ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bSlotsRedir != SettingManager->bBools[SETBOOL_SLOTS_LIMIT_REDIR]) {
        bUpdateSlotsLimitMessage = true;
    }

    SettingManager->SetBool(SETBOOL_SLOTS_LIMIT_REDIR, bSlotsRedir);

    iLen = ::GetWindowText(hWndPageItems[EDT_SLOTS_REDIR_ADDR], buf, 257);

    if((SettingManager->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS]) != NULL)) {
        bUpdateSlotsLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_SLOTS_LIMIT_REDIR_ADDRESS, buf, iLen);

    lResult = ::SendMessage(hWndPageItems[UD_HUBS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS]) {
            bUpdateHubSlotRatioMessage = true;
        }
        SettingManager->SetShort(SETSHORT_HUB_SLOT_RATIO_HUBS, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_SLOTS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS]) {
            bUpdateHubSlotRatioMessage = true;
        }
        SettingManager->SetShort(SETSHORT_HUB_SLOT_RATIO_SLOTS, LOWORD(lResult));
    }

    iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_SLOTS_MSG], buf, 257);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_HUB_SLOT_RATIO_MSG]) != NULL) {
        bUpdateHubSlotRatioMessage = true;
    }

    SettingManager->SetText(SETTXT_HUB_SLOT_RATIO_MSG, buf, iLen);

    bool bHubsSlotsRedir = ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bHubsSlotsRedir != SettingManager->bBools[SETBOOL_HUB_SLOT_RATIO_REDIR]) {
        bUpdateHubSlotRatioMessage = true;
    }

    SettingManager->SetBool(SETBOOL_HUB_SLOT_RATIO_REDIR, bHubsSlotsRedir);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], buf, 257);

    if((SettingManager->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS]) != NULL)) {
        bUpdateHubSlotRatioMessage = true;
    }

    SettingManager->SetText(SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS, buf, iLen);

    lResult = ::SendMessage(hWndPageItems[UD_MAX_HUBS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        if(LOWORD(lResult) != SettingManager->iShorts[SETSHORT_MAX_HUBS_LIMIT]) {
            bUpdateMaxHubsLimitMessage = true;
        }
        SettingManager->SetShort(SETSHORT_MAX_HUBS_LIMIT, LOWORD(lResult));
    }

    iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_MSG], buf, 257);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]) != NULL) {
        bUpdateMaxHubsLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_MAX_HUBS_LIMIT_MSG, buf, iLen);

    bool bHubsRedir = ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bHubsRedir != SettingManager->bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR]) {
        bUpdateMaxHubsLimitMessage = true;
    }

    SettingManager->SetBool(SETBOOL_MAX_HUBS_LIMIT_REDIR, bHubsRedir);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUBS_REDIR_ADDR], buf, 257);

    if((SettingManager->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS]) != NULL)) {
        bUpdateMaxHubsLimitMessage = true;
    }

    SettingManager->SetText(SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS, buf, iLen);

    lResult = ::SendMessage(hWndPageItems[UD_CTM_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_CTM_LEN, LOWORD(lResult));
    }

    lResult = ::SendMessage(hWndPageItems[UD_RCTM_LEN], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_RCTM_LEN, LOWORD(lResult));
    }
}
//------------------------------------------------------------------------------

void SettingPageRules2::GetUpdates(bool & /*bUpdateHubNameWelcome*/, bool & /*bUpdateHubName*/, bool & /*bUpdateTCPPorts*/, bool & /*bUpdateUDPPort*/,
    bool & /*bUpdateAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
    bUpdatedSlotsLimitMessage = bUpdateSlotsLimitMessage;
    bUpdatedHubSlotRatioMessage = bUpdateHubSlotRatioMessage;
    bUpdatedMaxHubsLimitMessage = bUpdateMaxHubsLimitMessage;
}

//------------------------------------------------------------------------------

bool SettingPageRules2::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_SLOTS_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SLOTS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 125,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SLOTS_MIN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 18, 40, 20, m_hWnd, (HMENU)EDT_SLOTS_MIN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SLOTS_MIN], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_SLOTS_MIN], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_SLOTS_MIN], 48, 18, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SLOTS_MIN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MIN_SLOTS_LIMIT], 0));

    hWndPageItems[LBL_SLOTS_MIN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MIN_SLOTS], WS_CHILD | WS_VISIBLE | SS_LEFT, 70, 21, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_SLOTS_MAX] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAX_SLOTS], WS_CHILD | WS_VISIBLE | SS_RIGHT, 226, 21, 151, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SLOTS_MAX] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 18, 40, 20, m_hWnd, (HMENU)EDT_SLOTS_MAX, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SLOTS_MAX], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_SLOTS_MAX], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_SLOTS_MAX], 422, 18, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SLOTS_MAX],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_SLOTS_LIMIT], 0));

    hWndPageItems[GB_SLOTS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 41, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SLOTS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_SLOTS_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 13, 56, 421, 18, m_hWnd, (HMENU)EDT_SLOTS_MSG, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SLOTS_MSG], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_SLOTS_MSG], LanguageManager->sTexts[LAN_SLOT_LIMIT_MSG_HINT]);

    hWndPageItems[GB_SLOTS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 82, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_SLOTS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 98, 80, 16, m_hWnd, (HMENU)BTN_SLOTS_REDIR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SLOTS_REDIR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[EDT_SLOTS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | ES_AUTOHSCROLL, 98, 97, 336, 18, m_hWnd, (HMENU)EDT_SLOTS_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SLOTS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_SLOTS_REDIR_ADDR], LanguageManager->sTexts[LAN_REDIRECT_HINT]);

    hWndPageItems[GB_HUBS_SLOTS_LIMITS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_SLOT_RATIO_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 128, 447, 125, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_HUBS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_HUBS], WS_CHILD | WS_VISIBLE | SS_RIGHT, 8, 146, 146, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUBS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        159, 143, 40, 20, m_hWnd, (HMENU)EDT_HUBS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUBS], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_HUBS], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_HUBS], 199, 143, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_HUBS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS], 0));

    hWndPageItems[LBL_DIVIDER] = ::CreateWindowEx(0, WC_STATIC, "/", WS_CHILD | WS_VISIBLE | SS_CENTER, 221, 146, 5, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_SLOTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        231, 143, 40, 20, m_hWnd, (HMENU)EDT_SLOTS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_SLOTS], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_SLOTS], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_SLOTS], 271, 143, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_SLOTS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS], 0));

    hWndPageItems[LBL_SLOTS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_SLOTS], WS_CHILD | WS_VISIBLE | SS_LEFT, 293, 146, 146, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_HUBS_SLOTS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 166, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUBS_SLOTS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_SLOT_RATIO_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 13, 181, 421, 18, m_hWnd, (HMENU)EDT_HUBS_SLOTS_MSG, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUBS_SLOTS_MSG], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_HUBS_SLOTS_MSG], LanguageManager->sTexts[LAN_HUB_SLOT_RATIO_MSG_HINT]);

    hWndPageItems[GB_HUBS_SLOTS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 207, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_HUBS_SLOTS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 223, 80, 16, m_hWnd, (HMENU)BTN_HUBS_SLOTS_REDIR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_HUBS_SLOTS_REDIR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | ES_AUTOHSCROLL, 98, 222, 336, 18, m_hWnd, (HMENU)EDT_HUBS_SLOTS_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], LanguageManager->sTexts[LAN_REDIRECT_HINT]);

    hWndPageItems[GB_HUBS_LIMIT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAX_HUBS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 253, 447, 125, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_HUBS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        8, 268, 40, 20, m_hWnd, (HMENU)EDT_MAX_HUBS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_HUBS], EM_SETLIMITTEXT, 3, 0);
    AddToolTip(hWndPageItems[EDT_MAX_HUBS], LanguageManager->sTexts[LAN_ZERO_IS_UNLIMITED_HINT]);

    AddUpDown(hWndPageItems[UD_MAX_HUBS], 48, 268, 17, 20, (LPARAM)MAKELONG(999, 0), (WPARAM)hWndPageItems[EDT_MAX_HUBS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_HUBS_LIMIT], 0));

    hWndPageItems[LBL_MAX_HUBS] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAX_HUBS], WS_CHILD | WS_VISIBLE | SS_LEFT, 70, 271, 369, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_HUBS_MSG] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MSG_TO_SND], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 291, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUBS_MSG] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 13, 306, 421, 18, m_hWnd, (HMENU)EDT_HUBS_MSG, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUBS_MSG], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_HUBS_MSG], LanguageManager->sTexts[LAN_HUB_LIMIT_MSG_HINT]);

    hWndPageItems[GB_HUBS_REDIR] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 332, 437, 41,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_HUBS_REDIR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_W_ARROW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, 348, 80, 16, m_hWnd, (HMENU)BTN_HUBS_REDIR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_HUBS_REDIR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[EDT_HUBS_REDIR_ADDR] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS], WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | ES_AUTOHSCROLL, 98, 347, 336, 18, m_hWnd, (HMENU)EDT_HUBS_REDIR_ADDR, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUBS_REDIR_ADDR], EM_SETLIMITTEXT, 256, 0);
    AddToolTip(hWndPageItems[EDT_HUBS_REDIR_ADDR], LanguageManager->sTexts[LAN_REDIRECT_HINT]);

    hWndPageItems[GB_CTM_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_CTM_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 378, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_CTM_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 396, 143, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_CTM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        156, 393, 40, 20, m_hWnd, (HMENU)EDT_CTM_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_CTM_LEN], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_CTM_LEN], 196, 393, 17, 20, (LPARAM)MAKELONG(512, 1), (WPARAM)hWndPageItems[EDT_CTM_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_CTM_LEN], 0));

    hWndPageItems[GB_RCTM_LEN] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_RCTM_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        226, 378, 221, 43, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_RCTM_LEN] = ::CreateWindowEx(0, WC_STATIC, LanguageManager->sTexts[LAN_MAXIMUM], WS_CHILD | WS_VISIBLE | SS_LEFT, 234, 396, 143, 16,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_RCTM_LEN] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        382, 393, 40, 20, m_hWnd, (HMENU)EDT_RCTM_LEN, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_RCTM_LEN], EM_SETLIMITTEXT, 3, 0);

    AddUpDown(hWndPageItems[UD_RCTM_LEN], 422, 393, 17, 20, (LPARAM)MAKELONG(512, 1), (WPARAM)hWndPageItems[EDT_RCTM_LEN],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_RCTM_LEN], 0));

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    if(SettingManager->iShorts[SETSHORT_MIN_SLOTS_LIMIT] == 0 && SettingManager->iShorts[SETSHORT_MAX_SLOTS_LIMIT] == 0) {
        ::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], FALSE);
        ::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], FALSE);
        ::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], FALSE);
    } else {
        ::EnableWindow(hWndPageItems[EDT_SLOTS_MSG], TRUE);
        ::EnableWindow(hWndPageItems[BTN_SLOTS_REDIR], TRUE);
        ::EnableWindow(hWndPageItems[EDT_SLOTS_REDIR_ADDR], SettingManager->bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true ? TRUE : FALSE);
    }

    if(SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_HUBS] == 0 || SettingManager->iShorts[SETSHORT_HUB_SLOT_RATIO_SLOTS] == 0) {
        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], FALSE);
        ::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], FALSE);
        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], FALSE);
    } else {
        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_MSG], TRUE);
        ::EnableWindow(hWndPageItems[BTN_HUBS_SLOTS_REDIR], TRUE);
        ::EnableWindow(hWndPageItems[EDT_HUBS_SLOTS_REDIR_ADDR], SettingManager->bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true ? TRUE : FALSE);
    }

    if(SettingManager->iShorts[SETSHORT_MAX_HUBS_LIMIT] == 0) {
        ::EnableWindow(hWndPageItems[EDT_HUBS_MSG], FALSE);
        ::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], FALSE);
        ::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], FALSE);
    } else {
        ::EnableWindow(hWndPageItems[EDT_HUBS_MSG], TRUE);
        ::EnableWindow(hWndPageItems[BTN_HUBS_REDIR], TRUE);
        ::EnableWindow(hWndPageItems[EDT_HUBS_REDIR_ADDR], SettingManager->bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true ? TRUE : FALSE);
    }

    ::SetWindowLongPtr(hWndPageItems[EDT_RCTM_LEN], GWLP_USERDATA, (LONG_PTR)this);
    wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_RCTM_LEN], GWLP_WNDPROC, (LONG_PTR)EditProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageRules2::GetPageName() {
    return LanguageManager->sTexts[LAN_MORE_RULES];
}
//------------------------------------------------------------------------------

void SettingPageRules2::FocusLastItem() {
    ::SetFocus(hWndPageItems[EDT_RCTM_LEN]);
}
//------------------------------------------------------------------------------

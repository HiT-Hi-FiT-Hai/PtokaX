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
#include "SettingPageAdvanced.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageAdvanced::SettingPageAdvanced() {
    bUpdateSysTray = bUpdateScripting = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageAdvanced::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case BTN_ENABLE_TRAY_ICON:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[BTN_MINIMIZE_ON_STARTUP],
                        ::SendMessage(hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }
                break;
            case BTN_ENABLE_SCRIPTING:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(hWndPageItems[BTN_ENABLE_SCRIPTING], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                    ::EnableWindow(hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_POPUP_SCRIPT_WINDOW_ON_ERROR], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], bEnable);
                }
                break;
            case BTN_FILTER_KICK_MESSAGES:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS],
                        ::SendMessage(hWndPageItems[BTN_FILTER_KICK_MESSAGES], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }
                break;
            case BTN_SEND_STATUS_MESSAGES_TO_OPS:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM],
                        ::SendMessage(hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
                }
                break;
            case BTN_DISALLOW_PINGER:
                if(HIWORD(wParam) == BN_CLICKED) {
                    ::EnableWindow(hWndPageItems[BTN_REPORT_PINGER],
                        ::SendMessage(hWndPageItems[BTN_DISALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE);
                }
                break;
            case EDT_PREFIXES_FOR_HUB_COMMANDS:
                if(HIWORD(wParam) == EN_CHANGE) {
                    char buf[6];
                    ::GetWindowText((HWND)lParam, buf, 6);

                    bool bChanged = false;

                    for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                        if(buf[ui16i] == '|') {
                            strcpy(buf+ui16i, buf+ui16i+1);
                            bChanged = true;
                            ui16i--;
                            continue;
                        }

                        for(uint16_t ui16j = 0; buf[ui16j] != '\0'; ui16j++) {
                            if(ui16j == ui16i) {
                                continue;
                            }

                            if(buf[ui16j] == buf[ui16i]) {
                                strcpy(buf+ui16j, buf+ui16j+1);
                                bChanged = true;
                                if(ui16i > ui16j) {
                                    ui16i--;
                                }
                                ui16j--;
                            }
                        }
                    }

                    if(bChanged == true) {
                        int iStart, iEnd;

                        ::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

                        ::SetWindowText((HWND)lParam, buf);

                        ::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
                    }

                    return 0;
                }
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageAdvanced::Save() {
    if(bCreated == false) {
        return;
    }

    SettingManager->SetBool(SETBOOL_AUTO_START, ::SendMessage(hWndPageItems[BTN_AUTO_START], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_CHECK_NEW_RELEASES, ::SendMessage(hWndPageItems[BTN_CHECK_FOR_UPDATE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    if((::SendMessage(hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON]) {
        bUpdateSysTray = true;
    }

    SettingManager->SetBool(SETBOOL_ENABLE_TRAY_ICON, ::SendMessage(hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_START_MINIMIZED, ::SendMessage(hWndPageItems[BTN_MINIMIZE_ON_STARTUP], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    char buf[6];
    int iLen = ::GetWindowText(hWndPageItems[EDT_PREFIXES_FOR_HUB_COMMANDS], buf, 6);
    SettingManager->SetText(SETTXT_CHAT_COMMANDS_PREFIXES, buf, iLen);

    SettingManager->SetBool(SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM, ::SendMessage(hWndPageItems[BTN_REPLY_TO_HUB_COMMANDS_IN_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    if((::SendMessage(hWndPageItems[BTN_ENABLE_SCRIPTING], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING]) {
        bUpdateScripting = true;
    }

    SettingManager->SetBool(SETBOOL_ENABLE_SCRIPTING, ::SendMessage(hWndPageItems[BTN_ENABLE_SCRIPTING], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_STOP_SCRIPT_ON_ERROR, ::SendMessage(hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_POPUP_SCRIPT_WINDOW, ::SendMessage(hWndPageItems[BTN_POPUP_SCRIPT_WINDOW_ON_ERROR], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_LOG_SCRIPT_ERRORS, ::SendMessage(hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager->SetBool(SETBOOL_FILTER_KICK_MESSAGES, ::SendMessage(hWndPageItems[BTN_FILTER_KICK_MESSAGES], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_SEND_KICK_MESSAGES_TO_OPS, ::SendMessage(hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager->SetBool(SETBOOL_SEND_STATUS_MESSAGES, ::SendMessage(hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_SEND_STATUS_MESSAGES_AS_PM, ::SendMessage(hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager->SetBool(SETBOOL_DONT_ALLOW_PINGERS, ::SendMessage(hWndPageItems[BTN_DISALLOW_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
    SettingManager->SetBool(SETBOOL_REPORT_PINGERS, ::SendMessage(hWndPageItems[BTN_REPORT_PINGER], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageAdvanced::GetUpdates(bool & /*bUpdatedHubNameWelcome*/, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
    bool & /*bUpdatedAutoReg*/, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool &bUpdatedSysTray, bool &bUpdatedScripting) {
    bUpdatedSysTray = bUpdateSysTray;
    bUpdatedScripting = bUpdateScripting;
}

//------------------------------------------------------------------------------
bool SettingPageAdvanced::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_HUB_STARTUP_AND_TRAY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_STARTUP_AND_TRAY_ICON],
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 3, 447, 93, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_AUTO_START] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_AUTO_START], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 17, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_AUTO_START], BM_SETCHECK, (SettingManager->bBools[SETBOOL_AUTO_START] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_CHECK_FOR_UPDATE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_UPDATE_CHECK_ON_STARTUP], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 36, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_CHECK_FOR_UPDATE], BM_SETCHECK, (SettingManager->bBools[SETBOOL_CHECK_NEW_RELEASES] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_ENABLE_TRAY_ICON] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_TRAY_ICON], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 55, 431, 16, m_hWnd, (HMENU)BTN_ENABLE_TRAY_ICON, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ENABLE_TRAY_ICON], BM_SETCHECK, (SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_MINIMIZE_ON_STARTUP] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_MINIMIZE_ON_STARTUP], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 74, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_MINIMIZE_ON_STARTUP], BM_SETCHECK, (SettingManager->bBools[SETBOOL_START_MINIMIZED] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_HUB_COMMANDS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_COMMANDS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 96, 447, 60, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[LBL_PREFIXES_FOR_HUB_COMMANDS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_STATIC, LanguageManager->sTexts[LAN_PREFIXES_FOR_HUB_CMDS],
        WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 113, 360, 16, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_PREFIXES_FOR_HUB_COMMANDS] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_CHAT_COMMANDS_PREFIXES], WS_CHILD |
        WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 373, 111, 66, 18, m_hWnd, (HMENU)EDT_PREFIXES_FOR_HUB_COMMANDS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_PREFIXES_FOR_HUB_COMMANDS], EM_SETLIMITTEXT, 5, 0);

    hWndPageItems[BTN_REPLY_TO_HUB_COMMANDS_IN_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REPLY_HUB_CMDS_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 134, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REPLY_TO_HUB_COMMANDS_IN_PM], BM_SETCHECK,
        (SettingManager->bBools[SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_SCRIPTING] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SCRIPTING], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 156, 447, 93, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_ENABLE_SCRIPTING] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_SCRIPTING], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 170, 431, 16, m_hWnd, (HMENU)BTN_ENABLE_SCRIPTING, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ENABLE_SCRIPTING], BM_SETCHECK, (SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_STOP_SCRIPT_ON_ERR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 189, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_STOP_SCRIPT_ON_ERROR] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_POPUP_SCRIPT_WINDOW_ON_ERROR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_POPUP_SCRIPT_WINDOW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 208, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_POPUP_SCRIPT_WINDOW_ON_ERROR], BM_SETCHECK, (SettingManager->bBools[SETBOOL_POPUP_SCRIPT_WINDOW] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_LOG_SCRIPT_ERRORS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 227, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], BM_SETCHECK, (SettingManager->bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_KICK_MESSAGES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_KICK_MESSAGES], WS_CHILD | WS_VISIBLE |
        BS_GROUPBOX, 0, 249, 447, 55, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_FILTER_KICK_MESSAGES] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_FILTER_KICKS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 263, 431, 16, m_hWnd, (HMENU)BTN_FILTER_KICK_MESSAGES, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_FILTER_KICK_MESSAGES], BM_SETCHECK, (SettingManager->bBools[SETBOOL_FILTER_KICK_MESSAGES] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_FILTERED_KICKS_TO_OPS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 282, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_SEND_KICK_MESSAGES_TO_OPS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_STATUS_MESSAGES] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_STATUS_MESSAGES], WS_CHILD | WS_VISIBLE |
        BS_GROUPBOX, 0, 304, 447, 55, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_SEND_STATUS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 318, 431, 16, m_hWnd, (HMENU)BTN_SEND_STATUS_MESSAGES_TO_OPS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SEND_STATUS_MESSAGES_TO_OPS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_SEND_STATUS_PM], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 337, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM], BM_SETCHECK, (SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_PINGER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PINGER], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 359, 447, 55, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_DISALLOW_PINGER] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISALLOW_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 373, 431, 16, m_hWnd, (HMENU)BTN_DISALLOW_PINGER, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_DISALLOW_PINGER], BM_SETCHECK, (SettingManager->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_REPORT_PINGER] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REPORT_PINGERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, 392, 431, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_REPORT_PINGER], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REPORT_PINGERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", GetPageName(), MB_OK);
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[BTN_MINIMIZE_ON_STARTUP], SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[BTN_STOP_SCRIPT_ON_ERROR], SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[BTN_POPUP_SCRIPT_WINDOW_ON_ERROR], SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[BTN_SAVE_SCRIPT_ERRORS_TO_LOG], SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[BTN_SEND_KICK_MESSAGES_TO_OPS], SettingManager->bBools[SETBOOL_FILTER_KICK_MESSAGES] == true ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[BTN_SEND_STATUS_MESSAGES_IN_PM], SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[BTN_REPORT_PINGER], SettingManager->bBools[SETBOOL_DONT_ALLOW_PINGERS] == true ? FALSE : TRUE);

    ::SetWindowLongPtr(hWndPageItems[BTN_REPORT_PINGER], GWLP_USERDATA, (LONG_PTR)this);
    wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_REPORT_PINGER], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageAdvanced::GetPageName() {
    return LanguageManager->sTexts[LAN_ADVANCED];
}
//------------------------------------------------------------------------------

void SettingPageAdvanced::FocusLastItem() {
    ::SetFocus(hWndPageItems[BTN_REPORT_PINGER]);
}
//------------------------------------------------------------------------------

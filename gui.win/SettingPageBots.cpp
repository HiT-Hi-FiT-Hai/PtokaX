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
#include "SettingPageBots.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
//---------------------------------------------------------------------------
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageBots::SettingPageBots() {
	bUpdateHubSec = bUpdateMOTD = bUpdateHubNameWelcome = bUpdateRegOnlyMessage = bUpdateShareLimitMessage = bUpdateSlotsLimitMessage = false;
	bUpdateHubSlotRatioMessage = bUpdateMaxHubsLimitMessage = bUpdateNoTagMessage = bUpdateNickLimitMessage = bUpdateBotsSameNick = false;
	bBotNickChanged = bUpdateBot = bOpChatNickChanged = bUpdateOpChat = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageBots::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_HUB_BOT_NICK:
            case EDT_OP_CHAT_BOT_NICK:
                if(HIWORD(wParam) == EN_CHANGE) {
                    char buf[65];
                    ::GetWindowText((HWND)lParam, buf, 65);

                    bool bChanged = false;

                    for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                        if(buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ') {
                            strcpy(buf+ui16i, buf+ui16i+1);
                            bChanged = true;
                            ui16i--;
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

                break;
            case EDT_HUB_BOT_DESCRIPTION:
            case EDT_HUB_BOT_EMAIL:
            case EDT_OP_CHAT_BOT_DESCRIPTION:
            case EDT_OP_CHAT_BOT_EMAIL:
                if(HIWORD(wParam) == EN_CHANGE) {
                    char buf[65];
                    ::GetWindowText((HWND)lParam, buf, 65);

                    bool bChanged = false;

                    for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                        if(buf[ui16i] == '|' || buf[ui16i] == '$') {
                            strcpy(buf+ui16i, buf+ui16i+1);
                            bChanged = true;
                            ui16i--;
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

                break;
            case BTN_HUB_BOT_ENABLE:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(hWndPageItems[BTN_HUB_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                    ::EnableWindow(hWndPageItems[EDT_HUB_BOT_NICK], bEnable);
                    ::EnableWindow(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], bEnable);
                    ::EnableWindow(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], bEnable);
                    ::EnableWindow(hWndPageItems[EDT_HUB_BOT_EMAIL], bEnable);
                }

                break;
            case BTN_OP_CHAT_BOT_ENABLE:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
                    ::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_NICK], bEnable);
                    ::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], bEnable);
                    ::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], bEnable);
                }

                break;
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageBots::Save() {
    if(bCreated == false) {
        return;
    }

    bool bRegBot = ::SendMessage(hWndPageItems[BTN_HUB_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bRegBot == false && SettingManager->bBools[SETBOOL_REG_BOT] == true) {
		SettingManager->DisableBot();
	}

    char buf[65];
    int iLen = ::GetWindowText(hWndPageItems[EDT_HUB_BOT_NICK], buf, 65);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_BOT_NICK]) != NULL) {
        bUpdateHubSec = true;
        bUpdateMOTD = true;
        bUpdateHubNameWelcome = true;
        bUpdateRegOnlyMessage = true;
        bUpdateShareLimitMessage = true;
        bUpdateSlotsLimitMessage = true;
        bUpdateHubSlotRatioMessage = true;
        bUpdateMaxHubsLimitMessage = true;
        bUpdateNoTagMessage = true;
        bUpdateNickLimitMessage = true;
        bUpdateBotsSameNick = true;
		bBotNickChanged = true;
        bUpdateBot = true;
        SettingManager->DisableBot();
    }

    SettingManager->SetText(SETTXT_BOT_NICK, buf, iLen);

	if((bUpdateBot == false || bBotNickChanged == false) && (bRegBot == true && SettingManager->bBools[SETBOOL_REG_BOT] == false)) {
		bUpdateBot = true;
		bBotNickChanged = true;
	}

    bool bHubBotIsHubSec = ::SendMessage(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    if(bHubBotIsHubSec != SettingManager->bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC]) {
        bUpdateHubSec = true;
        bUpdateMOTD = true;
        bUpdateHubNameWelcome = true;
        bUpdateRegOnlyMessage = true;
        bUpdateShareLimitMessage = true;
        bUpdateSlotsLimitMessage = true;
        bUpdateHubSlotRatioMessage = true;
        bUpdateMaxHubsLimitMessage = true;
        bUpdateNoTagMessage = true;
        bUpdateNickLimitMessage = true;
    }

    SettingManager->SetBool(SETBOOL_USE_BOT_NICK_AS_HUB_SEC, bHubBotIsHubSec);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], buf, 65);

    if(bUpdateBot == false &&
        ((SettingManager->sTexts[SETTXT_BOT_DESCRIPTION] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_BOT_DESCRIPTION] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_BOT_DESCRIPTION]) != NULL))) {
        bUpdateBot = true;
        SettingManager->DisableBot(false);
    }

    SettingManager->SetText(SETTXT_BOT_DESCRIPTION, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUB_BOT_EMAIL], buf, 65);

    if(bUpdateBot == false &&
        (SettingManager->sTexts[SETTXT_BOT_EMAIL] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_BOT_EMAIL] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_BOT_EMAIL]) != NULL)) {
        bUpdateBot = true;
        SettingManager->DisableBot(false);
    }

    SettingManager->SetText(SETTXT_BOT_EMAIL, buf, iLen);

    bool bRegOpChat = ::SendMessage(hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;

    iLen = ::GetWindowText(hWndPageItems[EDT_OP_CHAT_BOT_NICK], buf, 65);

    if(bUpdateBotsSameNick == false && (bRegBot != SettingManager->bBools[SETBOOL_REG_BOT] ||
        bRegOpChat != SettingManager->bBools[SETBOOL_REG_OP_CHAT] ||
        strcmp(buf, SettingManager->sTexts[SETTXT_OP_CHAT_NICK]) != NULL)) {
        bUpdateBotsSameNick = true;
    }

    SettingManager->SetBool(SETBOOL_REG_BOT, bRegBot);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_OP_CHAT_NICK]) != NULL) {
		bOpChatNickChanged = true;
		bUpdateOpChat = true;
        SettingManager->DisableOpChat();
    }
    
	if(bUpdateOpChat == false && (bRegOpChat == false && SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true)) {
		SettingManager->DisableOpChat();
	}

    SettingManager->SetText(SETTXT_OP_CHAT_NICK, buf, iLen);

	if((bUpdateOpChat == false || bOpChatNickChanged == false) &&
		(bRegOpChat == true && SettingManager->bBools[SETBOOL_REG_OP_CHAT] == false)) {
		bUpdateOpChat = true;
		bOpChatNickChanged = true;
	}

    SettingManager->SetBool(SETBOOL_REG_OP_CHAT, bRegOpChat);

    iLen = ::GetWindowText(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], buf, 65);

    if(bUpdateOpChat == false &&
        ((SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION]) != NULL))) {
        bUpdateOpChat = true;
        SettingManager->DisableOpChat(false);
    }

    SettingManager->SetText(SETTXT_OP_CHAT_DESCRIPTION, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], buf, 65);

    if(bUpdateOpChat == false &&
        (SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL] != NULL &&
        strcmp(buf, SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL]) != NULL)) {
        bUpdateOpChat = true;
        SettingManager->DisableOpChat(false);
    }

    SettingManager->SetText(SETTXT_OP_CHAT_EMAIL, buf, iLen);

    SettingManager->SetBool(SETBOOL_KEEP_SLOW_USERS, ::SendMessage(hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);
}
//------------------------------------------------------------------------------

void SettingPageBots::GetUpdates(bool &bUpdatedHubNameWelcome, bool & /*bUpdatedHubName*/, bool & /*bUpdatedTCPPorts*/, bool & /*bUpdatedUDPPort*/,
    bool & /*bUpdatedAutoReg*/, bool &bUpdatedMOTD, bool &bUpdatedHubSec, bool &bUpdatedRegOnlyMessage, bool &bUpdatedShareLimitMessage,
	bool &bUpdatedSlotsLimitMessage, bool &bUpdatedHubSlotRatioMessage, bool &bUpdatedMaxHubsLimitMessage, bool &bUpdatedNoTagMessage, 
	bool &bUpdatedNickLimitMessage, bool &bUpdatedBotsSameNick, bool &bUpdatedBotNick, bool &bUpdatedBot, bool &bUpdatedOpChatNick,
	bool &bUpdatedOpChat, bool & /*bUpdatedLanguage*/, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
    bUpdatedHubNameWelcome = bUpdateHubNameWelcome;
	bUpdatedMOTD = bUpdateMOTD;
	bUpdatedHubSec = bUpdateHubSec;
	bUpdatedRegOnlyMessage = bUpdateRegOnlyMessage;
	bUpdatedShareLimitMessage = bUpdateShareLimitMessage;
	bUpdatedSlotsLimitMessage = bUpdateSlotsLimitMessage;
	bUpdatedHubSlotRatioMessage = bUpdateHubSlotRatioMessage;
	bUpdatedMaxHubsLimitMessage = bUpdateMaxHubsLimitMessage;
	bUpdatedNoTagMessage = bUpdateNoTagMessage;
	bUpdatedNickLimitMessage = bUpdateNickLimitMessage;
	bUpdatedBotsSameNick = bUpdateBotsSameNick;
	bUpdatedBotNick = bBotNickChanged;
	bUpdatedBot = bUpdateBot;
	bUpdatedOpChatNick = bOpChatNickChanged;
	bUpdatedOpChat = bUpdateOpChat;
}

//------------------------------------------------------------------------------
bool SettingPageBots::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    int iPosX = iGroupBoxMargin + iCheckHeight + iOneLineOneChecksGB + (2 * iOneLineGB) + 1 + 5;

    hWndPageItems[GB_HUB_BOT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_BOT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, 0, iFullGB, iPosX, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_HUB_BOT_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_AND_REG_BOT_ON_HUB], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iGroupBoxMargin, iFullEDT, iCheckHeight, m_hWnd, (HMENU)BTN_HUB_BOT_ENABLE, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_HUB_BOT_ENABLE], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REG_BOT] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_HUB_BOT_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iGroupBoxMargin + iCheckHeight + 1, iGBinGB, iOneLineOneChecksGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_BOT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_BOT_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, (2 * iGroupBoxMargin) + iCheckHeight + 1, iGBinGBEDT, iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_NICK, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_BOT_NICK], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_USE_AS_ALIAS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        13, (2 * iGroupBoxMargin) + iCheckHeight + 1 + iEditHeight + 4, iGBinGBEDT, iCheckHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], BM_SETCHECK, (SettingManager->bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_HUB_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iGroupBoxMargin + iCheckHeight + 1 + iOneLineOneChecksGB, iGBinGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_BOT_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, (2 * iGroupBoxMargin) + iCheckHeight + 1 + iOneLineOneChecksGB, iGBinGBEDT, iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_DESCRIPTION, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[GB_HUB_BOT_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iGroupBoxMargin + iCheckHeight + 1 + iOneLineOneChecksGB + iOneLineGB, iGBinGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_BOT_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_BOT_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, (2 * iGroupBoxMargin) + iCheckHeight + 1 + iOneLineOneChecksGB + iOneLineGB, iGBinGBEDT, iEditHeight, m_hWnd, (HMENU)EDT_HUB_BOT_EMAIL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_BOT_EMAIL], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[GB_OP_CHAT_BOT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_OP_CHAT_BOT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iGroupBoxMargin + iCheckHeight + (3 * iOneLineGB) + 1 + 5, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_OP_CHAT_BOT_ENABLE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ENABLE_AND_REG_BOT_ON_HUB], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosX + iGroupBoxMargin, iFullEDT, iCheckHeight, m_hWnd, (HMENU)BTN_OP_CHAT_BOT_ENABLE, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_OP_CHAT_BOT_ENABLE], BM_SETCHECK, (SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_OP_CHAT_BOT_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosX + iGroupBoxMargin + iCheckHeight + 1, iGBinGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_OP_CHAT_BOT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_OP_CHAT_NICK], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, iPosX + (2 * iGroupBoxMargin) + iCheckHeight + 1, iGBinGBEDT, iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_NICK, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_OP_CHAT_BOT_NICK], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[GB_OP_CHAT_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosX + iGroupBoxMargin + iCheckHeight + 1 + iOneLineGB, iGBinGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_OP_CHAT_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_AUTOHSCROLL, 13, iPosX + (2 * iGroupBoxMargin) + iCheckHeight + 1 + iOneLineGB, iGBinGBEDT, iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_DESCRIPTION, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[GB_OP_CHAT_BOT_EMAIL] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_EMAIL], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosX + iGroupBoxMargin + iCheckHeight + 1 + (2 * iOneLineGB), iGBinGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_OP_CHAT_BOT_EMAIL] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_OP_CHAT_EMAIL], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        13, iPosX + (2 * iGroupBoxMargin) + iCheckHeight + 1 + (2 * iOneLineGB), iGBinGBEDT, iEditHeight, m_hWnd, (HMENU)EDT_OP_CHAT_BOT_EMAIL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], EM_SETLIMITTEXT, 64, 0);

    iPosX += iGroupBoxMargin + iCheckHeight + (3 * iOneLineGB) + 1 + 5;

    hWndPageItems[GB_EXPERTS_ONLY] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_EXPERTS_ONLY], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iOneCheckGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_KEEP_SLOW], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosX + iGroupBoxMargin, iFullEDT, iCheckHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], BM_SETCHECK, (SettingManager->bBools[SETBOOL_KEEP_SLOW_USERS] == true ? BST_CHECKED : BST_UNCHECKED), 0);


    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndPageItems[EDT_HUB_BOT_NICK], SettingManager->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[BTN_HUB_BOT_IS_HUB_SEC], SettingManager->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[EDT_HUB_BOT_DESCRIPTION], SettingManager->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[EDT_HUB_BOT_EMAIL], SettingManager->bBools[SETBOOL_REG_BOT] == true ? TRUE : FALSE);

    ::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_NICK], SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_DESCRIPTION], SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);
    ::EnableWindow(hWndPageItems[EDT_OP_CHAT_BOT_EMAIL], SettingManager->bBools[SETBOOL_REG_OP_CHAT] == true ? TRUE : FALSE);

    wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageBots::GetPageName() {
    return LanguageManager->sTexts[LAN_DEFAULT_BOTS];
}
//------------------------------------------------------------------------------

void SettingPageBots::FocusLastItem() {
    ::SetFocus(hWndPageItems[BTN_KEEP_SLOW_CLIENTS_ONLINE]);
}
//------------------------------------------------------------------------------

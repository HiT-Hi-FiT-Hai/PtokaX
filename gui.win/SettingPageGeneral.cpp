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
#include "SettingPageGeneral.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

SettingPageGeneral::SettingPageGeneral() {
    bUpdateHubNameWelcome = bUpdateHubName = bUpdateTCPPorts = bUpdateUDPPort = bUpdateAutoReg = bUpdateLanguage = false;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT SettingPageGeneral::SettingPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_COMMAND) {
        switch(LOWORD(wParam)) {
            case EDT_HUB_NAME:
            case EDT_HUB_TOPIC:
            case EDT_HUB_DESCRIPTION:
            case EDT_ADMIN_NICK:
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }
            case EDT_TCP_PORTS:
                if(HIWORD(wParam) == EN_CHANGE) {
                    char buf[65];
                    ::GetWindowText((HWND)lParam, buf, 65);

                    bool bChanged = false;

                    for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                        if(isdigit(buf[ui16i]) == 0 && buf[ui16i] != ';') {
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
            case EDT_MAX_USERS:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinOneMaxShort((HWND)lParam);

                    return 0;
                }
        }
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void SettingPageGeneral::Save() {
    if(bCreated == false) {
        return;
    }

    char buf[1025];
    int iLen = ::GetWindowText(hWndPageItems[EDT_HUB_NAME], buf, 1025);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_HUB_NAME]) != NULL) {
        bUpdateHubNameWelcome = true;
        bUpdateHubName = true;
    }

    SettingManager->SetText(SETTXT_HUB_NAME, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUB_TOPIC], buf, 1025);

    if(bUpdateHubName == false && ((SettingManager->sTexts[SETTXT_HUB_TOPIC] == NULL && iLen != 0) ||
        (SettingManager->sTexts[SETTXT_HUB_TOPIC] != NULL && strcmp(buf, SettingManager->sTexts[SETTXT_HUB_TOPIC]) != NULL))) {
        bUpdateHubNameWelcome = true;
        bUpdateHubName = true;
    }

    SettingManager->SetText(SETTXT_HUB_TOPIC, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUB_DESCRIPTION], buf, 1025);
    SettingManager->SetText(SETTXT_HUB_DESCRIPTION, buf, iLen);

    SettingManager->SetBool(SETBOOL_ANTI_MOGLO, ::SendMessage(hWndPageItems[BTN_ANTI_MOGLO], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUB_ADDRESS], buf, 1025);
    SettingManager->SetText(SETTXT_HUB_ADDRESS, buf, iLen);

    SettingManager->SetBool(SETBOOL_RESOLVE_TO_IP, ::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    SettingManager->SetBool(SETBOOL_BIND_ONLY_SINGLE_IP, ::SendMessage(hWndPageItems[BTN_BIND_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    iLen = ::GetWindowText(hWndPageItems[EDT_TCP_PORTS], buf, 1025);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_TCP_PORTS]) != NULL) {
        bUpdateTCPPorts = true;
    }

    SettingManager->SetText(SETTXT_TCP_PORTS, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_UDP_PORT], buf, 1025);

    if(strcmp(buf, SettingManager->sTexts[SETTXT_UDP_PORT]) != NULL) {
        bUpdateUDPPort = true;
    }

    SettingManager->SetText(SETTXT_UDP_PORT, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_ADMIN_NICK], buf, 1025);
    SettingManager->SetText(SETTXT_ADMIN_NICK, buf, iLen);

    iLen = ::GetWindowText(hWndPageItems[EDT_HUB_LISTS], buf, 1025);
    SettingManager->SetText(SETTXT_REGISTER_SERVERS, buf, iLen);

    if((::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false) != SettingManager->bBools[SETBOOL_AUTO_REG]) {
        bUpdateAutoReg = true;
    }

    SettingManager->SetBool(SETBOOL_AUTO_REG, ::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false);

    uint32_t ui32CurSel = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETCURSEL, 0, 0);

    if(ui32CurSel == 0) {
        if((SettingManager->sTexts[SETTXT_LANGUAGE] == NULL && ui32CurSel != 0) ||
			(SettingManager->sTexts[SETTXT_LANGUAGE] != NULL && (ui32CurSel == 0 || strcmp(buf, SettingManager->sTexts[SETTXT_LANGUAGE]) != NULL))) {
            bUpdateLanguage = true;
            bUpdateHubNameWelcome = true;
        }

        SettingManager->SetText(SETTXT_LANGUAGE, "", 0);
    } else {
        uint32_t ui32Len = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXTLEN, ui32CurSel, 0);

        char * buf = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui32Len+1);

        if(buf == NULL) {
            AppendSpecialLog("Cannot create buf in SettingPageGeneral::Save!");
            return;
        }

        if((SettingManager->sTexts[SETTXT_LANGUAGE] == NULL && ui32CurSel != 0) ||
			(SettingManager->sTexts[SETTXT_LANGUAGE] != NULL && (ui32CurSel == 0 || strcmp(buf, SettingManager->sTexts[SETTXT_LANGUAGE]) != NULL))) {
            bUpdateLanguage = true;
            bUpdateHubNameWelcome = true;
        }

        ::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXT, ui32CurSel, (LPARAM)buf);

        SettingManager->SetText(SETTXT_LANGUAGE, buf, ui32Len);

        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
            string sDbgstr = "[BUF] Cannot deallocate buf in SettingPageGeneral::Save! "+string((uint32_t)GetLastError())+" "+
                string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
    }

    LRESULT lResult = ::SendMessage(hWndPageItems[UD_MAX_USERS], UDM_GETPOS, 0, 0);
    if(HIWORD(lResult) == 0) {
        SettingManager->SetShort(SETSHORT_MAX_USERS, LOWORD(lResult));
    }
}
//------------------------------------------------------------------------------

void SettingPageGeneral::GetUpdates(bool &bUpdatedHubNameWelcome, bool &bUpdatedHubName, bool &bUpdatedTCPPorts, bool &bUpdatedUDPPort,
    bool &bUpdatedAutoReg, bool & /*bUpdatedMOTD*/, bool & /*bUpdatedHubSec*/, bool & /*bUpdatedRegOnlyMessage*/, bool & /*bUpdatedShareLimitMessage*/,
    bool & /*bUpdatedSlotsLimitMessage*/, bool & /*bUpdatedHubSlotRatioMessage*/, bool & /*bUpdatedMaxHubsLimitMessage*/, bool & /*bUpdatedNoTagMessage*/,
    bool & /*bUpdatedNickLimitMessage*/, bool & /*bUpdatedBotsSameNick*/, bool & /*bUpdatedBotNick*/, bool & /*bUpdatedBot*/, bool & /*bUpdatedOpChatNick*/,
    bool & /*bUpdatedOpChat*/, bool &bUpdatedLanguage, bool & /*bUpdatedTextFiles*/, bool & /*bUpdatedRedirectAddress*/, bool & /*bUpdatedTempBanRedirAddress*/,
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/) {
    bUpdatedHubNameWelcome = bUpdateHubNameWelcome;
    bUpdatedHubName = bUpdateHubName;
    bUpdatedTCPPorts = bUpdateTCPPorts;
    bUpdatedUDPPort = bUpdateUDPPort;
    bUpdatedAutoReg = bUpdateAutoReg;
    bUpdatedLanguage = bUpdateLanguage;
}

//------------------------------------------------------------------------------
bool SettingPageGeneral::CreateSettingPage(HWND hOwner) {
    CreateHWND(hOwner);
    
    if(bCreated == false) {
        return false;
    }

    hWndPageItems[GB_LANGUAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_LANGUAGE], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 0, 0, 297, 42, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_LANGUAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | WS_VSCROLL | CBS_DROPDOWNLIST, 7, 15, 283, 20, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MAX_USERS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAX_USERS_LIMIT], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 302, 0, 140, 42, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, "", WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL |
        ES_RIGHT, 309, 15, 109, 20, m_hWnd, (HMENU)EDT_MAX_USERS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_USERS], EM_SETLIMITTEXT, 5, 0);

    AddUpDown(hWndPageItems[UD_MAX_USERS], 418, 15, 14, 20, (LPARAM)MAKELONG(32767, 1), (WPARAM)hWndPageItems[EDT_MAX_USERS],
        (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_USERS], 0));

    hWndPageItems[GB_HUB_NAME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_NAME], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 0, 42, 447, 40, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_NAME] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_NAME], WS_CHILD | WS_VISIBLE |
        ES_AUTOHSCROLL, 7, 59, 433, 18, m_hWnd, (HMENU)EDT_HUB_NAME, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_NAME], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[GB_HUB_TOPIC] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_TOPIC], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 0, 82, 447, 40, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_TOPIC] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_TOPIC], WS_CHILD | WS_VISIBLE |
        ES_AUTOHSCROLL, 7, 97, 433, 18, m_hWnd, (HMENU)EDT_HUB_TOPIC, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_TOPIC], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[GB_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE |
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_GROUPBOX, 0, 122, 447, 58, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_DESCRIPTION], WS_CHILD |
        WS_VISIBLE | ES_AUTOHSCROLL, 7, 137, 433, 18, m_hWnd, (HMENU)EDT_HUB_DESCRIPTION, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_DESCRIPTION], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[BTN_ANTI_MOGLO] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ANTI_MOGLO], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        7, 159, 433, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ANTI_MOGLO], BM_SETCHECK, (SettingManager->bBools[SETBOOL_ANTI_MOGLO] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_ADDRESS], WS_CHILD | WS_VISIBLE |
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_GROUPBOX, 0, 180, 447, 76, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_ADDRESS], WS_CHILD |
        WS_VISIBLE | ES_AUTOHSCROLL, 7, 195, 433, 18, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_ADDRESS], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[BTN_RESOLVE_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_RESOLVE_IP], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        7, 217, 433, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[BTN_BIND_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_BIND_ONLY_ADDR], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        7, 235, 433, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_BIND_ADDRESS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    hWndPageItems[GB_TCP_PORTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_TCP_PORTS], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 0, 256, 362, 40, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_TCP_PORTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_TCP_PORTS], WS_CHILD | WS_VISIBLE |
        ES_AUTOHSCROLL, 7, 271, 348, 18, m_hWnd, (HMENU)EDT_TCP_PORTS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_TCP_PORTS], EM_SETLIMITTEXT, 64, 0);
    AddToolTip(hWndPageItems[EDT_TCP_PORTS], LanguageManager->sTexts[LAN_TCP_PORTS_HINT]);

    hWndPageItems[GB_UDP_PORT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_UDP_PORT], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 367, 256, 80, 40, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_UDP_PORT] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_UDP_PORT], WS_CHILD | WS_VISIBLE |
        ES_NUMBER | ES_AUTOHSCROLL, 374, 271, 66, 18, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_UDP_PORT], EM_SETLIMITTEXT, 5, 0);

    hWndPageItems[GB_ADMIN_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_ADMIN_NICK], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN | BS_GROUPBOX, 0, 296, 447, 40, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_ADMIN_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_ADMIN_NICK], WS_CHILD | WS_VISIBLE |
        ES_AUTOHSCROLL, 7, 311, 433, 18, m_hWnd, (HMENU)EDT_ADMIN_NICK, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_ADMIN_NICK], EM_SETLIMITTEXT, 64, 0);

    hWndPageItems[GB_HUB_LISTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_REG_ADRS], WS_CHILD | WS_VISIBLE |
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_GROUPBOX, 0, 336, 447, 58, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_LISTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TRANSPARENT, WC_EDIT, SettingManager->sTexts[SETTXT_REGISTER_SERVERS], WS_CHILD |
        WS_VISIBLE | ES_AUTOHSCROLL, 7, 351, 433, 18, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_LISTS], EM_SETLIMITTEXT, 1024, 0);

    hWndPageItems[BTN_HUBLIST_AUTO_REG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_AUTO_REG], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        7, 373, 433, 16, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_SETCHECK, (SettingManager->bBools[SETBOOL_AUTO_REG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", GetPageName(), MB_OK);
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    // add default language and select it
    ::SendMessage(hWndPageItems[CB_LANGUAGE], CB_ADDSTRING, 0, (LPARAM)"Defaut English");
    ::SendMessage(hWndPageItems[CB_LANGUAGE], CB_SETCURSEL, 0, 0);

    // add all languages found in language dir
    struct _finddata_t langfile;
    intptr_t hFile = _findfirst((PATH+"\\language\\"+"*.xml").c_str(), &langfile);

    if(hFile != -1) {
        do {
    		if(((langfile.attrib & _A_SUBDIR) == _A_SUBDIR) == true ||
				stricmp(langfile.name+(strlen(langfile.name)-4), ".xml") != NULL) {
    			continue;
    		}

            ::SendMessage(hWndPageItems[CB_LANGUAGE], CB_ADDSTRING, 0, (LPARAM)string(langfile.name, strlen(langfile.name)-4).c_str());
        } while(_findnext(hFile, &langfile) == 0);

    	_findclose(hFile);
    }

    // Select actual language in combobox
    if(SettingManager->sTexts[SETTXT_LANGUAGE] != NULL) {
        uint32_t ui32Count = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETCOUNT, 0, 0);
        for(uint32_t i = 1; i < ui32Count; i++) {
            uint32_t ui32Len = (uint32_t)::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXTLEN, i,0);
            if(ui32Len == (int32_t)SettingManager->ui16TextsLens[SETTXT_LANGUAGE]) {
                char * buf = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, ui32Len+1);

                if(buf == NULL) {
                    AppendSpecialLog("Cannot create buf in SettingPageGeneral::CreateSettingPage!");
                    return false;
                }

                ::SendMessage(hWndPageItems[CB_LANGUAGE], CB_GETLBTEXT, i, (LPARAM)buf);
                if(stricmp(buf, SettingManager->sTexts[SETTXT_LANGUAGE]) == NULL) {
                    ::SendMessage(hWndPageItems[CB_LANGUAGE], CB_SETCURSEL, i, 0);

                    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
                        string sDbgstr = "[BUF] Cannot deallocate buf in SettingPageGeneral::CreateSettingPage! "+string((uint32_t)GetLastError())+" "+
                            string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
                        AppendSpecialLog(sDbgstr);
                    }

                    break;
                }

                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
                    string sDbgstr = "[BUF] Cannot deallocate buf in SettingPageGeneral::CreateSettingPage1! "+string((uint32_t)GetLastError())+" "+
                        string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
                    AppendSpecialLog(sDbgstr);
                }
            }
        }
    }

	return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral::GetPageName() {
    return LanguageManager->sTexts[LAN_GENERAL_SETTINGS];
}
//------------------------------------------------------------------------------

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
#include "SettingPageGeneral.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiUtil.h"
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
                if(HIWORD(wParam) == EN_CHANGE) {
                    RemovePipes((HWND)lParam);

                    return 0;
                }

                break;
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

                break;
            case EDT_UDP_PORT:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 0, 65535);

                    return 0;
                }

                break;
            case EDT_MAX_USERS:
                if(HIWORD(wParam) == EN_CHANGE) {
                    MinMaxCheck((HWND)lParam, 1, 32767);

                    return 0;
                }

                break;
            case BTN_RESOLVE_ADDRESS:
                if(HIWORD(wParam) == BN_CLICKED) {
                    BOOL bEnable = ::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
                    ::SetWindowText(hWndPageItems[EDT_IPV4_ADDRESS], bEnable == FALSE ? sHubIP :
                        (SettingManager->sTexts[SETTXT_IPV4_ADDRESS] != NULL ? SettingManager->sTexts[SETTXT_IPV4_ADDRESS] : ""));
                    ::EnableWindow(hWndPageItems[EDT_IPV4_ADDRESS], bEnable);
                    ::SetWindowText(hWndPageItems[EDT_IPV6_ADDRESS], bEnable == FALSE ? sHubIP6 :
                        (SettingManager->sTexts[SETTXT_IPV6_ADDRESS] != NULL ? SettingManager->sTexts[SETTXT_IPV6_ADDRESS] : "");
                    ::EnableWindow(hWndPageItems[EDT_IPV6_ADDRESS], bEnable);
                }

                break;
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

    if(SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == false) {
        iLen = ::GetWindowText(hWndPageItems[EDT_IPV4_ADDRESS], buf, 1025);
        SettingManager->SetText(SETTXT_IPV4_ADDRESS, buf, iLen);

        iLen = ::GetWindowText(hWndPageItems[EDT_IPV6_ADDRESS], buf, 1025);
        SettingManager->SetText(SETTXT_IPV6_ADDRESS, buf, iLen);
    }

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
    bool & /*bUpdatedPermBanRedirAddress*/, bool & /*bUpdatedSysTray*/, bool & /*bUpdatedScripting*/, bool & /*bUpdatedMinShare*/, bool & /*bUpdatedMaxShare*/) {
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

    RECT rcThis = { 0 };
    ::GetWindowRect(m_hWnd, &rcThis);

    iFullGB = (rcThis.right - rcThis.left) - 5;
    iFullEDT = (rcThis.right - rcThis.left) - 21;
    iGBinGB = (rcThis.right - rcThis.left) - 15;
    iGBinGBEDT = (rcThis.right - rcThis.left) - 31;

    int iPosX = iOneLineGB;

    ::SetWindowPos(m_hWnd, NULL, 0, 0, rcThis.right, (3 * iOneLineTwoGroupGB) + iOneLineGB + 3, SWP_NOMOVE | SWP_NOZORDER);

    hWndPageItems[GB_LANGUAGE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_LANGUAGE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, ScaleGui(297), iOneLineGB,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[CB_LANGUAGE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST, 8, iGroupBoxMargin, ScaleGui(297) - 16, iEditHeight,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[GB_MAX_USERS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_MAX_USERS_LIMIT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        ScaleGui(297) + 5, 0, (rcThis.right - rcThis.left) - (ScaleGui(297) + 10), iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_MAX_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL | ES_RIGHT,
        ScaleGui(297) + 13, iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(297) + iUpDownWidth + 26), iEditHeight,
        m_hWnd, (HMENU)EDT_MAX_USERS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_MAX_USERS], EM_SETLIMITTEXT, 5, 0);

    AddUpDown(hWndPageItems[UD_MAX_USERS], (rcThis.right - rcThis.left)-iUpDownWidth-13, iGroupBoxMargin, iUpDownWidth, iEditHeight, (LPARAM)MAKELONG(32767, 1),
        (WPARAM)hWndPageItems[EDT_MAX_USERS], (LPARAM)MAKELONG(SettingManager->iShorts[SETSHORT_MAX_USERS], 0));

    hWndPageItems[GB_HUB_NAME] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_NAME], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_NAME] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_NAME], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        8, iOneLineGB + iGroupBoxMargin, iFullEDT, iEditHeight, m_hWnd, (HMENU)EDT_HUB_NAME, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_NAME], EM_SETLIMITTEXT, 256, 0);

    iPosX += iOneLineGB;

    hWndPageItems[GB_HUB_TOPIC] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_TOPIC], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_TOPIC] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_TOPIC], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        8, iPosX + iGroupBoxMargin, iFullEDT, iEditHeight, m_hWnd, (HMENU)EDT_HUB_TOPIC, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_TOPIC], EM_SETLIMITTEXT, 256, 0);

    iPosX += iOneLineGB;

    hWndPageItems[GB_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iOneLineOneChecksGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_DESCRIPTION] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_DESCRIPTION], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        8, iPosX + iGroupBoxMargin, iFullEDT, iEditHeight, m_hWnd, (HMENU)EDT_HUB_DESCRIPTION, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_DESCRIPTION], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[BTN_ANTI_MOGLO] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ANTI_MOGLO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosX + iGroupBoxMargin + iEditHeight + 4, iFullEDT, iCheckHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_ANTI_MOGLO], BM_SETCHECK, (SettingManager->bBools[SETBOOL_ANTI_MOGLO] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    iPosX += iOneLineOneChecksGB;

    hWndPageItems[GB_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iOneLineTwoChecksGB + iOneLineGB + 3, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_HUB_ADDRESS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        8, iPosX + iGroupBoxMargin, iFullEDT, iEditHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_ADDRESS], EM_SETLIMITTEXT, 256, 0);

    hWndPageItems[BTN_RESOLVE_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_RESOLVE_HOSTNAME], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosX + iGroupBoxMargin + iEditHeight + 4, iFullEDT, iCheckHeight, m_hWnd, (HMENU)BTN_RESOLVE_ADDRESS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_RESOLVE_ADDRESS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    int iFourtyPercent = (int)(iGBinGB * 0.4);
    hWndPageItems[GB_IPV4_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_IPV4_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        5, iPosX + iGroupBoxMargin + iEditHeight + iCheckHeight + 5, iFourtyPercent - 3, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_IPV4_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true ? sHubIP :
        (SettingManager->sTexts[SETTXT_IPV4_ADDRESS] != NULL ? SettingManager->sTexts[SETTXT_IPV4_ADDRESS] : ""),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 13, iPosX + iGroupBoxMargin + iEditHeight + iCheckHeight + 5 + iGroupBoxMargin, iFourtyPercent - 19, iEditHeight,
        m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_IPV4_ADDRESS], EM_SETLIMITTEXT, 15, 0);

    hWndPageItems[GB_IPV6_ADDRESS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_IPV6_ADDRESS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        10 + (iFourtyPercent - 3), iPosX + iGroupBoxMargin + iEditHeight + iCheckHeight + 5, (iGBinGB - (iFourtyPercent - 3)) - 5, iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_IPV6_ADDRESS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true ? sHubIP6 :
        (SettingManager->sTexts[SETTXT_IPV6_ADDRESS] != NULL ? SettingManager->sTexts[SETTXT_IPV6_ADDRESS] : ""),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 18 + (iFourtyPercent - 3), iPosX + iGroupBoxMargin + iEditHeight + iCheckHeight + 5 + iGroupBoxMargin,
        (iGBinGB - (iFourtyPercent - 3)) - 21, iEditHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_IPV6_ADDRESS], EM_SETLIMITTEXT, 39, 0);

    hWndPageItems[BTN_BIND_ADDRESS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_BIND_ONLY_ADDRS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosX + iGroupBoxMargin + iEditHeight + iCheckHeight + iOneLineGB + 10, iFullEDT, iCheckHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_BIND_ADDRESS], BM_SETCHECK, (SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    iPosX += iOneLineTwoChecksGB + iOneLineGB + 3;

    hWndPageItems[GB_TCP_PORTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_TCP_PORTS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, ScaleGui(362), iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_TCP_PORTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_TCP_PORTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        8, iPosX + iGroupBoxMargin, ScaleGui(362) - 16, iEditHeight, m_hWnd, (HMENU)EDT_TCP_PORTS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_TCP_PORTS], EM_SETLIMITTEXT, 64, 0);
    AddToolTip(hWndPageItems[EDT_TCP_PORTS], LanguageManager->sTexts[LAN_TCP_PORTS_HINT]);

    hWndPageItems[GB_UDP_PORT] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_UDP_PORT], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        ScaleGui(362) + 5, iPosX, (rcThis.right - rcThis.left) - (ScaleGui(362) + 10), iOneLineGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_UDP_PORT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_UDP_PORT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
        ScaleGui(362) + 13, iPosX + iGroupBoxMargin, (rcThis.right - rcThis.left) - (ScaleGui(362) + 26), iEditHeight, m_hWnd, (HMENU)EDT_UDP_PORT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_UDP_PORT], EM_SETLIMITTEXT, 5, 0);
    AddToolTip(hWndPageItems[EDT_UDP_PORT], LanguageManager->sTexts[LAN_ZERO_DISABLED]);

    iPosX += iOneLineGB;

    hWndPageItems[GB_HUB_LISTS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_HUB_REG_ADRS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        0, iPosX, iFullGB, iOneLineOneChecksGB, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[EDT_HUB_LISTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, SettingManager->sTexts[SETTXT_REGISTER_SERVERS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        8, iPosX + iGroupBoxMargin, iFullEDT, iEditHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_HUB_LISTS], EM_SETLIMITTEXT, 1024, 0);
    AddToolTip(hWndPageItems[EDT_HUB_LISTS], LanguageManager->sTexts[LAN_HUB_LIST_REGS_HINT]);

    hWndPageItems[BTN_HUBLIST_AUTO_REG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_AUTO_REG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        8, iPosX + iGroupBoxMargin + iEditHeight + 4, iFullEDT, iCheckHeight, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[BTN_HUBLIST_AUTO_REG], BM_SETCHECK, (SettingManager->bBools[SETBOOL_AUTO_REG] == true ? BST_CHECKED : BST_UNCHECKED), 0);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
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

    if(SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true) {
        ::EnableWindow(hWndPageItems[EDT_IPV4_ADDRESS], FALSE);
        ::EnableWindow(hWndPageItems[EDT_IPV6_ADDRESS], FALSE);
    }

    wpOldButtonProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[BTN_HUBLIST_AUTO_REG], GWLP_WNDPROC, (LONG_PTR)ButtonProc);

    return true;
}
//------------------------------------------------------------------------------

char * SettingPageGeneral::GetPageName() {
    return LanguageManager->sTexts[LAN_GENERAL_SETTINGS];
}
//------------------------------------------------------------------------------

void SettingPageGeneral::FocusLastItem() {
    ::SetFocus(hWndPageItems[BTN_HUBLIST_AUTO_REG]);
}
//------------------------------------------------------------------------------

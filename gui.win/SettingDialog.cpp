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
#include "SettingDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "MainWindow.h"
#include "Resources.h"
#include "SettingPageAdvanced.h"
#include "SettingPageBans.h"
#include "SettingPageBots.h"
#include "SettingPageGeneral.h"
#include "SettingPageMoreGeneral.h"
#include "SettingPageMOTD.h"
#include "SettingPageMyINFO.h"
#include "../core/TextFileManager.h"
//---------------------------------------------------------------------------

SettingDialog::SettingDialog() {
    m_hWnd = hWndTree = NULL;

    btnOK = btnCancel = NULL;

    memset(&SettingPages, 0, 12 * sizeof(SettingPage *));
    SettingPages[0] = new SettingPageGeneral();
    SettingPages[1] = new SettingPageMOTD();
    SettingPages[2] = new SettingPageBots();
    SettingPages[3] = new SettingPageMoreGeneral();
    SettingPages[4] = new SettingPageBans();
    SettingPages[5] = new SettingPageAdvanced();
    SettingPages[6] = new SettingPageMyINFO();
}
//---------------------------------------------------------------------------

SettingDialog::~SettingDialog() {
    for(uint8_t ui8i = 0; ui8i < 12; ui8i++) {
        if(SettingPages[ui8i] != NULL) {
            delete SettingPages[ui8i];
        }
    }
}
//---------------------------------------------------------------------------

INT_PTR CALLBACK SettingDialog::StaticSettingDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	SettingDialog * pParent;

	if(uMsg == WM_INITDIALOG) {
		pParent = (SettingDialog *)lParam;
		::SetWindowLongPtr(hWndDlg, GWLP_USERDATA, (LONG_PTR)pParent);

        pParent->m_hWnd = hWndDlg;
	} else {
		pParent = (SettingDialog *)::GetWindowLongPtr(hWndDlg, GWLP_USERDATA);
		if(pParent == NULL) {
			return FALSE;
		}
	}

    SetWindowLongPtr(hWndDlg, DWLP_MSGRESULT, 0);

	return pParent->SettingDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT SettingDialog::SettingDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG: {
            ::SetWindowText(m_hWnd, LanguageManager->sTexts[LAN_SETTINGS]);

            RECT rcParent;
            ::GetWindowRect(::GetParent(m_hWnd), &rcParent);

            if((rcParent.left-153) >= 5) {
                rcParent.left -= 153;
            } else {
                rcParent.left = 5;
            }

            if((rcParent.top-84) >= 5) {
                rcParent.top -= 84;
            } else {
                rcParent.top = 5;
            }

            ::SetWindowPos(m_hWnd, NULL, rcParent.left, rcParent.top, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            hWndTree = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP |
                TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, 5, 5, 150, 370, m_hWnd, NULL, g_hInstance, NULL);

            TVINSERTSTRUCT tvIS;
            memset(&tvIS, 0, sizeof(TVINSERTSTRUCT));
            tvIS.hInsertAfter = TVI_LAST;
            tvIS.item.mask = TVIF_PARAM | TVIF_TEXT;
            tvIS.itemex.mask |= TVIF_STATE;
            tvIS.itemex.state = TVIS_EXPANDED;
            tvIS.itemex.stateMask = TVIS_EXPANDED;

            tvIS.hParent = TVI_ROOT;
            tvIS.item.lParam = (LPARAM)SettingPages[0];
            tvIS.item.pszText = SettingPages[0]->GetPageName();
            tvIS.hParent = (HTREEITEM)::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = (LPARAM)SettingPages[1];
            tvIS.item.pszText = SettingPages[1]->GetPageName();
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = (LPARAM)SettingPages[2];
            tvIS.item.pszText = SettingPages[2]->GetPageName();
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = (LPARAM)SettingPages[3];
            tvIS.item.pszText = SettingPages[3]->GetPageName();
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = (LPARAM)SettingPages[4];
            tvIS.item.pszText = SettingPages[4]->GetPageName();
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.hParent = TVI_ROOT;
            tvIS.item.lParam = (LPARAM)SettingPages[5];
            tvIS.item.pszText = SettingPages[5]->GetPageName();
            tvIS.hParent = (HTREEITEM)::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = (LPARAM)SettingPages[6];
            tvIS.item.pszText = SettingPages[6]->GetPageName();
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.hParent = TVI_ROOT;
            tvIS.item.lParam = NULL;
            tvIS.item.pszText = LanguageManager->sTexts[LAN_RULES];
            tvIS.hParent = (HTREEITEM)::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = NULL;
            tvIS.item.pszText = LanguageManager->sTexts[LAN_MORE_RULES];
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.hParent = TVI_ROOT;
            tvIS.item.lParam = NULL;
            tvIS.item.pszText = LanguageManager->sTexts[LAN_DEFLOOD];
            tvIS.hParent = (HTREEITEM)::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = NULL;
            tvIS.item.pszText = LanguageManager->sTexts[LAN_MORE_DEFLOOD];
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            tvIS.item.lParam = NULL;
            tvIS.item.pszText = LanguageManager->sTexts[LAN_MORE_MORE_DEFLOOD];
            ::SendMessage(hWndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIS);

            btnOK = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                WS_TABSTOP | BS_PUSHBUTTON, 4, 379, 152, 20, m_hWnd, (HMENU)IDOK, g_hInstance, NULL);

            btnCancel = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                WS_TABSTOP | BS_PUSHBUTTON, 4, 402, 152, 20, m_hWnd, (HMENU)IDCANCEL, g_hInstance, NULL);

            HWND hWnds[] = { hWndTree, btnOK, btnCancel };

            for(uint8_t ui8i = 0; ui8i < (sizeof(hWnds) / sizeof(hWnds[0])); ui8i++) {
                ::SendMessage(hWnds[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
            }

            return TRUE;
        }
        case WM_SETFOCUS:
            return TRUE;
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDOK: {
                    bool bUpdateHubNameWelcome = false, bUpdateHubName = false, bUpdateTCPPorts = false, bUpdateUDPPort = false, bUpdateAutoReg = false,
                        bUpdateMOTD = false, bUpdateHubSec = false, bUpdateRegOnlyMessage = false, bUpdateShareLimitMessage = false,
                        bUpdateSlotsLimitMessage = false, bUpdateHubSlotRatioMessage = false, bUpdateMaxHubsLimitMessage = false,
                        bUpdateNoTagMessage = false, bUpdateNickLimitMessage = false, bUpdateBotsSameNick = false, bUpdateBotNick = false,
                        bUpdateBot = false, bUpdateOpChatNick = false, bUpdateOpChat = false, bUpdateLanguage = false, bUpdateTextFiles = false,
                        bUpdateRedirectAddress = false, bUpdateTempBanRedirAddress = false, bUpdatePermBanRedirAddress = false, bUpdateSysTray = false,
                        bUpdateScripting = false;

                    SettingManager->bUpdateLocked = true;

                    for(uint8_t ui8i = 0; ui8i < 12; ui8i++) {
                        if(SettingPages[ui8i] != NULL) {
                            SettingPages[ui8i]->Save();

                            SettingPages[ui8i]->GetUpdates(bUpdateHubNameWelcome, bUpdateHubName, bUpdateTCPPorts, bUpdateUDPPort, bUpdateAutoReg,
                                bUpdateMOTD, bUpdateHubSec, bUpdateRegOnlyMessage, bUpdateShareLimitMessage, bUpdateSlotsLimitMessage,
                                bUpdateHubSlotRatioMessage, bUpdateMaxHubsLimitMessage, bUpdateNoTagMessage, bUpdateNickLimitMessage,
                                bUpdateBotsSameNick, bUpdateBotNick, bUpdateBot, bUpdateOpChatNick, bUpdateOpChat, bUpdateLanguage, bUpdateTextFiles,
                                bUpdateRedirectAddress, bUpdateTempBanRedirAddress, bUpdatePermBanRedirAddress, bUpdateSysTray, bUpdateScripting);
                        }
                    }

                    SettingManager->bUpdateLocked = false;

                    if(bUpdateHubSec == true) {
                        SettingManager->UpdateHubSec();
                    }

                    if(bUpdateMOTD == true) {
                        SettingManager->UpdateMOTD();
                    }

                    if(bUpdateLanguage == true) {
                        SettingManager->UpdateLanguage();
                    }

                    if(bUpdateHubNameWelcome == true) {
                        SettingManager->UpdateHubNameWelcome();
                    }

                    if(bUpdateHubName == true) {
                        pMainWindow->UpdateTitleBar();
                        SettingManager->UpdateHubName();
                    }

                    if(bUpdateRedirectAddress == true) {
                        SettingManager->UpdateRedirectAddress();
                    }

                    if(bUpdateRegOnlyMessage == true) {
                        SettingManager->UpdateRegOnlyMessage();
                    }

                    if(bUpdateShareLimitMessage == true) {
                        SettingManager->UpdateShareLimitMessage();
                    }

                    if(bUpdateSlotsLimitMessage == true) {
                        SettingManager->UpdateSlotsLimitMessage();
                    }

                    if(bUpdateMaxHubsLimitMessage == true) {
                        SettingManager->UpdateMaxHubsLimitMessage();
                    }

                    if(bUpdateHubSlotRatioMessage == true) {
                        SettingManager->UpdateHubSlotRatioMessage();
                    }

                    if(bUpdateNoTagMessage == true) {
                        SettingManager->UpdateNoTagMessage();
                    }

                    if(bUpdateTempBanRedirAddress == true) {
                        SettingManager->UpdateTempBanRedirAddress();
                    }

                    if(bUpdatePermBanRedirAddress == true) {
                        SettingManager->UpdatePermBanRedirAddress();
                    }

                    if(bUpdateNickLimitMessage == true) {
                        SettingManager->UpdateNickLimitMessage();
                    }

                    if(bUpdateTCPPorts == true) {
                        SettingManager->UpdateTCPPorts();
                    }

                    if(bUpdateBotsSameNick == true) {
                        SettingManager->UpdateBotsSameNick();
                    }

                    if(bUpdateTextFiles == true) {
                        TextFileManager->RefreshTextFiles();
                    }

                    if(bUpdateBot == true) {
                        SettingManager->UpdateBot(bUpdateBotNick);
                    }

                    if(bUpdateOpChat == true) {
                        SettingManager->UpdateOpChat(bUpdateOpChatNick);
                    }

                    if(bUpdateUDPPort == true) {
                        SettingManager->UpdateUDPPort();
                    }

                    if(bUpdateSysTray == true) {
                        pMainWindow->UpdateSysTray();
                    }

                    if(bUpdateAutoReg == true) {
                        ServerUpdateAutoRegState();
                    }

                    if(bUpdateScripting == true) {
                        SettingManager->UpdateScripting();
                    }
                }
                case IDCANCEL:
                    ::EndDialog(m_hWnd, LOWORD(wParam));
                    return TRUE;
            }

            break;
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndTree) {
                if(((LPNMHDR)lParam)->code == TVN_SELCHANGED) {
                    OnSelChanged();
                    return TRUE;
                }
            }

            break;
    }

	return FALSE;
}
//------------------------------------------------------------------------------

INT_PTR SettingDialog::DoModal(HWND hWndParent) {
	return ::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hWndParent, StaticSettingDialogProc, (LPARAM)this);
}
//---------------------------------------------------------------------------

void SettingDialog::OnSelChanged() {
    HTREEITEM htiNode = (HTREEITEM)::SendMessage(hWndTree, TVM_GETNEXTITEM, TVGN_CARET, 0L);

    if(htiNode == NULL) {
        return;
    }

    char buf[256];

	TVITEM tvItem;
	memset(&tvItem, 0, sizeof(TVITEM));
	tvItem.hItem = htiNode;
	tvItem.mask = TVIF_PARAM | TVIF_TEXT;
	tvItem.pszText = buf;
	tvItem.cchTextMax = 256;

	if((BOOL)::SendMessage(hWndTree, TVM_GETITEM, 0, (LPARAM)&tvItem) == FALSE) {
        return;
    }

    if(tvItem.lParam == NULL) {
        ::MessageBox(m_hWnd, "Not implemented!", tvItem.pszText, MB_OK);

        return;
    }

    SettingPage * curSetPage = (SettingPage *)tvItem.lParam;

    if(curSetPage->bCreated == false) {
        if(curSetPage->CreateSettingPage(m_hWnd) == false) {
            ::MessageBox(m_hWnd, "Setting page creation failed!", tvItem.pszText, MB_OK);
            ::EndDialog(m_hWnd, IDCANCEL);
        }
    }

    ::BringWindowToTop(curSetPage->m_hWnd);
}
//---------------------------------------------------------------------------

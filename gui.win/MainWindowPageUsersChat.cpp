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
#include "MainWindowPageUsersChat.h"
//---------------------------------------------------------------------------
#include "../core/colUsers.h"
#include "../core/globalQueue.h"
#include "../core/hashBanManager.h"
#include "../core/hashUsrManager.h"
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/User.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LineDialog.h"
#include "Resources.h"
//---------------------------------------------------------------------------
MainWindowPageUsersChat *pMainWindowPageUsersChat = NULL;
//---------------------------------------------------------------------------
static WNDPROC wpOldEditProc = NULL;
//---------------------------------------------------------------------------

MainWindowPageUsersChat::MainWindowPageUsersChat() {
    pMainWindowPageUsersChat = this;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

MainWindowPageUsersChat::~MainWindowPageUsersChat() {
    pMainWindowPageUsersChat = NULL;
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageUsersChat::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_SETFOCUS:
            ::SetFocus(hWndPageItems[BTN_SHOW_CHAT]);

            return 0;
        case WM_WINDOWPOSCHANGED: {
            int iX = ((WINDOWPOS*)lParam)->cx;
            int iY = ((WINDOWPOS*)lParam)->cy;

            ::SetWindowPos(hWndPageItems[BTN_SHOW_CHAT], NULL, 0, 0, ((iX-150)/2)-3, 16, SWP_NOMOVE | SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_SHOW_COMMANDS], NULL, ((iX-150)/2)+1, 0, ((iX-150)/2)-3, 16, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], NULL, iX-148, 0, (iX-(iX-150))-4, 16, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[REDT_CHAT], NULL, 0, 0, iX-152, iY-43, SWP_NOMOVE | SWP_NOZORDER);
            ::SendMessage(hWndPageItems[REDT_CHAT], WM_VSCROLL, SB_BOTTOM, NULL);
            ::SetWindowPos(hWndPageItems[LV_USERS], NULL, iX-148, 16, (iX-(iX-148))-2, iY-43, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[EDT_CHAT], NULL, 2, iY-25, iX-152, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_UPDATE_USERS], NULL, iX-149, iY-25, (iX-(iX-148)), 23, SWP_NOZORDER);

            return 0;
        }
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndPageItems[REDT_CHAT] && ((LPNMHDR)lParam)->code == EN_LINK) {
                if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
                    TCHAR* sURL = new TCHAR[(((ENLINK *)lParam)->chrg.cpMax - ((ENLINK *)lParam)->chrg.cpMin)+1];

                    if(sURL == NULL) {
                        break;
                    }

                    TEXTRANGE tr = { 0 };
                    tr.chrg.cpMin = ((ENLINK *)lParam)->chrg.cpMin;
                    tr.chrg.cpMax = ((ENLINK *)lParam)->chrg.cpMax;
                    tr.lpstrText = sURL;

                    ::SendMessage(hWndPageItems[REDT_CHAT], EM_GETTEXTRANGE, 0, (LPARAM)&tr);

                    ::ShellExecute(NULL, NULL, sURL, NULL, NULL, SW_SHOWNORMAL);

                    delete[] sURL;
                }
            } else if(((LPNMHDR)lParam)->hwndFrom == hWndPageItems[LV_USERS] && ((LPNMHDR)lParam)->code == LVN_GETINFOTIP) {
                NMLVGETINFOTIP * pGetInfoTip = (LPNMLVGETINFOTIP)lParam;

                char msg[1024];

                LVITEM lvItem = { 0 };
                lvItem.mask = LVIF_PARAM | LVIF_TEXT;
                lvItem.iItem = pGetInfoTip->iItem;
                lvItem.pszText = msg;
                lvItem.cchTextMax = 1024;

                if((BOOL)::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
                    return 0;
                }

                User * curUser = (User *)lvItem.lParam;

                if(::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_UNCHECKED) {
                    User * testUser = hashManager->FindUser(lvItem.pszText, strlen(lvItem.pszText));

                    if(testUser == NULL || testUser != curUser) {
                        return 0;
                    }
                }

                string sInfoTip = string(LanguageManager->sTexts[LAN_NICK], (size_t)LanguageManager->ui16TextsLens[LAN_NICK]) + ": " + string(curUser->Nick, curUser->NickLen) +
                    "\n" + string(LanguageManager->sTexts[LAN_IP], (size_t)LanguageManager->ui16TextsLens[LAN_IP]) + ": " + string(curUser->IP);

                sInfoTip += "\n\n" + string(LanguageManager->sTexts[LAN_CLIENT], (size_t)LanguageManager->ui16TextsLens[LAN_CLIENT]) + ": " +
                    string(curUser->Client, (size_t)curUser->ui8ClientLen) +
                    "\n" + string(LanguageManager->sTexts[LAN_VERSION], (size_t)LanguageManager->ui16TextsLens[LAN_VERSION]) + ": " + string(curUser->Ver, (size_t)curUser->ui8VerLen);

                char sMode[2];
                sMode[0] = curUser->Mode;
                sMode[1] = '\0';
                sInfoTip += "\n\n" + string(LanguageManager->sTexts[LAN_MODE], (size_t)LanguageManager->ui16TextsLens[LAN_MODE]) + ": " + string(sMode) +
                    "\n" + string(LanguageManager->sTexts[LAN_SLOTS], (size_t)LanguageManager->ui16TextsLens[LAN_SLOTS]) + ": " + string(curUser->Slots) +
                    "\n" + string(LanguageManager->sTexts[LAN_HUBS], (size_t)LanguageManager->ui16TextsLens[LAN_HUBS]) + ": " + string(curUser->Hubs);

                if(curUser->OLimit != 0) {
                    sInfoTip += "\n" + string(LanguageManager->sTexts[LAN_AUTO_OPEN_SLOT_WHEN_UP_UNDER], (size_t)LanguageManager->ui16TextsLens[LAN_AUTO_OPEN_SLOT_WHEN_UP_UNDER]) + " " +
                    string(curUser->OLimit)+" kB/s";
                }

                if(curUser->DLimit != 0) {
                    sInfoTip += "\n" + string(LanguageManager->sTexts[LAN_LIMITER], (size_t)LanguageManager->ui16TextsLens[LAN_LIMITER])+ " D:" + string(curUser->DLimit) + " kB/s";
                }

                if(curUser->LLimit != 0) {
                    sInfoTip += "\n" + string(LanguageManager->sTexts[LAN_LIMITER], (size_t)LanguageManager->ui16TextsLens[LAN_LIMITER])+ " L:" + string(curUser->LLimit) + " kB/s";
                }

                sInfoTip += "\n\nRecvBuf: " + string(curUser->recvbuflen) + " bytes";
                sInfoTip += "\nSendBuf: " + string(curUser->sendbuflen) + " bytes";

                pGetInfoTip->cchTextMax = (int)(sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size());
                memcpy(pGetInfoTip->pszText, sInfoTip.c_str(), sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size());
                pGetInfoTip->pszText[(sInfoTip.size() > INFOTIPSIZE ? INFOTIPSIZE : sInfoTip.size()) - 1] = '\0';

                return 0;
            }
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_AUTO_UPDATE_USERLIST:
                    if(HIWORD(wParam) == BN_CLICKED && bServerRunning == true) {
                        bool bChecked = ::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED ? true : false;
                        ::EnableWindow(hWndPageItems[BTN_UPDATE_USERS], bChecked == true ? FALSE : TRUE);
                        if(bChecked == true) {
                            UpdateUserList();
                        }

                        return 0;
                    }

                    break;
                case BTN_UPDATE_USERS:
                    UpdateUserList();
                    return 0;
                case EDT_CHAT:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        int iLen = ::GetWindowTextLength((HWND)lParam);

                        char * buf = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+1);

                        if(buf == NULL) {
                            AppendSpecialLog("Cannot create buf in MainWindowPageUsersChat::MainWindowPageProc!");
                            return 0;
                        }

                        ::GetWindowText((HWND)lParam, buf, iLen+1);

                        bool bChanged = false;

                        for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                            if(buf[ui16i] == '|') {
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

                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
                            string sDbgstr = "[BUF] Cannot deallocate buf in MainWindowPageUsersChat::MainWindowPageProc! "+string((uint32_t)GetLastError())+" "+
                                string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
                            AppendSpecialLog(sDbgstr);
                        }

                        return 0;
                    }

                    break;
                case IDC_COPY:
                    ::SendMessage(hWndPageItems[REDT_CHAT], WM_COPY, 0, 0);
                    return 0;
                case IDC_SELECT_ALL: {
                    CHARRANGE cr = { 0, -1 };
                    ::SendMessage(hWndPageItems[REDT_CHAT], EM_EXSETSEL, 0, (LPARAM)&cr);
                    return 0;
                }
                case IDC_CLEAR_CHAT:
                    ::SetWindowText(hWndPageItems[REDT_CHAT], "");
                    return 0;
                case IDC_REG_USER:
                    return 0;
                case IDC_DISCONNECT_USER:
                    DisconnectUser();
                    return 0;
                case IDC_KICK_USER:
                    KickUser();
                    return 0;
                case IDC_BAN_USER:
                    BanUser();
                    return 0;
                case IDC_REDIRECT_USER:
                    RedirectUser();
                    return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

static LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_KEYDOWN) {
        if(wParam == VK_RETURN && (::GetKeyState(VK_CONTROL) & 0x8000) == 0) {
            MainWindowPageUsersChat * pParent = (MainWindowPageUsersChat *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if(pParent != NULL) {
                if(pParent->OnEditEnter() == true) {
                    return 0;
                }
            }
        } else if(wParam == '|') {
            return 0;
        }
    } else if(uMsg == WM_CHAR || uMsg == WM_KEYUP) {
        if(wParam == VK_RETURN && (::GetKeyState(VK_CONTROL) & 0x8000) == 0) {
            return 0;
        } else if(wParam == '|') {
            return 0;
        }
    }

    return ::CallWindowProc(wpOldEditProc, hWnd, uMsg, wParam, lParam);
}

//------------------------------------------------------------------------------

bool MainWindowPageUsersChat::CreateMainWindowPage(HWND hOwner) {
    CreateHWND(hOwner);

    RECT rcMain;
    ::GetClientRect(m_hWnd, &rcMain);

    hWndPageItems[BTN_SHOW_CHAT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_CHAT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        2, 0, ((rcMain.right-150)/2)-3, 16, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_SHOW_COMMANDS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_CMDS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        ((rcMain.right-150)/2)+1, 0, ((rcMain.right-150)/2)-3, 16, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[BTN_AUTO_UPDATE_USERLIST] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_AUTO], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        rcMain.right-148, 0, (rcMain.right-(rcMain.right-150))-4, 16, m_hWnd, (HMENU)BTN_AUTO_UPDATE_USERLIST, g_hInstance, NULL);

    hWndPageItems[REDT_CHAT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
        2, 16, rcMain.right-152, rcMain.bottom-43, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[REDT_CHAT], EM_AUTOURLDETECT, TRUE, 0);
    ::SendMessage(hWndPageItems[REDT_CHAT], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(hWndPageItems[REDT_CHAT], EM_GETEVENTMASK, 0, 0) | ENM_LINK);

    hWndPageItems[LV_USERS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL |
        LVS_SORTASCENDING, rcMain.right-148, 16, (rcMain.right-(rcMain.right-148))-2, rcMain.bottom-43, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[LV_USERS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);

    hWndPageItems[EDT_CHAT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL | ES_MULTILINE,
        2, rcMain.bottom-25, rcMain.right-152, 23, m_hWnd, (HMENU)EDT_CHAT, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[EDT_CHAT], EM_SETLIMITTEXT, 8192, 0);

    hWndPageItems[BTN_UPDATE_USERS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REFRESH_USERLIST], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right-149, rcMain.bottom-25, (rcMain.right-(rcMain.right-148)), 23, m_hWnd, (HMENU)BTN_UPDATE_USERS, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

	RECT rcUsers;
	::GetClientRect(hWndPageItems[LV_USERS], &rcUsers);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = rcUsers.right - rcUsers.left;

    ::SendMessage(hWndPageItems[LV_USERS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    ::SetWindowLongPtr(hWndPageItems[EDT_CHAT], GWLP_USERDATA, (LONG_PTR)this);
    wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[EDT_CHAT], GWLP_WNDPROC, (LONG_PTR)EditProc);

	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::UpdateLanguage() {
    ::SetWindowText(hWndPageItems[BTN_SHOW_CHAT], LanguageManager->sTexts[LAN_CHAT]);
    ::SetWindowText(hWndPageItems[BTN_SHOW_COMMANDS], LanguageManager->sTexts[LAN_CMDS]);
    ::SetWindowText(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], LanguageManager->sTexts[LAN_AUTO]);
    ::SetWindowText(hWndPageItems[BTN_UPDATE_USERS], LanguageManager->sTexts[LAN_REFRESH_USERLIST]);
}
//---------------------------------------------------------------------------

char * MainWindowPageUsersChat::GetPageName() {
    return LanguageManager->sTexts[LAN_USERS_CHAT];
}
//------------------------------------------------------------------------------

bool MainWindowPageUsersChat::OnEditEnter() {
    if(bServerRunning == false) {
        return false;
    }

    int iAllocLen = ::GetWindowTextLength(hWndPageItems[EDT_CHAT]);

    if(iAllocLen == 0) {
        return false;
    }

    char * buf = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iAllocLen+4+SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]);

    if(buf == NULL) {
        AppendSpecialLog("Cannot create buf in MainWindowPageUsersChat::OnEditEnter!");
        return false;
    }

    buf[0] = '<';
    memcpy(buf+1, SettingManager->sTexts[SETTXT_ADMIN_NICK], SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]);
    buf[SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]+1] = '>';
    buf[SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]+2] = ' ';
    ::GetWindowText(hWndPageItems[EDT_CHAT], buf+SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]+3, iAllocLen+1);
    buf[iAllocLen+3+SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]] = '|';

    globalQ->Store(buf, iAllocLen+4+SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]);

    buf[iAllocLen+3+SettingManager->ui16TextsLens[SETTXT_ADMIN_NICK]] = '\0';

    AppendText(buf);

    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)buf) == 0) {
    	string sDbgstr = "[BUF] Cannot deallocate buf in MainWindowPageUsersChat::OnEditEnter! "+string((uint32_t)GetLastError())+" "+
    		string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
        AppendSpecialLog(sDbgstr);
    }

    ::SetWindowText(hWndPageItems[EDT_CHAT], "");

    return true;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::AppendText(const char * sText) {
    time_t acc_time = time(NULL);
    struct tm *tm = localtime(&acc_time);

    char msg[10];
    strftime(msg, 10, "\n[%H:%M] ", tm);

    CHARRANGE cr = { 0, 0 };
    ::SendMessage(hWndPageItems[REDT_CHAT], EM_EXGETSEL, 0, (LPARAM)&cr);
    LONG lOldSelBegin = cr.cpMin;
    LONG lOldSelEnd = cr.cpMax;

    GETTEXTLENGTHEX gtle = { 0 };
    gtle.codepage = CP_ACP;
    gtle.flags = GTL_NUMCHARS;
    cr.cpMin = cr.cpMax = (LONG)::SendMessage(hWndPageItems[REDT_CHAT], EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
    ::SendMessage(hWndPageItems[REDT_CHAT], EM_EXSETSEL, 0, (LPARAM)&cr);

    ::SendMessage(hWndPageItems[REDT_CHAT], EM_REPLACESEL, FALSE, ::GetWindowTextLength(hWndPageItems[REDT_CHAT]) == 0 ? (LPARAM)msg+1 : (LPARAM)msg);

    ::SendMessage(hWndPageItems[REDT_CHAT], EM_REPLACESEL, FALSE, (LPARAM)sText);

    cr.cpMin = lOldSelBegin;
    cr.cpMax = lOldSelEnd;
    ::SendMessage(hWndPageItems[REDT_CHAT], EM_EXSETSEL, 0, (LPARAM)&cr);

    ::SendMessage(hWndPageItems[REDT_CHAT], WM_VSCROLL, SB_BOTTOM, NULL);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::AddUser(const User * curUser) {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = 0;
    lvItem.lParam = (LPARAM)curUser;
    lvItem.pszText = curUser->Nick;

    ::SendMessage(hWndPageItems[LV_USERS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::RemoveUser(const User * curUser) {
    LVFINDINFO lvFindItem = { 0 };
    lvFindItem.flags = LVFI_PARAM;
    lvFindItem.lParam = (LPARAM)curUser;

    int iItem = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_FINDITEM, (WPARAM)-1, (LPARAM)&lvFindItem);

    if(iItem != -1) {
        ::SendMessage(hWndPageItems[LV_USERS], LVM_DELETEITEM, iItem, 0);
    }
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::UpdateUserList() {
    ::SendMessage(hWndPageItems[LV_USERS], LVM_DELETEALLITEMS, 0, 0);

    uint32_t ui32InClose = 0, ui32InLogin = 0, ui32LoggedIn = 0, ui32Total = 0;

	User *next = colUsers->llist;
	while(next != NULL) {
        ui32Total++;

        User *u = next;
        next = u->next;

        switch(u->iState) {
            case User::STATE_ADDED:
                ui32LoggedIn++;

                AddUser(u);

                break;
            case User::STATE_CLOSING:
            case User::STATE_REMME:
                ui32InClose++;
                break;
            default:
                ui32InLogin++;
                break;
        }
    }

    AppendText((string(LanguageManager->sTexts[LAN_TOTAL], (size_t)LanguageManager->ui16TextsLens[LAN_TOTAL]) + ": " + string(ui32Total) + ", " +
        string(LanguageManager->sTexts[LAN_LOGGED], (size_t)LanguageManager->ui16TextsLens[LAN_LOGGED]) + ": " + string(ui32LoggedIn) + ", " +
		string(LanguageManager->sTexts[LAN_CLOSING], (size_t)LanguageManager->ui16TextsLens[LAN_CLOSING]) + ": " + string(ui32InClose) + ", " +
        string(LanguageManager->sTexts[LAN_LOGGING], (size_t)LanguageManager->ui16TextsLens[LAN_LOGGING]) + ": " + string(ui32InLogin)).c_str());
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow == hWndPageItems[REDT_CHAT]) {
        HMENU hMenu = ::CreatePopupMenu();

        ::AppendMenu(hMenu, MF_STRING, IDC_COPY, LanguageManager->sTexts[LAN_MENU_COPY]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_SELECT_ALL, LanguageManager->sTexts[LAN_MENU_SELECT_ALL]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_CLEAR_CHAT, LanguageManager->sTexts[LAN_MENU_CLEAR_CHAT]);

        int iX = GET_X_LPARAM(lParam);
        int iY = GET_Y_LPARAM(lParam);

        // -1, -1 is menu created by key. it need few trick to show menu on correct position ;o)
        if(iX == -1 && iY == -1) {
            CHARRANGE cr = { 0, 0 };
            ::SendMessage(hWndPageItems[REDT_CHAT], EM_EXGETSEL, 0, (LPARAM)&cr);

            POINT pt = { 0 };
            ::SendMessage(hWndPageItems[REDT_CHAT], EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)cr.cpMax);

            RECT rcChat;
            ::GetClientRect(hWndPageItems[REDT_CHAT], &rcChat);

            if(pt.y < rcChat.top) {
                pt.y = rcChat.top;
            } else if(pt.y > rcChat.bottom) {
                pt.y = rcChat.bottom;
            }

            ::ClientToScreen(hWndPageItems[REDT_CHAT], &pt);
            iX = pt.x;
            iY = pt.y;
        }

        ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, NULL);

        ::DestroyMenu(hMenu);
    } else if(hWindow == hWndPageItems[LV_USERS]) {
        int iSel = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

        if(iSel == -1) {
            return;
        }

        HMENU hMenu = ::CreatePopupMenu();

        ::AppendMenu(hMenu, MF_STRING, IDC_REG_USER, LanguageManager->sTexts[LAN_MENU_REG_USER]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_DISCONNECT_USER, LanguageManager->sTexts[LAN_MENU_DISCONNECT_USER]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_KICK_USER, LanguageManager->sTexts[LAN_MENU_KICK_USER]);
        ::AppendMenu(hMenu, MF_STRING, IDC_BAN_USER, LanguageManager->sTexts[LAN_MENU_BAN_USER]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_REDIRECT_USER, LanguageManager->sTexts[LAN_MENU_REDIRECT_USER]);

        int iX = GET_X_LPARAM(lParam);
        int iY = GET_Y_LPARAM(lParam);

        // -1, -1 is menu created by key. We need few tricks to show menu on correct position ;o)
        if(iX == -1 && iY == -1) {
            POINT pt = { 0 };
            if((BOOL)::SendMessage(hWndPageItems[LV_USERS], LVM_ISITEMVISIBLE, (WPARAM)iSel, 0) == FALSE) {
                RECT rcList;
                ::GetClientRect(hWndPageItems[LV_USERS], &rcList);

                ::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEMPOSITION, (WPARAM)iSel, (LPARAM)&pt);

                pt.y = (pt.y < rcList.top) ? rcList.top : rcList.bottom;
            } else {
                RECT rcItem;
                rcItem.left = LVIR_LABEL;
                ::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEMRECT, (WPARAM)iSel, (LPARAM)&rcItem);

                pt.x = rcItem.left;
                pt.y = rcItem.top + ((rcItem.bottom - rcItem.top) / 2);
            }

            ::ClientToScreen(hWndPageItems[LV_USERS], &pt);

            iX = pt.x;
            iY = pt.y;
        }

        ::EnableMenuItem(hMenu, IDC_REG_USER, MF_BYCOMMAND | MF_GRAYED);

        ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, NULL);

        ::DestroyMenu(hMenu);
    }
}
//------------------------------------------------------------------------------

User * MainWindowPageUsersChat::GetUser() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_USERS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return NULL;
    }

    char msg[1024];

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = iSel;
    lvItem.pszText = msg;
    lvItem.cchTextMax = 1024;

    if((BOOL)::SendMessage(hWndPageItems[LV_USERS], LVM_GETITEM, 0, (LPARAM)&lvItem) == FALSE) {
        return NULL;
    }

    User * curUser = (User *)lvItem.lParam;

    if(::SendMessage(hWndPageItems[BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_UNCHECKED) {
        User * testUser = hashManager->FindUser(lvItem.pszText, strlen(lvItem.pszText));

        if(testUser == NULL || testUser != curUser) {
            char buf[1024];
            int imsgLen = sprintf(buf, "<%s> *** %s %s.", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], lvItem.pszText, LanguageManager->sTexts[LAN_IS_NOT_ONLINE]);
            if(CheckSprintf(imsgLen, 1024, "MainWindowPageUsersChat::DisconnectUser") == true) {
                AppendText(buf);
            }

            return NULL;
        }
    }

    return curUser;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::DisconnectUser() {
    User * curUser = GetUser();

    if(curUser == NULL) {
        return;
    }

    char msg[1024];

    // disconnect the user
    int imsgLen = sprintf(msg, "[SYS] User %s (%s) closed by %s", curUser->Nick, curUser->IP, SettingManager->sTexts[SETTXT_ADMIN_NICK]);
    if(CheckSprintf(imsgLen, 1024, "MainWindowPageUsersChat::DisconnectUser1") == true) {
        UdpDebug->Broadcast(msg, imsgLen);
    }

    UserClose(curUser);

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            imsgLen = sprintf(msg, "%s $<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP], curUser->IP, LanguageManager->sTexts[LAN_WAS_CLOSED_BY], SettingManager->sTexts[SETTXT_ADMIN_NICK]);
            if(CheckSprintf(imsgLen, 1024, "MainWindowPageUsersChat::DisconnectUser2") == true) {
				QueueDataItem *newItem = globalQ->CreateQueueDataItem(msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
                globalQ->SingleItemsStore(newItem);
            }
        } else {
            imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP], curUser->IP,
                LanguageManager->sTexts[LAN_WAS_CLOSED_BY], SettingManager->sTexts[SETTXT_ADMIN_NICK]);
            if(CheckSprintf(imsgLen, 1024, "MainWindowPageUsersChat::DisconnectUser3") == true) {
                globalQ->OPStore(msg, imsgLen);
        }
        }
    }

    imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP], curUser->IP,
        LanguageManager->sTexts[LAN_WAS_CLOSED]);
    if(CheckSprintf(imsgLen, 1024, "MainWindowPageUsersChat::DisconnectUser4") == true) {
        AppendText(msg);
    }
}
//------------------------------------------------------------------------------

void OnKickOk(char * sLine, const int &iLen) {
    User * curUser = pMainWindowPageUsersChat->GetUser();

    if(curUser == NULL) {
        return;
    }

    hashBanManager->TempBan(curUser, iLen == 0 ? NULL : sLine, SettingManager->sTexts[SETTXT_ADMIN_NICK], 0, 0, false);

    char msg[1024];

    if(iLen == 0) {
        int imsgLen = sprintf(msg, "<%s> %s...|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_BEING_KICKED]);
        if(CheckSprintf(imsgLen, 1024, "OnKickOk") == true) {
            UserSendChar(curUser, msg, imsgLen);
        }
    } else {
        if(iLen > 256) {
            sLine[255] = '\0';
            sLine[254] = '.';
            sLine[253] = '.';
            sLine[252] = '.';
        }
        int imsgLen = sprintf(msg, "<%s> %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_BEING_KICKED_BCS], sLine);
        if(CheckSprintf(imsgLen, 1024, "OnKickOk1") == true) {
            UserSendChar(curUser, msg, imsgLen);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        int imsgLen = 0;
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
            CheckSprintf(imsgLen, 1024, "OnKickOk2");
        }

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP], curUser->IP,
            LanguageManager->sTexts[LAN_WAS_KICKED_BY], SettingManager->sTexts[SETTXT_ADMIN_NICK]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "OnKickOk3") == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
    			QueueDataItem *newItem = globalQ->CreateQueueDataItem(msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
                globalQ->SingleItemsStore(newItem);
            } else {
                globalQ->OPStore(msg, imsgLen);
            }
        }
    }

    int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP], curUser->IP,
        LanguageManager->sTexts[LAN_WAS_KICKED]);
    if(CheckSprintf(imsgLen, 1024, "OnKickOk4") == true) {
        pMainWindowPageUsersChat->AppendText(msg);
    }

    // disconnect the user
    imsgLen = sprintf(msg, "[SYS] User %s (%s) kicked by %s", curUser->Nick, curUser->IP, SettingManager->sTexts[SETTXT_ADMIN_NICK]);
    if(CheckSprintf(imsgLen, 1024, "OnKickOk5") == true) {
        UdpDebug->Broadcast(msg, imsgLen);
    }

    UserClose(curUser);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::KickUser() {
    User * curUser = GetUser();

    if(curUser == NULL) {
        return;
    }

	LineDialog * KickDlg = new LineDialog(&OnKickOk);
	KickDlg->DoModal(::GetParent(m_hWnd), LanguageManager->sTexts[LAN_PLEASE_ENTER_REASON], "");
}
//------------------------------------------------------------------------------

void OnBanOk(char * sLine, const int &iLen) {
    User * curUser = pMainWindowPageUsersChat->GetUser();

    if(curUser == NULL) {
        return;
    }

    hashBanManager->Ban(curUser, iLen == 0 ? NULL : sLine, SettingManager->sTexts[SETTXT_ADMIN_NICK], false);

    char msg[1024];

    if(iLen == 0) {
        int imsgLen = sprintf(msg, "<%s> %s...|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_BEING_KICKED]);
        if(CheckSprintf(imsgLen, 1024, "OnBanOk") == true) {
            UserSendChar(curUser, msg, imsgLen);
        }
    } else {
        if(iLen > 256) {
            sLine[255] = '\0';
            sLine[254] = '.';
            sLine[253] = '.';
            sLine[252] = '.';
        }
        int imsgLen = sprintf(msg, "<%s> %s: %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_BEING_BANNED_BECAUSE], sLine);
        if(CheckSprintf(imsgLen, 1024, "OnBanOk1") == true) {
            UserSendChar(curUser, msg, imsgLen);
        }
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        int imsgLen = 0;
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
            CheckSprintf(imsgLen, 1024, "OnBanOk2");
        }

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP],
            curUser->IP, LanguageManager->sTexts[LAN_HAS_BEEN], LanguageManager->sTexts[LAN_BANNED_LWR], LanguageManager->sTexts[LAN_BY_LWR], SettingManager->sTexts[SETTXT_ADMIN_NICK]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 1024, "OnBanOk3") == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
    			QueueDataItem *newItem = globalQ->CreateQueueDataItem(msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
                globalQ->SingleItemsStore(newItem);
            } else {
                globalQ->OPStore(msg, imsgLen);
            }
        }
    }

    int imsgLen = sprintf(msg, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WITH_IP], curUser->IP,
        LanguageManager->sTexts[LAN_HAS_BEEN], LanguageManager->sTexts[LAN_BANNED_LWR]);
    if(CheckSprintf(imsgLen, 1024, "OnBanOk4") == true) {
        pMainWindowPageUsersChat->AppendText(msg);
    }

    // disconnect the user
    imsgLen = sprintf(msg, "[SYS] User %s (%s) kicked by %s", curUser->Nick, curUser->IP, SettingManager->sTexts[SETTXT_ADMIN_NICK]);
    if(CheckSprintf(imsgLen, 1024, "OnBanOk5") == true) {
        UdpDebug->Broadcast(msg, imsgLen);
    }

    UserClose(curUser);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::BanUser() {
    User * curUser = GetUser();

    if(curUser == NULL) {
        return;
    }

	LineDialog * BanDlg = new LineDialog(&OnBanOk);
	BanDlg->DoModal(::GetParent(m_hWnd), LanguageManager->sTexts[LAN_PLEASE_ENTER_REASON], "");
}
//------------------------------------------------------------------------------

void OnRedirectOk(char * sLine, const int &iLen) {
    User * curUser = pMainWindowPageUsersChat->GetUser();

    if(curUser == NULL || iLen == 0 || iLen > 512) {
        return;
    }

    char msg[2048];

    int imsgLen = sprintf(msg, "<%s> %s %s %s %s.|$ForceMove %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_ARE_REDIRECTED_TO],
        sLine, LanguageManager->sTexts[LAN_BY_LWR], SettingManager->sTexts[SETTXT_ADMIN_NICK], sLine);
    if(CheckSprintf(imsgLen, 2048, "OnRedirectOk") == true) {
        UserSendCharDelayed(curUser, msg, imsgLen);
    }

    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        int imsgLen = 0;
        if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
            imsgLen = sprintf(msg, "%s $", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC]);
            CheckSprintf(imsgLen, 2048, "OnRedirectOk1");
        }

        int iret = sprintf(msg+imsgLen, "<%s> *** %s %s %s %s %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick,
            LanguageManager->sTexts[LAN_WAS_REDIRECTED_TO], sLine, LanguageManager->sTexts[LAN_BY_LWR], SettingManager->sTexts[SETTXT_ADMIN_NICK]);
        imsgLen += iret;
        if(CheckSprintf1(iret, imsgLen, 2048, "OnRedirectOk2") == true) {
            if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
    			QueueDataItem *newItem = globalQ->CreateQueueDataItem(msg, imsgLen, NULL, 0, globalqueue::PM2OPS);
                globalQ->SingleItemsStore(newItem);
            } else {
                globalQ->OPStore(msg, imsgLen);
            }
        }
    }

    imsgLen = sprintf(msg, "<%s> *** %s %s %s|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], curUser->Nick, LanguageManager->sTexts[LAN_WAS_REDIRECTED_TO], sLine);
    if(CheckSprintf(imsgLen, 2048, "OnRedirectOk3") == true) {
        pMainWindowPageUsersChat->AppendText(msg);
    }

    // disconnect the user
    imsgLen = sprintf(msg, "[SYS] User %s (%s) redirected by %s", curUser->Nick, curUser->IP, SettingManager->sTexts[SETTXT_ADMIN_NICK]);
    if(CheckSprintf(imsgLen, 2048, "OnRedirectOk4") == true) {
        UdpDebug->Broadcast(msg, imsgLen);
    }

    UserClose(curUser);
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::RedirectUser() {
    User * curUser = GetUser();

    if(curUser == NULL) {
        return;
    }

	LineDialog * RedirectDlg = new LineDialog(&OnRedirectOk);
	RedirectDlg->DoModal(::GetParent(m_hWnd), LanguageManager->sTexts[LAN_PLEASE_ENTER_REDIRECT_ADDRESS],
        SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS] == NULL ? "" : SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS]);
}
//------------------------------------------------------------------------------

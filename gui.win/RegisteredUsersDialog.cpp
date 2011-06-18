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
#include "RegisteredUsersDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/ProfileManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "RegisteredUserDialog.h"
//---------------------------------------------------------------------------
RegisteredUsersDialog * pRegisteredUsersDialog = NULL;
//---------------------------------------------------------------------------
#define IDC_CHANGE_REG      1000
#define IDC_REMOVE_REGS     1001
//---------------------------------------------------------------------------
static ATOM atomRegisteredUsersDialog = 0;
//---------------------------------------------------------------------------
static WNDPROC wpOldEditProc = NULL;
//---------------------------------------------------------------------------

RegisteredUsersDialog::RegisteredUsersDialog() {
    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    iFilterColumn = iSortColumn = 0;

    bSortAscending = true;
}
//---------------------------------------------------------------------------

RegisteredUsersDialog::~RegisteredUsersDialog() {
    pRegisteredUsersDialog = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RegisteredUsersDialog::StaticRegisteredUsersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RegisteredUsersDialog * pRegisteredUsersDialog = (RegisteredUsersDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pRegisteredUsersDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pRegisteredUsersDialog->RegisteredUsersDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RegisteredUsersDialog::RegisteredUsersDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_WINDOWPOSCHANGED: {
            RECT rcParent;
            ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

            ::SetWindowPos(hWndWindowItems[CB_FILTER], NULL, (rcParent.right / 2)+3, (rcParent.bottom - iOneLineGB - 3) + iGroupBoxMargin,
                rcParent.right - (rcParent.right/2) - 14, iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[EDT_FILTER], NULL, 11, (rcParent.bottom - iOneLineGB - 3) + iGroupBoxMargin, (rcParent.right / 2) - 14, iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[GB_FILTER], NULL, 3, rcParent.bottom - iOneLineGB - 3, rcParent.right - 6, iOneLineGB, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[LV_REGS], NULL, 0, 0, rcParent.right - 6, rcParent.bottom - iOneLineGB - iEditHeight - 11, SWP_NOMOVE | SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_ADD_REG], NULL, 0, 0, rcParent.right - 4, iEditHeight, SWP_NOMOVE | SWP_NOZORDER);

            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case (BTN_ADD_REG+100): {
                    pRegisteredUserDialog = new RegisteredUserDialog();
                    pRegisteredUserDialog->DoModal(hWndWindowItems[WINDOW_HANDLE]);

                    return 0;
                }
                case IDC_CHANGE_REG: {
                    ChangeReg();
                    return 0;
                }
                case IDC_REMOVE_REGS:
                    RemoveRegs();
                    return 0;
                case CB_FILTER:
                    if(HIWORD(wParam) == CBN_SELCHANGE) {
                        if(::GetWindowTextLength(hWndWindowItems[EDT_FILTER]) != 0) {
                            FilterRegs();
                        }
                    }

                    break;
                case IDOK: { // NM_RETURN
                    HWND hWndFocus = ::GetFocus();

                    if(hWndFocus == hWndWindowItems[LV_REGS]) {
                        ChangeReg();
                        return 0;
                    } else if(hWndFocus == hWndWindowItems[EDT_FILTER]) {
                        FilterRegs();
                        return 0;
                    }

                    break;
                }
                case IDCANCEL:
                    ::PostMessage(hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
					return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[LV_REGS]) {
                if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) {
                    OnColumnClick((LPNMLISTVIEW)lParam);
                } else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
                    if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
                        break;
                    }

                    RegUser * pReg = (RegUser *)ListViewGetItem(hWndWindowItems[LV_REGS],  ((LPNMITEMACTIVATE)lParam)->iItem);

                    pRegisteredUserDialog = new RegisteredUserDialog();
                    pRegisteredUserDialog->DoModal(hWndWindowItems[WINDOW_HANDLE], pReg);

                    return 0;
                }
            }

            break;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = ScaleGui(443);
            mminfo->ptMinTrackSize.y = ScaleGui(454);

            return 0;
        }
        case WM_CLOSE:
            ::EnableWindow(::GetParent(hWndWindowItems[WINDOW_HANDLE]), TRUE);
            g_hWndActiveDialog = NULL;
            break;
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
        case WM_SETFOCUS:
            if((UINT)::SendMessage(hWndWindowItems[LV_REGS], LVM_GETSELECTEDCOUNT, 0, 0) != 0) {
                ::SetFocus(hWndWindowItems[LV_REGS]);
            } else {
                ::SetFocus(hWndWindowItems[EDT_FILTER]);
            }

            return 0;
        case WM_ACTIVATE:
            if(LOWORD(wParam) != WA_INACTIVE) {
                g_hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];
            }

            break;
    }

	return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::DoModal(HWND hWndParent) {
    if(atomRegisteredUsersDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_RegisteredUsersDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomRegisteredUsersDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(443) / 2);
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(454) / 2);

    hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRegisteredUsersDialog), LanguageManager->sTexts[LAN_REG_USERS],
        WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(443), ScaleGui(454),
        hWndParent, NULL, g_hInstance, NULL);

    if(hWndWindowItems[WINDOW_HANDLE] == NULL) {
        return;
    }

    g_hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];

    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRegisteredUsersDialogProc);

    ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

    hWndWindowItems[BTN_ADD_REG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ADD_NEW_REG], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, 2, (rcParent.right / 3) - 2, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_REG+100), g_hInstance, NULL);

    hWndWindowItems[LV_REGS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        3, iEditHeight + 6, rcParent.right - 6, rcParent.bottom - iOneLineGB - iEditHeight - 11, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[LV_REGS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

    hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_FILTER_REGISTERED_USERS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, rcParent.bottom - iOneLineGB - 3, rcParent.right - 6, iOneLineGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, (rcParent.bottom - iOneLineGB - 3) + iGroupBoxMargin, (rcParent.right / 2)-14, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        (rcParent.right / 2) + 3, (rcParent.bottom - iOneLineGB - 3) + iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, iEditHeight,
        hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }

    ::SendMessage(hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_NICK]);
    ::SendMessage(hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PASSWORD]);
    ::SendMessage(hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[LAN_PROFILE]);

    ::SendMessage(hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);

	RECT rcRegs;
	::GetClientRect(hWndWindowItems[LV_REGS], &rcRegs);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = ((rcRegs.right - rcRegs.left)-20)/3;
    lvColumn.pszText = LanguageManager->sTexts[LAN_NICK];
    lvColumn.iSubItem = 0;

    ::SendMessage(hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    lvColumn.fmt = LVCFMT_RIGHT;
    lvColumn.pszText = LanguageManager->sTexts[LAN_PASSWORD];
    lvColumn.iSubItem = 1;
    ::SendMessage(hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);

    lvColumn.pszText = LanguageManager->sTexts[LAN_PROFILE];
    lvColumn.iSubItem = 2;
    ::SendMessage(hWndWindowItems[LV_REGS], LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);

    ListViewUpdateArrow(hWndWindowItems[LV_REGS], bSortAscending, iSortColumn);

    AddAllRegs();

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::AddAllRegs() {
    ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ::SendMessage(hWndWindowItems[LV_REGS], LVM_DELETEALLITEMS, 0, 0);

    RegUser * curReg,
        *nextReg = hashRegManager->RegListS;

    while(nextReg != NULL) {
        curReg = nextReg;
        nextReg = curReg->next;

        AddReg(curReg);
    }

    ListViewSelectFirstItem(hWndWindowItems[LV_REGS]);

    ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::AddReg(const RegUser * pReg) {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = ListViewGetInsertPosition(hWndWindowItems[LV_REGS], pReg, bSortAscending, CompareRegs);
    lvItem.pszText = pReg->sNick;
    lvItem.lParam = (LPARAM)pReg;

    int i = (int)::SendMessage(hWndWindowItems[LV_REGS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);

    if(i != -1) {
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 1;
        lvItem.pszText = pReg->sPass;

        ::SendMessage(hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);

        lvItem.iSubItem = 2;
        lvItem.pszText = ProfileMan->ProfilesTable[pReg->iProfile]->sName;

        ::SendMessage(hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
    }
}
//------------------------------------------------------------------------------

int RegisteredUsersDialog::CompareRegs(const void * pItem, const void * pOtherItem) {
    RegUser * pFirstReg = (RegUser *)pItem;
    RegUser * pSecondReg = (RegUser *)pOtherItem;

    switch(pRegisteredUsersDialog->iSortColumn) {
        case 0:
            return _stricmp(pFirstReg->sNick, pSecondReg->sNick);
        case 1:
            return _stricmp(pFirstReg->sPass, pSecondReg->sPass);
        case 2:
            return (pFirstReg->iProfile > pSecondReg->iProfile) ? 1 : ((pFirstReg->iProfile == pSecondReg->iProfile) ? 0 : -1);
        default:
            return 0; // never happen, but we need to make compiler/complainer happy ;o)
    }
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::OnColumnClick(const LPNMLISTVIEW &pListView) {
    if(pListView->iSubItem != iSortColumn) {
        bSortAscending = true;
        iSortColumn = pListView->iSubItem;
    } else {
        bSortAscending = !bSortAscending;
    }

    ListViewUpdateArrow(hWndWindowItems[LV_REGS], bSortAscending, iSortColumn);

    ::SendMessage(hWndWindowItems[LV_REGS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRegs);
}
//------------------------------------------------------------------------------

int CALLBACK RegisteredUsersDialog::SortCompareRegs(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
    int iResult = pRegisteredUsersDialog->CompareRegs((void *)lParam1, (void *)lParam2);

    return (pRegisteredUsersDialog->bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::RemoveRegs() {
    if(::MessageBox(hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), sTitle.c_str(),
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
        return;
    }

    ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    int iSel = -1;
    while((iSel = (int)::SendMessage(hWndWindowItems[LV_REGS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED)) != -1) {
        RegUser * pReg = (RegUser *)ListViewGetItem(hWndWindowItems[LV_REGS], iSel);

        hashRegManager->Delete(pReg, true);

        ::SendMessage(hWndWindowItems[LV_REGS], LVM_DELETEITEM, iSel, 0);
    }

    ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::FilterRegs() {
    int iTextLength = ::GetWindowTextLength(hWndWindowItems[EDT_FILTER]);

    if(iTextLength == 0) {
        sFilterString.clear();

    	AddAllRegs();
    } else {
        iFilterColumn = (int)::SendMessage(hWndWindowItems[CB_FILTER], CB_GETCURSEL, 0, 0);

        char buf[65];

        int iLen = ::GetWindowText(hWndWindowItems[EDT_FILTER], buf, 65);

        for(int i = 0; i < iLen; i++) {
            buf[i] = (char)tolower(buf[i]);
        }

        sFilterString = buf;

        ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

        ::SendMessage(hWndWindowItems[LV_REGS], LVM_DELETEALLITEMS, 0, 0);

        RegUser * curReg,
            *nextReg = hashRegManager->RegListS;

        while(nextReg != NULL) {
            curReg = nextReg;
            nextReg = curReg->next;

            switch(iFilterColumn) {
                case 0:
                    if(stristr2(curReg->sNick, sFilterString.c_str()) == NULL) {
                        continue;
                    }
                    break;
                case 1:
                    if(stristr2(curReg->sPass, sFilterString.c_str()) == NULL) {
                        continue;
                    }
                    break;
                case 2:
                    if(stristr2(ProfileMan->ProfilesTable[curReg->iProfile]->sName, sFilterString.c_str()) == NULL) {
                        continue;
                    }
                    break;
            }

            AddReg(curReg);
        }

        ListViewSelectFirstItem(hWndWindowItems[LV_REGS]);

        ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);
    }
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::RemoveReg(const RegUser * pReg) {
    int iPos = ListViewGetItemPosition(hWndWindowItems[LV_REGS], (void *)pReg);

    if(iPos != -1) {
        ::SendMessage(hWndWindowItems[LV_REGS], LVM_DELETEITEM, iPos, 0);
    }
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::UpdateProfiles() {
    int iItemCount = (int)::SendMessage(hWndWindowItems[LV_REGS], LVM_GETITEMCOUNT, 0, 0);

    if(iItemCount == 0) {
        return;
    }

    ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    RegUser * pReg = NULL;

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;
    lvItem.iSubItem = 2;

    for(int i = 0; i < iItemCount; i++) {
        pReg = (RegUser *)ListViewGetItem(hWndWindowItems[LV_REGS], i);

        lvItem.iItem = i;
        lvItem.pszText = ProfileMan->ProfilesTable[pReg->iProfile]->sName;

        ::SendMessage(hWndWindowItems[LV_REGS], LVM_SETITEM, 0, (LPARAM)&lvItem);
    }

    ::SendMessage(hWndWindowItems[LV_REGS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    if(iSortColumn == 2) {
        ::SendMessage(hWndWindowItems[LV_REGS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRegs);
    }
}
//------------------------------------------------------------------------------

void RegisteredUsersDialog::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow != hWndWindowItems[LV_REGS]) {
        return;
    }

    UINT UISelectedCount = (UINT)::SendMessage(hWndWindowItems[LV_REGS], LVM_GETSELECTEDCOUNT, 0, 0);

    if(UISelectedCount == 0) {
        return;
    }

    HMENU hMenu = ::CreatePopupMenu();

    if(UISelectedCount == 1) {
        ::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_REG, LanguageManager->sTexts[LAN_CHANGE]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }

    ::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_REGS, LanguageManager->sTexts[LAN_REMOVE]);

    int iX = GET_X_LPARAM(lParam);
    int iY = GET_Y_LPARAM(lParam);

    ListViewGetMenuPos(hWndWindowItems[LV_REGS], iX, iY);

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, hWndWindowItems[WINDOW_HANDLE], NULL);

    ::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RegisteredUsersDialog::ChangeReg() {
    int iSel = (int)::SendMessage(hWndWindowItems[LV_REGS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    RegUser * pReg = (RegUser *)ListViewGetItem(hWndWindowItems[LV_REGS], iSel);

    pRegisteredUserDialog = new RegisteredUserDialog();
    pRegisteredUserDialog->DoModal(hWndWindowItems[WINDOW_HANDLE], pReg);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

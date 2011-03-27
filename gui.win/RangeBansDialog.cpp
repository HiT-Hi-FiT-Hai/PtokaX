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
#include "RangeBansDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashBanManager.h"
#include "../core/LanguageManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "GuiUtil.h"
#include "RangeBanDialog.h"
//---------------------------------------------------------------------------
RangeBansDialog * pRangeBansDialog = NULL;
//---------------------------------------------------------------------------
#define IDC_CHANGE_RANGE_BAN      1000
#define IDC_REMOVE_RANGE_BANS     1001
//---------------------------------------------------------------------------
static ATOM atomRangeBansDialog = 0;
//---------------------------------------------------------------------------
static WNDPROC wpOldEditProc = NULL;
//---------------------------------------------------------------------------

RangeBansDialog::RangeBansDialog() {
    pRangeBansDialog = this;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    iFilterColumn = iSortColumn = 0;

    bSortAscending = true;
}
//---------------------------------------------------------------------------

RangeBansDialog::~RangeBansDialog() {
    pRangeBansDialog = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RangeBansDialog::StaticRangeBansDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RangeBansDialog * pRangeBansDialog = (RangeBansDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pRangeBansDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pRangeBansDialog->RangeBansDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RangeBansDialog::RangeBansDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_WINDOWPOSCHANGED: {
            RECT rcParent;
            ::GetClientRect(hWndWindowItems[WND_THIS], &rcParent);

            ::SetWindowPos(hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS], NULL, ((rcParent.right-rcParent.left)/2)+1, rcParent.bottom-25,
                (rcParent.right-rcParent.left)-((rcParent.right-rcParent.left)/2)-3, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS], NULL, 2, rcParent.bottom-25, ((rcParent.right-rcParent.left)/2)-2, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[CB_FILTER], NULL, ((rcParent.right-rcParent.left)/2)+3, rcParent.bottom-56,
                (rcParent.right-rcParent.left)-((rcParent.right-rcParent.left)/2)-14, 21, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[EDT_FILTER], NULL, 11, rcParent.bottom-56, ((rcParent.right-rcParent.left)/2)-14, 21, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[GB_FILTER], NULL, 3, rcParent.bottom-71, (rcParent.right-rcParent.left)-6, 44, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[LV_RANGE_BANS], NULL, 0, 0, (rcParent.right-rcParent.left)-6, (rcParent.bottom-rcParent.top)-99, SWP_NOMOVE | SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_ADD_RANGE_BAN], NULL, 0, 0, (rcParent.right-rcParent.left)-4, 23, SWP_NOMOVE | SWP_NOZORDER);

            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_ADD_RANGE_BAN: {

                    RangeBanDialog * pRangeBanDialog = new RangeBanDialog();
                    pRangeBanDialog->DoModal(hWndWindowItems[WND_THIS]);

                    return 0;
                }
                case IDC_CHANGE_RANGE_BAN: {
                    int iSel = (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

                    if(iSel == -1) {
                        return 0;
                    }

                    RangeBanItem * pRangeBan = (RangeBanItem *)ListViewGetItem(hWndWindowItems[LV_RANGE_BANS], iSel);

                    RangeBanDialog * pRangeBanDialog = new RangeBanDialog();
                    pRangeBanDialog->DoModal(hWndWindowItems[WND_THIS], pRangeBan);

                    return 0;
                }
                case IDC_REMOVE_RANGE_BANS:
                    RemoveRangeBans();
                    return 0;
                case CB_FILTER:
                    if(HIWORD(wParam) == CBN_SELCHANGE) {
                        if(::GetWindowTextLength(hWndWindowItems[EDT_FILTER]) != 0) {
                            FilterRangeBans();
                        }
                    }

                    break;
                case BTN_CLEAR_RANGE_TEMP_BANS:
                    if(::MessageBox(hWndWindowItems[WND_THIS], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
                        sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
                        return 0;
                    }

                    hashBanManager->ClearTempRange();
                    AddAllRangeBans();

                    return 0;
                case BTN_CLEAR_RANGE_PERM_BANS:
                    if(::MessageBox(hWndWindowItems[WND_THIS], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
                        sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
                        return 0;
                    }

                    hashBanManager->ClearPermRange();
                    AddAllRangeBans();

                    return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[LV_RANGE_BANS]) {
                if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) {
                    OnColumnClick((LPNMLISTVIEW)lParam);
                } else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
                    if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
                        break;
                    }

                    RangeBanItem * pRangeBan = (RangeBanItem *)ListViewGetItem(hWndWindowItems[LV_RANGE_BANS], ((LPNMITEMACTIVATE)lParam)->iItem);

                    RangeBanDialog * pRangeBanDialog = new RangeBanDialog();
                    pRangeBanDialog->DoModal(hWndWindowItems[WND_THIS], pRangeBan);

                    return 0;
                }
            }

            break;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = 443;
            mminfo->ptMinTrackSize.y = 454;

            return 0;
        }
        case WM_CLOSE:
            ::EnableWindow(::GetParent(hWndWindowItems[WND_THIS]), TRUE);
            break;
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(hWndWindowItems[WND_THIS], uMsg, wParam, lParam);
    }

	return ::DefWindowProc(hWndWindowItems[WND_THIS], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

static LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        pRangeBansDialog->FilterRangeBans();
        return 0;
    }

    return ::CallWindowProc(wpOldEditProc, hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RangeBansDialog::DoModal(HWND hWndParent) {
    if(atomRangeBansDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_RangeBansDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomRangeBansDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2))-221;
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2))-227;

    hWndWindowItems[WND_THIS] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBansDialog), LanguageManager->sTexts[LAN_RANGE_BANS],
        WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, 443, 454,
        hWndParent, NULL, g_hInstance, NULL);

    if(hWndWindowItems[WND_THIS] == NULL) {
        return;
    }

    ::SetWindowLongPtr(hWndWindowItems[WND_THIS], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(hWndWindowItems[WND_THIS], GWLP_WNDPROC, (LONG_PTR)StaticRangeBansDialogProc);

    ::GetClientRect(hWndWindowItems[WND_THIS], &rcParent);

    hWndWindowItems[BTN_ADD_RANGE_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ADD_NEW_RANGE_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, rcParent.top+2, ((rcParent.right-rcParent.left)/3)-2, 23, hWndWindowItems[WND_THIS], (HMENU)BTN_ADD_RANGE_BAN, g_hInstance, NULL);

    hWndWindowItems[LV_RANGE_BANS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        3, 27, (rcParent.right-rcParent.left)-6, (rcParent.bottom-rcParent.top)-99, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

    hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_FILTER_RANGE_BANS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, rcParent.bottom-71, (rcParent.right-rcParent.left)-6, 44, hWndWindowItems[WND_THIS], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, rcParent.bottom-56, ((rcParent.right-rcParent.left)/2)-14, 21, hWndWindowItems[WND_THIS], (HMENU)EDT_FILTER, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        ((rcParent.right-rcParent.left)/2)+3, rcParent.bottom-56, (rcParent.right-rcParent.left)-((rcParent.right-rcParent.left)/2)-14, 21, hWndWindowItems[WND_THIS], (HMENU)CB_FILTER,
        g_hInstance, NULL);

    hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_CLEAR_TEMP_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, rcParent.bottom-25, ((rcParent.right-rcParent.left)/2)-2, 23, hWndWindowItems[WND_THIS], (HMENU)BTN_CLEAR_RANGE_TEMP_BANS, g_hInstance, NULL);

    hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_CLEAR_PERM_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        ((rcParent.right-rcParent.left)/2)+1, rcParent.bottom-25, (rcParent.right-rcParent.left)-((rcParent.right-rcParent.left)/2)-3, 23, hWndWindowItems[WND_THIS],
        (HMENU)BTN_CLEAR_RANGE_PERM_BANS, g_hInstance, NULL);

    for(uint8_t ui8i = 1; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    wpOldEditProc = (WNDPROC)::SetWindowLongPtr(hWndWindowItems[EDT_FILTER], GWLP_WNDPROC, (LONG_PTR)EditProc);

	RECT rcRangeBans;
	::GetClientRect(hWndWindowItems[LV_RANGE_BANS], &rcRangeBans);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = 100;

    const int iRangeBansStrings[] = { LAN_RANGE, LAN_REASON, LAN_EXPIRE, LAN_BANNED_BY };

    for(uint8_t ui8i = 0; ui8i < 4; ui8i++) {
        lvColumn.pszText = LanguageManager->sTexts[iRangeBansStrings[ui8i]];
        lvColumn.iSubItem = ui8i;

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_INSERTCOLUMN, ui8i, (LPARAM)&lvColumn);

        ::SendMessage(hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[iRangeBansStrings[ui8i]]);
    }

    ListViewUpdateArrow(hWndWindowItems[LV_RANGE_BANS], bSortAscending, iSortColumn);

    ::SendMessage(hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);

    AddAllRangeBans();

    const int iRangeBansWidths[] = { 250, 125, 125, 100 };

    for(uint8_t ui8i = 0; ui8i < 4; ui8i++) {
        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETCOLUMNWIDTH, ui8i, LVSCW_AUTOSIZE);
        
        int iWidth = (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, ui8i, 0);

        if(iWidth < iRangeBansWidths[ui8i]) {
            ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETCOLUMNWIDTH, ui8i, iRangeBansWidths[ui8i]);
        }
    }

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(hWndWindowItems[WND_THIS], SW_SHOW);
}
//------------------------------------------------------------------------------

void RangeBansDialog::AddAllRangeBans() {
    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_DELETEALLITEMS, 0, 0);

    time_t acc_time; time(&acc_time);

    RangeBanItem * nextRangeBan = hashBanManager->RangeBanListS,
        * curRangeBan = NULL;

    while(nextRangeBan != NULL) {
		curRangeBan = nextRangeBan;
    	nextRangeBan = curRangeBan->next;

        if((((curRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) && acc_time > curRangeBan->tempbanexpire) {
            hashBanManager->RemRange(curRangeBan);
            delete curRangeBan;

            continue;
        }

        AddRangeBan(curRangeBan);
    }

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RangeBansDialog::AddRangeBan(const RangeBanItem * pRangeBan) {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = ListViewGetInsertPosition(hWndWindowItems[LV_RANGE_BANS], pRangeBan, bSortAscending, CompareRangeBans);

    string sTxt = string(pRangeBan->sIpFrom) + " - " + pRangeBan->sIpTo;
    if((pRangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) {
        sTxt += " (";
        sTxt += LanguageManager->sTexts[LAN_FULL_BANNED];
        sTxt += ")";
    }
    lvItem.pszText = sTxt.c_str();
    lvItem.lParam = (LPARAM)pRangeBan;

    int i = (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);

    if(i == -1) {
        return;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = i;
    lvItem.iSubItem = 1;
    lvItem.pszText = (pRangeBan->sReason == NULL ? "" : pRangeBan->sReason);

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);

    if((pRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) {
        char msg[256];
        struct tm * tm = localtime(&pRangeBan->tempbanexpire);
        strftime(msg, 256, "%c", tm);

        lvItem.iSubItem = 2;
        lvItem.pszText = msg;

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
    }

    lvItem.iSubItem = 3;
    lvItem.pszText = (pRangeBan->sBy == NULL ? "" : pRangeBan->sBy);

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

int RangeBansDialog::CompareRangeBans(const void * pItem, const void * pOtherItem) {
    RangeBanItem * pFirstRangeBan = (RangeBanItem *)pItem;
    RangeBanItem * pSecondRangeBan = (RangeBanItem *)pOtherItem;

    switch(pRangeBansDialog->iSortColumn) {
        case 0:
            return (pFirstRangeBan->ui32FromIpHash > pSecondRangeBan->ui32FromIpHash) ? 1 : ((pFirstRangeBan->ui32FromIpHash == pSecondRangeBan->ui32FromIpHash) ?
                (pFirstRangeBan->ui32ToIpHash > pSecondRangeBan->ui32ToIpHash) ? 1 : ((pFirstRangeBan->ui32ToIpHash == pSecondRangeBan->ui32ToIpHash) ? 0 : -1)
                : -1);
        case 1:
            return _stricmp(pFirstRangeBan->sReason == NULL ? "" : pFirstRangeBan->sReason, pSecondRangeBan->sReason == NULL ? "" : pSecondRangeBan->sReason);
        case 2:
            if((pFirstRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) {
                if((pSecondRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) {
                    return 0;
                } else {
                    return -1;
                }
            } else if((pSecondRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) {
                return 1;
            } else {
                return (pFirstRangeBan->tempbanexpire > pSecondRangeBan->tempbanexpire) ? 1 : ((pFirstRangeBan->tempbanexpire == pSecondRangeBan->tempbanexpire) ? 0 : -1);
            }
        case 3:
            return _stricmp(pFirstRangeBan->sBy == NULL ? "" : pFirstRangeBan->sBy, pSecondRangeBan->sBy == NULL ? "" : pSecondRangeBan->sBy);
        default:
            return 0; // never happen, but we need to make compiler/complainer happy ;o)
    }
}
//------------------------------------------------------------------------------

void RangeBansDialog::OnColumnClick(const LPNMLISTVIEW &pListView) {
    if(pListView->iSubItem != iSortColumn) {
        bSortAscending = true;
        iSortColumn = pListView->iSubItem;
    } else {
        bSortAscending = !bSortAscending;
    }

    ListViewUpdateArrow(hWndWindowItems[LV_RANGE_BANS], bSortAscending, iSortColumn);

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SORTITEMS, 0, (LPARAM)&SortCompareRangeBans);
}
//------------------------------------------------------------------------------

int CALLBACK RangeBansDialog::SortCompareRangeBans(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
    int iResult = pRangeBansDialog->CompareRangeBans((void *)lParam1, (void *)lParam2);

    return (pRangeBansDialog->bSortAscending == true ? iResult : -iResult);
}
//------------------------------------------------------------------------------

void RangeBansDialog::RemoveRangeBans() {
    if(::MessageBox(hWndWindowItems[WND_THIS], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), sTitle.c_str(),
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
        return;
    }

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    int iSel = -1;
    while((iSel = (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED)) != -1) {
        RangeBanItem * pRangeBan = (RangeBanItem *)ListViewGetItem(hWndWindowItems[LV_RANGE_BANS], iSel);

        hashBanManager->RemRange(pRangeBan, true);

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_DELETEITEM, iSel, 0);
    }

    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
}
//------------------------------------------------------------------------------

void RangeBansDialog::FilterRangeBans() {
    int iTextLength = ::GetWindowTextLength(hWndWindowItems[EDT_FILTER]);

    if(iTextLength == 0) {
        sFilterString.clear();

    	AddAllRangeBans();
    } else {
        iFilterColumn = (int)::SendMessage(hWndWindowItems[CB_FILTER], CB_GETCURSEL, 0, 0);

        char buf[65];

        int iLen = ::GetWindowText(hWndWindowItems[EDT_FILTER], buf, 65);

        for(int i = 0; i < iLen; i++) {
            buf[i] = (char)tolower(buf[i]);
        }

        sFilterString = buf;

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)FALSE, 0);

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_DELETEALLITEMS, 0, 0);

        time_t acc_time; time(&acc_time);

        RangeBanItem * nextRangeBan = hashBanManager->RangeBanListS,
            * curRangeBan = NULL;

        while(nextRangeBan != NULL) {
            curRangeBan = nextRangeBan;
            nextRangeBan = curRangeBan->next;

            if((((curRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) == true) && acc_time > curRangeBan->tempbanexpire) {
                hashBanManager->RemRange(curRangeBan);
                delete curRangeBan;

                continue;
            }

            if(FilterRangeBan(curRangeBan) == false) {
                AddRangeBan(curRangeBan);
            }
        }

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], WM_SETREDRAW, (WPARAM)TRUE, 0);
    }
}
//------------------------------------------------------------------------------

bool RangeBansDialog::FilterRangeBan(const RangeBanItem * pRangeBan) {
    switch(iFilterColumn) {
        case 0: {
            char sTxt[64];
            sprintf(sTxt, "%s - %s", pRangeBan->sIpFrom, pRangeBan->sIpTo);

            if(stristr2(sTxt, sFilterString.c_str()) != NULL) {
                return false;
            }
            break;
        }
        case 1:
            if(pRangeBan->sReason != NULL && stristr2(pRangeBan->sReason, sFilterString.c_str()) != NULL) {
                return false;
            }
            break;
        case 2:
            if((pRangeBan->ui8Bits & hashBanMan::TEMP) == hashBanMan::TEMP) {
                char msg[256];
                struct tm * tm = localtime(&pRangeBan->tempbanexpire);
                strftime(msg, 256, "%c", tm);

                if(stristr2(msg, sFilterString.c_str()) != NULL) {
                    return false;
                }
            }
            break;
        case 3:
            if(pRangeBan->sBy != NULL && stristr2(pRangeBan->sBy, sFilterString.c_str()) != NULL) {
                return false;
            }
            break;
    }

    return true;
}
//------------------------------------------------------------------------------

void RangeBansDialog::RemoveRangeBan(const RangeBanItem * pRangeBan) {
    int iPos = ListViewGetItemPosition(hWndWindowItems[LV_RANGE_BANS], (void *)pRangeBan);

    if(iPos != -1) {
        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_DELETEITEM, iPos, 0);
    }
}
//------------------------------------------------------------------------------

void RangeBansDialog::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow != hWndWindowItems[LV_RANGE_BANS]) {
        return;
    }

    UINT UISelectedCount = (UINT)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETSELECTEDCOUNT, 0, 0);

    if(UISelectedCount == 0) {
        return;
    }

    HMENU hMenu = ::CreatePopupMenu();

    if(UISelectedCount == 1) {
        ::AppendMenu(hMenu, MF_STRING, IDC_CHANGE_RANGE_BAN, LanguageManager->sTexts[LAN_CHANGE]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }

    ::AppendMenu(hMenu, MF_STRING, IDC_REMOVE_RANGE_BANS, LanguageManager->sTexts[LAN_REMOVE]);

    int iX = GET_X_LPARAM(lParam);
    int iY = GET_Y_LPARAM(lParam);

    // -1, -1 is menu created by key. We need few tricks to show menu on correct position ;o)
    if(iX == -1 && iY == -1) {
        int iSel = (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

        POINT pt = { 0 };
        if((BOOL)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_ISITEMVISIBLE, (WPARAM)iSel, 0) == FALSE) {
            RECT rcList;
            ::GetClientRect(hWndWindowItems[LV_RANGE_BANS], &rcList);

            ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETITEMPOSITION, (WPARAM)iSel, (LPARAM)&pt);

            pt.y = (pt.y < rcList.top) ? rcList.top : rcList.bottom;
        } else {
            RECT rcItem;
            rcItem.left = LVIR_LABEL;
            ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETITEMRECT, (WPARAM)iSel, (LPARAM)&rcItem);

            pt.x = rcItem.left;
            pt.y = rcItem.top + ((rcItem.bottom - rcItem.top) / 2);
        }

        ::ClientToScreen(hWndWindowItems[LV_RANGE_BANS], &pt);

        iX = pt.x;
        iY = pt.y;
    }

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, hWndWindowItems[WND_THIS], NULL);

    ::DestroyMenu(hMenu);
}
//------------------------------------------------------------------------------

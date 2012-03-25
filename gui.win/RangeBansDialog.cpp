/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2012  Petr Kozelka, PPK at PtokaX dot org

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
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "RangeBanDialog.h"
//---------------------------------------------------------------------------
RangeBansDialog * pRangeBansDialog = NULL;
//---------------------------------------------------------------------------
#define IDC_CHANGE_RANGE_BAN      1000
#define IDC_REMOVE_RANGE_BANS     1001
//---------------------------------------------------------------------------
static ATOM atomRangeBansDialog = 0;
//---------------------------------------------------------------------------

RangeBansDialog::RangeBansDialog() {
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
            ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

            ::SetWindowPos(hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS], NULL, (rcParent.right / 2) + 1, rcParent.bottom - iEditHeight - 2,
                rcParent.right - (rcParent.right / 2) - 3, iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS], NULL, 2, rcParent.bottom - iEditHeight - 2, (rcParent.right / 2) - 2, iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[CB_FILTER], NULL, (rcParent.right / 2) + 3, (rcParent.bottom - iEditHeight - iOneLineGB - 6) + iGroupBoxMargin,
                rcParent.right - (rcParent.right / 2) - 14, iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[EDT_FILTER], NULL, 11, (rcParent.bottom - iEditHeight - iOneLineGB - 6) + iGroupBoxMargin, (rcParent.right / 2) - 14, iEditHeight, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[GB_FILTER], NULL, 3, rcParent.bottom - iEditHeight - iOneLineGB - 6, rcParent.right - 6, iOneLineGB, SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[LV_RANGE_BANS], NULL, 0, 0, rcParent.right - 6, rcParent.bottom - iOneLineGB - (2 * iEditHeight) - 14, SWP_NOMOVE | SWP_NOZORDER);
            ::SetWindowPos(hWndWindowItems[BTN_ADD_RANGE_BAN], NULL, 0, 0, rcParent.right - 4, iEditHeight, SWP_NOMOVE | SWP_NOZORDER);

            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case (BTN_ADD_RANGE_BAN+100): {

                    pRangeBanDialog = new RangeBanDialog();

                    if(pRangeBanDialog != NULL) {
                        pRangeBanDialog->DoModal(hWndWindowItems[WINDOW_HANDLE]);
                    }

                    return 0;
                }
                case IDC_CHANGE_RANGE_BAN: {
                    ChangeRangeBan();
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
                    if(::MessageBox(hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
                        sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
                        return 0;
                    }

                    hashBanManager->ClearTempRange();
                    AddAllRangeBans();

                    return 0;
                case BTN_CLEAR_RANGE_PERM_BANS:
                    if(::MessageBox(hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
                        sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
                        return 0;
                    }

                    hashBanManager->ClearPermRange();
                    AddAllRangeBans();

                    return 0;
                case IDOK: { // NM_RETURN
                    HWND hWndFocus = ::GetFocus();

                    if(hWndFocus == hWndWindowItems[LV_RANGE_BANS]) {
                        ChangeRangeBan();
                        return 0;
                    } else if(hWndFocus == hWndWindowItems[EDT_FILTER]) {
                        FilterRangeBans();
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
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[LV_RANGE_BANS]) {
                if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK) {
                    OnColumnClick((LPNMLISTVIEW)lParam);
                } else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
                    if(((LPNMITEMACTIVATE)lParam)->iItem == -1) {
                        break;
                    }

                    RangeBanItem * pRangeBan = (RangeBanItem *)ListViewGetItem(hWndWindowItems[LV_RANGE_BANS], ((LPNMITEMACTIVATE)lParam)->iItem);

                    pRangeBanDialog = new RangeBanDialog();

                    if(pRangeBanDialog != NULL) {
                        pRangeBanDialog->DoModal(hWndWindowItems[WINDOW_HANDLE], pRangeBan);
                    }

                    return 0;
                }
            }

            break;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = ScaleGui(g_GuiSettingManager->GetDefaultInteger(GUISETINT_RANGE_BANS_WINDOW_WIDTH));
            mminfo->ptMinTrackSize.y = ScaleGui(g_GuiSettingManager->GetDefaultInteger(GUISETINT_RANGE_BANS_WINDOW_HEIGHT));

            return 0;
        }
        case WM_CLOSE: {
            RECT rcRangeBans;
            ::GetWindowRect(hWndWindowItems[WINDOW_HANDLE], &rcRangeBans);

            g_GuiSettingManager->SetInteger(GUISETINT_RANGE_BANS_WINDOW_WIDTH, rcRangeBans.right - rcRangeBans.left);
            g_GuiSettingManager->SetInteger(GUISETINT_RANGE_BANS_WINDOW_HEIGHT, rcRangeBans.bottom - rcRangeBans.top);

            g_GuiSettingManager->SetInteger(GUISETINT_RANGE_BANS_RANGE, (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 0, 0));
            g_GuiSettingManager->SetInteger(GUISETINT_RANGE_BANS_REASON, (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 1, 0));
            g_GuiSettingManager->SetInteger(GUISETINT_RANGE_BANS_EXPIRE, (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 2, 0));
            g_GuiSettingManager->SetInteger(GUISETINT_RANGE_BANS_BY, (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETCOLUMNWIDTH, 3, 0));

            ::EnableWindow(::GetParent(hWndWindowItems[WINDOW_HANDLE]), TRUE);
            g_hWndActiveDialog = NULL;

            break;
        }
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
        case WM_SETFOCUS:
            if((UINT)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETSELECTEDCOUNT, 0, 0) != 0) {
                ::SetFocus(hWndWindowItems[LV_RANGE_BANS]);
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

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_WIDTH) / 2);
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_HEIGHT) / 2);

    hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRangeBansDialog), LanguageManager->sTexts[LAN_RANGE_BANS],
        WS_POPUP | WS_CAPTION | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_RANGE_BANS_WINDOW_HEIGHT),
        hWndParent, NULL, g_hInstance, NULL);

    if(hWndWindowItems[WINDOW_HANDLE] == NULL) {
        return;
    }

    g_hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];

    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticRangeBansDialogProc);

    ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

    hWndWindowItems[BTN_ADD_RANGE_BAN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ADD_NEW_RANGE_BAN], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, 2, rcParent.right - 4, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)(BTN_ADD_RANGE_BAN+100), g_hInstance, NULL);

    hWndWindowItems[LV_RANGE_BANS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        3, iEditHeight + 6, rcParent.right - 6, rcParent.bottom - iOneLineGB - (2 * iEditHeight) - 14, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

    hWndWindowItems[GB_FILTER] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_FILTER_RANGE_BANS], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, rcParent.bottom - iEditHeight - iOneLineGB - 6, rcParent.right - 6, iOneLineGB, hWndWindowItems[WINDOW_HANDLE], NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_FILTER] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, (rcParent.bottom - iEditHeight - iOneLineGB - 6) + iGroupBoxMargin, (rcParent.right / 2) - 14, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)EDT_FILTER, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_FILTER], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[CB_FILTER] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        (rcParent.right / 2) + 3, (rcParent.bottom - iEditHeight - iOneLineGB - 6) + iGroupBoxMargin, rcParent.right - (rcParent.right / 2) - 14, iEditHeight,
        hWndWindowItems[WINDOW_HANDLE], (HMENU)CB_FILTER,
        g_hInstance, NULL);

    hWndWindowItems[BTN_CLEAR_RANGE_TEMP_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_CLEAR_TEMP_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, rcParent.bottom - iEditHeight - 2, (rcParent.right / 2) - 2, iEditHeight, hWndWindowItems[WINDOW_HANDLE], (HMENU)BTN_CLEAR_RANGE_TEMP_BANS, g_hInstance, NULL);

    hWndWindowItems[BTN_CLEAR_RANGE_PERM_BANS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_CLEAR_PERM_RANGE_BANS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        (rcParent.right / 2) + 1, rcParent.bottom - iEditHeight - 2, rcParent.right - (rcParent.right / 2) - 3, iEditHeight, hWndWindowItems[WINDOW_HANDLE],
        (HMENU)BTN_CLEAR_RANGE_PERM_BANS, g_hInstance, NULL);

    for(uint8_t ui8i = 1; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }

	RECT rcRangeBans;
	::GetClientRect(hWndWindowItems[LV_RANGE_BANS], &rcRangeBans);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;

    const int iRangeBansStrings[] = { LAN_RANGE, LAN_REASON, LAN_EXPIRE, LAN_BANNED_BY };
    const int iRangeBansWidths[] = { GUISETINT_RANGE_BANS_RANGE, GUISETINT_RANGE_BANS_REASON, GUISETINT_RANGE_BANS_EXPIRE, GUISETINT_RANGE_BANS_BY };

    for(uint8_t ui8i = 0; ui8i < 4; ui8i++) {
        lvColumn.cx = g_GuiSettingManager->iIntegers[iRangeBansWidths[ui8i]];
        lvColumn.pszText = LanguageManager->sTexts[iRangeBansStrings[ui8i]];
        lvColumn.iSubItem = ui8i;

        ::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_INSERTCOLUMN, ui8i, (LPARAM)&lvColumn);

        ::SendMessage(hWndWindowItems[CB_FILTER], CB_ADDSTRING, 0, (LPARAM)LanguageManager->sTexts[iRangeBansStrings[ui8i]]);
    }

    ListViewUpdateArrow(hWndWindowItems[LV_RANGE_BANS], bSortAscending, iSortColumn);

    ::SendMessage(hWndWindowItems[CB_FILTER], CB_SETCURSEL, 0, 0);

    AddAllRangeBans();

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
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

    ListViewSelectFirstItem(hWndWindowItems[LV_RANGE_BANS]);

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
			return (memcmp(pFirstRangeBan->ui128FromIpHash, pSecondRangeBan->ui128FromIpHash, 16) > 0) ? 1 : 
				((memcmp(pFirstRangeBan->ui128FromIpHash, pSecondRangeBan->ui128FromIpHash, 16) == 0) ?
				(memcmp(pFirstRangeBan->ui128ToIpHash, pSecondRangeBan->ui128ToIpHash, 16) > 0) ? 1 : 
				((memcmp(pFirstRangeBan->ui128ToIpHash, pSecondRangeBan->ui128ToIpHash, 16) == 0) ? 0 : -1) : -1);
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
    if(::MessageBox(hWndWindowItems[WINDOW_HANDLE], (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(), sTitle.c_str(),
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

        ListViewSelectFirstItem(hWndWindowItems[LV_RANGE_BANS]);

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

    ListViewGetMenuPos(hWndWindowItems[LV_RANGE_BANS], iX, iY);

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, hWndWindowItems[WINDOW_HANDLE], NULL);

    ::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RangeBansDialog::ChangeRangeBan() {
    int iSel = (int)::SendMessage(hWndWindowItems[LV_RANGE_BANS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    RangeBanItem * pRangeBan = (RangeBanItem *)ListViewGetItem(hWndWindowItems[LV_RANGE_BANS], iSel);

    pRangeBanDialog = new RangeBanDialog();

    if(pRangeBanDialog != NULL) {
        pRangeBanDialog->DoModal(hWndWindowItems[WINDOW_HANDLE], pRangeBan);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

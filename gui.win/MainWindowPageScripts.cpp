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
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "MainWindowPageScripts.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/LuaScript.h"
#include "../core/LuaScriptManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "GuiUtil.h"
#include "MainWindow.h"
#include "Resources.h"
#include "ScriptEditorDialog.h"
//---------------------------------------------------------------------------
MainWindowPageScripts *pMainWindowPageScripts = NULL;
//---------------------------------------------------------------------------
WNDPROC wpOldGroupBoxProc = NULL;
//---------------------------------------------------------------------------
#define IDC_OPEN_IN_EXT_EDITOR      500
#define IDC_OPEN_IN_SCRIPT_EDITOR   501
#define IDC_DELETE_SCRIPT           502
//---------------------------------------------------------------------------

LRESULT CALLBACK GroupBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_COMMAND: {
            MainWindowPageScripts * pMainWindowPageScripts = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if(pMainWindowPageScripts != NULL && RichEditCheckMenuCommands(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], LOWORD(wParam)) == true) {
                return 0;
            }

            break;
        }
        case WM_CONTEXTMENU: {
            MainWindowPageScripts * pMainWindowPageScripts = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if(pMainWindowPageScripts != NULL) {
                RichEditPopupMenu(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], pMainWindowPageScripts->m_hWnd, lParam);
            }
            break;
        }
        case WM_NOTIFY: {
            MainWindowPageScripts * pMainWindowPageScripts = (MainWindowPageScripts *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if(pMainWindowPageScripts != NULL) {
                if(((LPNMHDR)lParam)->hwndFrom == pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS] && ((LPNMHDR)lParam)->code == EN_LINK) {
                    if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
                        RichEditOpenLink(pMainWindowPageScripts->hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], (ENLINK *)lParam);
                    }
                }
            }

            break;
        }
    }

    return ::CallWindowProc(wpOldGroupBoxProc, hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

MainWindowPageScripts::MainWindowPageScripts() {
    pMainWindowPageScripts = this;

    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));

    bIgnoreItemChanged = false;
}
//---------------------------------------------------------------------------

MainWindowPageScripts::~MainWindowPageScripts() {
    pMainWindowPageScripts = NULL;
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageScripts::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_SETFOCUS:
            ::SetFocus(hWndPageItems[REDT_SCRIPTS_ERRORS]);

            return 0;
        case WM_WINDOWPOSCHANGED: {
            int iX = ((WINDOWPOS*)lParam)->cx;
            int iY = ((WINDOWPOS*)lParam)->cy;

            ::SetWindowPos(hWndPageItems[BTN_RESTART_SCRIPTS], NULL, iX-147, iY-25, 145, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_MOVE_DOWN], NULL, iX-74, iY-49, 72, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_MOVE_UP], NULL, iX-147, iY-49, 72, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[LV_SCRIPTS], NULL, iX-146, 50, 143, iY-101, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_REFRESH_SCRIPTS], NULL, iX-147, 25, 145, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[BTN_OPEN_SCRIPT_EDITOR], NULL, iX-147, 1, 145, 23, SWP_NOZORDER);
            ::SetWindowPos(hWndPageItems[REDT_SCRIPTS_ERRORS], NULL, 0, 0, iX-168, iY-25, SWP_NOMOVE | SWP_NOZORDER);
            ::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], WM_VSCROLL, SB_BOTTOM, NULL);
            ::SetWindowPos(hWndPageItems[GB_SCRIPTS_ERRORS], NULL, 0, 0, iX-152, iY-3, SWP_NOMOVE | SWP_NOZORDER);

            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_OPEN_SCRIPT_EDITOR:
                    OpenScriptEditor();
                    return 0;
                case BTN_REFRESH_SCRIPTS:
                    RefreshScripts();
                    return 0;
                case BTN_MOVE_UP:
                    MoveUp();
                    return 0;
                case BTN_MOVE_DOWN:
                    MoveDown();
                    return 0;
                case BTN_RESTART_SCRIPTS:
                    RestartScripts();
                    return 0;
                case IDC_OPEN_IN_EXT_EDITOR:
                    OpenInExternalEditor();
                    return 0;
                case IDC_OPEN_IN_SCRIPT_EDITOR:
                    OpenInScriptEditor();
                    return 0;
                case IDC_DELETE_SCRIPT:
                    DeleteScript();
                    return 0;
            }

            if(RichEditCheckMenuCommands(hWndPageItems[REDT_SCRIPTS_ERRORS], LOWORD(wParam)) == true) {
                return 0;
            }

            break;
        case WM_CONTEXTMENU:
            OnContextMenu((HWND)wParam, lParam);
            break;
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndPageItems[LV_SCRIPTS]) {
                if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
                    OnItemChanged((LPNMLISTVIEW)lParam);
                } else if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
                    OnDoubleClick((LPNMITEMACTIVATE)lParam);
                }
            }
            break;
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

bool MainWindowPageScripts::CreateMainWindowPage(HWND hOwner) {
    CreateHWND(hOwner);

    RECT rcMain;
    ::GetClientRect(m_hWnd, &rcMain);

    hWndPageItems[GB_SCRIPTS_ERRORS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_SCRIPTS_ERRORS],
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_GROUPBOX, 3, 0, rcMain.right-152, rcMain.bottom-3, m_hWnd, NULL, g_hInstance, NULL);

    hWndPageItems[REDT_SCRIPTS_ERRORS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
        8, 14, rcMain.right-168, rcMain.bottom-25, hWndPageItems[GB_SCRIPTS_ERRORS], (HMENU)REDT_SCRIPTS_ERRORS, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_EXLIMITTEXT, 0, (LPARAM)1048576);
    ::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_AUTOURLDETECT, TRUE, 0);
    ::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(hWndPageItems[REDT_SCRIPTS_ERRORS], EM_GETEVENTMASK, 0, 0) | ENM_LINK);

    hWndPageItems[BTN_OPEN_SCRIPT_EDITOR] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_OPEN_SCRIPT_EDITOR], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right-147, 1, 145, 23, m_hWnd, (HMENU)BTN_OPEN_SCRIPT_EDITOR, g_hInstance, NULL);

    hWndPageItems[BTN_REFRESH_SCRIPTS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REFRESH_SCRIPTS], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right-147, 25, 145, 23, m_hWnd, (HMENU)BTN_REFRESH_SCRIPTS, g_hInstance, NULL);

    hWndPageItems[LV_SCRIPTS] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        rcMain.right-146, 50, 143, rcMain.bottom-101, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP | LVS_EX_CHECKBOXES);

    hWndPageItems[BTN_MOVE_UP] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_MOVE_UP], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right-147, rcMain.bottom-49, 72, 23, m_hWnd, (HMENU)BTN_MOVE_UP, g_hInstance, NULL);

    hWndPageItems[BTN_MOVE_DOWN] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_MOVE_DOWN], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
        rcMain.right-74, rcMain.bottom-49, 72, 23, m_hWnd, (HMENU)BTN_MOVE_DOWN, g_hInstance, NULL);

    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
        if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false || bServerRunning == false) {
            dwStyle |= WS_DISABLED;
        }

        hWndPageItems[BTN_RESTART_SCRIPTS] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_RESTART_SCRIPTS], dwStyle,
            rcMain.right-147, rcMain.bottom-25, 145, 23, m_hWnd, (HMENU)BTN_RESTART_SCRIPTS, g_hInstance, NULL);
    }

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::SetWindowLongPtr(hWndPageItems[GB_SCRIPTS_ERRORS], GWLP_USERDATA, (LONG_PTR)this);
    wpOldGroupBoxProc = (WNDPROC)::SetWindowLongPtr(hWndPageItems[GB_SCRIPTS_ERRORS], GWLP_WNDPROC, (LONG_PTR)GroupBoxProc);

	RECT rcScripts;
	::GetClientRect(hWndPageItems[LV_SCRIPTS], &rcScripts);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = (((rcScripts.right - rcScripts.left)/3)*2)+1;
    lvColumn.pszText = LanguageManager->sTexts[LAN_SCRIPT_FILE];
    lvColumn.iSubItem = 0;

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    lvColumn.fmt = LVCFMT_RIGHT;
    lvColumn.cx = (lvColumn.cx/2);
    lvColumn.pszText = LanguageManager->sTexts[LAN_MEM_USAGE];
    lvColumn.iSubItem = 1;
    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);

	AddScriptsToList(false);

	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateLanguage() {
    ::SetWindowText(hWndPageItems[GB_SCRIPTS_ERRORS], LanguageManager->sTexts[LAN_SCRIPTS_ERRORS]);
    ::SetWindowText(hWndPageItems[BTN_OPEN_SCRIPT_EDITOR], LanguageManager->sTexts[LAN_OPEN_SCRIPT_EDITOR]);
    ::SetWindowText(hWndPageItems[BTN_REFRESH_SCRIPTS], LanguageManager->sTexts[LAN_REFRESH_SCRIPTS]);

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_TEXT;
    lvColumn.pszText = LanguageManager->sTexts[LAN_SCRIPT_FILE];

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETCOLUMN, 0, (LPARAM)&lvColumn);

    lvColumn.pszText = LanguageManager->sTexts[LAN_MEM_USAGE];

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETCOLUMN, 1, (LPARAM)&lvColumn);

    ::SetWindowText(hWndPageItems[BTN_MOVE_UP], LanguageManager->sTexts[LAN_MOVE_UP]);
    ::SetWindowText(hWndPageItems[BTN_MOVE_DOWN], LanguageManager->sTexts[LAN_MOVE_DOWN]);
    ::SetWindowText(hWndPageItems[BTN_RESTART_SCRIPTS], LanguageManager->sTexts[LAN_RESTART_SCRIPTS]);
}
//---------------------------------------------------------------------------

char * MainWindowPageScripts::GetPageName() {
    return LanguageManager->sTexts[LAN_SCRIPTS];
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OnContextMenu(HWND hWindow, LPARAM lParam) {
    if(hWindow == hWndPageItems[LV_SCRIPTS]) {
        int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

        if(iSel == -1) {
            return;
        }

        HMENU hMenu = ::CreatePopupMenu();

        ::AppendMenu(hMenu, MF_STRING, IDC_OPEN_IN_EXT_EDITOR, LanguageManager->sTexts[LAN_OPEN_EXT_EDIT]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_OPEN_IN_SCRIPT_EDITOR, LanguageManager->sTexts[LAN_OPEN_IN_SCRIPT_EDITOR]);
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        ::AppendMenu(hMenu, MF_STRING, IDC_DELETE_SCRIPT, LanguageManager->sTexts[LAN_DELETE_SCRIPT]);

        ::SetMenuDefaultItem(hMenu, IDC_OPEN_IN_SCRIPT_EDITOR, FALSE);

        int iX = GET_X_LPARAM(lParam);
        int iY = GET_Y_LPARAM(lParam);

        // -1, -1 is menu created by key. We need few tricks to show menu on correct position ;o)
        if(iX == -1 && iY == -1) {
            POINT pt = { 0 };
            if((BOOL)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ISITEMVISIBLE, (WPARAM)iSel, 0) == FALSE) {
                RECT rcList;
                ::GetClientRect(hWndPageItems[LV_SCRIPTS], &rcList);

                ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETITEMPOSITION, (WPARAM)iSel, (LPARAM)&pt);

                pt.y = (pt.y < rcList.top) ? rcList.top : rcList.bottom;
            } else {
                RECT rcItem;
                rcItem.left = LVIR_LABEL;
                ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETITEMRECT, (WPARAM)iSel, (LPARAM)&rcItem);

                pt.x = rcItem.left;
                pt.y = rcItem.top + ((rcItem.bottom - rcItem.top) / 2);
            }

            ::ClientToScreen(hWndPageItems[LV_SCRIPTS], &pt);

            iX = pt.x;
            iY = pt.y;
        }

        ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, m_hWnd, NULL);

        ::DestroyMenu(hMenu);
    }
}
//------------------------------------------------------------------------------

ScriptEditorDialog * MainWindowPageScripts::OpenScriptEditor() {
    ScriptEditorDialog * ScriptEditorDlg = new ScriptEditorDialog();
    ScriptEditorDlg->DoModal(pMainWindow->m_hWnd);

    return ScriptEditorDlg;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::RefreshScripts() {
    ScriptManager->CheckForDeletedScripts();
	ScriptManager->CheckForNewScripts();

	AddScriptsToList(true);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::AddScriptsToList(const bool &bDelete) {
    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    if(bDelete == true) {
        ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_DELETEALLITEMS, 0, 0);
    }

	for(uint8_t ui8i = 0; ui8i < ScriptManager->ui8ScriptCount; ui8i++) {
        ScriptToList(ui8i, true, false);
	}

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::ScriptToList(const uint8_t &ui8ScriptId, const bool &bInsert, const bool &bSelected) {
    bIgnoreItemChanged = true;

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_PARAM | LVIF_TEXT;
    lvItem.iItem = ui8ScriptId;
    lvItem.pszText = ScriptManager->ScriptTable[ui8ScriptId]->sName;
    lvItem.lParam = (LPARAM)ScriptManager->ScriptTable[ui8ScriptId];

    if(bSelected == true) {
        lvItem.mask |= LVIF_STATE;
        lvItem.state = LVIS_SELECTED;
        lvItem.stateMask = LVIS_SELECTED;
    }

    int i = -1;
    
    if(bInsert == true) {
        i = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_INSERTITEM, 0, (LPARAM)&lvItem);
    } else {
        i = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
    }

    if(i != -1 || bInsert == false) {
        lvItem.mask = LVIF_STATE;
        lvItem.state = INDEXTOSTATEIMAGEMASK(ScriptManager->ScriptTable[ui8ScriptId]->bEnabled == true ? 2 : 1);
        lvItem.stateMask = LVIS_STATEIMAGEMASK;

        ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEMSTATE, ui8ScriptId, (LPARAM)&lvItem);
    }

    bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OnItemChanged(const LPNMLISTVIEW &pListView) {
    UpdateUpDown();

    if(bIgnoreItemChanged == true || pListView->iItem == -1 || (pListView->uNewState & LVIS_STATEIMAGEMASK) == (pListView->uOldState & LVIS_STATEIMAGEMASK)) {
        return;
    }

    if((((pListView->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) == 0) {
        if(ScriptManager->ScriptTable[pListView->iItem]->bEnabled == false) {
            return;
        }

        ScriptManager->ScriptTable[pListView->iItem]->bEnabled = false;

        if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false || bServerRunning == false) {
			return;
        }

		ScriptManager->StopScript(ScriptManager->ScriptTable[pListView->iItem], false);
		ClearMemUsage((uint8_t)pListView->iItem);

		RichEditAppendText(hWndPageItems[REDT_SCRIPTS_ERRORS], (string(LanguageManager->sTexts[LAN_SCRIPT_STOPPED], (size_t)LanguageManager->ui16TextsLens[LAN_SCRIPT_STOPPED])+".").c_str());
    } else {
        if(ScriptManager->ScriptTable[pListView->iItem]->bEnabled == true) {
            return;
        }

        ScriptManager->ScriptTable[pListView->iItem]->bEnabled = true;

		if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == false || bServerRunning == false) {
            return;
        }

		if(ScriptManager->StartScript(ScriptManager->ScriptTable[pListView->iItem], false) == true) {
			RichEditAppendText(hWndPageItems[REDT_SCRIPTS_ERRORS], (string(LanguageManager->sTexts[LAN_SCRIPT_STARTED], (size_t)LanguageManager->ui16TextsLens[LAN_SCRIPT_STARTED])+".").c_str());
		}
    }
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OnDoubleClick(const LPNMITEMACTIVATE &pItemActivate) {
    ScriptEditorDialog * pScriptEditor = OpenScriptEditor();
    pScriptEditor->LoadScript(SCRIPT_PATH + ScriptManager->ScriptTable[pItemActivate->iItem]->sName);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::ClearMemUsageAll() {
	for(uint8_t ui8i = 0; ui8i < ScriptManager->ui8ScriptCount; ui8i++) {
        ClearMemUsage(ui8i);
	}
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateMemUsage() {
	for(uint8_t ui8i = 0; ui8i < ScriptManager->ui8ScriptCount; ui8i++) {
        if(ScriptManager->ScriptTable[ui8i]->bEnabled == false) {
            continue;
        }

        string tmp(lua_gc(ScriptManager->ScriptTable[ui8i]->LUA, LUA_GCCOUNT, 0));

        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = ui8i;
        lvItem.iSubItem = 1;

        string sMemUsage(lua_gc(ScriptManager->ScriptTable[ui8i]->LUA, LUA_GCCOUNT, 0));
        lvItem.pszText = sMemUsage.c_str();

        ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
	}
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::MoveUp() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

	ScriptManager->MoveScript((uint8_t)iSel, true);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ScriptToList((uint8_t)iSel, false, false);

    iSel--;

    ScriptToList((uint8_t)iSel, false, true);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, iSel, FALSE);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::MoveDown() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

	ScriptManager->MoveScript((uint8_t)iSel, false);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ScriptToList((uint8_t)iSel, false, false);

    iSel++;

    ScriptToList((uint8_t)iSel, false, true);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, iSel, FALSE);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::RestartScripts() {
    ScriptManager->Restart();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateUpDown() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], FALSE);
		::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], FALSE);
	} else if(iSel == 0) {
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], FALSE);
		if(iSel == (ScriptManager->ui8ScriptCount-1)) {
			::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], FALSE);
		} else {
            ::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], TRUE);
		}
	} else if(iSel == (ScriptManager->ui8ScriptCount-1)) {
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], FALSE);
	} else {
		::EnableWindow(hWndPageItems[BTN_MOVE_UP], TRUE);
		::EnableWindow(hWndPageItems[BTN_MOVE_DOWN], TRUE);
	}
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OpenInExternalEditor() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    ::ShellExecute(NULL, NULL, (SCRIPT_PATH + ScriptManager->ScriptTable[iSel]->sName).c_str(), NULL, NULL, SW_SHOWNORMAL);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::OpenInScriptEditor() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    ScriptEditorDialog * pScriptEditor = OpenScriptEditor();
    pScriptEditor->LoadScript(SCRIPT_PATH + ScriptManager->ScriptTable[iSel]->sName);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::DeleteScript() {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if(iSel == -1) {
        return;
    }

    if(::MessageBox(m_hWnd, (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
        "PtokaX", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
		return;
	}

	ScriptManager->DeleteScript((uint8_t)iSel);
	::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_DELETEITEM, iSel, 0);
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::MoveScript(uint8_t ui8ScriptId, const bool &bUp) {
    int iSel = (int)::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)FALSE, 0);

    ScriptToList(ui8ScriptId, false, (iSel != -1 && ((bUp == true && iSel == (int)ui8ScriptId-1) || (bUp == false && iSel == (int)ui8ScriptId+1))) ? true : false);

    if(bUp == true) {
        ui8ScriptId--;
    } else {
        ui8ScriptId++;
    }

    ScriptToList(ui8ScriptId, false, (iSel != -1 && iSel == (int)ui8ScriptId) ? true : false);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_ENSUREVISIBLE, ui8ScriptId, FALSE);

    ::SendMessage(hWndPageItems[LV_SCRIPTS], WM_SETREDRAW, (WPARAM)TRUE, 0);

    UpdateUpDown();
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::UpdateCheck(const uint8_t &ui8ScriptId) {
    bIgnoreItemChanged = true;

    ListView_SetItemState(hWndPageItems[LV_SCRIPTS], ui8ScriptId, INDEXTOSTATEIMAGEMASK(ScriptManager->ScriptTable[ui8ScriptId]->bEnabled == true ? 2 : 1), LVIS_STATEIMAGEMASK);

    if(ScriptManager->ScriptTable[ui8ScriptId]->bEnabled == false) {
        ClearMemUsage(ui8ScriptId);
    }

    bIgnoreItemChanged = false;
}
//------------------------------------------------------------------------------

void MainWindowPageScripts::ClearMemUsage(uint8_t ui8ScriptId) {
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = ui8ScriptId;
    lvItem.iSubItem = 1;
    lvItem.pszText = "";

    ::SendMessage(hWndPageItems[LV_SCRIPTS], LVM_SETITEM, 0, (LPARAM)&lvItem);
}
//------------------------------------------------------------------------------

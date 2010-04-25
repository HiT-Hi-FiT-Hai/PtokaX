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

//------------------------------------------------------------------------------
#ifndef SettingDialogH
#define SettingDialogH
//------------------------------------------------------------------------------
#include "SettingPage.h"
//---------------------------------------------------------------------------

class SettingDialog {
public:
    HWND m_hWnd, hWndTree;
    HWND btnOK, btnCancel;

    SettingDialog();
    ~SettingDialog();

    static INT_PTR CALLBACK StaticSettingDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	INT_PTR DoModal(HWND hWndParent);
private:
    SettingPage * SettingPages[12];

    LRESULT SettingDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnSelChanged();
};
//------------------------------------------------------------------------------

#endif

/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2014  Petr Kozelka, PPK at PtokaX dot org

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
#ifndef RegisteredUsersDialogH
#define RegisteredUsersDialogH
//------------------------------------------------------------------------------
struct RegUser;
//------------------------------------------------------------------------------

class clsRegisteredUsersDialog {
public:
    static clsRegisteredUsersDialog * mPtr;

    HWND hWndWindowItems[6];

    enum enmWindowItems {
        WINDOW_HANDLE,
        BTN_ADD_REG,
        LV_REGS,
        GB_FILTER,
        EDT_FILTER,
        CB_FILTER,
    };

    clsRegisteredUsersDialog();
    ~clsRegisteredUsersDialog();

    static LRESULT CALLBACK StaticRegisteredUsersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static int CompareRegs(const void * pItem, const void * pOtherItem);
    static int CALLBACK SortCompareRegs(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/);

	void DoModal(HWND hWndParent);
	void FilterRegs();
	void AddReg(const RegUser * pReg);
	void RemoveReg(const RegUser * pReg);
	void UpdateProfiles();
private:
    string sFilterString;

    int iFilterColumn, iSortColumn;

    bool bSortAscending;

    clsRegisteredUsersDialog(const clsRegisteredUsersDialog&);
    const clsRegisteredUsersDialog& operator=(const clsRegisteredUsersDialog&);

    LRESULT RegisteredUsersDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void AddAllRegs();
    void OnColumnClick(const LPNMLISTVIEW &pListView);
    void RemoveRegs();
    void OnContextMenu(HWND hWindow, LPARAM lParam);
    void ChangeReg();
};
//------------------------------------------------------------------------------

#endif

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
#ifndef MainWindowPageUsersChatH
#define MainWindowPageUsersChatH
//------------------------------------------------------------------------------
#include "MainWindowPage.h"
//------------------------------------------------------------------------------

class MainWindowPageUsersChat : public MainWindowPage {
public:
    HWND hWndPageItems[1];

    enum enmPageItems {
        REDT_CHAT,
    };

    MainWindowPageUsersChat();
    ~MainWindowPageUsersChat() { };

    bool CreateMainWindowPage(HWND hOwner);
    void UpdateLanguage();
    char * GetPageName();
private:
    LRESULT MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
//------------------------------------------------------------------------------

#endif

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
#include "../core/LanguageManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

MainWindowPageUsersChat::MainWindowPageUsersChat() {
    memset(&hWndPageItems, 0, (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

LRESULT MainWindowPageUsersChat::MainWindowPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

bool MainWindowPageUsersChat::CreateMainWindowPage(HWND hOwner) {
    CreateHWND(hOwner);
/*
    RECT rcMain;
    ::GetClientRect(m_hWnd, &rcMain);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndPageItems) / sizeof(hWndPageItems[0])); ui8i++) {
        if(hWndPageItems[ui8i] == NULL) {
            return false;
        }

        ::SendMessage(hWndPageItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }
*/
	return true;
}
//------------------------------------------------------------------------------

void MainWindowPageUsersChat::UpdateLanguage() {

}
//---------------------------------------------------------------------------

char * MainWindowPageUsersChat::GetPageName() {
    return LanguageManager->sTexts[LAN_USERS_CHAT];
}
//------------------------------------------------------------------------------

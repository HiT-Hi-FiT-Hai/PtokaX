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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GuiUtil.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../core/LanguageManager.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "Resources.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditOpenLink(const HWND &hRichEdit, const ENLINK * pEnLink) {
    TCHAR* sURL = new TCHAR[(pEnLink->chrg.cpMax - pEnLink->chrg.cpMin)+1];

    if(sURL == NULL) {
        return;
    }

    TEXTRANGE tr = { 0 };
    tr.chrg.cpMin = pEnLink->chrg.cpMin;
    tr.chrg.cpMax = pEnLink->chrg.cpMax;
    tr.lpstrText = sURL;

    ::SendMessage(hRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

    ::ShellExecute(NULL, NULL, sURL, NULL, NULL, SW_SHOWNORMAL);

    delete[] sURL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditPopupMenu(const HWND &hRichEdit, const HWND &hParent, const LPARAM &lParam) {
    HMENU hMenu = ::CreatePopupMenu();

    ::AppendMenu(hMenu, MF_STRING, IDC_COPY, LanguageManager->sTexts[LAN_MENU_COPY]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    ::AppendMenu(hMenu, MF_STRING, IDC_SELECT_ALL, LanguageManager->sTexts[LAN_MENU_SELECT_ALL]);
    ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    ::AppendMenu(hMenu, MF_STRING, IDC_CLEAR_ALL, LanguageManager->sTexts[LAN_CLEAR_ALL]);

    ::SetMenuDefaultItem(hMenu, IDC_CLEAR_ALL, FALSE);

    int iX = GET_X_LPARAM(lParam);
    int iY = GET_Y_LPARAM(lParam);

    // -1, -1 is menu created by key. This need few tricks to show menu on correct position ;o)
    if(iX == -1 && iY == -1) {
        CHARRANGE cr = { 0, 0 };
        ::SendMessage(hRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);

        POINT pt = { 0 };
        ::SendMessage(hRichEdit, EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)cr.cpMax);

        RECT rcChat;
        ::GetClientRect(hRichEdit, &rcChat);

        if(pt.y < rcChat.top) {
            pt.y = rcChat.top;
        } else if(pt.y > rcChat.bottom) {
            pt.y = rcChat.bottom;
        }

        ::ClientToScreen(hRichEdit, &pt);
        iX = pt.x;
        iY = pt.y;
    }

    ::TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, iX, iY, hParent, NULL);

    ::DestroyMenu(hMenu);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool RichEditCheckMenuCommands(const HWND &hRichEdit, const WORD &wID) {
    switch(wID) {
        case IDC_COPY:
            ::SendMessage(hRichEdit, WM_COPY, 0, 0);
            return true;
        case IDC_SELECT_ALL: {
            CHARRANGE cr = { 0, -1 };
            ::SendMessage(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
            return true;
        }
        case IDC_CLEAR_ALL:
            ::SetWindowText(hRichEdit, "");
            return true;
        default:
            return false;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RichEditAppendText(const HWND &hRichEdit, const char * sText) {
    time_t acc_time = time(NULL);
    struct tm *tm = localtime(&acc_time);

    char msg[13];
    strftime(msg, 13, "\n[%H:%M:%S] ", tm);

    CHARRANGE cr = { 0, 0 };
    ::SendMessage(hRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    LONG lOldSelBegin = cr.cpMin;
    LONG lOldSelEnd = cr.cpMax;

    GETTEXTLENGTHEX gtle = { 0 };
    gtle.codepage = CP_ACP;
    gtle.flags = GTL_NUMCHARS;
    cr.cpMin = cr.cpMax = (LONG)::SendMessage(hRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtle, 0);
    ::SendMessage(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

    ::SendMessage(hRichEdit, EM_REPLACESEL, FALSE, ::GetWindowTextLength(hRichEdit) == 0 ? (LPARAM)msg+1 : (LPARAM)msg);

    ::SendMessage(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)sText);

    cr.cpMin = lOldSelBegin;
    cr.cpMax = lOldSelEnd;
    ::SendMessage(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

    ::SendMessage(hRichEdit, WM_VSCROLL, SB_BOTTOM, NULL);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

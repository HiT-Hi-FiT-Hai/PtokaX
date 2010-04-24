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
#include "SettingPage.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
static ATOM atomSettingPage = 0;
//---------------------------------------------------------------------------

SettingPage::SettingPage() {
    m_hWnd = NULL;

    bCreated = false;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK SettingPage::StaticSettingPageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SettingPage * pSettingPage = (SettingPage *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pSettingPage == NULL) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pSettingPage->SettingPageProc(uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

void SettingPage::CreateHWND(HWND hOwner) {
    if(atomSettingPage == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_SettingPage";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomSettingPage = ::RegisterClassEx(&m_wc);
    }

    m_hWnd = ::CreateWindowEx(0, MAKEINTATOM(atomSettingPage), "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        160, 0, 452, 426, hOwner, NULL, g_hInstance, NULL);

    if(m_hWnd != NULL) {
        bCreated = true;

        ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
        ::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)StaticSettingPageProc);
    }
}
//---------------------------------------------------------------------------

void SettingPage::RemovePipes(HWND hWnd) {//RemovePipes((HWND)lParam);
    char buf[257];
    ::GetWindowText(hWnd, buf, 257);

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

        ::SendMessage(hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

        ::SetWindowText(hWnd, buf);

        ::SendMessage(hWnd, EM_SETSEL, iStart, iEnd);
    }
}
//---------------------------------------------------------------------------

void SettingPage::MinOneMaxShort(HWND hWnd) {//MinOneMaxShort((HWND)lParam);
    char buf[6];
    ::GetWindowText(hWnd, buf, 6);

    int iValue = atoi(buf);

    int iStart, iEnd;

    ::SendMessage(hWnd, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

    if(iValue > 32767) {
        ::SetWindowText(hWnd, "32767");
    } else if(iValue == 0) {
        ::SetWindowText(hWnd, "1");
    }

    ::SendMessage(hWnd, EM_SETSEL, iStart, iEnd);
}
//---------------------------------------------------------------------------

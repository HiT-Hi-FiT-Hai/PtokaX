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
#include "LineDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "Resources.h"
//---------------------------------------------------------------------------
static ATOM atomLineDialog = 0;
//---------------------------------------------------------------------------
void (*pOnOk)(const char * Line, const int &iLen) = NULL;
//---------------------------------------------------------------------------

LineDialog::LineDialog(void (*pOnOkFunction)(const char * Line, const int &iLen)) {
    m_hWnd = NULL;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    pOnOk = pOnOkFunction;
}
//---------------------------------------------------------------------------

LineDialog::~LineDialog() {
    pOnOk = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK LineDialog::StaticLineDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LineDialog * pLineDialog = (LineDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if(pLineDialog == NULL) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return pLineDialog->LineDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT LineDialog::LineDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_SETFOCUS:
            ::SetFocus(hWndWindowItems[EDT_LINE]);

            return 0;
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDOK: {
                    int iLen = ::GetWindowTextLength(hWndWindowItems[EDT_LINE]);
                    if(iLen != 0) {
                        char * sBuf = new char[iLen + 1];
                        ::GetWindowText(hWndWindowItems[EDT_LINE], sBuf, iLen + 1);
                        (*pOnOk)(sBuf, iLen);
                        delete [] sBuf;
                    }
                }
                case IDCANCEL:
                    ::EnableWindow(::GetParent(m_hWnd), TRUE);
                    g_hWndActiveDialog = NULL;
                    ::DestroyWindow(m_hWnd);
					return 0;
            }

            break;
        case WM_CLOSE:
            ::EnableWindow(::GetParent(m_hWnd), TRUE);
            g_hWndActiveDialog = NULL;
            break;
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void LineDialog::DoModal(HWND hWndParent, char * Caption, char * Line) {
    if(atomLineDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_LineDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomLineDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2))-153;
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2))-50;

    m_hWnd = ::CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomLineDialog), (string(Caption) + ":").c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX, iY, 306, 100, hWndParent, NULL, g_hInstance, NULL);

    if(m_hWnd == NULL) {
        return;
    }

    g_hWndActiveDialog = m_hWnd;

    ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)StaticLineDialogProc);

    hWndWindowItems[GB_LINE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 6, 0, 288, 42,
        m_hWnd, NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_LINE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, Line, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        14, 14, 272, 20, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_LINE], EM_SETSEL, 0, -1);

    hWndWindowItems[BTN_OK] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        5, 47, 143, 20, m_hWnd, (HMENU)IDOK, g_hInstance, NULL);

    hWndWindowItems[BTN_CANCEL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        152, 47, 143, 20, m_hWnd, (HMENU)IDCANCEL, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(m_hWnd, SW_SHOW);
}
//---------------------------------------------------------------------------

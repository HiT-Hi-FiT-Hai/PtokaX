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

LineDialog::LineDialog(char * Caption, char * Line) {
    m_hWnd = NULL;

    btnOK = btnCancel = NULL;

    gbLine = NULL;

    edtLine = NULL;

    sCaption = string(Caption) + ":";
    sLine = Line;
}
//---------------------------------------------------------------------------

LineDialog::~LineDialog() {

}
//---------------------------------------------------------------------------

INT_PTR CALLBACK LineDialog::StaticLineDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LineDialog * pParent;

	if(uMsg == WM_INITDIALOG) {
		pParent = (LineDialog *)lParam;
		::SetWindowLongPtr(hWndDlg, GWLP_USERDATA, (LONG_PTR)pParent);

        pParent->m_hWnd = hWndDlg;
	} else {
		pParent = (LineDialog *)::GetWindowLongPtr(hWndDlg, GWLP_USERDATA);
		if(pParent == NULL) {
			return FALSE;
		}
	}

    SetWindowLongPtr(hWndDlg, DWLP_MSGRESULT, 0);

	return pParent->LineDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT LineDialog::LineDialogProc(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) {
    switch(uMsg) {
        case WM_INITDIALOG: {
            ::SetWindowText(m_hWnd, sCaption.c_str());

            RECT rcMain;
            ::GetClientRect(m_hWnd, &rcMain);

            gbLine = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 6, 0, (rcMain.right - rcMain.left)-12, 32,
                m_hWnd, NULL, g_hInstance, NULL);

            edtLine = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                10, 10, (rcMain.right - rcMain.left)-20, 18, m_hWnd, NULL, g_hInstance, NULL);
            ::SetWindowText(edtLine, sLine.c_str());
            ::SendMessage(edtLine, EM_SETSEL, 0, -1);

            btnOK = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                5, 37, ((rcMain.right - rcMain.left)/2)-7, 20, m_hWnd, (HMENU)IDOK, g_hInstance, NULL);

            btnCancel = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                ((rcMain.right - rcMain.left)/2)+2, 37, ((rcMain.right - rcMain.left)/2)-7, 20, m_hWnd, (HMENU)IDCANCEL, g_hInstance, NULL);

            HWND hWnds[] = { btnOK, btnCancel, gbLine, edtLine };

            for(uint8_t ui8i = 0; ui8i < (sizeof(hWnds) / sizeof(hWnds[0])); ui8i++) {
                ::SendMessage(hWnds[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
            }

            RECT rcParent;
            ::GetWindowRect(::GetParent(m_hWnd), &rcParent);

            ::SetWindowPos(m_hWnd, NULL, rcParent.left, rcParent.top+86, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            ::PostMessage(m_hWnd, WM_SETFOCUS, 0, 0);

            return TRUE;
        }
        case WM_SETFOCUS:
            ::SetFocus(edtLine);

            return TRUE;
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDOK: {
                    int iLen = ::GetWindowTextLength(edtLine) + 1;
                    char * sBuf = new char[iLen];
                    ::GetWindowText(edtLine, sBuf, iLen);
                    sLine = sBuf;
                    delete [] sBuf;
                }
                case IDCANCEL:
                    ::EndDialog(m_hWnd, LOWORD(wParam));
                    return TRUE;
            }

            break;
    }

	return FALSE;
}
//------------------------------------------------------------------------------

INT_PTR LineDialog::DoModal(HWND hWndParent) {
	return ::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_LINE_DIALOG), hWndParent, StaticLineDialogProc, (LPARAM)this);
}
//---------------------------------------------------------------------------

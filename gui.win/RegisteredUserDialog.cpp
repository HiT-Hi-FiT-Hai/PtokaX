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
#include "RegisteredUserDialog.h"
//---------------------------------------------------------------------------
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/ProfileManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "GuiUtil.h"
//---------------------------------------------------------------------------
RegisteredUserDialog * pRegisteredUserDialog = NULL;
//---------------------------------------------------------------------------
static ATOM atomRegisteredUserDialog = 0;
//---------------------------------------------------------------------------

RegisteredUserDialog::RegisteredUserDialog() {
    pRegisteredUserDialog = this;

    m_hWnd = NULL;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    pRegToChange = NULL;
}
//---------------------------------------------------------------------------

RegisteredUserDialog::~RegisteredUserDialog() {
    pRegisteredUserDialog = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK RegisteredUserDialog::StaticRegisteredUserDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RegisteredUserDialog * pRegisteredUserDialog = (RegisteredUserDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pRegisteredUserDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pRegisteredUserDialog->RegisteredUserDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT RegisteredUserDialog::RegisteredUserDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case BTN_ACCEPT:
                    if(OnAccept() == false) {
                        return 0;
                    }
                case BTN_DISCARD:
                    ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                    return 0;
                case EDT_NICK:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        char buf[65];
                        ::GetWindowText((HWND)lParam, buf, 65);

                        bool bChanged = false;

                        for(uint16_t ui16i = 0; buf[ui16i] != '\0'; ui16i++) {
                            if(buf[ui16i] == '|' || buf[ui16i] == '$' || buf[ui16i] == ' ') {
                                strcpy(buf+ui16i, buf+ui16i+1);
                                bChanged = true;
                                ui16i--;
                            }
                        }

                        if(bChanged == true) {
                            int iStart, iEnd;

                            ::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

                            ::SetWindowText((HWND)lParam, buf);

                            ::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
                        }

                        return 0;
                    }

                    break;
                case EDT_PASSWORD:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        char buf[65];
                        ::GetWindowText((HWND)lParam, buf, 65);

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

                            ::SendMessage((HWND)lParam, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);

                            ::SetWindowText((HWND)lParam, buf);

                            ::SendMessage((HWND)lParam, EM_SETSEL, iStart, iEnd);
                        }

                        return 0;
                    }

                    break;
            }

            break;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = 300;
            mminfo->ptMinTrackSize.y = 185;

            return 0;
        }
        case WM_CLOSE:
            ::EnableWindow(::GetParent(m_hWnd), TRUE);
            break;
        case WM_NCDESTROY:
            delete this;
            return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::DoModal(HWND hWndParent, RegUser * pReg/* = NULL*/, char * sNick/* = NULL*/) {
    pRegToChange = pReg;

    if(atomRegisteredUserDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_RegisteredUserDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomRegisteredUserDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2))-150;
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2))-93;

    m_hWnd = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomRegisteredUserDialog), LanguageManager->sTexts[LAN_REGISTERED_USER],
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, 300, 185,
        hWndParent, NULL, g_hInstance, NULL);

    if(m_hWnd == NULL) {
        return;
    }

    ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)StaticRegisteredUserDialogProc);

    ::GetClientRect(m_hWnd, &rcParent);

    hWndWindowItems[GB_NICK] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_NICK], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 1, (rcParent.right-rcParent.left)-6, 41, m_hWnd, NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_NICK] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, 16, (rcParent.right-rcParent.left)-22, 18, m_hWnd, (HMENU)EDT_NICK, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_NICK], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[GB_PASSWORD] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PASSWORD], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 42, (rcParent.right-rcParent.left)-6, 41, m_hWnd, NULL, g_hInstance, NULL);

    hWndWindowItems[EDT_PASSWORD] = ::CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        11, 57, (rcParent.right-rcParent.left)-22, 18, m_hWnd, (HMENU)EDT_PASSWORD, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[EDT_PASSWORD], EM_SETLIMITTEXT, 64, 0);

    hWndWindowItems[GB_PROFILE] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, LanguageManager->sTexts[LAN_PROFILE], WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        3, 83, (rcParent.right-rcParent.left)-6, 44, m_hWnd, NULL, g_hInstance, NULL);

    hWndWindowItems[CB_PROFILE] = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
        11, 98, (rcParent.right-rcParent.left)-22, 21, m_hWnd, (HMENU)CB_PROFILE, g_hInstance, NULL);

    hWndWindowItems[BTN_ACCEPT] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_ACCEPT], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        2, 132, ((rcParent.right-rcParent.left)/2)-3, 23, m_hWnd, (HMENU)BTN_ACCEPT, g_hInstance, NULL);

    hWndWindowItems[BTN_DISCARD] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_DISCARD], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        ((rcParent.right-rcParent.left)/2)+2, 132, ((rcParent.right-rcParent.left)/2)-4, 23, m_hWnd, (HMENU)BTN_DISCARD, g_hInstance, NULL);

    for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
        if(hWndWindowItems[ui8i] == NULL) {
            return;
        }

        ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
    }

    UpdateProfiles();

    if(pRegToChange != NULL) {
        ::SetWindowText(hWndWindowItems[EDT_NICK], pRegToChange->sNick);
        ::EnableWindow(hWndWindowItems[EDT_NICK], FALSE);

        ::SetWindowText(hWndWindowItems[EDT_PASSWORD], pRegToChange->sPass);

        ::SendMessage(hWndWindowItems[CB_PROFILE], CB_SETCURSEL, pRegToChange->iProfile, 0);
    } else if(sNick != NULL) {
        ::SetWindowText(hWndWindowItems[EDT_NICK], sNick);
    }

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(m_hWnd, SW_SHOW);
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::UpdateProfiles() {
    int iSel = (int)::SendMessage(hWndWindowItems[CB_PROFILE], CB_GETCURSEL, 0, 0);

    for(uint16_t ui16i = 0; ui16i < ProfileMan->iProfileCount; ui16i++) {
        ::SendMessage(hWndWindowItems[CB_PROFILE], CB_ADDSTRING, 0, (LPARAM)ProfileMan->ProfilesTable[ui16i]->sName);
    }

    if(pRegToChange != NULL) {
        ::SendMessage(hWndWindowItems[CB_PROFILE], CB_SETCURSEL, pRegToChange->iProfile, 0);
    } else {
        iSel = (int)::SendMessage(hWndWindowItems[CB_PROFILE], CB_SETCURSEL, iSel, 0);

        if(iSel == CB_ERR) {
            ::SendMessage(hWndWindowItems[CB_PROFILE], CB_SETCURSEL, 0, 0);
        }
    }
}
//------------------------------------------------------------------------------

bool RegisteredUserDialog::OnAccept() {
    if(::GetWindowTextLength(hWndWindowItems[EDT_NICK]) == 0) {
        ::MessageBox(m_hWnd, LanguageManager->sTexts[LAN_NICK_MUST_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    if(::GetWindowTextLength(hWndWindowItems[EDT_PASSWORD]) == 0) {
        ::MessageBox(m_hWnd, LanguageManager->sTexts[LAN_PASS_MUST_SPECIFIED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    char sNick[65], sPassword[65];

    ::GetWindowText(hWndWindowItems[EDT_NICK], sNick, 65);
    ::GetWindowText(hWndWindowItems[EDT_PASSWORD], sPassword, 65);

    uint16_t ui16Profile = (uint16_t)::SendMessage(hWndWindowItems[CB_PROFILE], CB_GETCURSEL, 0, 0);

    if(pRegToChange == NULL) {
        if(hashRegManager->AddNew(sNick, sPassword, ui16Profile) == false) {
            ::MessageBox(m_hWnd, LanguageManager->sTexts[LAN_USER_IS_ALREDY_REG], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
            return false;
        }

        return true;
    } else {
        hashRegManager->ChangeReg(pRegToChange, sPassword, ui16Profile);
        return true;
    }
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::RegChanged(RegUser * pReg) {
    if(pRegToChange == NULL || pReg != pRegToChange) {
        return;
    }

    ::SetWindowText(hWndWindowItems[EDT_PASSWORD], pRegToChange->sPass);

    ::SendMessage(hWndWindowItems[CB_PROFILE], CB_SETCURSEL, pRegToChange->iProfile, 0);

    ::MessageBox(m_hWnd, LanguageManager->sTexts[LAN_USER_CHANGED], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------

void RegisteredUserDialog::RegDeleted(RegUser * pReg) {
    if(pRegToChange == NULL || pReg != pRegToChange) {
        return;
    }

    ::MessageBox(m_hWnd, LanguageManager->sTexts[LAN_USER_DELETED_ACCEPT_TO_NEW], sTitle.c_str(), MB_OK | MB_ICONEXCLAMATION);
}
//------------------------------------------------------------------------------

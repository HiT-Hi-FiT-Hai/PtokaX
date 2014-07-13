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

//---------------------------------------------------------------------------
#include "../core/stdinc.h"
//---------------------------------------------------------------------------
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "UpdateDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
//---------------------------------------------------------------------------
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
clsUpdateDialog * clsUpdateDialog::mPtr = NULL;
//---------------------------------------------------------------------------
static ATOM atomUpdateDialog = 0;
//---------------------------------------------------------------------------

clsUpdateDialog::clsUpdateDialog() {
    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));
}
//---------------------------------------------------------------------------

clsUpdateDialog::~clsUpdateDialog() {
    clsUpdateDialog::mPtr = NULL;
}
//---------------------------------------------------------------------------

LRESULT CALLBACK clsUpdateDialog::StaticUpdateDialogProc(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return clsUpdateDialog::mPtr->UpdateDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT clsUpdateDialog::UpdateDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[REDT_UPDATE] && ((LPNMHDR)lParam)->code == EN_LINK) {
                if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
                    RichEditOpenLink(hWndWindowItems[REDT_UPDATE], (ENLINK *)lParam);
                }
            }

            break;
        case WM_CLOSE:
            ::EnableWindow(::GetParent(hWndWindowItems[WINDOW_HANDLE]), TRUE);
            clsServerManager::hWndActiveDialog = NULL;
            break;
        case WM_NCDESTROY: {
            HWND hWnd = hWndWindowItems[WINDOW_HANDLE];
            delete this;
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    ::PostMessage(hWndWindowItems[WINDOW_HANDLE], WM_CLOSE, 0, 0);
					return 0;
            }

            break;
        case WM_SETFOCUS:
            ::SetFocus(hWndWindowItems[REDT_UPDATE]);
            return 0;
    }

	return ::DefWindowProc(hWndWindowItems[WINDOW_HANDLE], uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

void clsUpdateDialog::DoModal(HWND hWndParent) {
    if(atomUpdateDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_UpdateDialog";
        m_wc.hInstance = clsServerManager::hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomUpdateDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2)) - (ScaleGui(500) / 2);
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2)) - (ScaleGui(460) / 2);

    hWndWindowItems[WINDOW_HANDLE] = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomUpdateDialog), clsLanguageManager::mPtr->sTexts[LAN_CHECKING_FOR_UPDATE],
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, ScaleGui(500), ScaleGui(460),
        hWndParent, NULL, clsServerManager::hInstance, NULL);

    if(hWndWindowItems[WINDOW_HANDLE] == NULL) {
        return;
    }

    clsServerManager::hWndActiveDialog = hWndWindowItems[WINDOW_HANDLE];

    ::SetWindowLongPtr(hWndWindowItems[WINDOW_HANDLE], GWLP_WNDPROC, (LONG_PTR)StaticUpdateDialogProc);

    ::GetClientRect(hWndWindowItems[WINDOW_HANDLE], &rcParent);

    hWndWindowItems[REDT_UPDATE] = ::CreateWindowEx(WS_EX_CLIENTEDGE, /*MSFTEDIT_CLASS*/RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
        5, 5, rcParent.right - 10, rcParent.bottom - 10, hWndWindowItems[WINDOW_HANDLE], NULL, clsServerManager::hInstance, NULL);
    ::SendMessage(hWndWindowItems[REDT_UPDATE], EM_SETBKGNDCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
    ::SendMessage(hWndWindowItems[REDT_UPDATE], EM_AUTOURLDETECT, TRUE, 0);
    ::SendMessage(hWndWindowItems[REDT_UPDATE], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(hWndWindowItems[REDT_UPDATE], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
    ::SendMessage(hWndWindowItems[REDT_UPDATE], WM_SETFONT, (WPARAM)clsGuiSettingManager::hFont, MAKELPARAM(TRUE, 0));

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(hWndWindowItems[WINDOW_HANDLE], SW_SHOW);
}
//---------------------------------------------------------------------------

void clsUpdateDialog::Message(char * sData) {
    RichEditAppendText(hWndWindowItems[REDT_UPDATE], sData);
}
//---------------------------------------------------------------------------
bool clsUpdateDialog::ParseData(char * sData, HWND hWndParent) {
    char * sVersion = NULL;
    char * sBuildNumber = NULL;
    char * sReleaseDate = NULL;
    char * sChanges = NULL;

    char * sBegin = sData;
    char * sTemp = strchr(sBegin, '=');

    size_t szLen = 0;

    while(sTemp != NULL) {
        sTemp[0] = '\0';
        szLen = sTemp - sBegin;

        if(szLen == 7) {
            if(strcmp(sBegin, "Version") == 0) {
                sVersion = sTemp + 1;
            } else if(strcmp(sBegin, "Release") == 0) {
                sReleaseDate = sTemp + 1;
            } else if(strcmp(sBegin, "Changes") == 0) {
                sChanges = sTemp + 1;

                sTemp = strstr(sTemp + 1, "TestingVersion=");
                if(sTemp != NULL) {
                    sTemp[0] = '\0';
                }

                break;
            }
        } else if(szLen == 5 && strcmp(sBegin, "Build") == 0) {
            sBuildNumber = sTemp + 1;
        }

        sTemp = strchr(sTemp + 1, '\n');
        if(sTemp == NULL) {
            break;
        }

        sTemp[0] = '\0';

        sBegin = sTemp + 1;
        
        sTemp--;

        if(sTemp[0] == '\r') {
            sTemp[0] = '\0';
        }

        sTemp = strchr(sBegin, '=');
    }

    if(sVersion == NULL || sBuildNumber == NULL || sReleaseDate == NULL || sChanges == NULL) {
        RichEditAppendText(hWndWindowItems[REDT_UPDATE], clsLanguageManager::mPtr->sTexts[LAN_UPDATE_CHECK_FAILED]);
        return false;
    }

    uint64_t ui64BuildNumber = _strtoui64(BUILD_NUMBER, NULL, 10);
    uint64_t ui64NewBuildNumber = _strtoui64(sBuildNumber, NULL, 10);

    if(ui64NewBuildNumber > ui64BuildNumber) {
        string sMsg = string(clsLanguageManager::mPtr->sTexts[LAN_NEW_VERSION], clsLanguageManager::mPtr->ui16TextsLens[LAN_NEW_VERSION]) + " " + sVersion + " [build: " + sBuildNumber + "] " +
            clsLanguageManager::mPtr->sTexts[LAN_RELEASED_ON] + " " + sReleaseDate + " " + clsLanguageManager::mPtr->sTexts[LAN_IS_AVAILABLE] + ".\r\n\r\n" + clsLanguageManager::mPtr->sTexts[LAN_CHANGES] + ":\r\n" +
            sChanges;

        if(hWndWindowItems[WINDOW_HANDLE] == NULL) {
            DoModal(hWndParent);
            RichEditAppendText(hWndWindowItems[REDT_UPDATE], sMsg.c_str(), false);
        } else {
            RichEditAppendText(hWndWindowItems[REDT_UPDATE], sMsg.c_str());
        }

        return true;
    }

    if(hWndWindowItems[WINDOW_HANDLE] != NULL) {
        RichEditAppendText(hWndWindowItems[REDT_UPDATE], clsLanguageManager::mPtr->sTexts[LAN_SORRY_NO_NEW_VERSION_AVAILABLE]);
    }

    return false;
}
//---------------------------------------------------------------------------

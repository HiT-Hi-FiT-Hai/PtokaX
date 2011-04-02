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
#include "../core/LuaInc.h"
//---------------------------------------------------------------------------
#include "AboutDialog.h"
//---------------------------------------------------------------------------
#include "../core/LanguageManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "GuiUtil.h"
#include "Resources.h"
//---------------------------------------------------------------------------
static ATOM atomAboutDialog = 0;
//---------------------------------------------------------------------------

AboutDialog::AboutDialog() {
    m_hWnd = NULL;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    hiSpider = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDR_MAINICONBIG), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    hiLua = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDR_LUAICON), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

    LOGFONT lfFont;
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfFont);

    if(lfFont.lfHeight > 0) {
        lfFont.lfHeight = 20;
    } else {
        lfFont.lfHeight = -20;
    }

    lfFont.lfWeight = FW_BOLD;

    hfBigFont = ::CreateFontIndirect(&lfFont);
}
//---------------------------------------------------------------------------

AboutDialog::~AboutDialog() {
    ::DeleteObject(hiSpider);
    ::DeleteObject(hiLua);

    ::DeleteObject(hfBigFont);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK AboutDialog::StaticAboutDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AboutDialog * pAboutDialog = (AboutDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if(pAboutDialog == NULL) {
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

	return pAboutDialog->AboutDialogProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT AboutDialog::AboutDialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(m_hWnd, &ps);
            ::DrawIconEx(hdc, 5, 5, hiSpider, 0, 0, 0, NULL, DI_NORMAL);
            ::DrawIconEx(hdc, 368, 5, hiLua, 0, 0, 0, NULL, DI_NORMAL);
            ::EndPaint(m_hWnd, &ps);
            return 0;
        }
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[REDT_ABOUT] && ((LPNMHDR)lParam)->code == EN_LINK) {
                if(((ENLINK *)lParam)->msg == WM_LBUTTONUP) {
                    RichEditOpenLink(hWndWindowItems[REDT_ABOUT], (ENLINK *)lParam);
                }
            }
            break;
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

void AboutDialog::DoModal(HWND hWndParent) {
    if(atomAboutDialog == 0) {
        WNDCLASSEX m_wc;
        memset(&m_wc, 0, sizeof(WNDCLASSEX));
        m_wc.cbSize = sizeof(WNDCLASSEX);
        m_wc.lpfnWndProc = ::DefWindowProc;
        m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        m_wc.lpszClassName = "PtokaX_AboutDialog";
        m_wc.hInstance = g_hInstance;
        m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
        m_wc.style = CS_HREDRAW | CS_VREDRAW;

        atomAboutDialog = ::RegisterClassEx(&m_wc);
    }

    RECT rcParent;
    ::GetWindowRect(hWndParent, &rcParent);

    int iX = (rcParent.left + (((rcParent.right-rcParent.left))/2))-221;
    int iY = (rcParent.top + ((rcParent.bottom-rcParent.top)/2))-227;

    m_hWnd = ::CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, MAKEINTATOM(atomAboutDialog),
        (string(LanguageManager->sTexts[LAN_ABOUT], (size_t)LanguageManager->ui16TextsLens[LAN_ABOUT]) + " PtokaX").c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, iX >= 5 ? iX : 5, iY >= 5 ? iY : 5, 443, 454,
        hWndParent, NULL, g_hInstance, NULL);

    if(m_hWnd == NULL) {
        return;
    }

    ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
    ::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)StaticAboutDialogProc);

    hWndWindowItems[LBL_PTOKAX_VERSION] = ::CreateWindowEx(0, WC_STATIC, "PtokaX" PtokaXVersionString " [build " BUILD_NUMBER "]", WS_CHILD | WS_VISIBLE | SS_CENTER,
        73, 10, 290, 25, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[LBL_PTOKAX_VERSION], WM_SETFONT, (WPARAM)hfBigFont, MAKELPARAM(TRUE, 0));

    hWndWindowItems[LBL_LUA_VERSION] = ::CreateWindowEx(0, WC_STATIC, LUA_RELEASE, WS_CHILD | WS_VISIBLE | SS_CENTER, 73, 39, 290, 25,
        m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[LBL_LUA_VERSION], WM_SETFONT, (WPARAM)hfBigFont, MAKELPARAM(TRUE, 0));

    hWndWindowItems[REDT_ABOUT] = ::CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_CENTER | ES_READONLY,
        5, 74, 427, 347, m_hWnd, NULL, g_hInstance, NULL);
    ::SendMessage(hWndWindowItems[REDT_ABOUT], EM_SETBKGNDCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
    ::SendMessage(hWndWindowItems[REDT_ABOUT], EM_AUTOURLDETECT, TRUE, 0);
    ::SendMessage(hWndWindowItems[REDT_ABOUT], EM_SETEVENTMASK, 0, (LPARAM)::SendMessage(hWndWindowItems[REDT_ABOUT], EM_GETEVENTMASK, 0, 0) | ENM_LINK);
    ::SendMessage(hWndWindowItems[REDT_ABOUT], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));

	::SendMessage(hWndWindowItems[REDT_ABOUT], EM_REPLACESEL, FALSE, (LPARAM)"{\\rtf1\\ansi\\ansicpg1250{\\colortbl ;\\red0\\green0\\blue128;}"
    "PtokaX is a server-software for the Direct Connect P2P Network.\\par\n"
    "It's currently in developing by PPK and Ptaczek since May 2002.\\par\n"
    "\\par\n"
    "\\b Homepage:\\par\n"
    "http://www.PtokaX.org\\par\n"
    "\\par\n"
    "LUA Scripts forum:\\par\n"
    "http://board.ptokax.ch\\par\n"
    "\\par\n"
    "Wiki:\\par\n"
    "http://wiki.ptokax.ch\\par\n"
    "\\par\n"
    "Designers:\\par\n"
    "\\cf1 PPK\\cf0\\b0, PPK@PtokaX.org, ICQ: 122442343\\par\n"
    "\\b\\cf1 Ptaczek\\cf0\\b0, ptaczek@PtokaX.org, ICQ: 2294472\\par\n"
    "\\par\n"
    "\\b Respect:\\b0\\par\n"
    "aMutex, frontline3k, DirtyFinger, Yoshi, Nev.\\par\n"
	"\\par\n"
	"\\b Thanx to:\\par\n"
    "\\b\\cf1 Iceman\\cf0\\b0  for RAM and incredible and faithfull help with testing\\par\n"
    "\\b\\cf1 PPK\\cf0\\b0  for incredible ability to find hidden bugs from very beginning of PtokaX\\par\n"
    "\\b\\cf1 Beta-Team\\cf0\\b0  for quality work, ideas, suggestions and bugreports\\par\n"
    "\\b\\cf1 Opium Volage\\cf0\\b0  for LUA forum hosting\\par\n"
	"\\b\\cf1 Sechmet\\cf0\\b0  for blue PtokaX icon\\par\n"
	"\\par\n"
	"\\b This application uses the IP-to-Country Database provided by\\par\n"
    "WebHosting.Info (http://www.webhosting.info),\\par\n"
    "available from http://ip-to-country.webhosting.info.\\b0"
    "}");

    ::EnableWindow(hWndParent, FALSE);

    ::ShowWindow(m_hWnd, SW_SHOW);
}
//---------------------------------------------------------------------------

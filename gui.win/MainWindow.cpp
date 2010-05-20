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
#include "MainWindow.h"
//---------------------------------------------------------------------------
#include "../core/colUsers.h"
#include "../core/globalQueue.h"
#include "../core/hashBanManager.h"
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/LuaScriptManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/UdpDebug.h"
#include "../core/User.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "AboutDialog.h"
#include "LineDialog.h"
#include "Resources.h"
#include "SettingDialog.h"
#include "../core/TextFileManager.h"
//---------------------------------------------------------------------------
#define WM_TRAYICON (WM_USER+10)
//---------------------------------------------------------------------------
MainWindow *pMainWindow = NULL;
//---------------------------------------------------------------------------

static uint64_t (*GetActualTick)();
//---------------------------------------------------------------------------

uint64_t PXGetTickCount() {
	return (uint64_t)(::GetTickCount() / 1000);
}
//---------------------------------------------------------------------------

typedef ULONGLONG (WINAPI *GTC64)(void);
GTC64 pGTC64 = NULL;
//---------------------------------------------------------------------------

uint64_t PXGetTickCount64() {
	return (pGTC64() / 1000);
}
//---------------------------------------------------------------------------

MainWindow::MainWindow() {
	INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES |
        ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS };
	InitCommonControlsEx(&iccx);

    m_hWnd = NULL;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    uiTaskBarCreated = ::RegisterWindowMessage("TaskbarCreated");

    ui64LastTrayMouseMove = 0;

    hMainMenu = NULL;

    LOGFONT lfFont;
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfFont);

    if(lfFont.lfHeight > 12) {
        lfFont.lfHeight = 12;
    } else if(lfFont.lfHeight < -12) {
        lfFont.lfHeight = -12;
    }

    hfFont = ::CreateFontIndirect(&lfFont);

	OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(::GetVersionEx((OSVERSIONINFO*)&ver) != 0) {
        if(ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion >= 6) {
            pGTC64 = (GTC64)::GetProcAddress(::GetModuleHandle("kernel32.dll"), "GetTickCount64");
            if(pGTC64 != NULL) {
                GetActualTick = PXGetTickCount64;
                return;
            }
        }
    }

    GetActualTick = PXGetTickCount;
}
//---------------------------------------------------------------------------

MainWindow::~MainWindow() {
    NOTIFYICONDATA nid;
    memset(&nid, 0, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hWnd;
    nid.uID = 0;
    ::Shell_NotifyIcon(NIM_DELETE, &nid);
    ::DeleteObject(hfFont);

    if(hMainMenu != NULL) {
        ::DestroyMenu(hMainMenu);
    }
}
//---------------------------------------------------------------------------

LRESULT CALLBACK MainWindow::StaticMainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_NCCREATE) {
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pMainWindow);
		pMainWindow->m_hWnd = hWnd;
	} else {
        if(::GetWindowLongPtr(hWnd, GWLP_USERDATA) == NULL) {
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

	return pMainWindow->MainWindowProc(uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

LRESULT MainWindow::MainWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_CREATE: {
            RECT rcMain;
            ::GetClientRect(m_hWnd, &rcMain);

            hWndWindowItems[BTN_START_STOP] = ::CreateWindowEx(0, WC_BUTTON, bServerRunning == false ? LanguageManager->sTexts[LAN_START_HUB] : LanguageManager->sTexts[LAN_STOP_HUB],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 5, 4, (rcMain.right - rcMain.left)-10, 20, m_hWnd, (HMENU)IDC_START_STOP, g_hInstance, NULL);

            hWndWindowItems[GB_STATS] = ::CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, "", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                6, 23, (rcMain.right - rcMain.left)-12, 136, m_hWnd, NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_STATUS] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_STATUS], (size_t)LanguageManager->ui16TextsLens[LAN_STATUS])+":").c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 8, 12, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_STATUS_VALUE] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".").c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 145, 12, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_JOINS] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_ACCEPTED_CONNECTIONS], (size_t)LanguageManager->ui16TextsLens[LAN_ACCEPTED_CONNECTIONS])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 27, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_JOINS_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 27, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_PARTS] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_CLOSED_CONNECTIONS], (size_t)LanguageManager->ui16TextsLens[LAN_CLOSED_CONNECTIONS])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 42, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_PARTS_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 42, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_ACTIVE] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_ACTIVE_CONNECTIONS], (size_t)LanguageManager->ui16TextsLens[LAN_ACTIVE_CONNECTIONS])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 57, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_ACTIVE_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 57, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_ONLINE] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_USERS_ONLINE], (size_t)LanguageManager->ui16TextsLens[LAN_USERS_ONLINE])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 72, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_ONLINE_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 72, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_PEAK] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_USERS_PEAK], (size_t)LanguageManager->ui16TextsLens[LAN_USERS_PEAK])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 87, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_PEAK_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 87, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_RECEIVED] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_RECEIVED], (size_t)LanguageManager->ui16TextsLens[LAN_RECEIVED])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 102, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_RECEIVED_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0 B (0 B/s)", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 102, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_SENT] = ::CreateWindowEx(0, WC_STATIC,
                (string(LanguageManager->sTexts[LAN_SENT], (size_t)LanguageManager->ui16TextsLens[LAN_SENT])+":").c_str(),
                WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP, 8, 117, 140, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[LBL_SENT_VALUE] = ::CreateWindowEx(0, WC_STATIC, "0 B (0 B/s)", WS_CHILD | WS_VISIBLE | WS_DISABLED | SS_LEFTNOWORDWRAP,
                145, 117, (rcMain.right - rcMain.left)-161, 14, hWndWindowItems[GB_STATS], NULL, g_hInstance, NULL);

            hWndWindowItems[BTN_REDIRECT_ALL] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_REDIRECT_ALL_USERS], WS_CHILD | WS_VISIBLE | WS_DISABLED |
                WS_TABSTOP | BS_PUSHBUTTON, 5, 164, (rcMain.right - rcMain.left)-10, 20, m_hWnd, (HMENU)IDC_REDIRECT_ALL,
                g_hInstance, NULL);

            hWndWindowItems[BTN_MASS_MSG] = ::CreateWindowEx(0, WC_BUTTON, LanguageManager->sTexts[LAN_MASS_MSG], WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP |
                BS_PUSHBUTTON, 5, 188, (rcMain.right - rcMain.left)-10, 20, m_hWnd, (HMENU)IDC_MASS_MESSAGE, g_hInstance, NULL);

            for(uint8_t ui8i = 0; ui8i < (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])); ui8i++) {
                ::SendMessage(hWndWindowItems[ui8i], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));
            }

            return 0;
        }
        case WM_CLOSE:
            if(::MessageBox(m_hWnd, (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
                sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                bIsClose = true;

                // stop server if running and save settings
                if(bServerRunning == true) {
                    ServerStop();
                    return 0;
                } else {
                    ServerFinalClose();
                }

                break;
            }

            return 0;
        case WM_DESTROY:
            ::PostQuitMessage(1);
            break;
        case WM_TRAYICON:
            if(lParam == WM_MOUSEMOVE) {
                if(ui64LastTrayMouseMove != GetActualTick()) {
                    ui64LastTrayMouseMove = GetActualTick();

                    NOTIFYICONDATA nid;
                    memset(&nid, 0, sizeof(NOTIFYICONDATA));
                    nid.cbSize = sizeof(NOTIFYICONDATA);
                    nid.hWnd = m_hWnd;
                    nid.uID = 0;
                    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
                    nid.hIcon = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
                    nid.uCallbackMessage = WM_TRAYICON;

					char msg[256];
					int iret = 0;
                    if(bServerRunning == false) {
						msg[0] = '\0';
                    } else {
                        iret = sprintf(msg, " (%s: %I32d)", LanguageManager->sTexts[LAN_USERS], ui32Logged);
                        if(CheckSprintf(iret, 1024, "MainWindow::MainWindowProc") == false) {
                            return 0;
                        }
					}

					size_t iSize = sizeof(nid.szTip);

					if(iSize < (size_t)(SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iret+10)) {
						nid.szTip[iSize] = '\0';
						memcpy(nid.szTip+(iSize-(iret+1)), msg, iret);
						nid.szTip[iSize-(iret+2)] = '.';
						nid.szTip[iSize-(iret+3)] = '.';
						nid.szTip[iSize-(iret+4)] = '.';
						memcpy(nid.szTip+9, SettingManager->sTexts[SETTXT_HUB_NAME], iSize-(iret+13));
						memcpy(nid.szTip, "PtokaX - ", 9);
					} else {
						memcpy(nid.szTip, "PtokaX - ", 9);
						memcpy(nid.szTip+9, SettingManager->sTexts[SETTXT_HUB_NAME], SettingManager->ui16TextsLens[SETTXT_HUB_NAME]);
						memcpy(nid.szTip+9+SettingManager->ui16TextsLens[SETTXT_HUB_NAME], msg, iret);
						nid.szTip[9+SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iret] = '\0';
                    }
                    ::Shell_NotifyIcon(NIM_MODIFY, &nid);
				}
            } else if(lParam == WM_LBUTTONUP) {
                ::ShowWindow(m_hWnd, SW_SHOW);
                ::ShowWindow(m_hWnd, SW_RESTORE);
            }

            return 0;
        case WM_SIZE:
            if(wParam == SIZE_MINIMIZED && SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true) {
                ::ShowWindow(m_hWnd, SW_HIDE);
            }
            return 0;
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDC_EXIT:
                    ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                    return 0;
                case IDC_SETTINGS: {
                    SettingDialog * SettingDlg = new SettingDialog();
                    SettingDlg->DoModal(m_hWnd);

                    return 0;
                }
                case IDC_REG_USERS:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_PROF_MAN:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_BANS:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_RANGE_BANS:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_SCRIPTS:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_SCRIPTS_MEM:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_USERS_CHAT:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_ABOUT: {
                    AboutDialog * AboutDlg = new AboutDialog();
                    AboutDlg->DoModal(m_hWnd);

                    return 0;
                }
                case IDC_HOMEPAGE:
                    ::ShellExecute(NULL, NULL, "http://www.PtokaX.org", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                case IDC_BOARD:
                    ::ShellExecute(NULL, NULL, "http://board.ptokax.ch", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                case IDC_WIKI:
                    ::ShellExecute(NULL, NULL, "http://wiki.ptokax.ch", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                case IDC_UPDATE_CHECK:
                    ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                    return 0;
                case IDC_SAVE_SETTINGS:
                    hashBanManager->Save(true);
                    ProfileMan->SaveProfiles();
                    hashRegManager->Save();
                    ScriptManager->SaveScripts();
                    SettingManager->Save();

                    ::MessageBox(m_hWnd, LanguageManager->sTexts[LAN_SETTINGS_SAVED], sTitle.c_str(), MB_OK | MB_ICONINFORMATION);
                    return 0;
                case IDC_RELOAD_TXTS:
                    TextFileManager->RefreshTextFiles();

                    ::MessageBox(m_hWnd,
                        (string(LanguageManager->sTexts[LAN_TXT_FILES_RELOADED], (size_t)LanguageManager->ui16TextsLens[LAN_TXT_FILES_RELOADED])+".").c_str(),
                            sTitle.c_str(), MB_OK | MB_ICONINFORMATION);
                    return 0;
                case IDC_START_STOP:
                    if(bServerRunning == false) {
                        if(ServerStart() == false) {
                            ::SetWindowText(hWndWindowItems[LBL_STATUS_VALUE],
                                (string(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".").c_str());
                        }
                    } else {
                        ServerStop();
                    }
                    return 0;
                case IDC_REDIRECT_ALL:
                    OnRedirectAll();
                    return 0;
                case IDC_MASS_MESSAGE:
                    OnMassMessage();
                    return 0;
            }

            break;
        case WM_QUERYENDSESSION:
            hashBanManager->Save(true);
            ProfileMan->SaveProfiles();
            hashRegManager->Save();
            ScriptManager->SaveScripts();
            SettingManager->Save();

            AppendLog("Serving stopped (Windows shutdown).");

            return TRUE;
    }

	if(uMsg == uiTaskBarCreated) {
		UpdateSysTray();
		return 0;
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

HWND MainWindow::CreateEx() {
    hMainMenu = ::CreateMenu();

    HMENU hFileMenu = ::CreateMenu();
    ::AppendMenu(hFileMenu, MF_STRING, IDC_RELOAD_TXTS, (string(LanguageManager->sTexts[LAN_RELOAD_TEXT_FILES], (size_t)LanguageManager->ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
    ::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hFileMenu, MF_STRING, IDC_SETTINGS, (string(LanguageManager->sTexts[LAN_MENU_SETTINGS], (size_t)LanguageManager->ui16TextsLens[LAN_SETTINGS]) + "...").c_str());
    ::AppendMenu(hFileMenu, MF_STRING, IDC_SAVE_SETTINGS, (string(LanguageManager->sTexts[LAN_SAVE_SETTINGS], (size_t)LanguageManager->ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
    ::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hFileMenu, MF_STRING, IDC_EXIT, LanguageManager->sTexts[LAN_EXIT]);

    ::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hFileMenu, LanguageManager->sTexts[LAN_FILE]);

    HMENU hViewMenu = ::CreateMenu();
    ::AppendMenu(hViewMenu, MF_STRING, IDC_REG_USERS, LanguageManager->sTexts[LAN_REG_USERS]);
    ::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_PROF_MAN, LanguageManager->sTexts[LAN_PROFILE_MAN]);
    ::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_BANS, LanguageManager->sTexts[LAN_BANS]);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_RANGE_BANS, LanguageManager->sTexts[LAN_RANGE_BANS]);
    ::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_SCRIPTS, LanguageManager->sTexts[LAN_SCRIPTS]);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_SCRIPTS_MEM, LanguageManager->sTexts[LAN_SCRIPTS_MEMORY_USAGE]);
    ::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_USERS_CHAT, LanguageManager->sTexts[LAN_USERS_CHAT]);

    ::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hViewMenu, LanguageManager->sTexts[LAN_VIEW]);

    HMENU hHelpMenu = ::CreateMenu();
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_UPDATE_CHECK, (string(LanguageManager->sTexts[LAN_CHECK_FOR_UPDATE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
    ::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_HOMEPAGE, (string("PtokaX ") +LanguageManager->sTexts[LAN_WEBSITE]).c_str());
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_BOARD, (string("PtokaX ") +LanguageManager->sTexts[LAN_BOARD]).c_str());
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_WIKI, (string("PtokaX ") +LanguageManager->sTexts[LAN_WIKI]).c_str());
    ::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_ABOUT, (string(LanguageManager->sTexts[LAN_MENU_ABOUT], (size_t)LanguageManager->ui16TextsLens[LAN_MENU_ABOUT]) + " PtokaX").c_str());

    ::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hHelpMenu, LanguageManager->sTexts[LAN_HELP]);

    WNDCLASSEX m_wc;
    memset(&m_wc, 0, sizeof(WNDCLASSEX));
    m_wc.cbSize = sizeof(WNDCLASSEX);
    m_wc.lpfnWndProc = StaticMainWindowProc;
    m_wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    m_wc.lpszClassName = sTitle.c_str();
    m_wc.hInstance = g_hInstance;
	m_wc.hCursor = ::LoadCursor(m_wc.hInstance, IDC_ARROW);
	m_wc.style = CS_HREDRAW | CS_VREDRAW;
	m_wc.hIcon = (HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(IDR_MAINICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_SHARED);
	m_wc.hIconSm = (HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);

	ATOM atom = ::RegisterClassEx(&m_wc);

    m_hWnd = ::CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT, MAKEINTATOM(atom),
        (string(SettingManager->sTexts[SETTXT_HUB_NAME], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME]) + " | " + sTitle).c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 306, 261, NULL, hMainMenu, g_hInstance, NULL);

	return m_hWnd;
}
//---------------------------------------------------------------------------

void MainWindow::UpdateSysTray() {
    if(bCmdNoTray == false) {
        NOTIFYICONDATA nid;
        memset(&nid, 0, sizeof(NOTIFYICONDATA));
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = m_hWnd;
        nid.uID = 0;
        nid.uFlags = NIF_ICON | NIF_MESSAGE;
        nid.hIcon = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDR_MAINICONSMALL), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);
        nid.uCallbackMessage = WM_TRAYICON;

        if(SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true) {
            ::Shell_NotifyIcon(NIM_ADD, &nid);
        } else {
            ::Shell_NotifyIcon(NIM_DELETE, &nid);
        }
    }
}
//---------------------------------------------------------------------------

void MainWindow::UpdateStats() {
    ::SetWindowText(hWndWindowItems[LBL_JOINS_VALUE], string(ui32Joins).c_str());
    ::SetWindowText(hWndWindowItems[LBL_PARTS_VALUE], string(ui32Parts).c_str());
    ::SetWindowText(hWndWindowItems[LBL_ACTIVE_VALUE], string(ui32Joins - ui32Parts).c_str());
    ::SetWindowText(hWndWindowItems[LBL_ONLINE_VALUE], string(ui32Logged).c_str());
    ::SetWindowText(hWndWindowItems[LBL_PEAK_VALUE], string(ui32Peak).c_str());

    char msg[256];

	int imsglen = sprintf(msg, "%s (%s)", formatBytes(ui64BytesRead), formatBytesPerSecond(iActualBytesRead));
    if(CheckSprintf(imsglen, 256, "MainWindow::UpdateStats") == true) {
        ::SetWindowText(hWndWindowItems[LBL_RECEIVED_VALUE], msg);
	}

	imsglen = sprintf(msg, "%s (%s)", formatBytes(ui64BytesSent), formatBytesPerSecond(iActualBytesSent));
    if(CheckSprintf(imsglen, 256, "MainWindow::UpdateStats1") == true) {
		::SetWindowText(hWndWindowItems[LBL_SENT_VALUE], msg);
	}
}
//---------------------------------------------------------------------------

void MainWindow::UpdateTitleBar() {
    ::SetWindowText(m_hWnd, (string(SettingManager->sTexts[SETTXT_HUB_NAME], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME]) + " | " + sTitle).c_str());
}
//---------------------------------------------------------------------------

void MainWindow::UpdateLanguage() {
	if(bServerRunning == false) {
        ::SetWindowText(hWndWindowItems[BTN_START_STOP], LanguageManager->sTexts[LAN_START_HUB]);
        ::SetWindowText(hWndWindowItems[LBL_STATUS_VALUE], (string(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".").c_str());
    } else {
        ::SetWindowText(hWndWindowItems[BTN_START_STOP], LanguageManager->sTexts[LAN_STOP_HUB]);
        ::SetWindowText(hWndWindowItems[LBL_STATUS_VALUE], (string(LanguageManager->sTexts[LAN_RUNNING], (size_t)LanguageManager->ui16TextsLens[LAN_RUNNING])+"...").c_str());
    }

    ::SetWindowText(hWndWindowItems[LBL_STATUS], (string(LanguageManager->sTexts[LAN_STATUS], (size_t)LanguageManager->ui16TextsLens[LAN_STATUS])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_JOINS], (string(LanguageManager->sTexts[LAN_ACCEPTED_CONNECTIONS], (size_t)LanguageManager->ui16TextsLens[LAN_ACCEPTED_CONNECTIONS])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_PARTS], (string(LanguageManager->sTexts[LAN_CLOSED_CONNECTIONS], (size_t)LanguageManager->ui16TextsLens[LAN_CLOSED_CONNECTIONS])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_ACTIVE], (string(LanguageManager->sTexts[LAN_ACTIVE_CONNECTIONS], (size_t)LanguageManager->ui16TextsLens[LAN_ACTIVE_CONNECTIONS])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_ONLINE], (string(LanguageManager->sTexts[LAN_USERS_ONLINE], (size_t)LanguageManager->ui16TextsLens[LAN_USERS_ONLINE])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_PEAK], (string(LanguageManager->sTexts[LAN_USERS_PEAK], (size_t)LanguageManager->ui16TextsLens[LAN_USERS_PEAK])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_RECEIVED], (string(LanguageManager->sTexts[LAN_RECEIVED], (size_t)LanguageManager->ui16TextsLens[LAN_RECEIVED])+":").c_str());
    ::SetWindowText(hWndWindowItems[LBL_SENT], (string(LanguageManager->sTexts[LAN_SENT], (size_t)LanguageManager->ui16TextsLens[LAN_SENT])+":").c_str());

    ::SetWindowText(hWndWindowItems[BTN_REDIRECT_ALL], LanguageManager->sTexts[LAN_REDIRECT_ALL_USERS]);
    ::SetWindowText(hWndWindowItems[BTN_MASS_MSG], LanguageManager->sTexts[LAN_MASS_MSG]);

    HMENU hMenu = ::GetMenu(m_hWnd);

    ::ModifyMenu(hMenu, 0, MF_BYPOSITION, 0, LanguageManager->sTexts[LAN_FILE]);
    ::ModifyMenu(hMenu, 1, MF_BYPOSITION, 0, LanguageManager->sTexts[LAN_VIEW]);
    ::ModifyMenu(hMenu, 2, MF_BYPOSITION, 0, LanguageManager->sTexts[LAN_HELP]);

    ::ModifyMenu(hMenu, IDC_EXIT, MF_BYCOMMAND, IDC_EXIT, LanguageManager->sTexts[LAN_EXIT]);
    ::ModifyMenu(hMenu, IDC_REG_USERS, MF_BYCOMMAND, IDC_REG_USERS, LanguageManager->sTexts[LAN_REG_USERS]);
    ::ModifyMenu(hMenu, IDC_PROF_MAN, MF_BYCOMMAND, IDC_PROF_MAN, LanguageManager->sTexts[LAN_PROFILE_MAN]);
    ::ModifyMenu(hMenu, IDC_BANS, MF_BYCOMMAND, IDC_BANS, LanguageManager->sTexts[LAN_BANS]);
    ::ModifyMenu(hMenu, IDC_RANGE_BANS, MF_BYCOMMAND, IDC_RANGE_BANS, LanguageManager->sTexts[LAN_RANGE_BANS]);
    ::ModifyMenu(hMenu, IDC_SCRIPTS, MF_BYCOMMAND, IDC_SCRIPTS, LanguageManager->sTexts[LAN_SCRIPTS]);
    ::ModifyMenu(hMenu, IDC_SCRIPTS_MEM, MF_BYCOMMAND, IDC_SCRIPTS_MEM, LanguageManager->sTexts[LAN_SCRIPTS_MEMORY_USAGE]);
    ::ModifyMenu(hMenu, IDC_USERS_CHAT, MF_BYCOMMAND, IDC_USERS_CHAT, LanguageManager->sTexts[LAN_USERS_CHAT]);

    ::ModifyMenu(hMenu, IDC_HOMEPAGE, MF_BYCOMMAND, IDC_HOMEPAGE, (string("PtokaX ") +LanguageManager->sTexts[LAN_WEBSITE]).c_str());
    ::ModifyMenu(hMenu, IDC_BOARD, MF_BYCOMMAND, IDC_BOARD, (string("PtokaX ") +LanguageManager->sTexts[LAN_BOARD]).c_str());
    ::ModifyMenu(hMenu, IDC_WIKI, MF_BYCOMMAND, IDC_WIKI, (string("PtokaX ") +LanguageManager->sTexts[LAN_WIKI]).c_str());

    ::ModifyMenu(hMenu, IDC_ABOUT, MF_BYCOMMAND, IDC_ABOUT,
        (string(LanguageManager->sTexts[LAN_MENU_ABOUT], (size_t)LanguageManager->ui16TextsLens[LAN_MENU_ABOUT]) + " PtokaX").c_str());

    ::ModifyMenu(hMenu, IDC_UPDATE_CHECK, MF_BYCOMMAND, IDC_UPDATE_CHECK,
        (string(LanguageManager->sTexts[LAN_CHECK_FOR_UPDATE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
    ::ModifyMenu(hMenu, IDC_RELOAD_TXTS, MF_BYCOMMAND, IDC_RELOAD_TXTS,
        (string(LanguageManager->sTexts[LAN_RELOAD_TEXT_FILES], (size_t)LanguageManager->ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
    ::ModifyMenu(hMenu, IDC_SETTINGS, MF_BYCOMMAND, IDC_SETTINGS,
        (string(LanguageManager->sTexts[LAN_MENU_SETTINGS], (size_t)LanguageManager->ui16TextsLens[LAN_MENU_SETTINGS]) + "...").c_str());
    ::ModifyMenu(hMenu, IDC_SAVE_SETTINGS, MF_BYCOMMAND, IDC_SAVE_SETTINGS,
        (string(LanguageManager->sTexts[LAN_SAVE_SETTINGS], (size_t)LanguageManager->ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
}
//---------------------------------------------------------------------------

void OnMassMessageOk(const char * Line, const int &iLen) {
    char *sMSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+256);
    if(sMSG == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iLen+256)+
			" bytes of memory for sMSG in MainWindow::OnMassMessage! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
		AppendSpecialLog(sDbgstr);
        return;
    }

    int imsgLen = sprintf(sMSG, "%s $<%s> %s|",
        SettingManager->bBools[SETBOOL_REG_BOT] == false ? SettingManager->sTexts[SETTXT_ADMIN_NICK] : SettingManager->sTexts[SETTXT_BOT_NICK],
        SettingManager->bBools[SETBOOL_REG_BOT] == false ? SettingManager->sTexts[SETTXT_ADMIN_NICK] : SettingManager->sTexts[SETTXT_BOT_NICK],
        Line);
    if(CheckSprintf(imsgLen, iLen+256, "MainWindow::OnMassMessage1") == false) {
        return;
    }

    QueueDataItem * newItem = globalQ->CreateQueueDataItem(sMSG, imsgLen, NULL, 0, globalqueue::PM2ALL);

    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMSG) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sMSG in MainWindow::OnMassMessage! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
	}

    globalQ->SingleItemsStore(newItem);
}

//---------------------------------------------------------------------------

void MainWindow::OnMassMessage() {
	LineDialog * MassMsgDlg = new LineDialog(&OnMassMessageOk);
	MassMsgDlg->DoModal(m_hWnd, LanguageManager->sTexts[LAN_MASS_MSG], "");
}
//---------------------------------------------------------------------------

void OnRedirectAllOk(const char * Line, const int &iLen) {
    char *sMSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iLen+16);
    if(sMSG == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(iLen+16)+
			" bytes of memory for sMSG in MainWindow::OnRedirectAll! "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
		AppendSpecialLog(sDbgstr);
        return;
    }

    int imsgLen = sprintf(sMSG, "$ForceMove %s|", Line);
    if(CheckSprintf(imsgLen, iLen+16, "MainWindow::OnRedirectAll") == false) {
        return;
    }

    User *next = colUsers->llist;
    while(next != NULL) {
        User *curUser = next;
		next = curUser->next;
        UserSendChar(curUser, sMSG, imsgLen);
        // PPK ... close by hub needed !
        UserClose(curUser, true);
    }

    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMSG) == 0) {
		string sDbgstr = "[BUF] Cannot deallocate sMSG in MainWindow::OnRedirectAll! "+string((uint32_t)GetLastError())+" "+
			string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
		AppendSpecialLog(sDbgstr);
	}
}
//---------------------------------------------------------------------------

void MainWindow::OnRedirectAll() {
    UdpDebug->Broadcast("[SYS] Redirect All.");

	LineDialog * RedirectAllDlg = new LineDialog(&OnRedirectAllOk);
	RedirectAllDlg->DoModal(m_hWnd, LanguageManager->sTexts[LAN_REDIRECT_ALL_USERS_TO],
        SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS] == NULL ? "" : SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS]);
}
//---------------------------------------------------------------------------

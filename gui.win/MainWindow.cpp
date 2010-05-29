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
#include "../core/hashBanManager.h"
#include "../core/hashRegManager.h"
#include "../core/LanguageManager.h"
#include "../core/LuaScriptManager.h"
#include "../core/ProfileManager.h"
#include "../core/ServerManager.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "AboutDialog.h"
#include "MainWindowPageStats.h"
#include "MainWindowPageUsersChat.h"
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

    memset(&MainWindowPages, 0, 2 * sizeof(MainWindowPage *));

    MainWindowPages[0] = new MainWindowPageStats();
    MainWindowPages[1] = new MainWindowPageUsersChat();

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
    for(uint8_t ui8i = 0; ui8i < 2; ui8i++) {
        if(MainWindowPages[ui8i] != NULL) {
            delete MainWindowPages[ui8i];
        }
    }

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

            DWORD dwTabsStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_TABS | TCS_FOCUSNEVER;

            BOOL bHotTrackEnabled = FALSE;
            ::SystemParametersInfo(SPI_GETHOTTRACKING, 0, &bHotTrackEnabled, 0);

            if(bHotTrackEnabled == TRUE) {
                dwTabsStyle |= TCS_HOTTRACK;
            }

            hWndWindowItems[TC_TABS] = ::CreateWindowEx(0, WC_TABCONTROL, "", /*WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_TABS | TCS_FOCUSNEVER | TCS_HOTTRACK*/dwTabsStyle,
                0, 0, rcMain.right, 21, m_hWnd, NULL, g_hInstance, NULL);
            ::SendMessage(hWndWindowItems[TC_TABS], WM_SETFONT, (WPARAM)hfFont, MAKELPARAM(TRUE, 0));

            TCITEM tcItem = { 0 };
            tcItem.mask = TCIF_TEXT | TCIF_PARAM;

            for(uint8_t ui8i = 0; ui8i < 2; ui8i++) {
                if(MainWindowPages[ui8i] != NULL) {
                    if(MainWindowPages[ui8i]->CreateMainWindowPage(m_hWnd) == false) {
                        ::MessageBox(m_hWnd, "Main window creation failed!", sTitle.c_str(), MB_OK);
                    }

                    tcItem.pszText = MainWindowPages[ui8i]->GetPageName();
                    tcItem.lParam = (LPARAM)MainWindowPages[ui8i];
                     ::SendMessage(hWndWindowItems[TC_TABS], TCM_INSERTITEM, ui8i, (LPARAM)&tcItem);
                }
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
            } else {
                RECT rc;
                ::SetRect(&rc, 0, 0, GET_X_LPARAM(lParam), 21);
                ::SendMessage(hWndWindowItems[TC_TABS], TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);

                HDWP hdwp = ::BeginDeferWindowPos(3);

                ::DeferWindowPos(hdwp, hWndWindowItems[TC_TABS], NULL, 0, 0, GET_X_LPARAM(lParam), 21, SWP_NOMOVE | SWP_NOZORDER);

                for(uint8_t ui8i = 0; ui8i < 2; ui8i++) {
                    if(MainWindowPages[ui8i] != NULL) {
                        ::DeferWindowPos(hdwp, MainWindowPages[ui8i]->m_hWnd, NULL, 0, 22, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)-22, SWP_NOMOVE | SWP_NOZORDER);
                    }
                }

                ::EndDeferWindowPos(hdwp);
            }

            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = 400;
            mminfo->ptMinTrackSize.y = 321;

            return 0;
        }
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[TC_TABS]) {
                if(((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
                    OnSelChanged();
                    return 0;
                }
            }

            break;
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
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 321, NULL, hMainMenu, g_hInstance, NULL);

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
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_JOINS_VALUE], string(ui32Joins).c_str());
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_PARTS_VALUE], string(ui32Parts).c_str());
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_ACTIVE_VALUE], string(ui32Joins - ui32Parts).c_str());
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_ONLINE_VALUE], string(ui32Logged).c_str());
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_PEAK_VALUE], string(ui32Peak).c_str());

    char msg[256];

	int imsglen = sprintf(msg, "%s (%s)", formatBytes(ui64BytesRead), formatBytesPerSecond(iActualBytesRead));
    if(CheckSprintf(imsglen, 256, "MainWindow::UpdateStats") == true) {
        ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_RECEIVED_VALUE], msg);
	}

	imsglen = sprintf(msg, "%s (%s)", formatBytes(ui64BytesSent), formatBytesPerSecond(iActualBytesSent));
    if(CheckSprintf(imsglen, 256, "MainWindow::UpdateStats1") == true) {
		::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_SENT_VALUE], msg);
	}
}
//---------------------------------------------------------------------------

void MainWindow::UpdateTitleBar() {
    ::SetWindowText(m_hWnd, (string(SettingManager->sTexts[SETTXT_HUB_NAME], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME]) + " | " + sTitle).c_str());
}
//---------------------------------------------------------------------------

void MainWindow::UpdateLanguage() {
    for(uint8_t ui8i = 0; ui8i < 2; ui8i++) {
        if(MainWindowPages[ui8i] != NULL) {
            MainWindowPages[ui8i]->UpdateLanguage();

            TCITEM tcItem = { 0 };
            tcItem.mask = TCIF_TEXT;

            tcItem.pszText = MainWindowPages[ui8i]->GetPageName();
            ::SendMessage(hWndWindowItems[TC_TABS], TCM_SETITEM, ui8i, (LPARAM)&tcItem);
        }
    }

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

void MainWindow::EnableStartButton(const BOOL &bEnable) {
    ::EnableWindow(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::BTN_START_STOP], bEnable);
}
//---------------------------------------------------------------------------

void MainWindow::SetStartButtonText(const char * sText) {
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::BTN_START_STOP], sText);
}
//---------------------------------------------------------------------------

void MainWindow::SetStatusValue(const char * sText) {
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_STATUS_VALUE], sText);
}
//---------------------------------------------------------------------------

void MainWindow::EnableGuiItems(const BOOL &bEnable) {
    for(uint8_t ui8i = 2; ui8i < 20; ui8i++) {
        ::EnableWindow(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[ui8i], bEnable);
    }

    if(bEnable == FALSE || ::SendMessage(((MainWindowPageUsersChat *)MainWindowPages[1])->hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        ::EnableWindow(((MainWindowPageUsersChat *)MainWindowPages[1])->hWndPageItems[MainWindowPageUsersChat::BTN_UPDATE_USERS], FALSE);
    } else {
        ::EnableWindow(((MainWindowPageUsersChat *)MainWindowPages[1])->hWndPageItems[MainWindowPageUsersChat::BTN_UPDATE_USERS], TRUE);
    }

    if(bEnable == FALSE) {
        ::SendMessage(((MainWindowPageUsersChat *)MainWindowPages[1])->hWndPageItems[MainWindowPageUsersChat::LV_USERS], LVM_DELETEALLITEMS, 0, 0);
    }
}
//---------------------------------------------------------------------------

void MainWindow::OnSelChanged() {
    int iPage = (int)::SendMessage(hWndWindowItems[TC_TABS], TCM_GETCURSEL, 0, 0);

    if(iPage == -1) {
        return;
    }

    TCITEM tcItem = { 0 };
    tcItem.mask = TCIF_PARAM;

	if((BOOL)::SendMessage(hWndWindowItems[TC_TABS], TCM_GETITEM, iPage, (LPARAM)&tcItem) == FALSE) {
        return;
    }

    if(tcItem.lParam == NULL) {
        ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
    }

    ::BringWindowToTop(((SettingPage *)tcItem.lParam)->m_hWnd);
}
//---------------------------------------------------------------------------

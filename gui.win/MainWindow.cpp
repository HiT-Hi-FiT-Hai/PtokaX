/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2012  Petr Kozelka, PPK at PtokaX dot org

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
#include "GuiSettingManager.h"
#include "GuiUtil.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#include "AboutDialog.h"
#include "BansDialog.h"
#include "MainWindowPageScripts.h"
#include "MainWindowPageStats.h"
#include "MainWindowPageUsersChat.h"
#include "ProfilesDialog.h"
#include "RangeBansDialog.h"
#include "RegisteredUsersDialog.h"
#include "Resources.h"
#include "SettingDialog.h"
#include "UpdateDialog.h"
#include "../core/TextFileManager.h"
#include "../core/UpdateCheckThread.h"
//---------------------------------------------------------------------------
#define WM_TRAYICON (WM_USER+10)
//---------------------------------------------------------------------------
MainWindow * pMainWindow = NULL;
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
    g_GuiSettingManager = new GuiSettingManager();

    if(g_GuiSettingManager == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate g_GuiSettingManager in MainWindow::MainWindow\n", 0);
        exit(EXIT_FAILURE);
    }

	INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES |
        ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS };
	InitCommonControlsEx(&iccx);

    m_hWnd = NULL;

    memset(&hWndWindowItems, 0, (sizeof(hWndWindowItems) / sizeof(hWndWindowItems[0])) * sizeof(HWND));

    memset(&MainWindowPages, 0, (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])) * sizeof(MainWindowPage *));

    MainWindowPages[0] = new MainWindowPageStats();
    MainWindowPages[1] = new MainWindowPageUsersChat();
    MainWindowPages[2] = new MainWindowPageScripts();

    for(uint8_t ui8i = 0; ui8i < 3; ui8i++) {
        if(MainWindowPages[ui8i] == NULL) {
            AppendDebugLog("%s - [MEM] Cannot allocate MainWindowPage[%" PRIu64 "] in MainWindow::MainWindow\n", (uint64_t)ui8i);
            exit(EXIT_FAILURE);
        }
    }

    uiTaskBarCreated = ::RegisterWindowMessage("TaskbarCreated");

    ui64LastTrayMouseMove = 0;

	NONCLIENTMETRICS NCM = { 0 };
	NCM.cbSize = sizeof(NONCLIENTMETRICS);

    OSVERSIONINFOEX ver;
	memset(&ver, 0, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if(::GetVersionEx((OSVERSIONINFO*)&ver) != 0 && ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion < 6) {
        NCM.cbSize -= sizeof(int);
    }

	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NCM, 0);

    if(NCM.lfMessageFont.lfHeight > 0) {
        fScaleFactor = (float)(NCM.lfMessageFont.lfHeight / 12.0);
    } else if(NCM.lfMessageFont.lfHeight < 0) {
        fScaleFactor = float(NCM.lfMessageFont.lfHeight / -12.0);
    }

    hFont = ::CreateFontIndirect(&NCM.lfMessageFont);

    hArrowCursor = (HCURSOR)::LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
    hVerticalCursor = (HCURSOR)::LoadImage(NULL, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);

	if(::GetVersionEx((OSVERSIONINFO*)&ver) != 0 && ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion >= 6) {
        pGTC64 = (GTC64)::GetProcAddress(::GetModuleHandle("kernel32.dll"), "GetTickCount64");
        if(pGTC64 != NULL) {
            GetActualTick = PXGetTickCount64;
            return;
        }
    }

    GetActualTick = PXGetTickCount;
}
//---------------------------------------------------------------------------

MainWindow::~MainWindow() {
    delete g_GuiSettingManager;

    for(uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++) {
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
    ::DeleteObject(hFont);
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
            {
                HWND hCombo = ::CreateWindowEx(0, WC_COMBOBOX, "", WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, NULL,
                    g_hInstance, NULL);

                if(hCombo != NULL) {
                    ::SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

                    iGroupBoxMargin = (int)::SendMessage(hCombo, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
                    iEditHeight = iGroupBoxMargin + (::GetSystemMetrics(SM_CXFIXEDFRAME) * 2);
                    iTextHeight = iGroupBoxMargin - 2;
                    iCheckHeight = max(iTextHeight - 2, ::GetSystemMetrics(SM_CYSMICON));

                    ::DestroyWindow(hCombo);
                }

                HWND hUpDown = ::CreateWindowEx(0, UPDOWN_CLASS, "", WS_CHILD | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, iEditHeight, m_hWnd, NULL, g_hInstance, NULL);

                if(hUpDown != NULL) {
                    RECT rcUpDown;
                    ::GetWindowRect(hUpDown, &rcUpDown);

                    iUpDownWidth = rcUpDown.right - rcUpDown.left;

                    ::DestroyWindow(hUpDown);
                }

                iOneLineGB = iGroupBoxMargin + iEditHeight + 8;
                iOneLineOneChecksGB = iGroupBoxMargin + iCheckHeight + iEditHeight + 12;
                iOneLineTwoChecksGB = iGroupBoxMargin + (2 * iCheckHeight) + iEditHeight + 15;

                SettingPage::iOneCheckGB = iGroupBoxMargin + iCheckHeight + 8;
                SettingPage::iTwoChecksGB = iGroupBoxMargin + (2 * iCheckHeight) + 3 + 8;
                SettingPage::iOneLineTwoGroupGB = iGroupBoxMargin + iEditHeight + (2 * iOneLineGB) + 7;
                SettingPage::iTwoLineGB = iGroupBoxMargin  + (2 * iEditHeight) + 13;
                SettingPage::iThreeLineGB = iGroupBoxMargin + (3 * iEditHeight) + 18;
            }

            RECT rcMain;
            ::GetClientRect(m_hWnd, &rcMain);

            DWORD dwTabsStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP | TCS_TABS | TCS_FOCUSNEVER;

            BOOL bHotTrackEnabled = FALSE;
            ::SystemParametersInfo(SPI_GETHOTTRACKING, 0, &bHotTrackEnabled, 0);

            if(bHotTrackEnabled == TRUE) {
                dwTabsStyle |= TCS_HOTTRACK;
            }

            hWndWindowItems[TC_TABS] = ::CreateWindowEx(0, WC_TABCONTROL, "", dwTabsStyle, 0, 0, rcMain.right, iEditHeight, m_hWnd, NULL, g_hInstance, NULL);
            ::SendMessage(hWndWindowItems[TC_TABS], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

            TCITEM tcItem = { 0 };
            tcItem.mask = TCIF_TEXT | TCIF_PARAM;

            for(uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++) {
                if(MainWindowPages[ui8i] != NULL) {
                    if(MainWindowPages[ui8i]->CreateMainWindowPage(m_hWnd) == false) {
                        ::MessageBox(m_hWnd, "Main window creation failed!", sTitle.c_str(), MB_OK);
                    }

                    tcItem.pszText = MainWindowPages[ui8i]->GetPageName();
                    tcItem.lParam = (LPARAM)MainWindowPages[ui8i];
                    ::SendMessage(hWndWindowItems[TC_TABS], TCM_INSERTITEM, ui8i, (LPARAM)&tcItem);
                }

                if(ui8i == 0 && g_GuiSettingManager->iIntegers[GUISETINT_MAIN_WINDOW_HEIGHT] == g_GuiSettingManager->GetDefaultInteger(GUISETINT_MAIN_WINDOW_HEIGHT)) {
                    RECT rcPage = { 0 };
                    ::GetWindowRect(MainWindowPages[0]->m_hWnd, &rcPage);

                    int iDiff = (rcMain.bottom) - (rcPage.bottom-rcPage.top) - (iEditHeight + 1);

                    ::GetWindowRect(m_hWnd, &rcMain);

                    if(iDiff != 0) {
                        ::SetWindowPos(m_hWnd, NULL, 0, 0, (rcMain.right-rcMain.left), (rcMain.bottom-rcMain.top) - iDiff, SWP_NOMOVE | SWP_NOZORDER);
                    }
                }
            }

            wpOldTabsProc = (WNDPROC)::SetWindowLongPtr(hWndWindowItems[TC_TABS], GWLP_WNDPROC, (LONG_PTR)TabsProc);

            if(SettingManager->bBools[SETBOOL_CHECK_NEW_RELEASES] == true) {
                // Create update check thread
                pUpdateCheckThread = new UpdateCheckThread();
                if(pUpdateCheckThread == NULL) {
                    AppendDebugLog("%s - [MEM] Cannot allocate UpdateCheckThread in MainWindow::MainWindowProc::WM_CREATE\n", 0);
                    exit(EXIT_FAILURE);
                }

                // Start the update check thread
                pUpdateCheckThread->Resume();
            }

            return 0;
        }
        case WM_CLOSE:
            if(::MessageBox(m_hWnd, (string(LanguageManager->sTexts[LAN_ARE_YOU_SURE], (size_t)LanguageManager->ui16TextsLens[LAN_ARE_YOU_SURE])+" ?").c_str(),
                sTitle.c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                bIsClose = true;

                RECT rcMain;
                ::GetWindowRect(m_hWnd, &rcMain);

                g_GuiSettingManager->SetInteger(GUISETINT_MAIN_WINDOW_WIDTH, rcMain.right - rcMain.left);
                g_GuiSettingManager->SetInteger(GUISETINT_MAIN_WINDOW_HEIGHT, rcMain.bottom - rcMain.top);

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
                        iret = sprintf(msg, " (%s: %u)", LanguageManager->sTexts[LAN_USERS], ui32Logged);
                        if(CheckSprintf(iret, 256, "MainWindow::MainWindowProc") == false) {
                            return 0;
                        }
					}

					size_t szSize = sizeof(nid.szTip);

					if(szSize < (size_t)(SettingManager->ui16TextsLens[SETTXT_HUB_NAME]+iret+10)) {
						nid.szTip[szSize] = '\0';
						memcpy(nid.szTip+(szSize-(iret+1)), msg, iret);
						nid.szTip[szSize-(iret+2)] = '.';
						nid.szTip[szSize-(iret+3)] = '.';
						nid.szTip[szSize-(iret+4)] = '.';
						memcpy(nid.szTip+9, SettingManager->sTexts[SETTXT_HUB_NAME], szSize-(iret+13));
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
                ::SetRect(&rc, 0, 0, GET_X_LPARAM(lParam), iEditHeight);
                ::SendMessage(hWndWindowItems[TC_TABS], TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);

                HDWP hdwp = ::BeginDeferWindowPos(3);

                ::DeferWindowPos(hdwp, hWndWindowItems[TC_TABS], NULL, 0, 0, GET_X_LPARAM(lParam), iEditHeight, SWP_NOMOVE | SWP_NOZORDER);

                for(uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++) {
                    if(MainWindowPages[ui8i] != NULL) {
                        ::DeferWindowPos(hdwp, MainWindowPages[ui8i]->m_hWnd, NULL, 0, iEditHeight + 1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) - (iEditHeight + 1),
                            SWP_NOMOVE | SWP_NOZORDER);
                    }
                }

                ::EndDeferWindowPos(hdwp);
            }

            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = ScaleGui(g_GuiSettingManager->GetDefaultInteger(GUISETINT_MAIN_WINDOW_WIDTH));
            mminfo->ptMinTrackSize.y = ScaleGui(g_GuiSettingManager->GetDefaultInteger(GUISETINT_MAIN_WINDOW_HEIGHT));

            return 0;
        }
        case WM_NOTIFY:
            if(((LPNMHDR)lParam)->hwndFrom == hWndWindowItems[TC_TABS]) {
                if(((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
                    OnSelChanged();
                    return 0;
                } else if(((LPNMHDR)lParam)->code == TCN_KEYDOWN ) {
                    NMTCKEYDOWN * ptckd = (NMTCKEYDOWN *)lParam;
                    if(ptckd->wVKey == VK_TAB) {
                        int iPage = (int)::SendMessage(hWndWindowItems[TC_TABS], TCM_GETCURSEL, 0, 0);

                        if(iPage == -1) {
                            break;
                        }

                        TCITEM tcItem = { 0 };
                        tcItem.mask = TCIF_PARAM;

                        if((BOOL)::SendMessage(hWndWindowItems[TC_TABS], TCM_GETITEM, iPage, (LPARAM)&tcItem) == FALSE) {
                            break;
                        }

                        if(tcItem.lParam == NULL) {
                            ::MessageBox(m_hWnd, "Not implemented!", sTitle.c_str(), MB_OK);
                        }

                        MainWindowPage * curMainWindowPage = (MainWindowPage *)tcItem.lParam;

                        if((::GetKeyState(VK_SHIFT) & 0x8000) > 0) {
                            curMainWindowPage->FocusLastItem();
                            return 0;
                        } else {
                            curMainWindowPage->FocusFirstItem();
                            return 0;
                        }
                    }
                }
            }

            break;
        case WM_COMMAND:
           switch(LOWORD(wParam)) {
                case IDC_EXIT:
                    ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                    return 0;
                case IDC_SETTINGS: {
                    pSettingDialog = new SettingDialog();

                    if(pSettingDialog != NULL) {
                        pSettingDialog->DoModal(m_hWnd);
                    }

                    return 0;
                }
                case IDC_REG_USERS: {
                    pRegisteredUsersDialog = new RegisteredUsersDialog();

                    if(pRegisteredUsersDialog != NULL) {
                        pRegisteredUsersDialog->DoModal(m_hWnd);
                    }

                    return 0;
                }
                case IDC_PROFILES: {
                    pProfilesDialog = new ProfilesDialog();

                    if(pProfilesDialog != NULL) {
                        pProfilesDialog->DoModal(m_hWnd);
                    }

                    return 0;
                }
                case IDC_BANS: {
                    pBansDialog = new BansDialog();

                    if(pBansDialog != NULL) {
                        pBansDialog->DoModal(m_hWnd);
                    }

                    return 0;
				}
                case IDC_RANGE_BANS: {
                    pRangeBansDialog = new RangeBansDialog();

                    if(pRangeBansDialog != NULL) {
                        pRangeBansDialog->DoModal(m_hWnd);
                    }

                    return 0;
                }
                case IDC_ABOUT: {
                    AboutDialog * pAboutDlg = new AboutDialog();

                    if(pAboutDlg != NULL) {
                        pAboutDlg->DoModal(m_hWnd);
                    }

                    return 0;
                }
                case IDC_HOMEPAGE:
                    ::ShellExecute(NULL, NULL, "http://www.PtokaX.org", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                case IDC_FORUM:
                    ::ShellExecute(NULL, NULL, "http://forum.PtokaX.org/", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                case IDC_WIKI:
                    ::ShellExecute(NULL, NULL, "http://wiki.PtokaX.org", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                case IDC_UPDATE_CHECK: {
                    pUpdateDialog = new UpdateDialog();

                    if(pUpdateDialog != NULL) {
                        pUpdateDialog->DoModal(m_hWnd);

                        // First destroy old update check thread if any
                        if(pUpdateCheckThread != NULL) {
                            pUpdateCheckThread->Close();
                            pUpdateCheckThread->WaitFor();

                            delete pUpdateCheckThread;
                            pUpdateCheckThread = NULL;
                        }

                        // Create update check thread
                        pUpdateCheckThread = new UpdateCheckThread();
                        if(pUpdateCheckThread == NULL) {
                            AppendDebugLog("%s - [MEM] Cannot allocate UpdateCheckThread in MainWindow::MainWindowProc::IDC_UPDATE_CHECK\n", 0);
                            exit(EXIT_FAILURE);
                        }

                        // Start the update check thread
                        pUpdateCheckThread->Resume();
                    }

                    return 0;
                }
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
        case WM_UPDATE_CHECK_MSG: {
            char * sMsg = (char *)lParam;
            if(pUpdateDialog != NULL) {
                pUpdateDialog->Message(sMsg);
            }

            free(sMsg);

            return 0;
        }
        case WM_UPDATE_CHECK_DATA: {
            char * sMsg = (char *)lParam;

            if(pUpdateDialog == NULL) {
                pUpdateDialog = new UpdateDialog();

                if(pUpdateDialog != NULL) {
                    if(pUpdateDialog->ParseData(sMsg, m_hWnd) == false) {
                        delete pUpdateDialog;
                    }
                }
            } else {
                pUpdateDialog->ParseData(sMsg, m_hWnd);
            }

            free(sMsg);

            return 0;
        }
        case WM_UPDATE_CHECK_TERMINATE:
            if(pUpdateCheckThread != NULL) {
                pUpdateCheckThread->Close();
                pUpdateCheckThread->WaitFor();

                delete pUpdateCheckThread;
                pUpdateCheckThread = NULL;
            }

            return 0;
        case WM_SETFOCUS:
            ::SetFocus(hWndWindowItems[TC_TABS]);
            return 0;
    }

	if(uMsg == uiTaskBarCreated) {
		UpdateSysTray();
		return 0;
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
//------------------------------------------------------------------------------

HWND MainWindow::CreateEx() {
    HMENU hMainMenu = ::CreateMenu();

    HMENU hFileMenu = ::CreatePopupMenu();
    ::AppendMenu(hFileMenu, MF_STRING, IDC_RELOAD_TXTS, (string(LanguageManager->sTexts[LAN_RELOAD_TEXT_FILES], (size_t)LanguageManager->ui16TextsLens[LAN_RELOAD_TEXT_FILES]) + "...").c_str());
    ::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hFileMenu, MF_STRING, IDC_SETTINGS, (string(LanguageManager->sTexts[LAN_MENU_SETTINGS], (size_t)LanguageManager->ui16TextsLens[LAN_MENU_SETTINGS]) + "...").c_str());
    ::AppendMenu(hFileMenu, MF_STRING, IDC_SAVE_SETTINGS, (string(LanguageManager->sTexts[LAN_SAVE_SETTINGS], (size_t)LanguageManager->ui16TextsLens[LAN_SAVE_SETTINGS]) + "...").c_str());
    ::AppendMenu(hFileMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hFileMenu, MF_STRING, IDC_EXIT, LanguageManager->sTexts[LAN_EXIT]);

    ::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hFileMenu, LanguageManager->sTexts[LAN_FILE]);

    HMENU hViewMenu = ::CreatePopupMenu();
    ::AppendMenu(hViewMenu, MF_STRING, IDC_REG_USERS, LanguageManager->sTexts[LAN_REG_USERS]);
    ::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_PROFILES, LanguageManager->sTexts[LAN_PROFILES]);
    ::AppendMenu(hViewMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_BANS, LanguageManager->sTexts[LAN_BANS]);
    ::AppendMenu(hViewMenu, MF_STRING, IDC_RANGE_BANS, LanguageManager->sTexts[LAN_RANGE_BANS]);

    ::AppendMenu(hMainMenu, MF_POPUP, (UINT_PTR)hViewMenu, LanguageManager->sTexts[LAN_VIEW]);

    HMENU hHelpMenu = ::CreatePopupMenu();
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_UPDATE_CHECK, (string(LanguageManager->sTexts[LAN_CHECK_FOR_UPDATE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_FOR_UPDATE]) + "...").c_str());
    ::AppendMenu(hHelpMenu, MF_SEPARATOR, 0, 0);
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_HOMEPAGE, (string("PtokaX ") +LanguageManager->sTexts[LAN_WEBSITE]).c_str());
    ::AppendMenu(hHelpMenu, MF_STRING, IDC_FORUM, (string("PtokaX ") +LanguageManager->sTexts[LAN_FORUM]).c_str());
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

    m_hWnd = ::CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, MAKEINTATOM(atom),
        (string(SettingManager->sTexts[SETTXT_HUB_NAME], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_NAME]) + " | " + sTitle).c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, ScaleGuiDefaultsOnly(GUISETINT_MAIN_WINDOW_WIDTH), ScaleGuiDefaultsOnly(GUISETINT_MAIN_WINDOW_HEIGHT),
        NULL, hMainMenu, g_hInstance, NULL);

	return m_hWnd;
}
//---------------------------------------------------------------------------

void MainWindow::UpdateSysTray() const {
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

void MainWindow::UpdateStats() const {
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
    for(uint8_t ui8i = 0; ui8i < (sizeof(MainWindowPages) / sizeof(MainWindowPages[0])); ui8i++) {
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
    ::ModifyMenu(hMenu, IDC_PROFILES, MF_BYCOMMAND, IDC_PROFILES, LanguageManager->sTexts[LAN_PROFILES]);
    ::ModifyMenu(hMenu, IDC_BANS, MF_BYCOMMAND, IDC_BANS, LanguageManager->sTexts[LAN_BANS]);
    ::ModifyMenu(hMenu, IDC_RANGE_BANS, MF_BYCOMMAND, IDC_RANGE_BANS, LanguageManager->sTexts[LAN_RANGE_BANS]);

    ::ModifyMenu(hMenu, IDC_HOMEPAGE, MF_BYCOMMAND, IDC_HOMEPAGE, (string("PtokaX ") +LanguageManager->sTexts[LAN_WEBSITE]).c_str());
    ::ModifyMenu(hMenu, IDC_FORUM, MF_BYCOMMAND, IDC_FORUM, (string("PtokaX ") +LanguageManager->sTexts[LAN_FORUM]).c_str());
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

void MainWindow::EnableStartButton(const BOOL &bEnable) const {
    ::EnableWindow(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::BTN_START_STOP], bEnable);
}
//---------------------------------------------------------------------------

void MainWindow::SetStartButtonText(const char * sText) const {
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::BTN_START_STOP], sText);
}
//---------------------------------------------------------------------------

void MainWindow::SetStatusValue(const char * sText) const {
    ::SetWindowText(((MainWindowPageStats *)MainWindowPages[0])->hWndPageItems[MainWindowPageStats::LBL_STATUS_VALUE], sText);
}
//---------------------------------------------------------------------------

void MainWindow::EnableGuiItems(const BOOL &bEnable) const {
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
        ((MainWindowPageScripts *)MainWindowPages[1])->ClearMemUsageAll();
    }

    ::EnableWindow(((MainWindowPageScripts *)MainWindowPages[2])->hWndPageItems[MainWindowPageScripts::BTN_RESTART_SCRIPTS], bEnable);
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

    ::BringWindowToTop(((MainWindowPage *)tcItem.lParam)->m_hWnd);
}
//---------------------------------------------------------------------------

void MainWindow::SaveGuiSettings() {
    g_GuiSettingManager->Save();
};
//---------------------------------------------------------------------------

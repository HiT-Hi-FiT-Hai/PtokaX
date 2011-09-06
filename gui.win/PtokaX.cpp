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
#include "../core/LanguageManager.h"
#include "../core/ServerManager.h"
#include "../core/serviceLoop.h"
#include "../core/SettingManager.h"
#include "../core/utility.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#ifdef _MSC_VER
	#include "../core/ExceptionHandling.h"
#endif
#include "../core/LuaScript.h"
#include "MainWindow.h"
//---------------------------------------------------------------------------
#ifdef TIXML_USE_STL
	#undef TIXML_USE_STL
#endif
//---------------------------------------------------------------------------

#ifdef _DEBUG
//    #pragma link "lib/sqlited.lib"
//    #pragma message("Linking sqlited.lib")
	#pragma link "tinyxmld.lib"
	#pragma message("Linking tinyxmld.lib")
	#pragma link "zlibd.lib"
	#pragma message("Linking zlibd.lib")
#else
//    #pragma link "lib/sqlite.lib"
//    #pragma message("Linking sqlite.lib")
	#ifndef _MSC_VER
        #pragma link "tinyxml.lib"
	#else
        #ifndef _M_X64
            #pragma comment(lib, "tinyxml")
        #else
            #pragma comment(lib, "tinyxml-x64")
        #endif
    #endif
    #pragma message("Linking tinyxml.lib")

    #ifndef _MSC_VER
        #pragma link "zlib.lib"
    #else
        #ifndef _M_X64
            #pragma comment(lib, "zlib")
        #else
            #pragma comment(lib, "zlib-x64")
        #endif
    #endif
    #pragma message("Linking zlib.lib")
#endif
//---------------------------------------------------------------------------
HINSTANCE g_hInstance = NULL;
HWND g_hWndActiveDialog = NULL;
//---------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int nCmdShow) {
    ::SetDllDirectory("");

#ifndef _WIN64
    HINSTANCE hKernel32 = ::LoadLibrary("Kernel32.dll");

    typedef BOOL (WINAPI * SPDEPP)(DWORD);
    SPDEPP pSPDEPP = (SPDEPP)::GetProcAddress(hKernel32, "SetProcessDEPPolicy");

    if(pSPDEPP != NULL) {
        pSPDEPP(PROCESS_DEP_ENABLE);
    }

    ::FreeLibrary(hKernel32);
#endif

    g_hInstance = hInstance;

	sTitle = "PtokaX DC Hub " + string(PtokaXVersionString);
#ifdef _DEBUG
	sTitle += " [debug]";
#endif

#ifdef _DEBUG
//    AllocConsole();
//    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//    Cout("PtokaX Debug console\n");
#endif
	
	char sBuf[MAX_PATH+1];
	::GetModuleFileName(NULL, sBuf, MAX_PATH);
	char * sPath = strrchr(sBuf, '\\');
	if(sPath != NULL) {
		PATH = string(sBuf, sPath-sBuf);
	} else {
		PATH = sBuf;
	}

	size_t iCmdLen = strlen(lpCmdLine);
	if(iCmdLen != 0) {
	    char *sParam = lpCmdLine;

	    for(size_t i = 0; i < iCmdLen; i++) {
	        if(i == iCmdLen-1) {
	            if(lpCmdLine[i] != ' ') {
	                i++;
	            }
	        } else if(lpCmdLine[i] != ' ') {
	            continue;
	        }

			size_t iParamLen = (lpCmdLine+i)-sParam;

	        switch(iParamLen) {
	            case 7:
	                if(strnicmp(sParam, "/notray", 7) == NULL) {
	                    bCmdNoTray = true;
	                }
	                break;
	            case 10:
	                if(strnicmp(sParam, "/autostart", 10) == NULL) {
	                    bCmdAutoStart = true;
	                }
	                break;
	            case 11:
	                if(strnicmp(sParam, "/nokeycheck", 11) == NULL) {
	                    bCmdNoKeyCheck = true;
	                }
	                break;
	            case 12:
	                if(strnicmp(sParam, "/noautostart", 12) == NULL) {
	                    bCmdNoAutoStart = true;
	                }
	                break;
                case 20:
	                if(strnicmp(sParam, "/generatexmllanguage", 20) == NULL) {
	                    LangMan::GenerateXmlExample();
	                    return 0;
	                }
	                break;
	            default:
                    if(strnicmp(sParam, "-c ", 3) == NULL) {
                        size_t iLen = strlen(sParam+3);
                        if(iLen == 0) {
                            ::MessageBox(NULL, "Missing config directory!", "Error!", MB_OK);
                            return 0;
                        }

                        if(iLen >= 1 && sParam[0] != '\\' && sParam[0] != '/') {
                            if(iLen < 4 || (sParam[1] != ':' || (sParam[2] != '\\' && sParam[2] != '/'))) {
                                ::MessageBox(NULL, "Config directory must be absolute path!", "Error!", MB_OK);
                                return 0;
                            }
                        }

                        if(sParam[iLen - 1] == '/' || sParam[iLen - 1] == '\\') {
                            PATH = string(sParam, iLen - 1);
                        } else {
                            PATH = string(sParam, iLen);
                        }

                        if(DirExist(PATH.c_str()) == false) {
                            if(CreateDirectory(PATH.c_str(), NULL) == 0) {
                                ::MessageBox(NULL, "Config directory not exist and can't be created!", "Error!", MB_OK);
                                return 0;
                            }
                        }
                    }
	                break;
	        }

	        sParam = lpCmdLine+i+1;
		}
	}

    HINSTANCE hRichEdit = ::LoadLibrary("Riched20.dll");

#ifdef _MSC_VER
    ExceptionHandlingInitialize(PATH, sBuf);
#endif

	ServerInitialize();

	// systray icon on/off? added by Ptaczek 16.5.2003
	if(SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true) {
		pMainWindow->UpdateSysTray();
	}

	// If autostart is checked (or commandline /autostart given), then start the server
	if((SettingManager->bBools[SETBOOL_AUTO_START] == true || bCmdAutoStart == true) && bCmdNoAutoStart == false) {
	    if(ServerStart() == false) {
            pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".").c_str());
		}
	}

    if(SettingManager->bBools[SETBOOL_START_MINIMIZED] == true && SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true) {
        ::ShowWindow(pMainWindow->m_hWnd, SW_SHOWMINIMIZED);
    } else {
        ::ShowWindow(pMainWindow->m_hWnd, nCmdShow);
    }

	MSG msg;
	BOOL bRet;

	while((bRet = ::GetMessage(&msg, NULL, 0, 0)) != 0) {
	    if(bRet == -1) {
	        // handle the error and possibly exit
	    } else {
            if(msg.message == WM_USER+1) {
	            break;
	        } else if(msg.message == WM_TIMER) {
                if(msg.wParam == sectimer) {
                    ServerOnSecTimer();
                } else if(msg.wParam == srvLoopTimer) {
                    srvLoop->Looper();
                } else if(msg.wParam == regtimer) {
                    ServerOnRegTimer();
                } else {
                    //Must be script timer
                    ScriptOnTimer(msg.wParam);
                }
            }

	        if(g_hWndActiveDialog == NULL) {
                if(::IsDialogMessage(pMainWindow->m_hWnd, &msg) != 0) {
                    continue;
                }
            } else if(::IsDialogMessage(g_hWndActiveDialog, &msg) != 0) {
                continue;
            }

	    	::TranslateMessage(&msg);
	        ::DispatchMessage(&msg);
	    }
	}

    delete pMainWindow;

#ifdef _MSC_VER
    ExceptionHandlingUnitialize();
#endif

    ::FreeLibrary(hRichEdit);

    return (int)msg.wParam;
}
//---------------------------------------------------------------------------

/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2008  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"
#include "ServerManager.h"
#ifndef _WIN32
	#include "serviceLoop.h"
#endif
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _SERVICE
	    #pragma link "madExcept"
	    #pragma link "madLinkDisAsm"
	    #pragma link "madListModules"
	    #pragma link "madListProcesses"
	    #pragma link "madListHardware"
	#endif
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		#include "frmHub.h"
	#endif
#else
	#include "regtmrinc.h"
	#include "scrtmrinc.h"
#endif
//---------------------------------------------------------------------------
#ifdef TIXML_USE_STL
	#undef TIXML_USE_STL
#endif
//---------------------------------------------------------------------------

#ifdef _WIN32
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
		#pragma comment(lib, "tinyxml-x64")
	#endif
	    #pragma message("Linking tinyxml.lib")
	#ifndef _MSC_VER
	    #pragma link "zlib.lib"
	#else
		#pragma comment(lib, "zlib-x64")
	#endif
	    #pragma message("Linking zlib.lib")
	#endif
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		USEFORM("frmHub.cpp", hubForm);
	#endif
//---------------------------------------------------------------------------
	
	#ifdef _SERVICE
	    static int InstallService(const char * sServiceName, const char * sPath) {
	        SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	     
	        if(schSCManager == NULL) {
	            printf("OpenSCManager failed (%d)!", GetLastError());
	            return EXIT_FAILURE;
	        }
	
	        char sBuf[MAX_PATH+1];
			::GetModuleFileName(NULL, sBuf, MAX_PATH);
	
	        string sCmdLine = "\"" + string(sBuf) + "\" -s " + string(sServiceName);
	
	        if(sPath != NULL) {
	            sCmdLine += " -c " + string(sPath);
	        }
	
	    	SC_HANDLE schService = CreateService(schSCManager, sServiceName, sServiceName, 0, SERVICE_WIN32_OWN_PROCESS,
	    	   SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, sCmdLine.c_str(), 
	           NULL, NULL, NULL, NULL, NULL);
	
	        if(schService == NULL) {
	            printf("CreateService failed (%d)!", GetLastError()); 
	            CloseServiceHandle(schSCManager);
	            return EXIT_FAILURE;
	        } else {
	            printf("PtokaX service '%s' installed successfully.", sServiceName); 
	        }
	    
	        CloseServiceHandle(schService); 
	        CloseServiceHandle(schSCManager);
	
	        return EXIT_SUCCESS;
	    }
//---------------------------------------------------------------------------
	
	    static int UninstallService(const char * sServiceName) {
	        SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	     
	        if(schSCManager == NULL) {
	            printf("OpenSCManager failed (%d)!", GetLastError());
	            return EXIT_FAILURE;
	        }
	
	        SC_HANDLE schService = OpenService(schSCManager, sServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);
	     
	        if(schService == NULL) { 
	            printf("OpenService failed (%d)!", GetLastError()); 
	            CloseServiceHandle(schSCManager);
	            return EXIT_FAILURE;
	        }
	
	        SERVICE_STATUS_PROCESS ssp;
	        DWORD dwBytesNeeded;
	
	        if(QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded) != 0) {
	            if(ssp.dwCurrentState != SERVICE_STOPPED && ssp.dwCurrentState != SERVICE_STOP_PENDING) {
	                ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp);
	            }
	        }
	
	        if(DeleteService(schService) == false) {
	            printf("DeleteService failed (%d)!", GetLastError()); 
	            CloseServiceHandle(schService); 
	            CloseServiceHandle(schSCManager);
	            return EXIT_FAILURE;
	        } else {
	            printf("PtokaX service '%s' deleted successfully.", sServiceName);
	            CloseServiceHandle(schService); 
	            CloseServiceHandle(schSCManager);
	            return EXIT_SUCCESS;
	        }
	    }
//---------------------------------------------------------------------------
	static SERVICE_STATUS_HANDLE ssh; 
	static SERVICE_STATUS ss; 
//---------------------------------------------------------------------------
	
	static void WINAPI CtrlHandler(DWORD dwCtrl) {
	    switch(dwCtrl) {
	        case SERVICE_CONTROL_SHUTDOWN:
	        case SERVICE_CONTROL_STOP:
	            ss.dwCurrentState = SERVICE_STOP_PENDING;
	            bIsClose = true;
	            ServerStop();
	        case SERVICE_CONTROL_INTERROGATE: 
	            // Fall through to send current status.
	            break; 
	        default: 
	            break;
	    } 
	
	   	if(SetServiceStatus(ssh, &ss) == false) {
			AppendLog("CtrlHandler::SetServiceStatus failed ("+string((uint32_t)GetLastError())+")!");
		}
	}
//---------------------------------------------------------------------------
	
	static void WINAPI StartService(DWORD /*argc*/, char* argv[]) {
	    ssh = RegisterServiceCtrlHandler(argv[0], CtrlHandler);
	
	    if(ssh == NULL) { 
			AppendLog("RegisterServiceCtrlHandler failed ("+string((uint32_t)GetLastError())+")!");
	        return; 
	    }
	
	    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	    ss.dwCurrentState = SERVICE_START_PENDING;
	    ss.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
	    ss.dwWin32ExitCode = NO_ERROR;
	    ss.dwCheckPoint = 0;
	    ss.dwWaitHint = 10 * 1000;
	
		if(SetServiceStatus(ssh, &ss) == false) {
			AppendLog("StartService::SetServiceStatus failed ("+string((uint32_t)GetLastError())+")!");
			return;
		}
	
	    ServerInitialize();
	
	    if(ServerStart() == false) {
	        AppendLog("Server start failed!");
			ss.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(ssh, &ss);
			return;
	    }
	
		ss.dwCurrentState = SERVICE_RUNNING;
	
		if(SetServiceStatus(ssh, &ss) == false) {
			AppendLog("StartService::SetServiceStatus1 failed ("+string((uint32_t)GetLastError())+")!");
			return;
		}
	
	    MSG msg;
	    BOOL bRet;
	        
	    while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) { 
	        if(bRet == -1) {
	            // handle the error and possibly exit
	        } else {
	            if(msg.message == WM_USER+1) {
	                break;
	            }
	
	    		TranslateMessage(&msg);
	            DispatchMessage(&msg); 
	        }
	    }
	
	
	    ss.dwCurrentState = SERVICE_STOPPED;
	    SetServiceStatus(ssh, &ss);
	}
//---------------------------------------------------------------------------
	
		int __cdecl main(int argc, char* argv[]) {
	#else
		WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpstrCmdLine, int) {
			try {
				Application->Initialize();
	#endif
				sTitle = "PtokaX DC Hub " + string(PtokaXVersionString);
	#ifdef _DEBUG
				sTitle += " [debug]";
	#endif
	
	#ifndef _SERVICE
				Application->Title = sTitle.c_str();
	#endif
	
	#ifdef _DEBUG
	//        AllocConsole();
	//        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	//        Cout("PtokaX Debug console\n");
	#endif
	
			char sBuf[MAX_PATH+1];
			::GetModuleFileName(NULL, sBuf, MAX_PATH);
			char * sPath = strrchr(sBuf, '\\');
			if(sPath != NULL) {
				PATH = string(sBuf, sPath-sBuf);
			} else {
				PATH = sBuf;
			}
	
	#ifdef _SERVICE
	        char * sServiceName = NULL;
	
	        bool bInstallService = false;
	
	        for(int i = 0; i < argc; i++) {
	            if(stricmp(argv[i], "-s") == NULL || stricmp(argv[i], "/service") == NULL) {
	    			if(++i == argc) {
	                    AppendLog("Missing service name!");
	                    return EXIT_FAILURE;
	    			}
	    			sServiceName = argv[i];
	    			bService = true;
	        	} else if(stricmp(argv[i], "-c") == 0) {
	        		if(++i == argc) {
	                    printf("Missing config directory!");
	                    return EXIT_FAILURE;
	        		}
	
	                size_t iLen = strlen(argv[i]);
	                if(iLen >= 1 && argv[i][0] != '\\' && argv[i][0] != '/') {
	                    if(iLen < 4 || (argv[i][1] != ':' || (argv[i][2] != '\\' && argv[i][2] != '/'))) {
	                        printf("Config directory must be absolute path!");
	                        return EXIT_FAILURE;
	                    }
	    			}
	
	    			if(argv[i][iLen - 1] == '/' || argv[i][iLen - 1] == '\\') {
	
	                    PATH = string(argv[i], iLen - 1);
	    			} else {
	                    PATH = string(argv[i], iLen);
	                }
	    
	            	if(DirExist(PATH.c_str()) == false) {
	            		if(CreateDirectory(PATH.c_str(), NULL) == 0) {
	
	
	
	                        printf("Config directory not exist and can't be created!");
	                    }
	            	}
	            } else if(stricmp(argv[i], "-i") == NULL || stricmp(argv[i], "/install") == NULL) {
	    			if(++i == argc) {
	                    printf("Please specify service name!");
	    				return EXIT_FAILURE;
	    			}
	    			sServiceName = argv[i];
	    			bInstallService = true;
	            } else if(stricmp(argv[i], "-u") == NULL || stricmp(argv[i], "/uninstall") == NULL) {
	    			if(++i == argc) {
	                    printf("Please specify service name!");
	    				return EXIT_FAILURE;
	    			}
	    			sServiceName = argv[i];
	    			return UninstallService(sServiceName);
	            } else if(stricmp(argv[i], "-v") == NULL || stricmp(argv[i], "/version") == NULL) {
	    			printf((sTitle+" built on "+__DATE__+" "+__TIME__).c_str());
	    			return EXIT_SUCCESS;
	            } else if(stricmp(argv[i], "-h") == NULL || stricmp(argv[i], "/help") == NULL) {
	    			printf("PtokaX [-c <configdir>] [-i <servicename>] [-u <servicename>] [-v]");
	    			return EXIT_SUCCESS;
	            } else if(stricmp(argv[i], "/nokeycheck") == NULL) {
	                bCmdNoKeyCheck = true;
	            }
	#else
	        size_t iCmdLen = strlen(lpstrCmdLine);
	        if(iCmdLen != 0) {
	            char *sParam = lpstrCmdLine;
	
	            for(size_t i = 0; i < iCmdLen; i++) {
	                if(i == iCmdLen-1) {
	                    if(lpstrCmdLine[i] != ' ') {
	                        i++;
	                    }
	                } else if(lpstrCmdLine[i] != ' ') {
	                    continue;
	                }
	    
					size_t iParamLen = (lpstrCmdLine+i)-sParam;
	
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
	                    default:
	                        break;
	                }
	
	                sParam = lpstrCmdLine+i+1;
				}
	#endif
			}
	
	#ifndef _SERVICE
	        ServerInitialize();

			// systray icon on/off? added by Ptaczek 16.5.2003
			if(SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true) {
				hubForm->UpdateSysTray();
			}
	
	        // If autostart is checked (or commandline /autostart given), then start the server
	    	if((SettingManager->bBools[SETBOOL_AUTO_START] == true || bCmdAutoStart == true) && bCmdNoAutoStart == false) {
	            if(ServerStart() == false) {
					hubForm->StatusValue->Caption = String(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".";
				}
			}
	
	        if(SettingManager->bBools[SETBOOL_START_MINIMIZED] == true && SettingManager->bBools[SETBOOL_ENABLE_TRAY_ICON] == true) {
				PostMessage(hubForm->Handle, WM_MINIMIZEONSTARTUP, 0, 0);
			}
	
			Application->Run();
		} catch(Exception &exception) {
			Application->ShowException(&exception);
	    } catch(...) {
	        try {
	            throw Exception("");
	        }
	        catch(Exception &exception) {
	            Application->ShowException(&exception);
	        }
		}
	#else // not _SERVICE
	    if(bInstallService == true) {
	        if(sPath == NULL && strcmp(PATH.c_str(), sBuf) == 0) {
	            return InstallService(sServiceName, NULL);
			} else {
				return InstallService(sServiceName, PATH.c_str());
			}
	    }
	
	    if(bService == false) {
	        ServerInitialize();
	
	        if(ServerStart() == false) {
	            printf("Server start failed!");
	            return EXIT_FAILURE;
	        } else {
	            printf((sTitle+" running...\n").c_str());
	        }
	
	        MSG msg;
	        BOOL bRet;
	        
	        while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) { 
	            if(bRet == -1) {
	                // handle the error and possibly exit
	            } else {
	                if(msg.message == WM_USER+1) {
	                    break;
	                }
	
	    			TranslateMessage(&msg);
	                DispatchMessage(&msg); 
	            }
	        }
	    } else {
	        SERVICE_TABLE_ENTRY DispatchTable[] = {
	            { sServiceName, StartService }, 
	            { NULL, NULL } 
	        }; 
	       
	        if(StartServiceCtrlDispatcher(DispatchTable) == false) {
				AppendLog("StartServiceCtrlDispatcher failed ("+string((uint32_t)GetLastError())+")!");
	            return EXIT_FAILURE;
	        } 
	    }
	#endif
#else // _WIN32
	static void SigHandler(int sig) {
	    string str = "Received signal ";
	
	    if(sig == SIGINT) {
	        str += "SIGINT";
	    } else if(sig == SIGTERM) {
	        str += "SIGTERM";
	    } else if(sig == SIGQUIT) {
	        str += "SIGQUIT";
	    } else if(sig == SIGHUP) {
	        str += "SIGHUP";
	    } else {
	        str += string(sig);
	    }
	
	    str += " ending...";
	
	    AppendLog(str);
	
	    bIsClose = true;
	    ServerStop();
	
	    // restore to default...
	    struct sigaction sigact;
	    sigact.sa_handler = SIG_DFL;
	    sigemptyset(&sigact.sa_mask);
	    sigact.sa_flags = 0;
	    
	    sigaction(sig, &sigact, NULL);
	}
	//---------------------------------------------------------------------------
	int main(int argc, char* argv[]) {
		sTitle = "PtokaX DC Hub " + string(PtokaXVersionString);
	
	#ifdef _DEBUG
		sTitle += " [debug]";
	#endif
	
	    for(int i = 0; i < argc; i++) {
	        if(strcasecmp(argv[i], "-d") == 0) {
	    		bDaemon = true;
	    	} else if(strcasecmp(argv[i], "-c") == 0) {
	    		if(++i == argc) {
	                printf("Missing config directory!\n");
	                return EXIT_FAILURE;
	    		}
	
				if(argv[i][0] != '/') {
	                printf("Config directory must be absolute path!\n");
	                return EXIT_FAILURE;
				}
	
	            size_t iLen = strlen(argv[i]);
				if(argv[i][iLen - 1] == '/') {
	                PATH = string(argv[i], iLen - 1);
				} else {
	                PATH = string(argv[i], iLen);
	            }
	
	        	if(DirExist(PATH.c_str()) == false) {
	        		if(mkdir(PATH.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
	                    if(bDaemon == true) {
	                        syslog(LOG_USER | LOG_ERR, "Config directory not exist and can't be created!\n");
	                    } else {
	                        printf("Config directory not exist and can't be created!");
	                    }
	                }
	        	}
	        } else if(strcasecmp(argv[i], "-v") == 0) {
	        	printf((sTitle+" built on "+__DATE__+" "+__TIME__+"\n").c_str());
	        	return EXIT_SUCCESS;
	        } else if(strcasecmp(argv[i], "-h") == 0) {
	        	printf("PtokaX [-d] [-c <configdir>] [-v]\n");
	        	return EXIT_SUCCESS;
	        } else if(strcasecmp(argv[i], "/nokeycheck") == 0) {
	            bCmdNoKeyCheck = true;
	        }
	    }
	
	    if(PATH.size() == 0) {
	        char* home;
	        char curdir[PATH_MAX];
	        if(bDaemon == true && (home = getenv("HOME")) != NULL) {
	            PATH = string(home) + "/.PtokaX";
	            
	            if(DirExist(PATH.c_str()) == false) {
	            	if(mkdir(PATH.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
	                    syslog(LOG_USER | LOG_ERR, "Config directory not exist and can't be created!\n");
	                }
	            }
	        } else if(getcwd(curdir, PATH_MAX) != NULL) {
	            PATH = curdir;
	        } else {
	            PATH = ".";
	        }
	    }
	
	    if(bDaemon == true) {
	        printf(("Starting "+sTitle+" as daemon using "+PATH+" as config directory.\n").c_str());
	
	        pid_t pid1 = fork();
	        if(pid1 == -1) {
	            syslog(LOG_USER | LOG_ERR, "First fork failed!\n");
	            return EXIT_FAILURE;
	        } else if(pid1 > 0) {
	            return EXIT_SUCCESS;
	        }
	
	    	if(setsid() == -1) {
	            syslog(LOG_USER | LOG_ERR, "Setsid failed!\n");
	            return EXIT_FAILURE;
	    	}
	
	        pid_t pid2 = fork();
	        if(pid2 == -1) {
	            syslog(LOG_USER | LOG_ERR, "Second fork failed!\n");
	            return EXIT_FAILURE;
	        } else if(pid2 > 0) {
	            return EXIT_SUCCESS;
	        }
	
	    	chdir("/");
	
	        close(STDIN_FILENO);
	        close(STDOUT_FILENO);
	        close(STDERR_FILENO);
	
	        umask(117);
	
	    	if(open("/dev/null", O_RDWR) == -1) {
	            syslog(LOG_USER | LOG_ERR, "Failed to open /dev/null!\n");
	            return EXIT_FAILURE;
	        }
	
	    	dup(0);
	        dup(0);
	    }
	
	    sigset_t sst;
	    sigemptyset(&sst);
	    sigaddset(&sst, SIGPIPE);
	    sigaddset(&sst, SIGURG);
	    sigaddset(&sst, SIGALRM);
	    sigaddset(&sst, SIGSCRTMR);
	    sigaddset(&sst, SIGREGTMR);
	
	    if(bDaemon == true) {
	        sigaddset(&sst, SIGHUP);
	    }
	
	    pthread_sigmask(SIG_BLOCK, &sst, NULL);
	
	    struct sigaction sigact;
	    sigact.sa_handler = SigHandler;
	    sigemptyset(&sigact.sa_mask);
	    sigact.sa_flags = 0;
	
	    if(sigaction(SIGINT, &sigact, NULL) == -1) {
	    	string sDbgstr = "[BUF] Cannot create sigaction SIGINT! "+string(strerror(errno));
	    	AppendSpecialLog(sDbgstr);
	        exit(EXIT_FAILURE);
	    }
	
	    if(sigaction(SIGTERM, &sigact, NULL) == -1) {
	    	string sDbgstr = "[BUF] Cannot create sigaction SIGTERM! "+string(strerror(errno));
	    	AppendSpecialLog(sDbgstr);
	        exit(EXIT_FAILURE);
	    }
	
	    if(sigaction(SIGQUIT, &sigact, NULL) == -1) {
	    	string sDbgstr = "[BUF] Cannot create sigaction SIGQUIT! "+string(strerror(errno));
	    	AppendSpecialLog(sDbgstr);
	        exit(EXIT_FAILURE);
	    }
	
	    if(bDaemon == false && sigaction(SIGHUP, &sigact, NULL) == -1) {
	        string sDbgstr = "[BUF] Cannot create sigaction SIGHUP! "+string(strerror(errno));
	        AppendSpecialLog(sDbgstr);
	        exit(EXIT_FAILURE);
	    }
	
	    ServerInitialize();
	
	    if(ServerStart() == false) {
	        if(bDaemon == false) {
	            printf("Server start failed!\n");
	        } else {
	            syslog(LOG_USER | LOG_ERR, "Server start failed!\n");
	        }
	        return EXIT_FAILURE;
	    } else if(bDaemon == false) {
	        printf((sTitle+" running...\n").c_str());
	    }
	
	    while(true) {
	        srvLoop->Looper();
	
	        if(bServerTerminated == true) {
	            break;
	        }
	
	        usleep(100000);
	    }
	
	    if(bDaemon == false) {
	        printf((sTitle+" ending...\n").c_str());
	    }
#endif

    return EXIT_SUCCESS;
}
//---------------------------------------------------------------------------

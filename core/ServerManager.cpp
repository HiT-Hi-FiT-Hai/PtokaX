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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "HubCommands.h"
#include "IP2Country.h"
#include "LuaScript.h"
#include "RegThread.h"
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
//#include "TLSManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindow.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------
#ifndef _WIN32
    #include "regtmrinc.h"
#endif
//---------------------------------------------------------------------------
static ServerThread *ServersE = NULL;
#ifdef _WIN32
    UINT_PTR sectimer = 0;
    UINT_PTR regtimer = 0;
#else
    static int SIGSECTMR = SIGRTMIN+1;
	static timer_t sectimer, regtimer;
#endif
//---------------------------------------------------------------------------
double CpuUsage[60], cpuUsage = 0;
uint64_t ui64ActualTick = 0, ui64TotalShare = 0;
uint64_t ui64BytesRead = 0, ui64BytesSent = 0, ui64BytesSentSaved = 0;
uint64_t iLastBytesRead = 0, iLastBytesSent = 0;
uint32_t ui32Joins = 0, ui32Parts = 0, ui32Logged = 0, ui32Peak = 0;
#ifndef _WIN32
	uint32_t ui32CpuCount = 0;
#endif
uint32_t UploadSpeed[60], DownloadSpeed[60];
uint32_t iActualBytesRead = 0, iActualBytesSent = 0;
uint32_t iAverageBytesRead = 0, iAverageBytesSent = 0;
ServerThread *ServersS = NULL;
time_t starttime = 0;
uint64_t iMins = 0, iHours = 0, iDays = 0;
bool bServerRunning = false, bServerTerminated = false, bIsRestart = false, bIsClose = false;
#ifdef _WIN32
	#ifndef _BUILD_GUI
	    bool bService = false;
	#endif
#else
	bool bDaemon = false;
#endif
char sHubIP[16], sHubIP6[46];
uint8_t ui8SrCntr = 0, ui8MinTick = 0;
//---------------------------------------------------------------------------

#ifdef _WIN32
void ServerOnSecTimer() {
	FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
	GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
	int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
	int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
	double dcpuSec = double(kernelTime + userTime) / double(10000000I64);
	cpuUsage = dcpuSec - CpuUsage[ui8MinTick];
	CpuUsage[ui8MinTick] = dcpuSec;
#else
static void SecTimerHandler(int /*sig*/) {
    struct rusage rs;

    getrusage(RUSAGE_SELF, &rs);

	double dcpuSec = double(rs.ru_utime.tv_sec) + (double(rs.ru_utime.tv_usec)/1000000) + 
	double(rs.ru_stime.tv_sec) + (double(rs.ru_stime.tv_usec)/1000000);
	cpuUsage = dcpuSec - CpuUsage[ui8MinTick];
	CpuUsage[ui8MinTick] = dcpuSec;
#endif

	if(++ui8MinTick == 60) {
		ui8MinTick = 0;
    }

#ifdef _WIN32
	if(bServerRunning == false) {
		return;
	}
#endif

    ui64ActualTick++;

	iActualBytesRead = (uint32_t)(ui64BytesRead - iLastBytesRead);
	iActualBytesSent = (uint32_t)(ui64BytesSent - iLastBytesSent);
	iLastBytesRead = ui64BytesRead;
	iLastBytesSent = ui64BytesSent;

	iAverageBytesSent -= UploadSpeed[ui8MinTick];
	iAverageBytesRead -= DownloadSpeed[ui8MinTick];

	UploadSpeed[ui8MinTick] = iActualBytesSent;
	DownloadSpeed[ui8MinTick] = iActualBytesRead;

	iAverageBytesSent += UploadSpeed[ui8MinTick];
	iAverageBytesRead += DownloadSpeed[ui8MinTick];

#ifdef _BUILD_GUI
    pMainWindow->UpdateStats();
    pMainWindowPageScripts->UpdateMemUsage();
#endif
}
//---------------------------------------------------------------------------

#ifdef _WIN32
    void ServerOnRegTimer() {
	    if(SettingManager->bBools[SETBOOL_AUTO_REG] == true && SettingManager->sTexts[SETTXT_REGISTER_SERVERS] != NULL) {
			// First destroy old hublist reg thread if any
	        if(RegisterThread != NULL) {
	            RegisterThread->Close();
	            RegisterThread->WaitFor();
	            delete RegisterThread;
	            RegisterThread = NULL;
	        }
	        
	        // Create hublist reg thread
	        RegisterThread = new RegThread();
	        if(RegisterThread == NULL) {
	        	AppendDebugLog("%s - [MEM] Cannot allocate RegisterThread in ServerOnRegTimer\n", 0);
	        	return;
	        }
	        
	        // Setup hublist reg thread
	        RegisterThread->Setup(SettingManager->sTexts[SETTXT_REGISTER_SERVERS], SettingManager->ui16TextsLens[SETTXT_REGISTER_SERVERS]);
	        
	        // Start the hublist reg thread
	    	RegisterThread->Resume();
	    }
	}
#endif
//---------------------------------------------------------------------------

void ServerInitialize() {
    setlocale(LC_ALL, "");
#ifdef _WIN32
	time_t acctime;
	time(&acctime);
	srand((uint32_t)acctime);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

    hPtokaXHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x100000, 0);

	if(DirExist((PATH+"\\cfg").c_str()) == false) {
		CreateDirectory((PATH+"\\cfg").c_str(), NULL);
	}
	if(DirExist((PATH+"\\logs").c_str()) == false) {
		CreateDirectory((PATH+"\\logs").c_str(), NULL);
	}
	if(DirExist((PATH+"\\scripts").c_str()) == false) {
		CreateDirectory((PATH+"\\scripts").c_str(), NULL);
    }
	if(DirExist((PATH+"\\texts").c_str()) == false) {
		CreateDirectory((PATH+"\\texts").c_str(), NULL);
    }

	SCRIPT_PATH = PATH + "\\scripts\\";

	PATH_LUA = PATH + "/";

	char * sLuaPath = PATH_LUA.c_str();
	for(size_t szi = 0; szi < PATH.size(); szi++) {
		if(sLuaPath[szi] == '\\') {
			sLuaPath[szi] = '/';
		}
	}

    SetupOsVersion();
#else
    time_t acctime;
    time(&acctime);
    srandom(acctime);

	if(DirExist((PATH+"/logs").c_str()) == false) {
		if(mkdir((PATH+"/logs").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
            if(bDaemon == true) {
                syslog(LOG_USER | LOG_ERR, "Creating  of logs directory failed!\n");
            } else {
                printf("Creating  of logs directory failed!");
            }
        }
	}
	if(DirExist((PATH+"/cfg").c_str()) == false) {
		if(mkdir((PATH+"/cfg").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
            AppendLog("Creating of cfg directory failed!");
        }
	}
	if(DirExist((PATH+"/scripts").c_str()) == false) {
		if(mkdir((PATH+"/scripts").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
            AppendLog("Creating of scripts directory failed!");
        }
    }
	if(DirExist((PATH+"/texts").c_str()) == false) {
		if(mkdir((PATH+"/texts").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP) == -1) {
            AppendLog("Creating of texts directory failed!");
        }
    }

	SCRIPT_PATH = PATH + "/scripts/";

    // get cpu count
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(fp != NULL) {
        char buf[1024];
        while(fgets(buf, 1024, fp) != NULL) {
            if(strncasecmp (buf, "model name", 10) == 0 || strncmp (buf, "Processor", 9) == 0 || strncmp (buf, "cpu model", 9) == 0) {
                ui32CpuCount++;
            }
        }
    
        fclose(fp);
    }

    if(ui32CpuCount == 0) {
        ui32CpuCount = 1;
    }
#endif
    CreateGlobalBuffer();

    CheckForIPv6();

	ResNickManager = new ResNickMan();
	if(ResNickManager == NULL) {
	    AppendDebugLog("%s - [MEM] Cannot allocate ResNickManager in ServerInitialize\n", 0);
	    exit(EXIT_FAILURE);
	}

    ui64ActualTick = ui64TotalShare = 0;
	ui64BytesRead = ui64BytesSent = ui64BytesSentSaved = 0;

	iActualBytesRead = iActualBytesSent = iAverageBytesRead = iAverageBytesSent = 0;

    ui32Joins = ui32Parts = ui32Logged = ui32Peak = 0;

    ServersS = NULL;
    ServersE = NULL;

    starttime = 0;

    iMins = iHours = iDays = 0;

    bServerRunning = bIsRestart = bIsClose = false;

    sHubIP[0] = '\0';
    sHubIP6[0] = '\0';

    ui8SrCntr = 0;

    ZlibUtility = new clsZlibUtility();
    if(ZlibUtility == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate ZlibUtility in ServerInitialize\n", 0);
    	exit(EXIT_FAILURE);
    }

    ui8MinTick = 0;

    iLastBytesRead = iLastBytesSent = 0;

	for(uint8_t ui8i = 0 ; ui8i < 60; ui8i++) {
		CpuUsage[ui8i] = 0;
		UploadSpeed[ui8i] = 0;
		DownloadSpeed[ui8i] = 0;
	}

	cpuUsage = 0.0;

	SettingManager = new SetMan();
    if(SettingManager == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate SettingManager in ServerInitialize\n", 0);
    	exit(EXIT_FAILURE);
    }

    LanguageManager = new LangMan();
    if(LanguageManager == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate LanguageManager in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    LanguageManager->LoadLanguage();

    ProfileMan = new ProfileManager();
    if(ProfileMan == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate ProfileMan in ServerInitialize\n", 0);
    	exit(EXIT_FAILURE);
    }

    hashRegManager = new hashRegMan();
    if(hashRegManager == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate hashRegManager in ServerInitialize\n", 0);
    	exit(EXIT_FAILURE);
    }

    // Load registered users
	hashRegManager->Load();

    hashBanManager = new hashBanMan();
    if(hashBanManager == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate hashBanManager in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    // load banlist
	hashBanManager->Load();

    TextFileManager = new TextFileMan();
    if(TextFileManager == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate TextFileManager in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    UdpDebug = new clsUdpDebug();
    if(UdpDebug == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate UdpDebug in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    ScriptManager = new ScriptMan();
    if(ScriptManager == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate ScriptManager in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

#ifdef _BUILD_GUI
    pMainWindow = new MainWindow();

    if(pMainWindow == NULL || pMainWindow->CreateEx() == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate pMainWindow in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }
#endif

	SettingManager->UpdateAll();

#ifdef _WIN32
    sectimer = SetTimer(NULL, 0, 1000, NULL);

	if(sectimer == 0) {
		AppendDebugLog("%s - [ERR] Cannot startsectimer in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    regtimer = 0;
#else
    struct sigaction sigactsec;
    sigactsec.sa_handler = SecTimerHandler;
    sigemptyset(&sigactsec.sa_mask);
    sigactsec.sa_flags = 0;

    if(sigaction(SIGSECTMR, &sigactsec, NULL) == -1) {
		AppendDebugLog("%s - [ERR] Cannot create sigaction SIGSECTMR in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    struct sigevent sigevsec;
    sigevsec.sigev_notify = SIGEV_SIGNAL;
    sigevsec.sigev_signo = SIGSECTMR;

    int iRet = timer_create(CLOCK_REALTIME, &sigevsec, &sectimer);
    
	if(iRet == -1) {
		AppendDebugLog("%s - [ERR] Cannot create sectimer in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }

    struct sigevent sigevreg;
    sigevreg.sigev_notify = SIGEV_SIGNAL;
    sigevreg.sigev_signo = SIGREGTMR;

    iRet = timer_create(CLOCK_REALTIME, &sigevreg, &regtimer);
    
	if(iRet == -1) {
		AppendDebugLog("%s - [ERR] Cannot create regtimer in ServerInitialize\n", 0);
        exit(EXIT_FAILURE);
    }
#endif
}
//---------------------------------------------------------------------------

bool ServerStart() {
    time(&starttime);

#ifndef _WIN32
    struct itimerspec sectmrspec;
    sectmrspec.it_interval.tv_sec = 1;
    sectmrspec.it_interval.tv_nsec = 0;
    sectmrspec.it_value.tv_sec = 1;
    sectmrspec.it_value.tv_nsec = 0;

    int iRet = timer_settime(sectimer, 0, &sectmrspec, NULL);
    if(iRet == -1) {
		AppendDebugLog("%s - [ERR] Cannot start sectimer in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }
#endif

    SettingManager->UpdateAll();

    TextFileManager->RefreshTextFiles();

#ifdef _BUILD_GUI
    pMainWindow->EnableStartButton(FALSE);
#endif

    ui64ActualTick = ui64TotalShare = 0;

    ui64BytesRead = ui64BytesSent = ui64BytesSentSaved = 0;

	iActualBytesRead = iActualBytesSent = iAverageBytesRead = iAverageBytesSent = 0;

    ui32Joins = ui32Parts = ui32Logged = ui32Peak = 0;

    iMins = iHours = iDays = 0;

    ui8SrCntr = 0;

    sHubIP[0] = '\0';
    sHubIP6[0] = '\0';

    if(SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true) {
        if(isIP(SettingManager->sTexts[SETTXT_HUB_ADDRESS], SettingManager->ui16TextsLens[SETTXT_HUB_ADDRESS]) == false) {
#ifdef _BUILD_GUI
            pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_RESOLVING_HUB_ADDRESS], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_HUB_ADDRESS])+"...").c_str());
#endif

            struct addrinfo hints;
            memset(&hints, 0, sizeof(addrinfo));

            if(bUseIPv6 == true) {
                hints.ai_family = AF_UNSPEC;
            } else {
                hints.ai_family = AF_INET;
            }

            struct addrinfo *res;

            if(::getaddrinfo(SettingManager->sTexts[SETTXT_HUB_ADDRESS], NULL, &hints, &res) != 0 ||
                (res->ai_family != AF_INET && res->ai_family != AF_INET6)) {
#ifdef _WIN32
            	int err = WSAGetLastError();
	#ifdef _BUILD_GUI
				::MessageBox(pMainWindow->m_hWnd,(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+": "+
					string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
					string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str(),
					LanguageManager->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
                pMainWindow->EnableStartButton(TRUE);
	#else
                AppendLog(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+": "+
					string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
					string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
	#endif
#else
                AppendLog(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
#endif
                return false;
            } else {
				Memo("*** "+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS], (size_t)SettingManager->ui16TextsLens[SETTXT_HUB_ADDRESS])+" "+
					string(LanguageManager->sTexts[LAN_RESOLVED_SUCCESSFULLY], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVED_SUCCESSFULLY])+".");

                if(bUseIPv6 == true) {
                    struct addrinfo *next = res;
                    while(next != NULL) {
                        if(next->ai_family == AF_INET) {
                            if(((sockaddr_in *)(next->ai_addr))->sin_addr.s_addr != INADDR_ANY) {
                                strcpy(sHubIP, inet_ntoa(((sockaddr_in *)(next->ai_addr))->sin_addr));
                            }
                        } else if(next->ai_family == AF_INET6) {
#ifdef _WIN32
                            win_inet_ntop(&((struct sockaddr_in6 *)next->ai_addr)->sin6_addr, sHubIP6, 46);
#else
                            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)next->ai_addr)->sin6_addr, sHubIP6, 46);
#endif
                        }

                        next = next->ai_next;
                    }
                } else if(((sockaddr_in *)(res->ai_addr))->sin_addr.s_addr != INADDR_ANY) {
                    strcpy(sHubIP, inet_ntoa(((sockaddr_in *)(res->ai_addr))->sin_addr));
                }

                if(sHubIP[0] != '\0') {
                    string msg = "*** "+string(sHubIP);
                    if(sHubIP6[0] != '\0') {
                        msg += " / "+string(sHubIP6);
                    }

				    Memo(msg);
                } else if(sHubIP6[0] != '\0') {
				    Memo("*** "+string(sHubIP6));
                }

				freeaddrinfo(res);
            }
        } else {
            strcpy(sHubIP, SettingManager->sTexts[SETTXT_HUB_ADDRESS]);
        }
    } else {
        if(SettingManager->sTexts[SETTXT_IPV4_ADDRESS] != NULL) {
            strcpy(sHubIP, SettingManager->sTexts[SETTXT_IPV4_ADDRESS]);
        } else {
            sHubIP[0] = '\0';
        }

        if(SettingManager->sTexts[SETTXT_IPV6_ADDRESS] != NULL) {
            strcpy(sHubIP6, SettingManager->sTexts[SETTXT_IPV6_ADDRESS]);
        } else {
            sHubIP6[0] = '\0';
        }
    }

    for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
        if(SettingManager->iPortNumbers[ui8i] == 0) {
            break;
        }

        if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
            if(bUseIPv6 == true) {
                ServerCreateServerThread(AF_INET6, SettingManager->iPortNumbers[ui8i]);
            }

            ServerCreateServerThread(AF_INET, SettingManager->iPortNumbers[ui8i]);
        } else {
            ServerCreateServerThread(bUseIPv6 == true ? AF_INET6 : AF_INET, SettingManager->iPortNumbers[ui8i]);
        }
    }

	if(ServersS == NULL) {
#ifdef _BUILD_GUI
		::MessageBox(pMainWindow->m_hWnd, LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED], LanguageManager->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
        pMainWindow->EnableStartButton(TRUE);
#else
		AppendLog(LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED]);
#endif
        return false;
    }

    AppendLog("Serving started");

//  if(tlsenabled == true) {
/*        TLSManager = new TLSMan();
        if(TLSManager == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot allocate TLSManager in ServerStart\n", 0);
        	exit(EXIT_FAILURE);
        }*/
//  }

    IP2Country = new IP2CC();
    if(IP2Country == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate IP2Country in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    eventqueue = new eventq();
    if(eventqueue == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate eventqueue in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    hashManager = new hashMan();
    if(hashManager == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate hashManager in ServerStart\n", 0);
        exit(EXIT_FAILURE);
    }

    colUsers = new classUsers();
	if(colUsers == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate colUsers in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    g_GlobalDataQueue = new GlobalDataQueue();
    if(g_GlobalDataQueue == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate g_GlobalDataQueue in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    HubCmds = new HubCommands();
    if(HubCmds == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate HubCmds in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    DcCommands = new cDcCommands();
    if(DcCommands == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate DcCommands in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    // add botname to reserved nicks
    ResNickManager->AddReservedNick(SettingManager->sTexts[SETTXT_BOT_NICK]);
    SettingManager->UpdateBot();

    // add opchat botname to reserved nicks
    ResNickManager->AddReservedNick(SettingManager->sTexts[SETTXT_OP_CHAT_NICK]);
    SettingManager->UpdateOpChat();

    ResNickManager->AddReservedNick(SettingManager->sTexts[SETTXT_ADMIN_NICK]);

    if((uint16_t)atoi(SettingManager->sTexts[SETTXT_UDP_PORT]) != 0) {
        if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
            if(bUseIPv6 == true) {
                g_pUDPThread6 = UDPThread::Create(AF_INET6);
            }

            g_pUDPThread4 = UDPThread::Create(AF_INET);
        } else {
            g_pUDPThread6 = UDPThread::Create(bUseIPv6 == true ? AF_INET6 : AF_INET);
        }
    }
    
    if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager->Start();
    }

    srvLoop = new theLoop();
    if(srvLoop == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate srvLoop in ServerStart\n", 0);
    	exit(EXIT_FAILURE);
    }

    // Start the server socket threads
    ServerThread *next = ServersS;
    while(next != NULL) {
        ServerThread *cur = next;
        next = cur->next;

		cur->Resume();
    }

    bServerRunning = true;

    // Call lua_Main
	ScriptManager->OnStartup();

#ifdef _BUILD_GUI
    pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_RUNNING], (size_t)LanguageManager->ui16TextsLens[LAN_RUNNING])+"...").c_str());
    pMainWindow->SetStartButtonText(LanguageManager->sTexts[LAN_STOP_HUB]);
    pMainWindow->EnableStartButton(TRUE);
    pMainWindow->EnableGuiItems(TRUE);
#endif

    //Start the HubRegistration timer
    if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
#ifdef _WIN32
		regtimer = SetTimer(NULL, 0, 901000, NULL);

        if(regtimer == 0) {
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 901;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 901;
        regtmrspec.it_value.tv_nsec = 0;
    
        iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
#endif

			AppendDebugLog("%s - [ERR] Cannot start regtimer in ServerStart\n", 0);
        	exit(EXIT_FAILURE);
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void ServerStop() {
#ifdef _BUILD_GUI
    pMainWindow->EnableStartButton(FALSE);
#endif
#ifndef _WIN32
    struct itimerspec sectmrspec;
    sectmrspec.it_interval.tv_sec = 0;
    sectmrspec.it_interval.tv_nsec = 0;
    sectmrspec.it_value.tv_sec = 0;
    sectmrspec.it_value.tv_nsec = 0;

    int iRet = timer_settime(sectimer, 0, &sectmrspec, NULL);
    if(iRet == -1) {
		AppendDebugLog("%s - [ERR] Cannot stop sectimer in ServerStop\n", 0);
    	exit(EXIT_FAILURE);
    }
#endif

    char msg[1024];
    int iret = sprintf(msg, "Serving stopped (UL: %" PRIu64 " [%" PRIu64 "], DL: %" PRIu64 ")", ui64BytesSent, ui64BytesSentSaved, ui64BytesRead);
    if(CheckSprintf(iret, 1024, "ServerMan::StopServer") == true) {
        AppendLog(msg);
    }

	//Stop the HubRegistration timer
	if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
#ifdef _WIN32
        if(KillTimer(NULL, regtimer) == 0) {
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 0;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 0;
        regtmrspec.it_value.tv_nsec = 0;
    
        iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
#endif
    		AppendDebugLog("%s - [ERR] Cannot stop regtimer in ServerStop\n", 0);
        	exit(EXIT_FAILURE);
        }
    }

    ServerThread *next = ServersS;
    while(next != NULL) {
        ServerThread *cur = next;
        next = cur->next;

		cur->Close();
		cur->WaitFor();

		delete cur;
    }

    ServersS = NULL;
    ServersE = NULL;

	// stop the main hub loop
	if(srvLoop != NULL) {
		bServerTerminated = true;
	} else {
		ServerFinalStop(false);
    }
}
//---------------------------------------------------------------------------

void ServerFinalStop(const bool &bFromServiceLoop) {
    if(bFromServiceLoop == true) {
		delete srvLoop;
		srvLoop = NULL;
    }

    if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager->Stop();
    }

    UDPThread::Destroy(g_pUDPThread6);
    g_pUDPThread6 = NULL;

    UDPThread::Destroy(g_pUDPThread4);
    g_pUDPThread4 = NULL;

	// delete userlist field
	if(colUsers != NULL) {
		colUsers->DisconnectAll();
		delete colUsers;
		colUsers = NULL;
    }

	delete DcCommands;
    DcCommands = NULL;

    delete HubCmds;
    HubCmds = NULL;

	// delete hashed userlist manager
    delete hashManager;
    hashManager = NULL;

	delete g_GlobalDataQueue;
    g_GlobalDataQueue = NULL;

    if(RegisterThread != NULL) {
        RegisterThread->Close();
        RegisterThread->WaitFor();
        delete RegisterThread;
        RegisterThread = NULL;
    }

	delete eventqueue;
    eventqueue = NULL;

	delete IP2Country;
    IP2Country = NULL;

/*	if(TLSManager != NULL) {
		delete TLSManager;
        TLSManager = NULL;
    }*/

	//userstat  // better here ;)
//    sqldb->FinalizeAllVisits();

#ifdef _BUILD_GUI
    pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_STOPPED], (size_t)LanguageManager->ui16TextsLens[LAN_STOPPED])+".").c_str());
    pMainWindow->SetStartButtonText(LanguageManager->sTexts[LAN_START_HUB]);
    pMainWindow->EnableStartButton(TRUE);
    pMainWindow->EnableGuiItems(FALSE);
#endif

    ui8SrCntr = 0;
    ui32Joins = ui32Parts = ui32Logged = 0;

    UdpDebug->Cleanup();

#ifdef _WIN32
    HeapCompact(GetProcessHeap(), 0);
    HeapCompact(hPtokaXHeap, 0);
#endif

    bServerRunning = false;

    if(bIsRestart == true) {
        bIsRestart = false;

		// start hub
#ifdef _BUILD_GUI
        if(ServerStart() == false) {
            pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".").c_str());
        }
#else
		if(ServerStart() == false) {
            AppendLog("[ERR] Server start failed in ServerFinalStop");
            exit(EXIT_FAILURE);
        }
#endif
    } else if(bIsClose == true) {
		ServerFinalClose();
    }
}
//---------------------------------------------------------------------------

void ServerFinalClose() {
#ifdef _WIN32
    KillTimer(NULL, sectimer);
#else
    timer_delete(sectimer);
    timer_delete(regtimer);
#endif

	hashBanManager->Save(true);

    ProfileMan->SaveProfiles();

    hashRegManager->Save();

	ScriptManager->SaveScripts();

	SettingManager->Save();

    delete ScriptManager;
	ScriptManager = NULL;

    delete TextFileManager;
    TextFileManager = NULL;

    delete ProfileMan;
    ProfileMan = NULL;

    delete UdpDebug;
    UdpDebug = NULL;

    delete hashRegManager;
    hashRegManager = NULL;

    delete hashBanManager;
    hashBanManager = NULL;

    delete ZlibUtility;
    ZlibUtility = NULL;

    delete LanguageManager;
    LanguageManager = NULL;

    delete SettingManager;
    SettingManager = NULL;

    delete ResNickManager;
    ResNickManager = NULL;

#ifdef _BUILD_GUI
    pMainWindow->SaveGuiSettings();
#endif

    DeleteGlobalBuffer();

#ifdef _WIN32
	HeapDestroy(hPtokaXHeap);
	
	WSACleanup();
	
	::PostMessage(NULL, WM_USER+1, 0, 0);
#endif
}
//---------------------------------------------------------------------------

void ServerUpdateServers() {
    // Remove servers for ports we don't want use anymore
    ServerThread *next = ServersS;
    while(next != NULL) {
        ServerThread *cur = next;
        next = cur->next;

        bool bFound = false;

        for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
            if(SettingManager->iPortNumbers[ui8i] == 0) {
                break;
            }

            if(cur->ui16Port == SettingManager->iPortNumbers[ui8i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    ServersS = NULL;
                    ServersE = NULL;
                } else {
                    cur->next->prev = NULL;
                    ServersS = cur->next;
                }
            } else if(cur->next == NULL) {
                cur->prev->next = NULL;
                ServersE = cur->prev;
            } else {
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
            }

            cur->Close();
        	cur->WaitFor();

        	delete cur;
        }
    }

    // Add servers for ports that not running
    for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
        if(SettingManager->iPortNumbers[ui8i] == 0) {
            break;
        }

        bool bFound = false;

        ServerThread *next = ServersS;
        while(next != NULL) {
            ServerThread *cur = next;
            next = cur->next;

            if(cur->ui16Port == SettingManager->iPortNumbers[ui8i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || (bUseIPv6 == true && bIPv6DualStack == false)) {
                if(bUseIPv6 == true) {
                    ServerCreateServerThread(AF_INET6, SettingManager->iPortNumbers[ui8i], true);
                }
                ServerCreateServerThread(AF_INET, SettingManager->iPortNumbers[ui8i], true);
            } else {
                ServerCreateServerThread(bUseIPv6 == true ? AF_INET6 : AF_INET, SettingManager->iPortNumbers[ui8i], true);
            }
        }
    }
}
//---------------------------------------------------------------------------

void ServerResumeAccepts() {
	if(bServerRunning == false) {
        return;
    }

    ServerThread *next = ServersS;
    while(next != NULL) {
        ServerThread *cur = next;
        next = cur->next;

        cur->ResumeSck();
    }
}
//---------------------------------------------------------------------------

void ServerSuspendAccepts(const uint32_t &iTime) {
	if(bServerRunning == false) {
        return;
    }

    if(iTime != 0) {
        UdpDebug->Broadcast("[SYS] Suspending listening threads to " + string(iTime) + " seconds.");
    } else {
        UdpDebug->Broadcast("[SYS] Suspending listening threads.");
    }

    ServerThread *next = ServersS;
    while(next != NULL) {
        ServerThread *cur = next;
        next = cur->next;

        cur->SuspendSck(iTime);
    }
}
//---------------------------------------------------------------------------

void ServerUpdateAutoRegState() {
    if(bServerRunning == false) {
        return;
    }

    if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
#ifdef _WIN32
        regtimer = SetTimer(NULL, 0, 901000, NULL);

        if(regtimer == 0) {
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 901;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 901;
        regtmrspec.it_value.tv_nsec = 0;
    
        int iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
#endif
			AppendDebugLog("%s - [ERR] Cannot start regtimer in ServerUpdateAutoRegState\n", 0);
            exit(EXIT_FAILURE);
        }
    } else {
#ifdef _WIN32
        if(KillTimer(NULL, regtimer) == 0) {
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 0;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 0;
        regtmrspec.it_value.tv_nsec = 0;
    
        int iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
#endif
    		AppendDebugLog("%s - [ERR] Cannot stop regtimer in ServerUpdateAutoRegState\n", 0);
        	exit(EXIT_FAILURE);
        }
    }
}
//---------------------------------------------------------------------------

void ServerCreateServerThread(const int &iAddrFamily, const uint16_t &ui16PortNumber, const bool &bResume/* = false*/) {
	ServerThread * pServer = new ServerThread(iAddrFamily, ui16PortNumber);
    if(pServer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pServer in ServerCreateServerThread\n", 0);
        exit(EXIT_FAILURE);
    }

	if(pServer->Listen() == true) {
		if(ServersE == NULL) {
            ServersS = pServer;
            ServersE = pServer;
        } else {
            pServer->prev = ServersE;
            ServersE->next = pServer;
            ServersE = pServer;
        }
    } else {
        delete pServer;
    }

    if(bResume == true) {
        pServer->Resume();
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

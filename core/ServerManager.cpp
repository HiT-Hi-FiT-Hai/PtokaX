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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "globalQueue.h"
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
#include "ClientTagManager.h"
#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			#include "frmHub.h"
		#endif
	#endif
#endif
#include "HubCommands.h"
#include "IP2Country.h"
#include "LuaScript.h"
#ifdef _WIN32
	#ifndef _SERVICE
		#ifdef _MSC_VER
			#include "../gui.win/MainWindow.h"
		#endif
	#endif
#endif
#include "RegThread.h"
#ifndef _WIN32
	#include "regtmrinc.h"
#endif
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
//#include "TLSManager.h"
#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			#include "TScriptMemoryForm.h"
			#include "TScriptsForm.h"
		#endif
	#endif
#endif
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#else
	static int SIGSECTMR = SIGRTMIN+1;
#endif
//---------------------------------------------------------------------------
static ServerThread *ServersE = NULL;
#ifdef _WIN32
    UINT_PTR sectimer = 0;
    UINT_PTR regtimer = 0;
#else
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
	#ifdef _SERVICE
	    bool bService = false;
	#endif
#else
	bool bDaemon = false;
#endif
char sHubIP[16];
uint8_t ui8SrCntr = 0, ui8MinTick = 0;
//---------------------------------------------------------------------------

#ifdef _WIN32
    #ifndef _SERVICE
        #ifndef _MSC_VER
	       VOID CALLBACK SecTimerProc(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) {
        #else
            void ServerOnSecTimer() {
        #endif
    #else
        void ServerOnSecTimer() {
    #endif

	if(b2K == true) {
		FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
		GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
		int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
		int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
		double dcpuSec = double(kernelTime + userTime) / double(10000000I64);
		cpuUsage = dcpuSec - CpuUsage[ui8MinTick];
		CpuUsage[ui8MinTick] = dcpuSec;
	}
#else
static void SecTimerHandler(int sig) {
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

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			hubForm->UpdateGui();
		#else
			pMainWindow->UpdateStats();
		#endif
	#endif
#endif
}
//---------------------------------------------------------------------------

#ifdef _WIN32
    #ifndef _SERVICE
        #ifndef _MSC_VER
	       VOID CALLBACK RegTimerProc(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) {
        #else
            void ServerOnRegTimer() {
        #endif
    #else
        void ServerOnRegTimer() {
    #endif
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
				string sDbgstr = "[BUF] Cannot allocate RegisterThread! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
	        	AppendSpecialLog(sDbgstr);
	        	exit(EXIT_FAILURE);
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
	#ifndef _MSC_VER
	    randomize();
	#else
	    time_t acctime;
	    time(&acctime);
	    srand((uint32_t)acctime);
	#endif

	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);

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
	for(size_t i = 0; i < PATH.size(); i++) {
		if(sLuaPath[i] == '\\') {
			sLuaPath[i] = '/';
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

	ResNickManager = new ResNickMan();
	if(ResNickManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate ResNickManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
	    AppendSpecialLog(sDbgstr);
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

    ui8SrCntr = 0;

    ZlibUtility = new clsZlibUtility();
    if(ZlibUtility == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate ZlibUtility!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    ClientTagManager = new ClientTagMan();
    if(ClientTagManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate ClientTagManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    // PPK ... check OS if is NT
    OSVERSIONINFOEX osvi;
	memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if(GetVersionEx((OSVERSIONINFO *)&osvi) && osvi.dwMajorVersion >= 5) {
        // PPK ... set b2K to true
        b2K = true;
	}
#endif

    ui8MinTick = 0;

    iLastBytesRead = iLastBytesSent = 0;

	for(uint8_t i = 0 ; i < 60; i++) {
		CpuUsage[i] = 0;
		UploadSpeed[i] = 0;
		DownloadSpeed[i] = 0;
	}

	cpuUsage = 0.0;

#ifdef _WIN32
    #ifndef _SERVICE
		#ifndef _MSC_VER
            sectimer = SetTimer(NULL, 0, 1000, (TIMERPROC)SecTimerProc);
		#else
            sectimer = SetTimer(NULL, 0, 1000, NULL);
		#endif
    #else
		sectimer = SetTimer(NULL, 0, 1000, NULL);
    #endif

	if(sectimer == 0) {
		string sDbgstr = "[BUF] Cannot start Timer in ServerThread::ServerThread! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    regtimer = 0;
#endif

	SettingManager = new SetMan();
    if(SettingManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate SettingManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    LanguageManager = new LangMan();
    if(LanguageManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate LanguageManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    LanguageManager->LoadLanguage();

    ProfileMan = new ProfileManager();
    if(ProfileMan == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate ProfileMan!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    hashRegManager = new hashRegMan();
    if(hashRegManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate hashRegManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    // Load registered users
	hashRegManager->Load();

    hashBanManager = new hashBanMan();
    if(hashBanManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate hashBanManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    // load banlist
	hashBanManager->Load();

    TextFileManager = new TextFileMan();
    if(TextFileManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate TextFileManager!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    UdpDebug = new clsUdpDebug();
    if(UdpDebug == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate UdpDebug!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
	#ifndef _SERVICE
        #ifdef _MSC_VER
            pMainWindow = new MainWindow();

            if(pMainWindow->CreateEx() == NULL) {
                exit(EXIT_FAILURE);
            }
        #else
	        Application->CreateForm(__classid(ThubForm), &hubForm);
	    #endif
	#endif
#endif

    ScriptManager = new ScriptMan();
    if(ScriptManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate ScriptManager!";
#ifdef _WIN32
        sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

	SettingManager->UpdateAll();

#ifndef _WIN32
    struct sigaction sigactsec;
    sigactsec.sa_handler = SecTimerHandler;
    sigemptyset(&sigactsec.sa_mask);
    sigactsec.sa_flags = 0;

    if(sigaction(SIGSECTMR, &sigactsec, NULL) == -1) {
		string sDbgstr = "[BUF] Cannot create sigaction SIGSECTMR in ServerInitialize()!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    struct sigevent sigevsec;
    sigevsec.sigev_notify = SIGEV_SIGNAL;
    sigevsec.sigev_signo = SIGSECTMR;

    int iRet = timer_create(CLOCK_REALTIME, &sigevsec, &sectimer);
    
	if(iRet == -1) {
		string sDbgstr = "[BUF] Cannot create sectimer in ServerInitialize()! "+string(strerror(errno));
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    struct sigevent sigevreg;
    sigevreg.sigev_notify = SIGEV_SIGNAL;
    sigevreg.sigev_signo = SIGREGTMR;

    iRet = timer_create(CLOCK_REALTIME, &sigevreg, &regtimer);
    
	if(iRet == -1) {
		string sDbgstr = "[BUF] Cannot create regtimer in ServerInitialize()! "+string(strerror(errno));
		AppendSpecialLog(sDbgstr);
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
		string sDbgstr = "[BUF] Cannot start sectimer in ServerStart()!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }
#endif

    SettingManager->UpdateAll();

    TextFileManager->RefreshTextFiles();

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			hubForm->ButtonStart->Enabled = false;
		#else
            pMainWindow->EnableStartButton(FALSE);
		#endif
	#endif
#endif

    ui64ActualTick = ui64TotalShare = 0;

    ui64BytesRead = ui64BytesSent = ui64BytesSentSaved = 0;

	iActualBytesRead = iActualBytesSent = iAverageBytesRead = iAverageBytesSent = 0;

    ui32Joins = ui32Parts = ui32Logged = ui32Peak = 0;

    iMins = iHours = iDays = 0;

    ui8SrCntr = 0;

    if(SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true) {
        if(isIP(SettingManager->sTexts[SETTXT_HUB_ADDRESS], SettingManager->ui16TextsLens[SETTXT_HUB_ADDRESS]) == false) {
#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
				hubForm->StatusValue->Caption = String(LanguageManager->sTexts[LAN_RESOLVING_HUB_ADDRESS],
					(size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_HUB_ADDRESS])+"...";
        #else
                pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_RESOLVING_HUB_ADDRESS], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_HUB_ADDRESS])+"...").c_str());
		#endif
	#endif
#endif
            hostent *host = gethostbyname(SettingManager->sTexts[SETTXT_HUB_ADDRESS]);
			if(host == NULL) {
#ifdef _WIN32
            	int err = WSAGetLastError();
	#ifdef _SERVICE
                AppendLog(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+": "+
					string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
					string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
	#else
		#ifndef _MSC_VER
				MessageBox(Application->Handle,(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+": "+
					string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
					string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str(),
					LanguageManager->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
				hubForm->ButtonStart->Enabled = true;
        #else
				::MessageBox(pMainWindow->m_hWnd,(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+": "+
					string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
					string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str(),
					LanguageManager->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
                pMainWindow->EnableStartButton(TRUE);
		#endif
	#endif
#else
                AppendLog(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
#endif
                return false;
            } else {
				Memo("*** "+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS], (size_t)LanguageManager->ui16TextsLens[SETTXT_HUB_ADDRESS])+" "+
					string(LanguageManager->sTexts[LAN_RESOLVED_SUCCESSFULLY], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVED_SUCCESSFULLY])+".");
				sockaddr_in target;
				target.sin_addr.s_addr = *(unsigned long *)host->h_addr;
                strcpy(sHubIP, inet_ntoa(target.sin_addr));
				Memo("*** "+string(sHubIP));
            }
        } else {
            strcpy(sHubIP, SettingManager->sTexts[SETTXT_HUB_ADDRESS]);
        }
    } else {
        sHubIP[0] = '\0';
    }

    for(uint8_t i = 0; i < 25; i++) {
        if(SettingManager->iPortNumbers[i] == 0) {
            break;
        }

		ServerThread *Server = new ServerThread();
        if(Server == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate Server in ServerMan::StartServer!";
#ifdef _WIN32
			sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }

		if(Server->Listen(SettingManager->iPortNumbers[i]) == true) {
		    if(ServersE == NULL) {
                ServersS = Server;
                ServersE = Server;
            } else {
                Server->prev = ServersE;
                ServersE->next = Server;
                ServersE = Server;
            }
        } else {
            delete Server;
        }
    }

	if(ServersS == NULL) {
#ifdef _WIN32
	#ifdef _SERVICE
        AppendLog(LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED]);
	#else
		#ifndef _MSC_VER
			MessageBox(Application->Handle, LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED],
				LanguageManager->sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
			hubForm->ButtonStart->Enabled = true;
		#else
			::MessageBox(pMainWindow->m_hWnd, LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED], LanguageManager->sTexts[LAN_ERROR],
                MB_OK|MB_ICONERROR);
            pMainWindow->EnableStartButton(TRUE);
		#endif
	#endif
#else
		AppendLog(LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED]);
#endif
        return false;
    }

    AppendLog("Serving started");

//  if(tlsenabled == true) {
/*        TLSManager = new TLSMan();
        if(TLSManager == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate TLSManager!";
#ifdef _WIN32
    		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    		AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }*/
//  }

    IP2Country = new IP2CC();
    if(IP2Country == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate IP2Country!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    eventqueue = new eventq();
    if(eventqueue == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate eventqueue!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    hashManager = new hashMan();
    if(hashManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate hashManager!";
#ifdef _WIN32
    	sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    colUsers = new classUsers();
	if(colUsers == NULL) {
		string sDbgstr = "[BUF] Cannot allocate colUsers!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    globalQ = new globalqueue();
    if(globalQ == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate globalQ!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    HubCmds = new HubCommands();
    if(HubCmds == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate HubCmds!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    DcCommands = new cDcCommands();
    if(DcCommands == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate DcCommands!";
#ifdef _WIN32
    	sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
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
        UDPThread = new UDPRecvThread();
        if(UDPThread == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate UDPThread!";
#ifdef _WIN32
    		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        	AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }

        if(UDPThread->Listen() == true) {
            UDPThread->Resume();
        } else {
            delete UDPThread;
            UDPThread = NULL;
        }
    }
    
    if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager->Start();
    }
    
    srvLoop = new theLoop();
    if(srvLoop == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate srvLoop!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
    	AppendSpecialLog(sDbgstr);
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

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			hubForm->StatusValue->Caption = String(LanguageManager->sTexts[LAN_RUNNING], (size_t)LanguageManager->ui16TextsLens[LAN_RUNNING])+"...";
			hubForm->ButtonStart->Caption = String(LanguageManager->sTexts[LAN_STOP_HUB], (size_t)LanguageManager->ui16TextsLens[LAN_STOP_HUB]);
			hubForm->ButtonStart->Enabled = true;
		
			hubForm->Accepts->Visible = true;
			hubForm->Parts->Visible = true;
			hubForm->Total->Visible = true;
			hubForm->LoggedIn->Visible = true;
			hubForm->Rx->Visible = true;
			hubForm->Tx->Visible = true;
			hubForm->Peak->Visible = true;
			hubForm->AcceptsNumber->Visible = true;
			hubForm->PartsNumber->Visible = true;
			hubForm->TotalNumber->Visible = true;
			hubForm->LoggedInNumber->Visible = true;
			hubForm->RxValue->Visible = true;
			hubForm->TxValue->Visible = true;
			hubForm->PeakValue->Visible = true;
		
			if(ScriptsForm != NULL) {
				ScriptsForm->LUArst->Enabled = true;
			}
		#else
            pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_RUNNING], (size_t)LanguageManager->ui16TextsLens[LAN_RUNNING])+"...").c_str());
            pMainWindow->SetStartButtonText(LanguageManager->sTexts[LAN_STOP_HUB]);
            pMainWindow->EnableStartButton(TRUE);
            pMainWindow->EnableStatsItems(TRUE);
		#endif
	#endif
#endif

    //Start the HubRegistration timer
    if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
#ifdef _WIN32
        #ifndef _SERVICE
			#ifndef _MSC_VER
    			regtimer = SetTimer(NULL, 0, 901000, (TIMERPROC)RegTimerProc);
			#else
				regtimer = SetTimer(NULL, 0, 901000, NULL);
			#endif
        #else
			regtimer = SetTimer(NULL, 0, 901000, NULL);
        #endif

        if(regtimer == 0) {
			string sDbgstr = "[BUF] Cannot start RegTimer in ServerMan::StartServer! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 901;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 901;
        regtmrspec.it_value.tv_nsec = 0;
    
        iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
    		string sDbgstr = "[BUF] Cannot start regtimer in ServerStart()!";
#endif

			AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void ServerStop() {
#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			hubForm->ButtonStart->Enabled = false;
		#else
            pMainWindow->EnableStartButton(FALSE);
		#endif
	#endif
#else
    struct itimerspec sectmrspec;
    sectmrspec.it_interval.tv_sec = 0;
    sectmrspec.it_interval.tv_nsec = 0;
    sectmrspec.it_value.tv_sec = 0;
    sectmrspec.it_value.tv_nsec = 0;

    int iRet = timer_settime(sectimer, 0, &sectmrspec, NULL);
    if(iRet == -1) {
		string sDbgstr = "[BUF] Cannot stop sectimer in ServerStop()!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }
#endif

    char msg[1024];
#ifdef _WIN32
    int iret = sprintf(msg, "Serving stopped (UL: %I64d [%I64d], DL: %I64d)", ui64BytesSent, ui64BytesSentSaved, ui64BytesRead);
#else
    int iret = sprintf(msg, "Serving stopped (UL: %" PRIu64 " [%" PRIu64 "], DL: %" PRIu64 ")", ui64BytesSent, 
        ui64BytesSentSaved, ui64BytesRead);
#endif
    if(CheckSprintf(iret, 1024, "ServerMan::StopServer") == true) {
        AppendLog(msg);
    }

	//Stop the HubRegistration timer
	if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
#ifdef _WIN32
	   KillTimer(NULL, regtimer);
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 0;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 0;
        regtmrspec.it_value.tv_nsec = 0;
    
        iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
    		string sDbgstr = "[BUF] Cannot stop regtimer in ServerStop()!";
    		AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
#endif
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
		srvLoop->Terminate();
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

    if(UDPThread != NULL) {
        UDPThread->Close();
        UDPThread->WaitFor();
        delete UDPThread;
        UDPThread = NULL;
    }

	// delete userlist field
	if(colUsers != NULL) {
		colUsers->DisconnectAll();
		delete colUsers;
		colUsers = NULL;
    }

	if(DcCommands != NULL) {
		delete DcCommands;
        DcCommands = NULL;
    }

    if(HubCmds != NULL) {
        delete HubCmds;
        HubCmds = NULL;
    }

	// delete hashed userlist manager
	if(hashManager != NULL) {
        delete hashManager;
        hashManager = NULL;
    }

	// delete global send queue manager
	if(globalQ != NULL) {
		delete globalQ;
        globalQ = NULL;
    }

    if(RegisterThread != NULL) {
        RegisterThread->Close();
        RegisterThread->WaitFor();
        delete RegisterThread;
        RegisterThread = NULL;
    }

	if(eventqueue != NULL) {
		delete eventqueue;
        eventqueue = NULL;
    }

	if(IP2Country != NULL) {
		delete IP2Country;
        IP2Country = NULL;
    }

/*	if(TLSManager != NULL) {
		delete TLSManager;
        TLSManager = NULL;
    }*/

	//userstat  // better here ;)
//    sqldb->FinalizeAllVisits();

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			if(ScriptsForm != NULL) {
				ScriptsForm->LUArst->Enabled = false;
			}
		
			hubForm->ButtonStart->Caption = String(LanguageManager->sTexts[LAN_START_HUB], (size_t)LanguageManager->ui16TextsLens[LAN_START_HUB]);
			hubForm->ButtonStart->Enabled = true;
			hubForm->StatusValue->Caption = String(LanguageManager->sTexts[LAN_STOPPED], (size_t)LanguageManager->ui16TextsLens[LAN_STOPPED])+".";
		
			hubForm->Accepts->Visible = false;
			hubForm->Parts->Visible = false;
			hubForm->Total->Visible = false;
			hubForm->LoggedIn->Visible = false;
			hubForm->Rx->Visible = false;
			hubForm->Tx->Visible = false;
			hubForm->Peak->Visible = false;
			hubForm->AcceptsNumber->Visible = false;
			hubForm->PartsNumber->Visible = false;
			hubForm->TotalNumber->Visible = false;
			hubForm->LoggedInNumber->Visible = false;
			hubForm->RxValue->Visible = false;
			hubForm->TxValue->Visible = false;
			hubForm->PeakValue->Visible = false;
        #else
            pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_STOPPED], (size_t)LanguageManager->ui16TextsLens[LAN_STOPPED])+".").c_str());
            pMainWindow->SetStartButtonText(LanguageManager->sTexts[LAN_START_HUB]);
            pMainWindow->EnableStartButton(TRUE);
            pMainWindow->EnableStatsItems(FALSE);
		#endif
	#endif
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
#ifdef _WIN32
	#ifdef _SERVICE
		if(ServerStart() == false) {
			AppendLog("Server start failed!");
			exit(EXIT_FAILURE);
		}
	#else
		#ifndef _MSC_VER
			hubForm->ButtonStartClick(hubForm);
		#else
            if(ServerStart() == false) {
                pMainWindow->SetStatusValue((string(LanguageManager->sTexts[LAN_READY], (size_t)LanguageManager->ui16TextsLens[LAN_READY])+".").c_str());
            }
		#endif
	#endif
#else
		if(ServerStart() == false) {
            AppendLog("Server start failed!");
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

    delete ClientTagManager;
    ClientTagManager = NULL;

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

#ifdef _WIN32
	HeapDestroy(hPtokaXHeap);
	
	WSACleanup();
	
	#ifdef _SERVICE
	    PostMessage(NULL, WM_USER+1, 0, 0);
	#else
		#ifndef _MSC_VER
			Application->Terminate();
		#else
			PostMessage(NULL, WM_USER+1, 0, 0);
		#endif
	#endif
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

        for(uint8_t i = 0; i < 25; i++) {
            if(SettingManager->iPortNumbers[i] == 0) {
                break;
            }

            if(cur->usPort == SettingManager->iPortNumbers[i]) {
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
    for(uint8_t i = 0; i < 25; i++) {
        if(SettingManager->iPortNumbers[i] == 0) {
            break;
        }

        bool bFound = false;

        ServerThread *next = ServersS;
        while(next != NULL) {
            ServerThread *cur = next;
            next = cur->next;

            if(cur->usPort == SettingManager->iPortNumbers[i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
        	ServerThread *Server = new ServerThread();
            if(Server == NULL) {
            	string sDbgstr = "[BUF] Cannot allocate Server in ServerMan::UpdateServers!";
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
				AppendSpecialLog(sDbgstr);
                exit(EXIT_FAILURE);
            }
    
    		if(Server->Listen(SettingManager->iPortNumbers[i]) == true) {
    		    if(ServersE == NULL) {
                    ServersS = Server;
                    ServersE = Server;
                } else {
                    Server->prev = ServersE;
                    ServersE->next = Server;
                    ServersE = Server;
                }

    		    Server->Resume();
            } else {
                delete Server;
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
        #ifndef _SERVICE
			#ifndef _MSC_VER
    			regtimer = SetTimer(NULL, 0, 901000, (TIMERPROC)RegTimerProc);
			#else
				regtimer = SetTimer(NULL, 0, 901000, NULL);
			#endif
        #else
			regtimer = SetTimer(NULL, 0, 901000, NULL);
        #endif

        if(regtimer == 0) {
            string sDbgstr = "[BUF] Cannot start RegTimer in ServerMan::UpdateAutoRegState! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 901;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 901;
        regtmrspec.it_value.tv_nsec = 0;
    
        int iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
    		string sDbgstr = "[BUF] Cannot start regtimer in ServerUpdateAutoRegState(!";
#endif
			AppendSpecialLog(sDbgstr);
            exit(EXIT_FAILURE);
        }
    } else {
#ifdef _WIN32
        KillTimer(NULL, regtimer);
#else
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 0;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 0;
        regtmrspec.it_value.tv_nsec = 0;
    
        int iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
    		string sDbgstr = "[BUF] Cannot stop regtimer in ServerUpdateAutoRegState(!";
    		AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
#endif
    }
}
//---------------------------------------------------------------------------

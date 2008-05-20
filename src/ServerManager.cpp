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
#include "ClientTagManager.h"
#include "HubCommands.h"
#include "IP2Country.h"
#include "LuaScript.h"
#include "RegThread.h"
#include "regtmrinc.h"
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
static int SIGSECTMR = SIGRTMIN+1;
//---------------------------------------------------------------------------
static ServerThread *ServersE = NULL;
static timer_t sectimer, regtimer;
//---------------------------------------------------------------------------
double CpuUsage[60], cpuUsage = 0;
uint64_t ui64ActualTick = 0, ui64TotalShare = 0;
uint64_t ui64BytesRead = 0, ui64BytesSent = 0, ui64BytesSentSaved = 0;
uint64_t iLastBytesRead = 0, iLastBytesSent = 0;
uint32_t ui32Joins = 0, ui32Parts = 0, ui32Logged = 0, ui32Peak = 0;
uint32_t UploadSpeed[60], DownloadSpeed[60];
uint32_t iActualBytesRead = 0, iActualBytesSent = 0;
uint32_t iAverageBytesRead = 0, iAverageBytesSent = 0;
ServerThread *ServersS = NULL;
time_t starttime = 0;
uint64_t iMins = 0, iHours = 0, iDays = 0;
bool bServerRunning = false, bServerTerminated = false, bIsRestart = false, bIsClose = false, bDaemon = false;
char sHubIP[16];
uint8_t ui8SrCntr = 0, ui8MinTick = 0;
//---------------------------------------------------------------------------

static void SecTimerHandler(int sig) {
    struct rusage rs;

    getrusage(RUSAGE_SELF, &rs);

	double dcpuSec = double(rs.ru_utime.tv_sec) + (double(rs.ru_utime.tv_usec)/1000000) + 
        double(rs.ru_stime.tv_sec) + (double(rs.ru_stime.tv_usec)/1000000);
	cpuUsage = dcpuSec - CpuUsage[ui8MinTick];
	CpuUsage[ui8MinTick] = dcpuSec;

	if(++ui8MinTick == 60) {
		ui8MinTick = 0;
    }

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
}
//---------------------------------------------------------------------------

void ServerInitialize() {
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

	ResNickManager = new ResNickMan();
	if(ResNickManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate ResNickManager!";
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
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    ClientTagManager = new ClientTagMan();
    if(ClientTagManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate ClientTagManager!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    ui8MinTick = 0;

    iLastBytesRead = iLastBytesSent = 0;

	for(uint8_t i = 0 ; i < 60; i++) {
		CpuUsage[i] = 0;
		UploadSpeed[i] = 0;
		DownloadSpeed[i] = 0;
	}

	cpuUsage = 0.0;

	SettingManager = new SetMan();
    if(SettingManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate SettingManager!";
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    LanguageManager = new LangMan();
    if(LanguageManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate LanguageManager!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    LanguageManager->LoadLanguage();

    ProfileMan = new ProfileManager();
    if(ProfileMan == NULL) {
		string sDbgstr = "[BUF] Cannot allocate ProfileMan!";
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    hashRegManager = new hashRegMan();
    if(hashRegManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate hashRegManager!";
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    // Load registered users
    hashRegManager->Load();

    hashBanManager = new hashBanMan();
    if(hashBanManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate hashBanManager!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    // load banlist
    hashBanManager->Load();

    TextFileManager = new TextFileMan();
    if(TextFileManager == NULL) {
		string sDbgstr = "[BUF] Cannot allocate TextFileManager!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    UdpDebug = new clsUdpDebug();
    if(UdpDebug == NULL) {
		string sDbgstr = "[BUF] Cannot allocate UdpDebug!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    ScriptManager = new ScriptMan();
    if(ScriptManager == NULL) {
        string sDbgstr = "[BUF] Cannot allocate ScriptManager!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

	SettingManager->UpdateAll();

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
}
//---------------------------------------------------------------------------

bool ServerStart() {
    time(&starttime);

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

    SettingManager->UpdateAll();

    TextFileManager->RefreshTextFiles();

    ui64ActualTick = ui64TotalShare = 0;

    ui64BytesRead = ui64BytesSent = ui64BytesSentSaved = 0;

	iActualBytesRead = iActualBytesSent = iAverageBytesRead = iAverageBytesSent = 0;

    ui32Joins = ui32Parts = ui32Logged = ui32Peak = 0;

    iMins = iHours = iDays = 0;

    ui8SrCntr = 0;

    if(SettingManager->bBools[SETBOOL_RESOLVE_TO_IP] == true) {
        if(isIP(SettingManager->sTexts[SETTXT_HUB_ADDRESS], SettingManager->ui16TextsLens[SETTXT_HUB_ADDRESS]) == false) {
            hostent *host = gethostbyname(SettingManager->sTexts[SETTXT_HUB_ADDRESS]);
			if(host == NULL) {
                AppendLog(string(LanguageManager->sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager->ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
					" '"+string(SettingManager->sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager->sTexts[LAN_HAS_FAILED], (size_t)LanguageManager->ui16TextsLens[LAN_HAS_FAILED])+
					".\n"+string(LanguageManager->sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager->ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".");
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
        AppendLog(LanguageManager->sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED]);
        return false;
    }

    AppendLog("Serving started");

    IP2Country = new IP2CC();
    if(IP2Country == NULL) {
		string sDbgstr = "[BUF] Cannot allocate IP2Country!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    eventqueue = new eventq();
    if(eventqueue == NULL) {
		string sDbgstr = "[BUF] Cannot allocate eventqueue!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    hashManager = new hashMan();
    if(hashManager == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate hashManager!";
    	AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    colUsers = new classUsers;
	if(colUsers == NULL) {
		string sDbgstr = "[BUF] Cannot allocate colUsers!";
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    globalQ = new queue;
    if(globalQ == NULL) {
		string sDbgstr = "[BUF] Cannot allocate globalQ!";
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    HubCmds = new HubCommands;
    if(HubCmds == NULL) {
		string sDbgstr = "[BUF] Cannot allocate HubCmds!";
    	AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    DcCommands = new cDcCommands;
    if(DcCommands == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate DcCommands!";
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
        	AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
        UDPThread->Resume();
    }

	ScriptManager->SaveScripts();
    
    if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager->Start();
    }
    
    srvLoop = new theLoop();
    if(srvLoop == NULL) {
		string sDbgstr = "[BUF] Cannot allocate srvLoop!";
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

    //Start the HubRegistration timer
    if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 901;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 901;
        regtmrspec.it_value.tv_nsec = 0;
    
        iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
    		string sDbgstr = "[BUF] Cannot start regtimer in ServerStart()!";
    		AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
    }

    return true;
}
//---------------------------------------------------------------------------

void ServerStop() {
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

    char msg[1024];
    int iret = sprintf(msg, "Serving stopped (UL: %" PRIu64 " [%" PRIu64 "], DL: %" PRIu64 ")", ui64BytesSent, 
        ui64BytesSentSaved, ui64BytesRead);
    if(CheckSprintf(iret, 1024, "ServerMan::StopServer") == true) {
        AppendLog(msg);
    }

	//Stop the HubRegistration timer
	if(SettingManager->bBools[SETBOOL_AUTO_REG] == true) {
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

	//userstat  // better here ;)
//    sqldb->FinalizeAllVisits();

    ui8SrCntr = 0;
    ui32Joins = ui32Parts = ui32Logged = 0;

    UdpDebug->Cleanup();

    bServerRunning = false;

    if(bIsRestart == true) {
        bIsRestart = false;
		// start hub
		if(ServerStart() == false) {
            AppendLog("Server start failed!");
            exit(EXIT_FAILURE);
        }
    } else if(bIsClose == true) {
		ServerFinalClose();
    }
}
//---------------------------------------------------------------------------

void ServerFinalClose() {
    timer_delete(sectimer);
    timer_delete(regtimer);

    hashBanManager->Save(true);

    hashRegManager->Save();

	ScriptManager->SaveScripts();

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
        struct itimerspec regtmrspec;
        regtmrspec.it_interval.tv_sec = 901;
        regtmrspec.it_interval.tv_nsec = 0;
        regtmrspec.it_value.tv_sec = 901;
        regtmrspec.it_value.tv_nsec = 0;
    
        int iRet = timer_settime(regtimer, 0, &regtmrspec, NULL);
        if(iRet == -1) {
    		string sDbgstr = "[BUF] Cannot start regtimer in ServerUpdateAutoRegState(!";
    		AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
    } else {
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
    }
}
//---------------------------------------------------------------------------

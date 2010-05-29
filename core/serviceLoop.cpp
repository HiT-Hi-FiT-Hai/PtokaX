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
#include "serviceLoop.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "globalQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _SERVICE
		#ifndef _MSC_VER
			#include "TUsersChatForm.h"
		#else
            #include "../gui.win/MainWindowPageUsersChat.h"
		#endif
	#endif
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#else
	#include "regtmrinc.h"
	#include "scrtmrinc.h"
#endif
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
theLoop *srvLoop = NULL;
#ifdef _WIN32
    UINT_PTR srvLoopTimer = 0;
#endif
//---------------------------------------------------------------------------

#ifndef _WIN32
static void RegTimerHandler() {
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
			string sDbgstr = "[BUF] Cannot allocate RegisterThread!";
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

#ifdef _WIN32
    #ifndef _SERVICE
        #ifndef _MSC_VER
            VOID CALLBACK LooperProc(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) {
        #else
            void theLoop::Looper() {
        #endif
    #else
    void theLoop::Looper() {
    #endif
		KillTimer(NULL, srvLoopTimer);

		// PPK ... two loop stategy for saving badwith
		if(srvLoop->bRecv == true) {
			srvLoop->ReceiveLoop();
	    } else {
			srvLoop->SendLoop();
			eventqueue->ProcessEvents();
	    }
	
		if(bServerTerminated == false) {
			srvLoop->bRecv = !srvLoop->bRecv;
	#ifndef _SERVICE
		#ifndef _MSC_VER
	    	srvLoopTimer = SetTimer(NULL, 0, 100, (TIMERPROC)LooperProc);
		#else
			srvLoopTimer = SetTimer(NULL, 0, 100, NULL);
		#endif
    #else
            srvLoopTimer = SetTimer(NULL, 0, 100, NULL);
    #endif

	        if(srvLoopTimer == 0) {
				string sDbgstr = "[BUF] Cannot start Looper in LooperProc! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
	        	AppendSpecialLog(sDbgstr);
	        	exit(EXIT_FAILURE);
	        }
	    } else {
	        // tell the scripts about the end
	    	ScriptManager->OnExit();
	    
	        // send last possible global data
	        globalQ->SendGlobalQ();
	    
	        ServerFinalStop(true);
	
	    }
	}
#else
	void theLoop::Looper() {
		// PPK ... two loop stategy for saving badwith
		if(bRecv == true) {
			ReceiveLoop();
	    } else {
			SendLoop();
			eventqueue->ProcessEvents();
	    }
	
		if(bServerTerminated == false) {
			bRecv = !bRecv;
	    } else {
	        // tell the scripts about the end
	    	ScriptManager->OnExit();
	
	        // send last possible global data
	        globalQ->SendGlobalQ();
	
	        ServerFinalStop(true);
	    }
	}
#endif
//---------------------------------------------------------------------------

theLoop::theLoop() {
#ifdef _WIN32
    InitializeCriticalSection(&csAcceptQueue);
#else
	pthread_mutex_init(&mtxAcceptQueue, NULL);
#endif

    AcceptedSocketsS = NULL;
    AcceptedSocketsE = NULL;
    
    bRecv = true;
    
    iLstUptmTck = ui64ActualTick;

	bServerTerminated = false;
	iSendRestsPeak = iRecvRestsPeak = iLastSendRest = iLastRecvRest = iLoopsForLogins = 0;
	dLoggedUsers = dActualSrvLoopLogins = 0;

#ifdef _WIN32
    #ifndef _SERVICE
		#ifndef _MSC_VER
			srvLoopTimer = SetTimer(NULL, 0, 100, (TIMERPROC)LooperProc);
		#else
			srvLoopTimer = SetTimer(NULL, 0, 100, NULL);
		#endif
    #else
		srvLoopTimer = SetTimer(NULL, 0, 100, NULL);
    #endif

    if(srvLoopTimer == 0) {
		string sDbgstr = "[BUF] Cannot start Looper in theLoop::theLoop! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }
#else
    sigemptyset(&sst);
    sigaddset(&sst, SIGSCRTMR);
    sigaddset(&sst, SIGREGTMR);

    zerotime.tv_sec = 0;
    zerotime.tv_nsec = 0;
#endif

	Cout("MainLoop executed.");
}
//---------------------------------------------------------------------------

theLoop::~theLoop() {
    AcceptedSocket *nextsck = AcceptedSocketsS;
        
    while(nextsck != NULL) {
        AcceptedSocket *cursck = nextsck;
		nextsck = cursck->next;
#ifdef _WIN32
		shutdown(cursck->s, SD_SEND);
        closesocket(cursck->s);
#else
		shutdown(cursck->s, SHUT_RDWR);
        close(cursck->s);
#endif
        delete cursck;
	}
   
    AcceptedSocketsS = NULL;
    AcceptedSocketsE = NULL;

#ifdef _WIN32
	DeleteCriticalSection(&csAcceptQueue);
#else
	pthread_mutex_destroy(&mtxAcceptQueue);
#endif

	Cout("MainLoop terminated.");
}
//---------------------------------------------------------------------------

void theLoop::AcceptUser(AcceptedSocket *AccptSocket) {
    // set the recv buffer
#ifdef _WIN32
    int32_t bufsize = 8192;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
    	int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %s (%d)", 
            inet_ntoa(AccptSocket->addr.sin_addr), WSErrorStr(iError), iError);
#else
    int bufsize = 8192;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
    	int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
#endif
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser1") == true) {
    	   UdpDebug->Broadcast(msg, imsgLen);
        }
#ifdef _WIN32
    	shutdown(AccptSocket->s, SD_SEND);
        closesocket(AccptSocket->s);
#else
    	shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
#endif
        return;
    }

    // set the send buffer
    bufsize = 32768;
#ifdef _WIN32
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %s (%d)", 
            inet_ntoa(AccptSocket->addr.sin_addr), WSErrorStr(iError), iError);
#else
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
#endif
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser2") == true) {
	       UdpDebug->Broadcast(msg, imsgLen);
        }
#ifdef _WIN32
	    shutdown(AccptSocket->s, SD_SEND);
        closesocket(AccptSocket->s);
#else
	    shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
#endif
        return;
    }

    // set sending of keepalive packets
#ifdef _WIN32
    bool bKeepalive = true;
    setsockopt(AccptSocket->s, SOL_SOCKET, SO_KEEPALIVE, (char *) &bKeepalive, sizeof(bKeepalive));
#else
    int iKeepAlive = 1;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_KEEPALIVE, &iKeepAlive, sizeof(iKeepAlive)) == -1) {
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_KEEPALIVE. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser3") == true) {
	       UdpDebug->Broadcast(msg, imsgLen);
        }
	    shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
        return;
    }
#endif

    // set non-blocking mode
#ifdef _WIN32
    uint32_t block = 1;
	if(ioctlsocket(AccptSocket->s, FIONBIO, (unsigned long *)&block) == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
    	int imsgLen = sprintf(msg, "[SYS] ioctlsocket failed on attempt to set FIONBIO. IP: %s Err: %s (%d)", 
            inet_ntoa(AccptSocket->addr.sin_addr), WSErrorStr(iError), iError);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser3") == true) {
#else
    int oldFlag = fcntl(AccptSocket->s, F_GETFL, 0);
    if(fcntl(AccptSocket->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
    	int imsgLen = sprintf(msg, "[SYS] fcntl failed on attempt to set O_NONBLOCK. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser4") == true) {
#endif
		  UdpDebug->Broadcast(msg, imsgLen);
		}
#ifdef _WIN32
		shutdown(AccptSocket->s, SD_SEND);
		closesocket(AccptSocket->s);
#else
		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
#endif
		return;
    }
    
    if(SettingManager->bBools[SETBOOL_REDIRECT_ALL] == true) {
       	int imsgLen = sprintf(msg, "<%s> %s %s|%s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_REDIR_TO],
            SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS], SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS]);
#ifdef _WIN32
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser4") == true) {
#else
		if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser5") == true) {
#endif
            send(AccptSocket->s, msg, imsgLen, 0);
            ui64BytesSent += imsgLen;
        }
#ifdef _WIN32
        shutdown(AccptSocket->s, SD_SEND);
        closesocket(AccptSocket->s);
#else
        shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
#endif
		return;
    }

#ifdef _WIN32
    uint32_t hash = 16777216 * AccptSocket->addr.sin_addr.S_un.S_un_b.s_b1 + 65536 * AccptSocket->addr.sin_addr.S_un.S_un_b.s_b2
		+ 256 * AccptSocket->addr.sin_addr.S_un.S_un_b.s_b3 + AccptSocket->addr.sin_addr.S_un.S_un_b.s_b4;
#else
    uint32_t hash;
    HashIP(AccptSocket->IP, strlen(AccptSocket->IP), hash);
#endif

    time_t acc_time;
    time(&acc_time);

	BanItem *Ban = hashBanManager->FindFull(hash, acc_time);

	if(Ban != NULL) {
        if(((Ban->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
            int imsglen;
            char *messg = GenerateBanMessage(Ban, imsglen, acc_time);
            send(AccptSocket->s, messg, imsglen, 0);
#ifdef _WIN32
            shutdown(AccptSocket->s, SD_SEND);
            closesocket(AccptSocket->s);
#else
            shutdown(AccptSocket->s, SHUT_RD);
            close(AccptSocket->s);
#endif
/*            int imsgLen = sprintf(msg, "[SYS] Banned ip %s - connection closed.", AccptSocket->IP);
            if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser5") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }*/
            return;
        }
    }

	RangeBanItem *RangeBan = hashBanManager->FindFullRange(hash, acc_time);

	if(RangeBan != NULL) {
        if(((RangeBan->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
            int imsglen;
            char *messg = GenerateRangeBanMessage(RangeBan, imsglen, acc_time);
            send(AccptSocket->s, messg, imsglen, 0);
#ifdef _WIN32
            shutdown(AccptSocket->s, SD_SEND);
            closesocket(AccptSocket->s);
#else
            shutdown(AccptSocket->s, SHUT_RD);
            close(AccptSocket->s);
#endif
/*            int imsgLen = sprintf(msg, "[SYS] Range Banned ip %s - connection closed.", AccptSocket->IP);
            if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser6") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }*/
            return;
        }
    }

    ui32Joins++;

    // set properties of the new user object
	User *u = new User();

	if(u == NULL) {
		string sDbgstr = "[BUF] Cannot allocate new user!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
#ifdef _WIN32
		shutdown(AccptSocket->s, SD_SEND);
		closesocket(AccptSocket->s);
#else
		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
#endif
		return;
	}

    u->uLogInOut->logonClk = ui64ActualTick;
    u->Sck = AccptSocket->s;
//    u->sin_len = AccptSocket->sin_len;
//    u->addr = AccptSocket->addr;
    u->ui32IpHash = hash;
    UserSetIP(u, AccptSocket->IP);
//    u->PORT = ntohs(u->addr.sin_port);
    u->iState = User::STATE_KEY_OR_SUP;

    UserMakeLock(u);
    
    if(Ban != NULL) {
        uint32_t hash = 0;
        if(((Ban->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
            hash = Ban->ui32NickHash;
        }
        int imsglen;
        char *messg = GenerateBanMessage(Ban, imsglen, acc_time);
        u->uLogInOut->uBan = new UserBan(messg, imsglen, hash);
        if(u->uLogInOut->uBan == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate new uBan!";
#ifdef _WIN32
            sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        	AppendSpecialLog(sDbgstr);
        }
    } else if(RangeBan != NULL) {
        int imsglen;
        char *messg = GenerateRangeBanMessage(RangeBan, imsglen, acc_time);
        u->uLogInOut->uBan = new UserBan(messg, imsglen, 0);
        if(u->uLogInOut->uBan == NULL) {
        	string sDbgstr = "[BUF] Cannot allocate new uBan!";
#ifdef _WIN32
            sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
        	AppendSpecialLog(sDbgstr);
        }
    }
        
    // Everything is ok, now add to users...
    colUsers->AddUser(u);
}
//---------------------------------------------------------------------------

void theLoop::ReceiveLoop() {
#ifndef _WIN32
    while((iSIG = sigtimedwait(&sst, &info, &zerotime)) != -1) {
        if(iSIG == SIGSCRTMR) {
            ScriptOnTimer((ScriptTimer *)info.si_value.sival_ptr);
        } else if(iSIG == SIGREGTMR) {
            RegTimerHandler();
        }
    }
#endif

    // Receiving loop for process all incoming data and store in queues
    uint32_t iRecvRests = 0;
    
    ui8SrCntr++;
    
    if(ui8SrCntr >= 7 || (colUsers->ui16ActSearchs + colUsers->ui16PasSearchs) > 8 || 
        colUsers->ui16ActSearchs > 5) {
        ui8SrCntr = 0;
    }

    if(ui64ActualTick-iLstUptmTck > 60) {
        time_t acctime;
        time(&acctime);
        acctime -= starttime;
    
        uint64_t iValue = acctime / 86400;
		acctime -= (time_t)(86400*iValue);
        iDays = iValue;
    
        iValue = acctime / 3600;
    	acctime -= (time_t)(3600*iValue);
        iHours = iValue;
    
    	iValue = acctime / 60;
        iMins = iValue;

#ifdef _WIN32
        if(iMins == 0 || iMins == 15 || iMins == 30 || iMins == 45) {
            if(HeapValidate(GetProcessHeap(), 0, 0) == 0) {
                AppendSpecialLog("Process memory heap corrupted!");
            }
            HeapCompact(GetProcessHeap(), 0);

            if(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendSpecialLog("PtokaX memory heap corrupted!");
            }
            HeapCompact(hPtokaXHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendSpecialLog("Recv memory heap corrupted!");
            }
            HeapCompact(hRecvHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendSpecialLog("Send memory heap corrupted!");
            }
            HeapCompact(hSendHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(ScriptManager->hLuaHeap, 0, 0) == 0) {
                AppendSpecialLog("Lua memory heap corrupted!");
            }
            HeapCompact(ScriptManager->hLuaHeap, 0);
        }
#endif

        iLstUptmTck = ui64ActualTick;
    }

    AcceptedSocket *NextSck = NULL;

#ifdef _WIN32
    EnterCriticalSection(&csAcceptQueue);
#else
	pthread_mutex_lock(&mtxAcceptQueue);
#endif

    if(AcceptedSocketsS != NULL) {
        NextSck = AcceptedSocketsS;
        AcceptedSocketsS = NULL;
        AcceptedSocketsE = NULL;
    }

#ifdef _WIN32
    LeaveCriticalSection(&csAcceptQueue);
#else
	pthread_mutex_unlock(&mtxAcceptQueue);
#endif

    while(NextSck != NULL) {
        AcceptedSocket *CurSck = NextSck;
        NextSck = CurSck->next;
        AcceptUser(CurSck);
        delete CurSck;
    }

    User *nextUser = colUsers->llist;
    while(nextUser != 0 && bServerTerminated == false) {
        User *curUser = nextUser;
        nextUser = curUser->next;

        // PPK ... true == we have rest ;)
        if(UserDoRecv(curUser) == true) {
            iRecvRests++;
            //Memo("Rest " + string(curUser->Nick, curUser->NickLen) + ": '" + string(curUser->recvbuf) + "'");
        }

        //    curUser->ProcessLines();
        //}

        switch(curUser->iState) {
            case User::STATE_KEY_OR_SUP:{
                // check logon timeout for iState 1
                if(ui64ActualTick - curUser->uLogInOut->logonClk > 20) {
                    int imsgLen = sprintf(msg, "[SYS] Login timeout 1 for %s - user disconnected.", curUser->IP);
                    if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop3") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }
                    UserClose(curUser);
                    continue;
                }
                break;
            }
            case User::STATE_ADDME: {
                // PPK ... Add user, but only if send $GetNickList (or have quicklist supports) <- important, used by flooders !!!
                if(((curUser->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == false && 
                    ((curUser->ui32BoolBits & User::BIT_SUPPORT_QUICKLIST) == User::BIT_SUPPORT_QUICKLIST) == false && 
                    ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                    continue;

                int imsgLen = GetWlcmMsg(msg);
                UserSendCharDelayed(curUser, msg, imsgLen);
                curUser->iState = User::STATE_ADDME_1LOOP;
                continue;
            }
            case User::STATE_ADDME_1LOOP: {
                // PPK ... added login delay.
                if(dLoggedUsers >= dActualSrvLoopLogins && ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    if(ui64ActualTick - curUser->uLogInOut->logonClk > 300) {
                        int imsgLen = sprintf(msg, "[SYS] Login timeout (%d) 3 for %s (%s) - user disconnected.", curUser->iState, curUser->Nick, curUser->IP);
                        if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop4") == true) {
                            UdpDebug->Broadcast(msg, imsgLen);
                        }
                        UserClose(curUser);
                    }
                    continue;
                }

                // PPK ... is not more needed, free mem ;)
                if(curUser->uLogInOut->sLockUsrConn != NULL) {
#ifdef _WIN32
                    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->uLogInOut->sLockUsrConn) == 0) {
						string sDbgstr = "[BUF] Cannot deallocate curUser->uLogInOut->sLockUsrConn in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr);
                    }
#else
					free(curUser->uLogInOut->sLockUsrConn);
#endif
                    curUser->uLogInOut->sLockUsrConn = NULL;
                }

                //New User Connected ... the user is operator ? invoke lua User/OpConnected
                uint32_t iBeforeLuaLen = curUser->sbdatalen;

				ScriptManager->UserConnected(curUser);
				if(curUser->iState >= User::STATE_CLOSING) {// connection closed by script?
					continue;
				}

                if(iBeforeLuaLen < curUser->sbdatalen) {
                    size_t iNdLen = curUser->sbdatalen-iBeforeLuaLen;
#ifdef _WIN32
                    curUser->uLogInOut->sLockUsrConn = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iNdLen+1);
#else
					curUser->uLogInOut->sLockUsrConn = (char *) malloc(iNdLen+1);
#endif
                    if(curUser->uLogInOut->sLockUsrConn == NULL) {
						string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iNdLen+1))+
							" bytes of memory for sLockUsrConn in theLoop::ReceiveLoop!";
#ifdef _WIN32
						sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
						AppendSpecialLog(sDbgstr);
                		return;
                    }
                    memcpy(curUser->uLogInOut->sLockUsrConn, curUser->sendbuf+iBeforeLuaLen, iNdLen);
                	curUser->uLogInOut->iUserConnectedLen = (uint32_t)iNdLen;
                	curUser->uLogInOut->sLockUsrConn[curUser->uLogInOut->iUserConnectedLen] = '\0';
                	curUser->sbdatalen = iBeforeLuaLen;
                	curUser->sendbuf[curUser->sbdatalen] = '\0';
                }

                // PPK ... wow user is accepted, now add it :)
                if(((curUser->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG) == true) {
                    UserHasSuspiciousTag(curUser);
                }
                    
                UserAdd2Userlist(curUser);
                
                ui32Logged++;
                dLoggedUsers++;
                curUser->iState = User::STATE_ADDME_2LOOP;
                ui64TotalShare += curUser->sharedSize;
                curUser->ui32BoolBits |= User::BIT_HAVE_SHARECOUNTED;
                
                if(ui32Peak < ui32Logged) {
                    ui32Peak = ui32Logged;
                    if(SettingManager->iShorts[SETSHORT_MAX_USERS_PEAK] < (int16_t)ui32Peak)
                        SettingManager->SetShort(SETSHORT_MAX_USERS_PEAK, (int16_t)ui32Peak);
                }

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
					if(UsersChatForm != NULL && UsersChatForm->UsersAutoUpd->Checked == true) {
	               	    if(curUser->NickLen > 1) {
	                   	    if(UsersChatForm->userList->Items->IndexOf(curUser->Nick) == -1) {
	                            UsersChatForm->userList->AddItem(curUser->Nick, NULL);
	                        }
	                    } else {
	                        int imsgLen = sprintf(msg, "[SYS] User nick length == 1, user wasn't added to UserList listbox. %s (%s)", curUser->Nick, curUser->IP);
	                        if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop5") == true) {
	              		        UdpDebug->Broadcast(msg, imsgLen);
	                        }
	                    }
					}
        #else
                    if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        pMainWindowPageUsersChat->AddUser(curUser);
                    }
		#endif
	#endif
#endif
//                if(sqldb) sqldb->AddVisit(curUser);

                // PPK ... change to NoHello supports
            	int imsgLen = sprintf(msg, "$Hello %s|", curUser->Nick);
            	if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop6") == true) {
                    globalQ->HStore(msg, imsgLen);
                }

                globalQ->UserIPStore(curUser);

                switch(SettingManager->ui8FullMyINFOOption) {
                    case 0:
                        globalQ->InfoStore(curUser->MyInfoTag, curUser->iMyInfoTagLen);
                        break;
                    case 1:
                        globalQ->FullInfoStore(curUser->MyInfoTag, curUser->iMyInfoTagLen);
                        globalQ->StrpInfoStore(curUser->MyInfo, curUser->iMyInfoLen);
                        break;
                    case 2:
                        globalQ->InfoStore(curUser->MyInfo, curUser->iMyInfoLen);
                        break;
                    default:
                        break;
                }
                
                if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                    globalQ->OpListStore(curUser->Nick);
                }
                
                curUser->iLastMyINFOSendTick = ui64ActualTick;
                break;
            }
            case User::STATE_ADDED:
                if(curUser->cmdToUserStrt != NULL) {
                    PrcsdToUsrCmd *next = curUser->cmdToUserStrt;
                        
                    curUser->cmdToUserStrt = NULL;
                    curUser->cmdToUserEnd = NULL;
            
                    while(next != NULL) {
                        PrcsdToUsrCmd *cur = next;
                        next = cur->next;
                                               
                        if(cur->iLoops >= 2) {
                            User * ToUser = hashManager->FindUser(cur->ToNick, cur->iToNickLen);
                            if(ToUser == cur->To) {
                                if(SettingManager->iShorts[SETSHORT_MAX_PM_COUNT_TO_USER] == 0 || cur->iPmCount == 0) {
                                    UserSendCharDelayed(cur->To, cur->sCommand, cur->iLen);
                                } else {
                                    if(cur->To->iReceivedPmCount == 0) {
                                        cur->To->iReceivedPmTick = ui64ActualTick;
                                    } else if(cur->To->iReceivedPmCount >= (uint32_t)SettingManager->iShorts[SETSHORT_MAX_PM_COUNT_TO_USER]) {
                                        if(cur->To->iReceivedPmTick+60 < ui64ActualTick) {
                                            cur->To->iReceivedPmTick = ui64ActualTick;
                                            cur->To->iReceivedPmCount = 0;
                                        } else {
                                            bool bSprintfCheck;
                                            int imsgLen;
                                            if(cur->iPmCount == 1) {
                                                imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s %s %s!|", curUser->Nick, cur->ToNick, 
                                                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SRY_LST_MSG_BCS], 
                                                    cur->ToNick, LanguageManager->sTexts[LAN_EXC_MSG_LIMIT]);
                                                bSprintfCheck = CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop1");
                                            } else {
                                                imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s %d %s %s %s!|", curUser->Nick, 
                                                    cur->ToNick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_LAST], 
                                                    cur->iPmCount, LanguageManager->sTexts[LAN_MSGS_NOT_SENT], cur->ToNick, 
                                                    LanguageManager->sTexts[LAN_EXC_MSG_LIMIT]);
                                                bSprintfCheck = CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop2");
                                            }
                                            if(bSprintfCheck == true) {
                                                UserSendCharDelayed(curUser, msg, imsgLen);
                                            }

#ifdef _WIN32
                                            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
												string sDbgstr = "[BUF] Cannot deallocate cur->sCommand in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
													string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
												AppendSpecialLog(sDbgstr);
                                            }
#else
											free(cur->sCommand);
#endif
                                            cur->sCommand = NULL;

#ifdef _WIN32
                                            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->ToNick) == 0) {
												string sDbgstr = "[BUF] Cannot deallocate cur->ToNick in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
													string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
												AppendSpecialLog(sDbgstr);
                                            }
#else
											free(cur->ToNick);
#endif
                                            cur->ToNick = NULL;
        
                                            delete cur;

                                            continue;
                                        }
                                    }  
                                    UserSendCharDelayed(cur->To, cur->sCommand, cur->iLen);
                                    cur->To->iReceivedPmCount += cur->iPmCount;
                                }
                            }
                        } else {
                            cur->iLoops++;
                            if(curUser->cmdToUserStrt == NULL) {
                                cur->next = NULL;
                                curUser->cmdToUserStrt = cur;
                                curUser->cmdToUserEnd = cur;
                            } else {
                                curUser->cmdToUserEnd->next = cur;
                                curUser->cmdToUserEnd = cur;
                            }
                            continue;
                        }

#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate cur->sCommand1 in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(cur->sCommand);
#endif
                        cur->sCommand = NULL;

#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->ToNick) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate cur->ToNick1 in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(cur->ToNick);
#endif
                        cur->ToNick = NULL;
        
                        delete cur;
					}
                }
        
                if(ui8SrCntr == 0) {
                    if(curUser->cmdASearch != NULL) {
                        globalQ->AStore(curUser->cmdASearch->sCommand, curUser->cmdASearch->iLen);
                    }
                    if(curUser->cmdPSearch != NULL) {
                        globalQ->PStore(curUser->cmdPSearch->sCommand, curUser->cmdPSearch->iLen);

#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->cmdPSearch->sCommand) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate curUser->cmdPSearch->sCommand in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(curUser->cmdPSearch->sCommand);
#endif
                        curUser->cmdPSearch->sCommand = NULL;
        
                        delete curUser->cmdPSearch;
                        curUser->cmdPSearch = NULL;
                    }
                }

                // PPK ... deflood memory cleanup, if is not needed anymore
				if(ui8SrCntr == 0) {
					if(curUser->sLastChat != NULL && curUser->ui16LastChatLines < 2 && 
                        (curUser->ui64SameChatsTick+SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_TIME]) < ui64ActualTick) {
#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->sLastChat) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate curUser->sLastChat in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
                            AppendSpecialLog(sDbgstr);
                        }
#else
						free(curUser->sLastChat);
#endif
                        curUser->sLastChat = NULL;
						curUser->ui16LastChatLen = 0;
						curUser->ui16SameMultiChats = 0;
						curUser->ui16LastChatLines = 0;
                    }

					if(curUser->sLastPM != NULL && curUser->ui16LastPmLines < 2 && 
                        (curUser->ui64SamePMsTick+SettingManager->iShorts[SETSHORT_SAME_PM_TIME]) < ui64ActualTick) {
#ifdef _WIN32
						if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->sLastPM) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate curUser->sLastPM in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(curUser->sLastPM);
#endif
                        curUser->sLastPM = NULL;
                        curUser->ui16LastPMLen = 0;
						curUser->ui16SameMultiPms = 0;
                        curUser->ui16LastPmLines = 0;
                    }
                    
					if(curUser->sLastSearch != NULL && (curUser->ui64SameSearchsTick+SettingManager->iShorts[SETSHORT_SAME_SEARCH_TIME]) < ui64ActualTick) {
#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->sLastSearch) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate curUser->sLastSearch in theLoop::ReceiveLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(curUser->sLastSearch);
#endif
                        curUser->sLastSearch = NULL;
                        curUser->ui16LastSearchLen = 0;
                    }
                }
                continue;
            case User::STATE_CLOSING: {
                if(((curUser->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) == false && curUser->sbdatalen != 0) {
                  	if(curUser->uLogInOut->iToCloseLoops != 0 || ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
                		UserTry2Send(curUser);
                		curUser->uLogInOut->iToCloseLoops--;
                		continue;
                	}
                }
            	curUser->iState = User::STATE_REMME;
            	continue;
            }
            // if user is marked as dead, remove him
            case User::STATE_REMME: {
#ifdef _WIN32
                shutdown(curUser->Sck, SD_SEND);
                closesocket(curUser->Sck);
#else
                shutdown(curUser->Sck, SHUT_RD);
                close(curUser->Sck);
#endif

                // linked list
                colUsers->RemUser(curUser);

                delete curUser;
                continue;
            }
            default: {
                // check logon timeout
                if(ui64ActualTick - curUser->uLogInOut->logonClk > 60) {
                    int imsgLen = sprintf(msg, "[SYS] Login timeout (%d) 2 for %s (%s) - user disconnected.", curUser->iState, curUser->Nick, curUser->IP);
                    if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop7") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }
                    UserClose(curUser);
                    continue;
                }
                break;
            }
        }
    }
    
    if(ui8SrCntr == 0) {
        colUsers->ui16ActSearchs = 0;
        colUsers->ui16PasSearchs = 0;
    }

    iLastRecvRest = iRecvRests;
    iRecvRestsPeak = iRecvRests > iRecvRestsPeak ? iRecvRests : iRecvRestsPeak;
}
//---------------------------------------------------------------------------

void theLoop::SendLoop() {
    // PPK ... send loop
    // now logging users get changed myinfo with myinfos
    // old users get it in this loop from queue -> badwith saving !!! no more twice myinfo =)
    // Sending Loop
    uint32_t iSendRests = 0;

    // PPK ... now finalize queues
    globalQ->FinalizeQueues();

    User *nextUser = colUsers->llist;
    while(nextUser != 0 && bServerTerminated == false) {
        User *curUser = nextUser;
        nextUser = curUser->next;

        switch(curUser->iState) {
            case User::STATE_ADDME_2LOOP: {
            	curUser->iState = User::STATE_ADDED;

            	// finaly send the nicklist/myinfos/oplist
                UserAddUserList(curUser);
                
                // PPK ... UserIP2 supports
                if(SettingManager->bBools[SETBOOL_SEND_USERIP2_TO_USER_ON_LOGIN] == true && ProfileMan->IsAllowed(curUser, ProfileManager::SENDALLUSERIP) == false && 
                    ((curUser->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
            		int imsgLen = sprintf(msg, "$UserIP %s %s|", curUser->Nick, curUser->IP);
            		if(CheckSprintf(imsgLen, 1024, "theLoop::SendLoop1") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                
                curUser->ui32BoolBits &= ~User::BIT_GETNICKLIST;

                // PPK ... send motd ???
                if(SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD] != 0) {
                    if(SettingManager->bBools[SETBOOL_MOTD_AS_PM] == true) {
#ifdef _WIN32
                        char * sMSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65);
#else
						char * sMSG = (char *) malloc(SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65);
#endif
                        if(sMSG == NULL) {
							string sDbgstr = "[BUF] Cannot allocate "+string(SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65)+
								" bytes of memory in theLoop::SendLoop!";
#ifdef _WIN32
							sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
							AppendSpecialLog(sDbgstr);
                            exit(EXIT_FAILURE);
                        }

                        int imsgLen = sprintf(sMSG, SettingManager->sPreTexts[SetMan::SETPRETXT_MOTD], curUser->Nick);
                        if(CheckSprintf(imsgLen, SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65, "theLoop::SendLoop2") == true) {
                            UserSendCharDelayed(curUser, sMSG, imsgLen);
                        }

#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMSG) == 0) {
							string sDbgstr = "[BUF] Cannot deallocate memory in theLoop::SendLoop! "+string((uint32_t)GetLastError())+" "+
								string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
							AppendSpecialLog(sDbgstr);
                        }
#else
						free(sMSG);
#endif
                    } else {
                        UserSendCharDelayed(curUser, SettingManager->sPreTexts[SetMan::SETPRETXT_MOTD],
                            SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]);
                    }
                }

                // check for Debug subscription
                if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true)
                    UdpDebug->CheckUdpSub(curUser, true);

                if(curUser->uLogInOut->iUserConnectedLen != 0) {
                    UserPutInSendBuf(curUser, curUser->uLogInOut->sLockUsrConn, curUser->uLogInOut->iUserConnectedLen);
#ifdef _WIN32
                    if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->uLogInOut->sLockUsrConn) == 0) {
						string sDbgstr = "[BUF] Cannot deallocate curUser->uLogInOut->sLockUsrConn in theLoop::SendLoop! "+string((uint32_t)GetLastError())+" "+
							string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
						AppendSpecialLog(sDbgstr);
                    }
#else
					free(curUser->uLogInOut->sLockUsrConn);
#endif
                    curUser->uLogInOut->sLockUsrConn = NULL;
                }

                // Login struct no more needed, free mem ! ;)
                delete curUser->uLogInOut;
                curUser->uLogInOut = NULL;

            	// PPK ... send all login data from buffer !
            	if(curUser->sbdatalen > 0) {
                    UserTry2Send(curUser);
                }
                    
                // apply one loop delay to avoid double sending of hello and oplist
            	continue;
            }
            case User::STATE_ADDED: {
                // process global data queues
                if(globalQ->bHaveQueue == true) {
                    globalQ->ProcessQueues(curUser);
                } else if(((curUser->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == true) {
                    UserAddUserList(curUser);
                    curUser->ui32BoolBits &= ~User::BIT_GETNICKLIST;
                }
                
            	if(globalQ->SingleItemsQueueS != NULL) {
                    globalQ->ProcessSingleItems(curUser);
            	}

                // send data acumulated by queues above
                // if sending caused error, close the user
                if(curUser->sbdatalen != 0) {
                    // PPK ... true = we have rest ;)
                	if(UserTry2Send(curUser) == true) {
                	    iSendRests++;
                	}
                }
                break;
            }
            case User::STATE_CLOSING:
            case User::STATE_REMME:
                continue;
            default:
                if(curUser->sbdatalen > 0) {
                    UserTry2Send(curUser);
                }
                break;
        }
    }

    globalQ->ClearQueues();

    if(iLoopsForLogins >= 40) {
        dLoggedUsers = 0;
        iLoopsForLogins = 0;
        dActualSrvLoopLogins = 0;
    }

    dActualSrvLoopLogins += (double)SettingManager->iShorts[SETSHORT_MAX_SIMULTANEOUS_LOGINS]/40;
    iLoopsForLogins++;

    iLastSendRest = iSendRests;
    iSendRestsPeak = iSendRests > iSendRestsPeak ? iSendRests : iSendRestsPeak;
}
//---------------------------------------------------------------------------

void theLoop::Terminate() {
	bServerTerminated = true;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
void theLoop::AcceptSocket(const SOCKET &s, const sockaddr_in &addr, const int &sin_len) {
    EnterCriticalSection(&csAcceptQueue);
#else
void theLoop::AcceptSocket(const int &s, const sockaddr_in &addr, const socklen_t &sin_len) {
    pthread_mutex_lock(&mtxAcceptQueue);
#endif
    
    AcceptedSocket *newSocket = new AcceptedSocket();
    if(newSocket == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate newSocket in theLoop::AcceptSocket!";
#ifdef _WIN32
    	sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
#ifdef _WIN32
    	LeaveCriticalSection(&csAcceptQueue);
#else
		pthread_mutex_unlock(&mtxAcceptQueue);
#endif
    	return;
    }

    newSocket->s = s;
    newSocket->addr = addr;
    newSocket->sin_len = sin_len;
    strcpy(newSocket->IP, inet_ntoa(addr.sin_addr));
    newSocket->next = NULL;

    if(AcceptedSocketsS == NULL) {
        AcceptedSocketsS = newSocket;
        AcceptedSocketsE = newSocket;
    } else {
        AcceptedSocketsE->next = newSocket;
        AcceptedSocketsE = newSocket;
    }

#ifdef _WIN32
    LeaveCriticalSection(&csAcceptQueue);
#else
	pthread_mutex_unlock(&mtxAcceptQueue);
#endif
}
//---------------------------------------------------------------------------

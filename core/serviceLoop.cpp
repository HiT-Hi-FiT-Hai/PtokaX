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
#include "serviceLoop.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
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
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
#ifndef _WIN32
	#include "regtmrinc.h"
	#include "scrtmrinc.h"
#endif
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
        	AppendDebugLog("%s - [MEM] Cannot allocate RegisterThread in RegTimerHandler\n", 0);
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

void theLoop::Looper() {
#ifdef _WIN32
	KillTimer(NULL, srvLoopTimer);
#endif

	// PPK ... two loop stategy for saving badwith
	if(bRecv == true) {
		ReceiveLoop();
	} else {
		SendLoop();
		eventqueue->ProcessEvents();
	}

	if(bServerTerminated == false) {
		bRecv = !bRecv;

#ifdef _WIN32
        srvLoopTimer = SetTimer(NULL, 0, 100, NULL);

	    if(srvLoopTimer == 0) {
	        AppendDebugLog("%s - [ERR] Cannot start Looper in theLoop::Looper\n", 0);
	        exit(EXIT_FAILURE);
	    }
#endif
	} else {
	    // tell the scripts about the end
	    ScriptManager->OnExit();
	    
	    // send last possible global data
	    g_GlobalDataQueue->SendFinalQueue();
	    
	    ServerFinalStop(true);
	}
}
//---------------------------------------------------------------------------

theLoop::theLoop() {
    msg[0] = '\0';

#ifdef _WIN32
    InitializeCriticalSection(&csAcceptQueue);
#else
	iSIG = 0;

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
	srvLoopTimer = SetTimer(NULL, 0, 100, NULL);

    if(srvLoopTimer == 0) {
		AppendDebugLog("%s - [ERR] Cannot start Looper in theLoop::theLoop\n", 0);
    	exit(EXIT_FAILURE);
    }
#else
    sigemptyset(&sst);
    sigaddset(&sst, SIGSCRTMR);
    sigaddset(&sst, SIGREGTMR);

    zerotime.tv_sec = 0;
    zerotime.tv_nsec = 0;
#endif
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
    bool bIPv6 = false;

    char sIP[46];

    uint8_t ui128IpHash[16];
    memset(ui128IpHash, 0, 16);

    uint16_t ui16IpTableIdx = 0;

    if(AccptSocket->addr.ss_family == AF_INET6) {
        memcpy(ui128IpHash, &((struct sockaddr_in6 *)&AccptSocket->addr)->sin6_addr.s6_addr, 16);

        if(IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&AccptSocket->addr)->sin6_addr)) {
			in_addr ipv4addr;
			memcpy(&ipv4addr, ((struct sockaddr_in6 *)&AccptSocket->addr)->sin6_addr.s6_addr + 12, 4);
			strcpy(sIP, inet_ntoa(ipv4addr));

            ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
        } else {
            bIPv6 = true;
#ifdef _WIN32
            win_inet_ntop(&((struct sockaddr_in6 *)&AccptSocket->addr)->sin6_addr, sIP, 46);
#else
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&AccptSocket->addr)->sin6_addr, sIP, 46);
#endif
            ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
        }
    } else {
        strcpy(sIP, inet_ntoa(((struct sockaddr_in *)&AccptSocket->addr)->sin_addr));

        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;
        memcpy(ui128IpHash+12, &((struct sockaddr_in *)&AccptSocket->addr)->sin_addr.s_addr, 4);

        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    }

    // set the recv buffer
#ifdef _WIN32
    int32_t bufsize = 8192;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
    	int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %s (%d)",  sIP, WSErrorStr(iError), iError);
#else
    int bufsize = 8192;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
    	int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %d", sIP, errno);
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
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %s (%d)", sIP, WSErrorStr(iError), iError);
#else
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %d", sIP, errno);
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
    setsockopt(AccptSocket->s, SOL_SOCKET, SO_KEEPALIVE, (char *)&bKeepalive, sizeof(bKeepalive));
#else
    int iKeepAlive = 1;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_KEEPALIVE, &iKeepAlive, sizeof(iKeepAlive)) == -1) {
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_KEEPALIVE. IP: %s Err: %d", sIP, errno);
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
    	int imsgLen = sprintf(msg, "[SYS] ioctlsocket failed on attempt to set FIONBIO. IP: %s Err: %s (%d)", sIP, WSErrorStr(iError), iError);
#else
    int oldFlag = fcntl(AccptSocket->s, F_GETFL, 0);
    if(fcntl(AccptSocket->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
    	int imsgLen = sprintf(msg, "[SYS] fcntl failed on attempt to set O_NONBLOCK. IP: %s Err: %d", sIP, errno);
#endif
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser3") == true) {
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
        if(SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS] != NULL) {
       	    int imsgLen = sprintf(msg, "<%s> %s %s|%s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_REDIR_TO],
               SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS], SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS]);
            if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser4") == true) {
                send(AccptSocket->s, msg, imsgLen, 0);
                ui64BytesSent += imsgLen;
            }
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

    time_t acc_time;
    time(&acc_time);

	BanItem * Ban = hashBanManager->FindFull(ui128IpHash, acc_time);

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
/*            int imsgLen = sprintf(msg, "[SYS] Banned ip %s - connection closed.", sIP);
            if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser5") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }*/
            return;
        }
    }

	RangeBanItem * RangeBan = hashBanManager->FindFullRange(ui128IpHash, acc_time);

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
/*            int imsgLen = sprintf(msg, "[SYS] Range Banned ip %s - connection closed.", sIP);
            if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser6") == true) {
                UdpDebug->Broadcast(msg, imsgLen);
            }*/
            return;
        }
    }

    ui32Joins++;

    // set properties of the new user object
	User * pUser = new User();

	if(pUser == NULL) {
#ifdef _WIN32
		shutdown(AccptSocket->s, SD_SEND);
		closesocket(AccptSocket->s);
#else
		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
#endif
        AppendDebugLog("%s - [MEM] Cannot allocate pUser in theLoop::AcceptUser\n", 0);
		return;
	}

	pUser->uLogInOut = new LoginLogout();

    if(pUser->uLogInOut == NULL) {
#ifdef _WIN32
		shutdown(AccptSocket->s, SD_SEND);
		closesocket(AccptSocket->s);
#else
		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
#endif
        delete pUser;

        AppendDebugLog("%s - [MEM] Cannot allocate uLogInOut in theLoop::AcceptUser\n", 0);
		return;
    }

    pUser->uLogInOut->logonClk = ui64ActualTick;
    pUser->Sck = AccptSocket->s;
    pUser->ui8State = User::STATE_KEY_OR_SUP;

    memcpy(pUser->ui128IpHash, ui128IpHash, 16);
    pUser->ui16IpTableIdx = ui16IpTableIdx;

    UserSetIP(pUser, sIP);

    if(bIPv6 == true) {
        pUser->ui32BoolBits |= User::BIT_IPV6;
    } else {
        pUser->ui32BoolBits |= User::BIT_IPV4;
    }

    if(UserMakeLock(pUser) == false) {
#ifdef _WIN32
		shutdown(AccptSocket->s, SD_SEND);
		closesocket(AccptSocket->s);
#else
		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
#endif
        delete pUser;
        return;
    }
    
    if(Ban != NULL) {
        uint32_t hash = 0;
        if(((Ban->ui8Bits & hashBanMan::NICK) == hashBanMan::NICK) == true) {
            hash = Ban->ui32NickHash;
        }
        int imsglen;
        char *messg = GenerateBanMessage(Ban, imsglen, acc_time);
        pUser->uLogInOut->uBan = new UserBan(messg, imsglen, hash);
        if(pUser->uLogInOut->uBan == NULL || pUser->uLogInOut->uBan->sMessage == NULL) {
#ifdef _WIN32
            shutdown(AccptSocket->s, SD_SEND);
            closesocket(AccptSocket->s);
#else
            shutdown(AccptSocket->s, SHUT_RDWR);
            close(AccptSocket->s);
#endif
            if(pUser->uLogInOut->uBan == NULL) {
                AppendDebugLog("%s - [MEM] Cannot allocate new uBan in theLoop::AcceptUser\n", 0);
            } else {
                AppendDebugLog("%s - [MEM] Cannot allocate uBan->sMessage in theLoop::AcceptUser\n", 0);
            }

            delete pUser;

            return;
        }
    } else if(RangeBan != NULL) {
        int imsglen;
        char *messg = GenerateRangeBanMessage(RangeBan, imsglen, acc_time);
        pUser->uLogInOut->uBan = new UserBan(messg, imsglen, 0);
        if(pUser->uLogInOut->uBan == NULL || pUser->uLogInOut->uBan->sMessage == NULL) {
#ifdef _WIN32
            shutdown(AccptSocket->s, SD_SEND);
            closesocket(AccptSocket->s);
#else
            shutdown(AccptSocket->s, SHUT_RDWR);
            close(AccptSocket->s);
#endif
            if(pUser->uLogInOut->uBan == NULL) {
        	   AppendDebugLog("%s - [MEM] Cannot allocate new uBan in theLoop::AcceptUser1\n", 0);
            } else {
                AppendDebugLog("%s - [MEM] Cannot allocate uBan->sMessage in theLoop::AcceptUser1\n", 0);
            }

            delete pUser;

        	return;
        }
    }
        
    // Everything is ok, now add to users...
    colUsers->AddUser(pUser);
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
                AppendDebugLog("%s - [ERR] Process memory heap corrupted\n", 0);
            }
            HeapCompact(GetProcessHeap(), 0);

            if(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendDebugLog("%s - [ERR] PtokaX memory heap corrupted\n", 0);
            }
            HeapCompact(hPtokaXHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(hRecvHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendDebugLog("%s - [ERR] Recv memory heap corrupted\n", 0);
            }
            HeapCompact(hRecvHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(hSendHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendDebugLog("%s - [ERR] Send memory heap corrupted\n", 0);
            }
            HeapCompact(hSendHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(hLuaHeap, 0, 0) == 0) {
                AppendDebugLog("%s - [ERR] Lua memory heap corrupted\n", 0);
            }
            HeapCompact(hLuaHeap, 0);
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

        switch(curUser->ui8State) {
            case User::STATE_KEY_OR_SUP:{
                // check logon timeout for iState 1
                if(ui64ActualTick - curUser->uLogInOut->logonClk > 20) {
                    int imsgLen = sprintf(msg, "[SYS] Login timeout 1 for %s - user disconnected.", curUser->sIP);
                    if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop3") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }
                    UserClose(curUser);
                    continue;
                }
                break;
            }
            case User::STATE_IPV4_CHECK: {
                // check IPv4Check timeout
                if((ui64ActualTick - curUser->uLogInOut->ui64IPv4CheckTick) > 10) {
                    int imsgLen = sprintf(msg, "[SYS] IPv4Check timeout for %s (%s).", curUser->sNick, curUser->sIP);
                    if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop31") == true) {
                        UdpDebug->Broadcast(msg, imsgLen);
                    }
                    curUser->ui8State = User::STATE_ADDME;
                    continue;
                }
                break;
            }
            case User::STATE_ADDME: {
                // PPK ... Add user, but only if send $GetNickList (or have quicklist supports) <- important, used by flooders !!!
                if(((curUser->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == false &&
                    ((curUser->ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false &&
                    ((curUser->ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                    continue;

                int imsgLen = GetWlcmMsg(msg);
                UserSendCharDelayed(curUser, msg, imsgLen);
                curUser->ui8State = User::STATE_ADDME_1LOOP;
                continue;
            }
            case User::STATE_ADDME_1LOOP: {
                // PPK ... added login delay.
                if(dLoggedUsers >= dActualSrvLoopLogins && ((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    if(ui64ActualTick - curUser->uLogInOut->logonClk > 300) {
                        int imsgLen = sprintf(msg, "[SYS] Login timeout (%d) 3 for %s (%s) - user disconnected.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
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
						AppendDebugLog("%s - [MEM] Cannot deallocate curUser->uLogInOut->sLockUsrConn in theLoop::ReceiveLoop\n", 0);
                    }
#else
					free(curUser->uLogInOut->sLockUsrConn);
#endif
                    curUser->uLogInOut->sLockUsrConn = NULL;
                }

                if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && ((curUser->ui32BoolBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) == false) {
                    in_addr ipv4addr;
					ipv4addr.s_addr = INADDR_NONE;

                    if(curUser->ui128IpHash[0] == 32 && curUser->ui128IpHash[1] == 2) { // 6to4 tunnel
                        memcpy(&ipv4addr, curUser->ui128IpHash + 2, 4);
                    } else if(curUser->ui128IpHash[0] == 32 && curUser->ui128IpHash[1] == 1 && curUser->ui128IpHash[2] == 0 && curUser->ui128IpHash[3] == 0) { // teredo tunnel
                        uint32_t ui32Ip = 0;
                        memcpy(&ui32Ip, curUser->ui128IpHash + 12, 4);
                        ui32Ip ^= 0xffffffff;
                        memcpy(&ipv4addr, &ui32Ip, 4);
                    }

                    if(ipv4addr.s_addr != INADDR_NONE) {
                        strcpy(curUser->sIPv4, inet_ntoa(ipv4addr));
                        curUser->ui8IPv4Len = (uint8_t)strlen(curUser->sIPv4);
                        curUser->ui32BoolBits |= User::BIT_IPV4;
                    }
                }

                //New User Connected ... the user is operator ? invoke lua User/OpConnected
                uint32_t iBeforeLuaLen = curUser->sbdatalen;

				bool bRet = ScriptManager->UserConnected(curUser);
				if(curUser->ui8State >= User::STATE_CLOSING) {// connection closed by script?
                    if(bRet == false) { // only when all scripts process userconnected
                        ScriptManager->UserDisconnected(curUser);
                    }

					continue;
				}

                if(iBeforeLuaLen < curUser->sbdatalen) {
                    size_t szNeededLen = curUser->sbdatalen-iBeforeLuaLen;
#ifdef _WIN32
                    curUser->uLogInOut->sLockUsrConn = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededLen+1);
#else
					curUser->uLogInOut->sLockUsrConn = (char *)malloc(szNeededLen+1);
#endif
                    if(curUser->uLogInOut->sLockUsrConn == NULL) {
                        curUser->ui32BoolBits |= User::BIT_ERROR;
                        UserClose(curUser);

						AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for sLockUsrConn in theLoop::ReceiveLoop\n", (uint64_t)(szNeededLen+1));

                		continue;
                    }
                    memcpy(curUser->uLogInOut->sLockUsrConn, curUser->sendbuf+iBeforeLuaLen, szNeededLen);
                	curUser->uLogInOut->iUserConnectedLen = (uint32_t)szNeededLen;
                	curUser->uLogInOut->sLockUsrConn[curUser->uLogInOut->iUserConnectedLen] = '\0';
                	curUser->sbdatalen = iBeforeLuaLen;
                	curUser->sendbuf[curUser->sbdatalen] = '\0';
                }

                // PPK ... wow user is accepted, now add it :)
                if(((curUser->ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG) == true) {
                    UserHasSuspiciousTag(curUser);
                }
                    
                UserAdd2Userlist(curUser);
                
                dLoggedUsers++;
                curUser->ui8State = User::STATE_ADDME_2LOOP;
                ui64TotalShare += curUser->ui64SharedSize;
                curUser->ui32BoolBits |= User::BIT_HAVE_SHARECOUNTED;
                
#ifdef _BUILD_GUI
                if(::SendMessage(pMainWindowPageUsersChat->hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    pMainWindowPageUsersChat->AddUser(curUser);
                }
#endif
//                if(sqldb) sqldb->AddVisit(curUser);

                // PPK ... change to NoHello supports
            	int imsgLen = sprintf(msg, "$Hello %s|", curUser->sNick);
            	if(CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop6") == true) {
                    g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_HELLO);
                }

                g_GlobalDataQueue->UserIPStore(curUser);

                switch(SettingManager->ui8FullMyINFOOption) {
                    case 0:
                        g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoLong, curUser->ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                        break;
                    case 1:
                        g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoShort, curUser->ui16MyInfoShortLen, curUser->sMyInfoLong, curUser->ui16MyInfoLongLen, GlobalDataQueue::CMD_MYINFO);
                        break;
                    case 2:
                        g_GlobalDataQueue->AddQueueItem(curUser->sMyInfoShort, curUser->ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                        break;
                    default:
                        break;
                }
                
                if(((curUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                    g_GlobalDataQueue->OpListStore(curUser->sNick);
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
                                                imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s %s %s!|", curUser->sNick, cur->ToNick, SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC],
                                                    LanguageManager->sTexts[LAN_SRY_LST_MSG_BCS], cur->ToNick, LanguageManager->sTexts[LAN_EXC_MSG_LIMIT]);
                                                bSprintfCheck = CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop1");
                                            } else {
                                                imsgLen = sprintf(msg, "$To: %s From: %s $<%s> %s %d %s %s %s!|", curUser->sNick, cur->ToNick,
                                                    SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_SORRY_LAST], cur->iPmCount,
                                                    LanguageManager->sTexts[LAN_MSGS_NOT_SENT], cur->ToNick, LanguageManager->sTexts[LAN_EXC_MSG_LIMIT]);
                                                bSprintfCheck = CheckSprintf(imsgLen, 1024, "theLoop::ReceiveLoop2");
                                            }
                                            if(bSprintfCheck == true) {
                                                UserSendCharDelayed(curUser, msg, imsgLen);
                                            }

#ifdef _WIN32
                                            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sCommand) == 0) {
												AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in theLoop::ReceiveLoop\n", 0);
                                            }
#else
											free(cur->sCommand);
#endif
                                            cur->sCommand = NULL;

#ifdef _WIN32
                                            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->ToNick) == 0) {
												AppendDebugLog("%s - [MEM] Cannot deallocate cur->ToNick in theLoop::ReceiveLoop\n", 0);
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
							AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand1 in theLoop::ReceiveLoop\n", 0);
                        }
#else
						free(cur->sCommand);
#endif
                        cur->sCommand = NULL;

#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->ToNick) == 0) {
							AppendDebugLog("%s - [MEM] Cannot deallocate cur->ToNick1 in theLoop::ReceiveLoop\n", 0);
                        }
#else
						free(cur->ToNick);
#endif
                        cur->ToNick = NULL;
        
                        delete cur;
					}
                }
        
                if(ui8SrCntr == 0) {
                    if(curUser->cmdActive6Search != NULL) {
						if(curUser->cmdActive4Search != NULL) {
							g_GlobalDataQueue->AddQueueItem(curUser->cmdActive6Search->sCommand, curUser->cmdActive6Search->iLen, 
								curUser->cmdActive4Search->sCommand, curUser->cmdActive4Search->iLen, GlobalDataQueue::CMD_ACTIVE_SEARCH_V64);
						} else {
							g_GlobalDataQueue->AddQueueItem(curUser->cmdActive6Search->sCommand, curUser->cmdActive6Search->iLen, NULL, 0, GlobalDataQueue::CMD_ACTIVE_SEARCH_V6);
						}
                    } else if(curUser->cmdActive4Search != NULL) {
						g_GlobalDataQueue->AddQueueItem(curUser->cmdActive4Search->sCommand, curUser->cmdActive4Search->iLen, NULL, 0, GlobalDataQueue::CMD_ACTIVE_SEARCH_V4);
					}

                    if(curUser->cmdPassiveSearch != NULL) {
						uint8_t ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V4;
						if((curUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
							if((curUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                                if((curUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) {
                                    ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V4_ONLY;
                                } else if((curUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                                    ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V6_ONLY;
                                } else {
                                    ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V64;
                                }
                            } else {
								ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V6;
                            }
						}

						g_GlobalDataQueue->AddQueueItem(curUser->cmdPassiveSearch->sCommand, curUser->cmdPassiveSearch->iLen, NULL, 0, ui8CmdType);

                        UserDeletePrcsdUsrCmd(curUser->cmdPassiveSearch);
                        curUser->cmdPassiveSearch = NULL;
                    }
                }

                // PPK ... deflood memory cleanup, if is not needed anymore
				if(ui8SrCntr == 0) {
					if(curUser->sLastChat != NULL && curUser->ui16LastChatLines < 2 && 
                        (curUser->ui64SameChatsTick+SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_TIME]) < ui64ActualTick) {
#ifdef _WIN32
                        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->sLastChat) == 0) {
                            AppendDebugLog("%s - [MEM] Cannot deallocate curUser->sLastChat in theLoop::ReceiveLoop\n", 0);
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
							AppendDebugLog("%s - [MEM] Cannot deallocate curUser->sLastPM in theLoop::ReceiveLoop\n", 0);
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
							AppendDebugLog("%s - [MEM] Cannot deallocate curUser->sLastSearch in theLoop::ReceiveLoop\n", 0);
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
            	curUser->ui8State = User::STATE_REMME;
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
                    int imsgLen = sprintf(msg, "[SYS] Login timeout (%d) 2 for %s (%s) - user disconnected.", (int)curUser->ui8State, curUser->sNick, curUser->sIP);
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
    g_GlobalDataQueue->PrepareQueueItems();

    // PPK ... send loop
    // now logging users get changed myinfo with myinfos
    // old users get it in this loop from queue -> badwith saving !!! no more twice myinfo =)
    // Sending Loop
    uint32_t iSendRests = 0;

    User *nextUser = colUsers->llist;
    while(nextUser != 0 && bServerTerminated == false) {
        User *curUser = nextUser;
        nextUser = curUser->next;

        switch(curUser->ui8State) {
            case User::STATE_ADDME_2LOOP: {
                ui32Logged++;

                if(ui32Peak < ui32Logged) {
                    ui32Peak = ui32Logged;
                    if(SettingManager->iShorts[SETSHORT_MAX_USERS_PEAK] < (int16_t)ui32Peak)
                        SettingManager->SetShort(SETSHORT_MAX_USERS_PEAK, (int16_t)ui32Peak);
                }

            	curUser->ui8State = User::STATE_ADDED;

            	// finaly send the nicklist/myinfos/oplist
                UserAddUserList(curUser);
                
                // PPK ... UserIP2 supports
                if(((curUser->ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == true &&
                    ((curUser->ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) == false &&
                    ProfileMan->IsAllowed(curUser, ProfileManager::SENDALLUSERIP) == false) {
            		int imsgLen = sprintf(msg, "$UserIP %s %s|", curUser->sNick, (curUser->sIPv4[0] == '\0' ? curUser->sIP : curUser->sIPv4));
            		if(CheckSprintf(imsgLen, 1024, "theLoop::SendLoop1") == true) {
                        UserSendCharDelayed(curUser, msg, imsgLen);
                    }
                }
                
                curUser->ui32BoolBits &= ~User::BIT_GETNICKLIST;

                // PPK ... send motd ???
                if(SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD] != 0) {
                    if(SettingManager->bBools[SETBOOL_MOTD_AS_PM] == true) {
                        int imsgLen = sprintf(g_sBuffer, SettingManager->sPreTexts[SetMan::SETPRETXT_MOTD], curUser->sNick);
                        if(CheckSprintf(imsgLen, g_szBufferSize, "theLoop::SendLoop2") == true) {
                            UserSendCharDelayed(curUser, g_sBuffer, imsgLen);
                        }
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
						AppendDebugLog("%s - [MEM] Cannot deallocate curUser->uLogInOut->sLockUsrConn in theLoop::SendLoop\n", 0);
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
                if(((curUser->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == true) {
                    UserAddUserList(curUser);
                    curUser->ui32BoolBits &= ~User::BIT_GETNICKLIST;
                }

                // process global data queues
                if(g_GlobalDataQueue->pQueueItems != NULL) {
                    g_GlobalDataQueue->ProcessQueues(curUser);
                }
                
            	if(g_GlobalDataQueue->pSingleItems != NULL) {
                    g_GlobalDataQueue->ProcessSingleItems(curUser);
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

    g_GlobalDataQueue->ClearQueues();

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

#ifdef _WIN32
void theLoop::AcceptSocket(const SOCKET &s, const sockaddr_storage &addr) {
#else
void theLoop::AcceptSocket(const int &s, const sockaddr_storage &addr) {
#endif
    AcceptedSocket * pNewSocket = new AcceptedSocket();
    if(pNewSocket == NULL) {
#ifdef _WIN32
		shutdown(s, SD_SEND);
		closesocket(s);
#else
		shutdown(s, SHUT_RDWR);
		close(s);
#endif

		AppendDebugLog("%s - [MEM] Cannot allocate pNewSocket in theLoop::AcceptSocket\n", 0);
    	return;
    }

    pNewSocket->s = s;

    memcpy(&pNewSocket->addr, &addr, sizeof(sockaddr_storage));

    pNewSocket->next = NULL;

#ifdef _WIN32
    EnterCriticalSection(&csAcceptQueue);
#else
    pthread_mutex_lock(&mtxAcceptQueue);
#endif

    if(AcceptedSocketsS == NULL) {
        AcceptedSocketsS = pNewSocket;
        AcceptedSocketsE = pNewSocket;
    } else {
        AcceptedSocketsE->next = pNewSocket;
        AcceptedSocketsE = pNewSocket;
    }

#ifdef _WIN32
    LeaveCriticalSection(&csAcceptQueue);
#else
	pthread_mutex_unlock(&mtxAcceptQueue);
#endif
}
//---------------------------------------------------------------------------

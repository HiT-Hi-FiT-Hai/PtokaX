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
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
theLoop *srvLoop = NULL;
//---------------------------------------------------------------------------

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
//---------------------------------------------------------------------------

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
//---------------------------------------------------------------------------

theLoop::theLoop() {
    pthread_mutex_init(&mtxAcceptQueue, NULL);

    AcceptedSocketsS = NULL;
    AcceptedSocketsE = NULL;
    
    bRecv = true;
    
    iLstUptmTck = ui64ActualTick;

	bServerTerminated = false;
	iSendRestsPeak = iRecvRestsPeak = iLastSendRest = iLastRecvRest = iLoopsForLogins = 0;
	dLoggedUsers = dActualSrvLoopLogins = 0;

    sigemptyset(&sst);
    sigaddset(&sst, SIGSCRTMR);
    sigaddset(&sst, SIGREGTMR);

    zerotime.tv_sec = 0;
    zerotime.tv_nsec = 0;

	Cout("MainLoop executed.");
}
//---------------------------------------------------------------------------

theLoop::~theLoop() {
    AcceptedSocket *nextsck = AcceptedSocketsS;
        
    while(nextsck != NULL) {
        AcceptedSocket *cursck = nextsck;
		nextsck = cursck->next;
		shutdown(cursck->s, SHUT_RDWR);
        close(cursck->s);
        delete cursck;
	}
   
    AcceptedSocketsS = NULL;
    AcceptedSocketsE = NULL;

	pthread_mutex_destroy(&mtxAcceptQueue);

	Cout("MainLoop terminated.");
}
//---------------------------------------------------------------------------

void theLoop::AcceptUser(AcceptedSocket *AccptSocket) {
    // set the recv buffer
    int bufsize = 8192;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
    	int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser1") == true) {
    	   UdpDebug->Broadcast(msg, imsgLen);
        }
    	shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
        return;
    }

    // set the send buffer
    bufsize = 32768;
    if(setsockopt(AccptSocket->s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        int imsgLen = sprintf(msg, "[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser2") == true) {
	       UdpDebug->Broadcast(msg, imsgLen);
        }
	    shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
        return;
    }

    // set sending of keepalive packets
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

    // set non-blocking mode
    int oldFlag = fcntl(AccptSocket->s, F_GETFL, 0);
    if(fcntl(AccptSocket->s, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
    	int imsgLen = sprintf(msg, "[SYS] fcntl failed on attempt to set O_NONBLOCK. IP: %s Err: %d", 
            inet_ntoa(AccptSocket->addr.sin_addr), errno);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser4") == true) {
		  UdpDebug->Broadcast(msg, imsgLen);
		}
		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
		return;
    }
    
    if(SettingManager->bBools[SETBOOL_REDIRECT_ALL] == true) {
       	int imsgLen = sprintf(msg, "<%s> %s %s|%s", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_YOU_REDIR_TO],
            SettingManager->sTexts[SETTXT_REDIRECT_ADDRESS], SettingManager->sPreTexts[SetMan::SETPRETXT_REDIRECT_ADDRESS]);
        if(CheckSprintf(imsgLen, 1024, "theLoop::AcceptUser5") == true) {
            send(AccptSocket->s, msg, imsgLen, 0);
            ui64BytesSent += imsgLen;
        }
        shutdown(AccptSocket->s, SHUT_RDWR);
        close(AccptSocket->s);
		return;
    }

    uint32_t hash;
    HashIP(AccptSocket->IP, strlen(AccptSocket->IP), hash);

    time_t acc_time;
    time(&acc_time);

	BanItem *Ban = hashBanManager->FindFull(hash, acc_time);

	if(Ban != NULL) {
        if(((Ban->ui8Bits & hashBanMan::FULL) == hashBanMan::FULL) == true) {
            int imsglen;
            char *messg = GenerateBanMessage(Ban, imsglen, acc_time);
            send(AccptSocket->s, messg, imsglen, 0);
            shutdown(AccptSocket->s, SHUT_RD);
            close(AccptSocket->s);
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
            shutdown(AccptSocket->s, SHUT_RD);
            close(AccptSocket->s);
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
		AppendSpecialLog(sDbgstr);

		shutdown(AccptSocket->s, SHUT_RDWR);
		close(AccptSocket->s);
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
        	AppendSpecialLog(sDbgstr);
        }
    } else if(RangeBan != NULL) {
        int imsglen;
        char *messg = GenerateRangeBanMessage(RangeBan, imsglen, acc_time);
        u->uLogInOut->uBan = new UserBan(messg, imsglen, 0);
        if(u->uLogInOut->uBan == NULL) {
            string sDbgstr = "[BUF] Cannot allocate new uBan!";
        	AppendSpecialLog(sDbgstr);
        }
    }
        
    // Everything is ok, now add to users...
    colUsers->AddUser(u);
}
//---------------------------------------------------------------------------

void theLoop::ReceiveLoop() {
    while((iSIG = sigtimedwait(&sst, &info, &zerotime)) != -1) {
        if(iSIG == SIGSCRTMR) {
            ScriptOnTimer((ScriptTimer *)info.si_value.sival_ptr);
        } else if(iSIG == SIGREGTMR) {
            RegTimerHandler();
        }
    }

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

        iLstUptmTck = ui64ActualTick;
    }

    AcceptedSocket *NextSck = NULL;

    pthread_mutex_lock(&mtxAcceptQueue);

    if(AcceptedSocketsS != NULL) {
        NextSck = AcceptedSocketsS;
        AcceptedSocketsS = NULL;
        AcceptedSocketsE = NULL;
    }

    pthread_mutex_unlock(&mtxAcceptQueue);

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
                    free(curUser->uLogInOut->sLockUsrConn);
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
                    curUser->uLogInOut->sLockUsrConn = (char *) malloc(iNdLen+1);
                    if(curUser->uLogInOut->sLockUsrConn == NULL) {
						string sDbgstr = "[BUF] Cannot allocate "+string(iNdLen+1)+
							" bytes of memory for sLockUsrConn in theLoop::ReceiveLoop!";
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
        
                                            free(cur->sCommand);
                                            cur->sCommand = NULL;
        
                                            free(cur->ToNick);
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
                            
                        free(cur->sCommand);
                        cur->sCommand = NULL;
        
                        free(cur->ToNick);
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
        
                        free(curUser->cmdPSearch->sCommand);
                        curUser->cmdPSearch->sCommand = NULL;
        
                        delete curUser->cmdPSearch;
                        curUser->cmdPSearch = NULL;
                    }
                }

                // PPK ... deflood memory cleanup, if is not needed anymore
				if(ui8SrCntr == 0) {
					if(curUser->sLastChat != NULL && curUser->ui16LastChatLines < 2 && 
                        (curUser->ui64SameChatsTick+SettingManager->iShorts[SETSHORT_SAME_MAIN_CHAT_TIME]) < ui64ActualTick) {
                        free(curUser->sLastChat);
                        curUser->sLastChat = NULL;
						curUser->ui16LastChatLen = 0;
						curUser->ui16SameMultiChats = 0;
						curUser->ui16LastChatLines = 0;
                    }

					if(curUser->sLastPM != NULL && curUser->ui16LastPmLines < 2 && 
                        (curUser->ui64SamePMsTick+SettingManager->iShorts[SETSHORT_SAME_PM_TIME]) < ui64ActualTick) {
						free(curUser->sLastPM);
                        curUser->sLastPM = NULL;
                        curUser->ui16LastPMLen = 0;
						curUser->ui16SameMultiPms = 0;
                        curUser->ui16LastPmLines = 0;
                    }
                    
					if(curUser->sLastSearch != NULL && (curUser->ui64SameSearchsTick+SettingManager->iShorts[SETSHORT_SAME_SEARCH_TIME]) < ui64ActualTick) {
                        free(curUser->sLastSearch);
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
                shutdown(curUser->Sck, SHUT_RD);
                close(curUser->Sck);

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
                        char * sMSG = (char *) malloc(SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65);
                        if(sMSG == NULL) {
							string sDbgstr = "[BUF] Cannot allocate "+string(SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65)+
								" bytes of memory in theLoop::SendLoop!";
							AppendSpecialLog(sDbgstr);
                            exit(EXIT_FAILURE);
                        }

                        int imsgLen = sprintf(sMSG, SettingManager->sPreTexts[SetMan::SETPRETXT_MOTD], curUser->Nick);
                        if(CheckSprintf(imsgLen, SettingManager->ui16PreTextsLens[SetMan::SETPRETXT_MOTD]+65, "theLoop::SendLoop2") == true) {
                            UserSendCharDelayed(curUser, sMSG, imsgLen);
                        }
                        
                        free(sMSG);
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
                    free(curUser->uLogInOut->sLockUsrConn);
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

void theLoop::AcceptSocket(const int &s, const sockaddr_in &addr, const socklen_t &sin_len) {
    pthread_mutex_lock(&mtxAcceptQueue);
    
    AcceptedSocket *newSocket = new AcceptedSocket();
    if(newSocket == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate newSocket in theLoop::AcceptSocket!";
		AppendSpecialLog(sDbgstr);
    	pthread_mutex_unlock(&mtxAcceptQueue);
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

    pthread_mutex_unlock(&mtxAcceptQueue);
}
//---------------------------------------------------------------------------

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
#include "eventqueue.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
//---------------------------------------------------------------------------
#include "ServerThread.h"
//---------------------------------------------------------------------------

ServerThread::ServerThread() {
	bActive = false;
	bSuspended = false;
	bTerminated = false;

    threadId = 0;
	iSuspendTime = 0;
	usPort = 0;

	pthread_mutex_init(&mtxServerThread, NULL);

    AntiFloodList = NULL;

    prev = NULL;
    next = NULL;
}
//---------------------------------------------------------------------------

ServerThread::~ServerThread() {
    if(threadId != 0) {
        Close();
        WaitFor();
    }

    pthread_mutex_destroy(&mtxServerThread);
        
    AntiConFlood *acfnext = AntiFloodList;
        
    while(acfnext != NULL) {
        AntiConFlood *acfcur = acfnext;
        acfnext = acfcur->next;
		delete acfcur;
    }
}
//---------------------------------------------------------------------------

static void* ExecuteServerThread(void* SrvThread) {
	((ServerThread *)SrvThread)->Run();
	return 0;
}
//---------------------------------------------------------------------------

void ServerThread::Resume() {
    int iRet = pthread_create(&threadId, NULL, ExecuteServerThread, this);
    if(iRet != 0) {
		AppendSpecialLog("[ERR] Failed to create new ServerThread!");
    }
}
//---------------------------------------------------------------------------

void ServerThread::Run() {
    bActive = true;
    int s;
    sockaddr_in addr; //addr.sin_family = AF_INET;
    socklen_t len = sizeof(addr);
    // struct hostent *h; h = gethostbyname("PtokaX.org"); memcpy(&test, h->h_addr_list[0], h->h_length);

    while(bTerminated == false) {
        s = accept(server, (sockaddr *)&addr, &len);

        if(iSuspendTime == 0) {
            if(bTerminated == true) {
                if(s != -1)
                    shutdown(s, SHUT_RDWR);
                        
                close(s);
                continue;
            }
    
            if(s == -1) {
                if(errno != EWOULDBLOCK) {
                    if(errno == EMFILE) { // max opened file descriptors limit reached
                        sleep(1); // longer sleep give us better chance to have free file descriptor available on next accept call
                    } else {
                        eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                            ("[ERR] accept() for port "+string(usPort)+" has returned an INVALID_SOCKET.").c_str());
                    }
                }
            } else {
                if(isFlooder(s, addr, len) == true) {
                    shutdown(s, SHUT_RDWR);
                    close(s);
                }

                usleep(1000);
            }
        } else {
            uint32_t iSec = 0;
            while(bTerminated == false) {
                if(iSuspendTime > iSec) {
                    sleep(1);
                    if(bSuspended == false) {
                        iSec++;
                    }
                    continue;
                }

                pthread_mutex_lock(&mtxServerThread);
                iSuspendTime = 0;
                pthread_mutex_unlock(&mtxServerThread);

                if(Listen(usPort, true) == true) {
                    eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                        ("[SYS] Server socket for port "+string(usPort)+" sucessfully recovered from suspend state.").c_str());
                } else {
					Close();
                }
                break;
            }
        }
	}

    bActive = false;
}
//---------------------------------------------------------------------------

void ServerThread::Close() {
    bTerminated = true;
    shutdown(server, SHUT_RDWR);
	close(server);
}
//---------------------------------------------------------------------------

void ServerThread::WaitFor() {
	if(threadId != 0) {
		pthread_join(threadId, NULL);
        threadId = 0;
	}
}
//---------------------------------------------------------------------------

bool ServerThread::Listen(const uint16_t &port, bool bSilent/* = false*/) {
    usPort = port;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if(server == -1) {
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                ("[ERR] Unable to create server socket for port "+string(port)+" ! ErrorCode "+string(errno)).c_str());
		} else {
            AppendLog(string(LanguageManager->sTexts[LAN_UNB_CRT_SRVR_SCK], (size_t)LanguageManager->ui16TextsLens[LAN_UNB_CRT_SRVR_SCK])+" "+
				string(port)+" ! "+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+
				" "+string(errno));
        }
        return false;
    }

    // set the socket properties
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(usPort);

    if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && sHubIP[0] != '\0') {
        sa.sin_addr.s_addr = inet_addr(sHubIP);
    } else {
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    int on = 1;
    if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                ("[ERR] Server socket setsockopt error: " + string(errno)+" for port: "+string(port)).c_str());
        } else {
            AppendLog(string(LanguageManager->sTexts[LAN_SRV_SCKOPT_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_SCKOPT_ERR])+
                ": " + string(ErrnoStr(errno))+" (" + string(errno)+") "+
                string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
        }
        close(server);
        return false;
    }

    // bind it
    if(bind(server, (sockaddr *)&sa, sizeof(sa)) == -1) {
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                ("[ERR] Server socket bind error: " + string(ErrnoStr(errno))+" ("+string(errno)+") for port: "+string(port)).c_str());
        } else {
            AppendLog(string(LanguageManager->sTexts[LAN_SRV_BIND_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_BIND_ERR])+
				": " + string(ErrnoStr(errno))+" (" + string(errno)+") "+
				string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
		}
        close(server);
        return false;
    }

    // set listen mode
    if(listen(server, 512) == -1) {
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                ("[ERR] Server socket listen() error: " + string(errno)+" for port: "+string(port)).c_str());
        } else {
            AppendLog(string(LanguageManager->sTexts[LAN_SRV_LISTEN_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_LISTEN_ERR])+
				": " + string(errno)+" "+
				string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
        }
        close(server);
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

bool ServerThread::isFlooder(const int &s, const sockaddr_in &addr, const socklen_t &sin_len) {
    uint32_t iAddr = addr.sin_addr.s_addr;

    int16_t iConDefloodCount = SettingManager->GetShort(SETSHORT_NEW_CONNECTIONS_COUNT);
    int16_t iConDefloodTime = SettingManager->GetShort(SETSHORT_NEW_CONNECTIONS_TIME);
   
    AntiConFlood *nxt = AntiFloodList;
	while(nxt != NULL) {
		AntiConFlood *cur = nxt;
		nxt = cur->next;
    	if(iAddr == cur->addr) {
            if(cur->Time+((uint64_t)iConDefloodTime) >= ui64ActualTick) {
                cur->hits++;
                if(cur->hits > iConDefloodCount) {
                    return true;
                } else {
                    srvLoop->AcceptSocket(s, addr, sin_len);
                    return false;
                }
            } else {
                RemoveConFlood(cur);
                delete cur;
            }
        } else if(cur->Time+((uint64_t)iConDefloodTime) < ui64ActualTick) {
            RemoveConFlood(cur);
            delete cur;
        }
    }

    AntiConFlood *newItem = new AntiConFlood();
    if(newItem == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate newItem in theLoop::isFlooder!";
		AppendSpecialLog(sDbgstr);
    	return true;
    }
    newItem->addr = iAddr;
    newItem->Time = ui64ActualTick;
    newItem->prev = NULL;
    newItem->next = AntiFloodList;
    newItem->hits = 1;
    if(AntiFloodList != NULL) {
        AntiFloodList->prev = newItem;
    }
    AntiFloodList = newItem;

    srvLoop->AcceptSocket(s, addr, sin_len);

    return false;
}
//---------------------------------------------------------------------------

void ServerThread::RemoveConFlood(AntiConFlood *cur) {
    if(cur->prev == NULL) {
        if(cur->next == NULL) {
            AntiFloodList = NULL;
        } else {
            cur->next->prev = NULL;
            AntiFloodList = cur->next;
        }
    } else if(cur->next == NULL) {
        cur->prev->next = NULL;
    } else {
        cur->prev->next = cur->next;
        cur->next->prev = cur->prev;
    }
}
//---------------------------------------------------------------------------

void ServerThread::ResumeSck() {
    if(bActive == true) {
        pthread_mutex_lock(&mtxServerThread);
        bSuspended = false;
        iSuspendTime = 0;
        pthread_mutex_unlock(&mtxServerThread);
    }
}
//---------------------------------------------------------------------------

void ServerThread::SuspendSck(const uint32_t &iTime) {
    if(bActive == true) {
        pthread_mutex_lock(&mtxServerThread);
        if(iTime != 0) {
            iSuspendTime = iTime;
        } else {
            bSuspended = true;
            iSuspendTime = 1;            
        }
        pthread_mutex_unlock(&mtxServerThread);
    	close(server);
    }
}
//---------------------------------------------------------------------------

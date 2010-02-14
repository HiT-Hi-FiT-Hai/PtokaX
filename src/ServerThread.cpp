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
#include "eventqueue.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "ServerThread.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------

ServerThread::ServerThread() {
	bActive = false;
	bSuspended = false;
	bTerminated = false;

    threadId = 0;
	iSuspendTime = 0;
	usPort = 0;

#ifdef _WIN32
    threadHandle = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&csServerThread);
#else
	pthread_mutex_init(&mtxServerThread, NULL);
#endif

    AntiFloodList = NULL;

    prev = NULL;
    next = NULL;
}
//---------------------------------------------------------------------------

ServerThread::~ServerThread() {
#ifdef _WIN32
    DeleteCriticalSection(&csServerThread);
#else
    if(threadId != 0) {
        Close();
        WaitFor();
    }

    pthread_mutex_destroy(&mtxServerThread);
#endif
        
    AntiConFlood *acfnext = AntiFloodList;
        
    while(acfnext != NULL) {
        AntiConFlood *acfcur = acfnext;
        acfnext = acfcur->next;
		delete acfcur;
    }

#ifdef _WIN32
    if(threadHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(threadHandle);
    }
#endif
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	unsigned __stdcall ExecuteServerThread(void* SrvThread) {
#else
	static void* ExecuteServerThread(void* SrvThread) {
#endif
	((ServerThread *)SrvThread)->Run();
	return 0;
}
//---------------------------------------------------------------------------

void ServerThread::Resume() {
#ifdef _WIN32
    threadHandle = (HANDLE)_beginthreadex(NULL, 0, ExecuteServerThread, this, 0, &threadId);
    if(threadHandle == 0) {
#else
    int iRet = pthread_create(&threadId, NULL, ExecuteServerThread, this);
    if(iRet != 0) {
#endif
		AppendSpecialLog("[ERR] Failed to create new ServerThread!");
    }
}
//---------------------------------------------------------------------------

void ServerThread::Run() {
    bActive = true;
#ifdef _WIN32
    SOCKET s;
#else
	int s;
#endif
    sockaddr_in addr; //addr.sin_family = AF_INET;
#ifdef _WIN32
    int len = sizeof(addr);
#else
	socklen_t len = sizeof(addr);
#endif
    // struct hostent *h; h = gethostbyname("PtokaX.org"); memcpy(&test, h->h_addr_list[0], h->h_length);

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			try {
		#endif
	#endif
#endif
	while(bTerminated == false) {
		s = accept(server, (sockaddr *)&addr, &len);

		if(iSuspendTime == 0) {
			if(bTerminated == true) {
#ifdef _WIN32
				if(s != INVALID_SOCKET)
					shutdown(s, SD_SEND);
                        
				closesocket(s);
#else
                if(s != -1)
                    shutdown(s, SHUT_RDWR);
                        
				close(s);
#endif
				continue;
			}

#ifdef _WIN32
			if(s == INVALID_SOCKET) {
				if(WSAEWOULDBLOCK != WSAGetLastError()) {
#else
            if(s == -1) {
                if(errno != EWOULDBLOCK) {
                    if(errno == EMFILE) { // max opened file descriptors limit reached
                        sleep(1); // longer sleep give us better chance to have free file descriptor available on next accept call
                    } else {
#endif
						eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
                            ("[ERR] accept() for port "+string(usPort)+" has returned error.").c_str());
                    }
#ifndef _WIN32
				}
#endif
			} else {
				if(isFlooder(s, addr, len) == true) {
#ifdef _WIN32
					shutdown(s, SD_SEND);
					closesocket(s);
#else
                    shutdown(s, SHUT_RDWR);
                    close(s);
#endif
				}

#ifdef _WIN32
				::Sleep(1);
#else
                usleep(1000);
#endif
			}
		} else {
			uint32_t iSec = 0;
			while(bTerminated == false) {
				if(iSuspendTime > iSec) {
#ifdef _WIN32
					::Sleep(1000);
#else
					sleep(1);
#endif
					if(bSuspended == false) {
						iSec++;
					}
					continue;
				}

#ifdef _WIN32
				EnterCriticalSection(&csServerThread);
#else
				pthread_mutex_lock(&mtxServerThread);
#endif
				iSuspendTime = 0;
#ifdef _WIN32
				LeaveCriticalSection(&csServerThread);
#else
				pthread_mutex_unlock(&mtxServerThread);
#endif

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

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			} catch(Exception &e) {
				AppendSpecialLog("[ERR] Exception in accept thread: " + string(e.Message.c_str(), e.Message.Length()));
				Application->ShowException(&e);
			} catch(...) {
				try {
					throw Exception("");
				}
				catch(Exception &exception) {
					Application->ShowException(&exception);
				}
			}
		#endif
	#endif
#endif

    bActive = false;
}
//---------------------------------------------------------------------------

void ServerThread::Close() {
    bTerminated = true;
#ifdef _WIN32
	closesocket(server);
#else
    shutdown(server, SHUT_RDWR);
	close(server);
#endif
}
//---------------------------------------------------------------------------

void ServerThread::WaitFor() {
#ifdef _WIN32
    WaitForSingleObject(threadHandle, INFINITE);
#else
	if(threadId != 0) {
		pthread_join(threadId, NULL);
        threadId = 0;
	}
#endif
}
//---------------------------------------------------------------------------

bool ServerThread::Listen(const uint16_t &port, bool bSilent/* = false*/) {
    usPort = port;

#ifdef _WIN32
    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server == INVALID_SOCKET) {
#else
    server = socket(AF_INET, SOCK_STREAM, 0);
    if(server == -1) {
#endif
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
#ifdef _WIN32
                ("[ERR] Unable to create server socket for port "+string(port)+" ! ErrorCode "+string(WSAGetLastError())).c_str());
#else
				("[ERR] Unable to create server socket for port "+string(port)+" ! ErrorCode "+string(errno)).c_str());
#endif
		} else {
#ifdef _WIN32
	#ifdef _SERVICE
	            AppendLog(string(LanguageManager->sTexts[LAN_UNB_CRT_SRVR_SCK], (size_t)LanguageManager->ui16TextsLens[LAN_UNB_CRT_SRVR_SCK])+" "+
					string(port)+" ! "+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+
					" "+string(WSAGetLastError()));
	#else
				ShowMessage(String(LanguageManager->sTexts[LAN_UNB_CRT_SRVR_SCK], (size_t)LanguageManager->ui16TextsLens[LAN_UNB_CRT_SRVR_SCK])+" "+
					String(port)+" ! "+String(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+
					" "+String(WSAGetLastError()));
	#endif
#else
            AppendLog(string(LanguageManager->sTexts[LAN_UNB_CRT_SRVR_SCK], (size_t)LanguageManager->ui16TextsLens[LAN_UNB_CRT_SRVR_SCK])+" "+
				string(port)+" ! "+string(LanguageManager->sTexts[LAN_ERROR_CODE], (size_t)LanguageManager->ui16TextsLens[LAN_ERROR_CODE])+
				" "+string(errno));
#endif
        }
        return false;
    }

    // set the socket properties
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    
#ifdef _WIN32
    sa.sin_port = htons((unsigned short)usPort);
#else
	sa.sin_port = htons(usPort);
#endif

    if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && sHubIP[0] != '\0') {
#ifdef _WIN32
        sa.sin_addr.S_un.S_addr = inet_addr(sHubIP);
#else
		sa.sin_addr.s_addr = inet_addr(sHubIP);
#endif
    } else {
#ifdef _WIN32
        sa.sin_addr.S_un.S_addr = INADDR_ANY;
#else
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    }

#ifndef _WIN32
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
#endif

    // bind it
#ifdef _WIN32
    if(bind(server, (sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR) {
    	int err = WSAGetLastError();
#else
	if(bind(server, (sockaddr *)&sa, sizeof(sa)) == -1) {
#endif
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
#ifdef _WIN32
                ("[ERR] Server socket bind error: " + string(WSErrorStr(err))+" ("+string(err)+") for port: "+string(port)).c_str());
#else
				("[ERR] Server socket bind error: " + string(ErrnoStr(errno))+" ("+string(errno)+") for port: "+string(port)).c_str());
#endif
		} else {
#ifdef _WIN32
	#ifdef _SERVICE
			AppendLog(string(LanguageManager->sTexts[LAN_SRV_BIND_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_BIND_ERR])+
				": " + string(WSErrorStr(err))+" ("+string(err)+") "+
				string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
	#else
			ShowMessage(String(LanguageManager->sTexts[LAN_SRV_BIND_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_BIND_ERR])+
				": " + String(WSErrorStr(err))+" ("+String(err)+") "+
			String(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+String(port));
	#endif
		}
        closesocket(server);
#else
            AppendLog(string(LanguageManager->sTexts[LAN_SRV_BIND_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_BIND_ERR])+
				": " + string(ErrnoStr(errno))+" (" + string(errno)+") "+
				string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
		}
        close(server);
#endif
        return false;
    }

    // set listen mode
#ifdef _WIN32
    if(listen(server, 512) == SOCKET_ERROR) {
	    int err = WSAGetLastError();
#else
	if(listen(server, 512) == -1) {
#endif
        if(bSilent == true) {
            eventqueue->AddThread(eventq::EVENT_SRVTHREAD_MSG, 
#ifdef _WIN32
                ("[ERR] Server socket listen() error: " + string(WSErrorStr(err))+" ("+string(err)+") for port: "+string(port)).c_str());
#else
				("[ERR] Server socket listen() error: " + string(errno)+" for port: "+string(port)).c_str());
#endif
        } else {
#ifdef _WIN32
	#ifdef _SERVICE
			AppendLog(string(LanguageManager->sTexts[LAN_SRV_LISTEN_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_LISTEN_ERR])+
				": " + string(WSErrorStr(err))+" ("+string(err)+") "+
				string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
	#else
			ShowMessage(String(LanguageManager->sTexts[LAN_SRV_LISTEN_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_LISTEN_ERR])+
				": " + String(WSErrorStr(err))+" ("+String(err)+") "+
				String(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+String(port));
	#endif
        }
        closesocket(server);
#else
            AppendLog(string(LanguageManager->sTexts[LAN_SRV_LISTEN_ERR], (size_t)LanguageManager->ui16TextsLens[LAN_SRV_LISTEN_ERR])+
				": " + string(errno)+" "+
				string(LanguageManager->sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager->ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(port));
        }
        close(server);
#endif
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
bool ServerThread::isFlooder(const SOCKET &s, const sockaddr_in &addr, const int &sin_len) {
    uint32_t iAddr = addr.sin_addr.S_un.S_addr;
#else
bool ServerThread::isFlooder(const int &s, const sockaddr_in &addr, const socklen_t &sin_len) {
    uint32_t iAddr = addr.sin_addr.s_addr;
#endif

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
#ifdef _WIN32
    	sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
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
#ifdef _WIN32
        EnterCriticalSection(&csServerThread);
#else
		pthread_mutex_lock(&mtxServerThread);
#endif
        bSuspended = false;
        iSuspendTime = 0;
#ifdef _WIN32
        LeaveCriticalSection(&csServerThread);
#else
		pthread_mutex_unlock(&mtxServerThread);
#endif
    }
}
//---------------------------------------------------------------------------

void ServerThread::SuspendSck(const uint32_t &iTime) {
    if(bActive == true) {
#ifdef _WIN32
        EnterCriticalSection(&csServerThread);
#else
		pthread_mutex_lock(&mtxServerThread);
#endif
        if(iTime != 0) {
            iSuspendTime = iTime;
        } else {
            bSuspended = true;
            iSuspendTime = 1;            
        }
#ifdef _WIN32
        LeaveCriticalSection(&csServerThread);
    	closesocket(server);
#else
        pthread_mutex_unlock(&mtxServerThread);
    	close(server);
#endif
    }
}
//---------------------------------------------------------------------------

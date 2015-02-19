/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2015  Petr Kozelka, PPK at PtokaX dot org

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
//---------------------------------------------------------------------------
#include "DcCommands.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
clsEventQueue * clsEventQueue::mPtr = NULL;
//---------------------------------------------------------------------------

clsEventQueue::event::event() : sMsg(NULL), pPrev(NULL), pNext(NULL), ui8Id(0) {
    memset(&ui128IpHash, 0, 16);
};
//---------------------------------------------------------------------------

clsEventQueue::clsEventQueue() : pNormalE(NULL), pThreadE(NULL), pNormalS(NULL), pThreadS(NULL) {
#ifdef _WIN32
    InitializeCriticalSection(&csEventQueue);
#else
	pthread_mutex_init(&mtxEventQueue, NULL);
#endif
}
//---------------------------------------------------------------------------

clsEventQueue::~clsEventQueue() {
    event * cur = NULL,
        * next = pNormalS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

#ifdef _WIN32
        if(cur->sMsg != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sMsg) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate cur->sMsg in clsEventQueue::~clsEventQueue\n", 0);
            }
        }
#else
		free(cur->sMsg);
#endif

        delete cur;
    }

    next = pThreadS;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        free(cur->sMsg);

        delete cur;
    }

#ifdef _WIN32
	DeleteCriticalSection(&csEventQueue);
#else
	pthread_mutex_destroy(&mtxEventQueue);
#endif
}
//---------------------------------------------------------------------------

void clsEventQueue::AddNormal(uint8_t ui8Id, char * sMsg) {
	if(ui8Id != EVENT_RSTSCRIPT && ui8Id != EVENT_STOPSCRIPT) {
		event * cur = NULL,
            * next = pNormalS;

		while(next != NULL) {
			cur = next;
			next = cur->pNext;

			if(cur->ui8Id == ui8Id) {
                return;
            }
		}
	}

    event * pNewEvent = new (std::nothrow) event();

	if(pNewEvent == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewEvent in clsEventQueue::AddNormal\n", 0);
        return;
    }

    if(sMsg != NULL) {
        size_t szLen = strlen(sMsg);
#ifdef _WIN32
		pNewEvent->sMsg = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
		pNewEvent->sMsg = (char *)malloc(szLen+1);
#endif
		if(pNewEvent->sMsg == NULL) {
            delete pNewEvent;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for pNewEvent->sMsg in clsEventQueue::AddNormal\n", (uint64_t)(szLen+1));

            return;
        }

        memcpy(pNewEvent->sMsg, sMsg, szLen);
        pNewEvent->sMsg[szLen] = '\0';
    } else {
		pNewEvent->sMsg = NULL;
    }

    pNewEvent->ui8Id = ui8Id;

    if(pNormalS == NULL) {
        pNormalS = pNewEvent;
        pNewEvent->pPrev = NULL;
    } else {
        pNewEvent->pPrev = pNormalE;
        pNormalE->pNext = pNewEvent;
    }

    pNormalE = pNewEvent;
    pNewEvent->pNext = NULL;
}
//---------------------------------------------------------------------------

void clsEventQueue::AddThread(uint8_t ui8Id, char * sMsg, const sockaddr_storage * sas/* = NULL*/) {
	event * pNewEvent = new (std::nothrow) event();

	if(pNewEvent == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewEvent in clsEventQueue::AddThread\n", 0);
        return;
    }

    if(sMsg != NULL) {
        size_t szLen = strlen(sMsg);
		pNewEvent->sMsg = (char *)malloc(szLen+1);
		if(pNewEvent->sMsg == NULL) {
            delete pNewEvent;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for pNewEvent->sMsg in clsEventQueue::AddThread\n", (uint64_t)(szLen+1));

            return;
        }

        memcpy(pNewEvent->sMsg, sMsg, szLen);
        pNewEvent->sMsg[szLen] = '\0';
    } else {
		pNewEvent->sMsg = NULL;
    }

    pNewEvent->ui8Id = ui8Id;

    if(sas != NULL) {
        if(sas->ss_family == AF_INET6) {
            memcpy(pNewEvent->ui128IpHash, &((struct sockaddr_in6 *)sas)->sin6_addr.s6_addr, 16);
        } else {
            memset(pNewEvent->ui128IpHash, 0, 16);
            pNewEvent->ui128IpHash[10] = 255;
            pNewEvent->ui128IpHash[11] = 255;
            memcpy(pNewEvent->ui128IpHash+12, &((struct sockaddr_in *)sas)->sin_addr.s_addr, 4);
        }
    } else {
        memset(pNewEvent->ui128IpHash, 0, 16);
    }

#ifdef _WIN32
    EnterCriticalSection(&csEventQueue);
#else
	pthread_mutex_lock(&mtxEventQueue);
#endif

    if(pThreadS == NULL) {
        pThreadS = pNewEvent;
        pNewEvent->pPrev = NULL;
    } else {
        pNewEvent->pPrev = pThreadE;
        pThreadE->pNext = pNewEvent;
    }

    pThreadE = pNewEvent;
    pNewEvent->pNext = NULL;

#ifdef _WIN32
    LeaveCriticalSection(&csEventQueue);
#else
	pthread_mutex_unlock(&mtxEventQueue);
#endif
}
//---------------------------------------------------------------------------

void clsEventQueue::ProcessEvents() {
	event * cur = NULL,
        * next = pNormalS;

	pNormalS = NULL;
	pNormalE = NULL;

	while(next != NULL) {
		cur = next;
		next = cur->pNext;

        switch(cur->ui8Id) {
			case EVENT_RESTART:
				clsServerManager::bIsRestart = true;
				clsServerManager::Stop();
                break;
			case EVENT_RSTSCRIPTS:
                clsScriptManager::mPtr->Restart();
                break;
            case EVENT_RSTSCRIPT: {
            	Script * curScript = clsScriptManager::mPtr->FindScript(cur->sMsg);
                if(curScript == NULL || curScript->bEnabled == false || curScript->pLUA == NULL) {
                    return;
                }

                clsScriptManager::mPtr->StopScript(curScript, false);

				clsScriptManager::mPtr->StartScript(curScript, false);

                break;
            }
			case EVENT_STOPSCRIPT: {
				Script * curScript = clsScriptManager::mPtr->FindScript(cur->sMsg);
            	if(curScript == NULL || curScript->bEnabled == false || curScript->pLUA == NULL) {
                    return;
                }

				clsScriptManager::mPtr->StopScript(curScript, true);

                break;
            }
			case EVENT_STOP_SCRIPTING:
                if(clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
                    clsSettingManager::mPtr->bBools[SETBOOL_ENABLE_SCRIPTING] = false;
                    clsScriptManager::mPtr->OnExit(true);
                    clsScriptManager::mPtr->Stop();
                }

                break;
			case EVENT_SHUTDOWN:
                if(clsServerManager::bIsClose == true) {
                    break;
                }

                clsServerManager::bIsClose = true;
                clsServerManager::Stop();

                break;
            default:
                break;
        }

#ifdef _WIN32
        if(cur->sMsg != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sMsg) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate cur->sMsg in clsEventQueue::ProcessEvents\n", 0);
            }
        }
#else
		free(cur->sMsg);
#endif

        delete cur;
    }

#ifdef _WIN32
    EnterCriticalSection(&csEventQueue);
#else
	pthread_mutex_lock(&mtxEventQueue);
#endif

    next = pThreadS;

    pThreadS = NULL;
    pThreadE = NULL;

#ifdef _WIN32
    LeaveCriticalSection(&csEventQueue);
#else
	pthread_mutex_unlock(&mtxEventQueue);
#endif

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        switch(cur->ui8Id) {
            case EVENT_REGSOCK_MSG:
                clsUdpDebug::mPtr->Broadcast(cur->sMsg);
                break;
            case EVENT_SRVTHREAD_MSG:
                clsUdpDebug::mPtr->Broadcast(cur->sMsg);
                break;
            case EVENT_UDP_SR: {
                size_t szMsgLen = strlen(cur->sMsg);
                clsServerManager::ui64BytesRead += (uint64_t)szMsgLen;

                char *temp = strchr(cur->sMsg+4, ' ');
                if(temp == NULL) {
                    break;;
                }

                size_t szLen = (temp-cur->sMsg)-4;
                if(szLen > 64 || szLen == 0) {
                    break;
                }

				// terminate nick, needed for strcasecmp in clsHashManager
                temp[0] = '\0';

                User *u = clsHashManager::mPtr->FindUser(cur->sMsg+4, szLen);
                if(u == NULL) {
                    break;
                }

                // add back space after nick...
                temp[0] = ' ';

                if(memcmp(cur->ui128IpHash, u->ui128IpHash, 16) != 0) {
                    break;
                }

#ifdef _BUILD_GUI
    if(::SendMessage(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
        char msg[128];
        int imsglen = sprintf(msg, "UDP > %s (%s) > ", u->sNick, u->sIP);
        if(CheckSprintf(imsglen, 128, "clsEventQueue::ProcessEvents") == true) {
            RichEditAppendText(clsMainWindowPageUsersChat::mPtr->hWndPageItems[clsMainWindowPageUsersChat::REDT_CHAT], (string(msg, imsglen)+cur->sMsg).c_str());
        }
    }
#endif

                clsDcCommands::mPtr->SRFromUDP(u, cur->sMsg, szMsgLen);
                break;
            }
            default:
                break;
        }

        free(cur->sMsg);

        delete cur;
    }
}
//---------------------------------------------------------------------------

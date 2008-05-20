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
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
eventq *eventqueue = NULL;
//---------------------------------------------------------------------------

eventq::eventq() {
    pthread_mutex_init(&mtxEventQueue, NULL);

    NormalS = NULL;
    NormalE = NULL;

    ThreadS = NULL;
    ThreadE = NULL;
}
//---------------------------------------------------------------------------

eventq::~eventq() {
    event * next = NormalS;

    while(next != NULL) {
        event * cur = next;
        next = cur->next;

        if(cur->sMsg != NULL) {
            free(cur->sMsg);
        }

        delete cur;
    }

    next = ThreadS;

    while(next != NULL) {
        event * cur = next;
        next = cur->next;

        if(cur->sMsg != NULL) {
            free(cur->sMsg);
        }

        delete cur;
    }

	pthread_mutex_destroy(&mtxEventQueue);
}
//---------------------------------------------------------------------------

void eventq::AddNormal(uint8_t ui8Id, char * sMsg) {
	if(ui8Id != EVENT_RSTSCRIPT && ui8Id != EVENT_STOPSCRIPT) {
		event * next = NormalS;

		while(next != NULL) {
			event * cur = next;
			next = cur->next;

			if(cur->ui8Id == ui8Id) {
                return;
            }
		}
	}

    event * newevent = new event();

	if(newevent == NULL) {
		string sDbgstr = "[BUF] Cannot allocate new event in eventq::AddNormal!";
		AppendSpecialLog(sDbgstr);
        return;
    }

    if(sMsg != NULL) {
        size_t iLen = strlen(sMsg);
		newevent->sMsg = (char *) malloc(iLen+1);
		if(newevent->sMsg == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
				" bytes of memory for newevent->sMsg in eventq::AddNormal!";
			AppendSpecialLog(sDbgstr);
            delete newevent;
            return;
        }

        memcpy(newevent->sMsg, sMsg, iLen);
        newevent->sMsg[iLen] = '\0';
    } else {
		newevent->sMsg = NULL;
    }

    newevent->ui8Id = ui8Id;

    if(NormalS == NULL) {
        NormalS = newevent;
        newevent->prev = NULL;
    } else {
        newevent->prev = NormalE;
        NormalE->next = newevent; 
    }

    NormalE = newevent;
    newevent->next = NULL;
}
//---------------------------------------------------------------------------

void eventq::AddThread(uint8_t ui8Id, char * sMsg, const uint32_t &ui32Hash/* = 0*/) {
	event * newevent = new event();

	if(newevent == NULL) {
		string sDbgstr = "[BUF] Cannot allocate new event in eventq::AddNormal!";
		AppendSpecialLog(sDbgstr);
        return;
    }

    if(sMsg != NULL) {
        size_t iLen = strlen(sMsg);
		newevent->sMsg = (char *) malloc(iLen+1);
		if(newevent->sMsg == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
				" bytes of memory for newevent->sMsg in eventq::AddThread!";
			AppendSpecialLog(sDbgstr);
            delete newevent;
            return;
        }

        memcpy(newevent->sMsg, sMsg, iLen);
        newevent->sMsg[iLen] = '\0';
    } else {
		newevent->sMsg = NULL;
    }

    newevent->ui8Id = ui8Id;
    newevent->ui32Hash = ui32Hash;

    pthread_mutex_lock(&mtxEventQueue);

    if(ThreadS == NULL) {
        ThreadS = newevent;
        newevent->prev = NULL;
    } else {
        newevent->prev = ThreadE;
        ThreadE->next = newevent;
    }

    ThreadE = newevent;
    newevent->next = NULL;

    pthread_mutex_unlock(&mtxEventQueue);
}
//---------------------------------------------------------------------------

void eventq::ProcessEvents() {
	event * next = NormalS;

	NormalS = NULL;
	NormalE = NULL;

	while(next != NULL) {
		event * cur = next;
		next = cur->next;

        switch(cur->ui8Id) {
			case EVENT_RESTART:
				bIsRestart = true;
				ServerStop();
                break;
			case EVENT_RSTSCRIPTS:
                ScriptManager->Restart();
                break;
            case EVENT_RSTSCRIPT: {
            	Script * curScript = ScriptManager->FindScript(cur->sMsg);
                if(curScript == NULL || curScript->bEnabled == false || curScript->LUA == NULL) {
                    return;
                }

                ScriptManager->StopScript(curScript);

				ScriptManager->StartScript(curScript);

                break;
            }
			case EVENT_STOPSCRIPT: {
				Script * curScript = ScriptManager->FindScript(cur->sMsg);
            	if(curScript == NULL || curScript->bEnabled == false || curScript->LUA == NULL) {
                    return;
                }

				ScriptManager->StopScript(curScript);

				curScript->bEnabled = false;

                break;
            }
			case EVENT_STOP_SCRIPTING:
                if(SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
                    SettingManager->bBools[SETBOOL_ENABLE_SCRIPTING] = false;
                    ScriptManager->OnExit(true);
                    ScriptManager->Stop();
                }

                break;
			case EVENT_SHUTDOWN:
                bIsClose = true;
                ServerStop();
                break;
            default:
                break;
        }

        if(cur->sMsg != NULL) {
            free(cur->sMsg);
        }

        delete cur;
    }

    pthread_mutex_lock(&mtxEventQueue);

    next = ThreadS;

    ThreadS = NULL;
    ThreadE = NULL;

    pthread_mutex_unlock(&mtxEventQueue);

    while(next != NULL) {
        event * cur = next;
        next = cur->next;

        switch(cur->ui8Id) {
            case EVENT_REGSOCK_MSG:
                UdpDebug->Broadcast(cur->sMsg);
                break;
            case EVENT_SRVTHREAD_MSG:
                UdpDebug->Broadcast(cur->sMsg);
                break;
            case EVENT_UDP_SR: {
                size_t iMsgLen = strlen(cur->sMsg);
                ui64BytesRead += (uint64_t)iMsgLen;

                char *temp = strchr(cur->sMsg+4, ' ');
                if(temp == NULL) {
                    break;;
                }

                size_t iLen = (temp-cur->sMsg)-4;
                if(iLen > 64 || iLen == 0) {
                    break;
                }

                // terminate nick, needed for strcasecmp in hashmanager
                temp[0] = '\0';

                User *u = hashManager->FindUser(cur->sMsg+4, iLen);
                if(u == NULL) {
                    break;
                }

                // add back space after nick...
                temp[0] = ' ';

                if(cur->ui32Hash != u->ui32IpHash) {
                    break;
                }

                DcCommands->SRFromUDP(u, cur->sMsg, iMsgLen);
                break;
            }
            default:
                break;
        }

        if(cur->sMsg != NULL) {
            free(cur->sMsg);
        }

        delete cur;
    }
}
//---------------------------------------------------------------------------

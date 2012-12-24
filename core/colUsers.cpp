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
#include "colUsers.h"
//---------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
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
static const uint32_t MYINFOLISTSIZE = 1024*256;
static const uint32_t IPLISTSIZE = 1024*64;
//---------------------------------------------------------------------------
classUsers *colUsers = NULL;
//---------------------------------------------------------------------------

classUsers::classUsers() {
    msg[0] = '\0';
    llist = NULL;
    elist = NULL;

    RecTimeList = NULL;
    
    ui16ActSearchs = 0;
    ui16PasSearchs = 0;

#ifdef _WIN32
    hRecvHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x20000, 0);
    hSendHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);

    nickList = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, NICKLISTSIZE);
#else
	nickList = (char *)calloc(NICKLISTSIZE, 1);
#endif
    if(nickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create nickList\n", 0);
		exit(EXIT_FAILURE);
    }
    memcpy(nickList, "$NickList |", 11);
    nickList[11] = '\0';
    nickListLen = 11;
    nickListSize = NICKLISTSIZE - 1;
#ifdef _WIN32
    sZNickList = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	sZNickList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(sZNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create sZNickList\n", 0);
		exit(EXIT_FAILURE);
    }
    iZNickListLen = 0;
    iZNickListSize = ZLISTSIZE - 1;
#ifdef _WIN32
    opList = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, OPLISTSIZE);
#else
	opList = (char *)calloc(OPLISTSIZE, 1);
#endif
    if(opList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create opList\n", 0);
		exit(EXIT_FAILURE);
    }
    memcpy(opList, "$OpList |", 9);
    opList[9] = '\0';
    opListLen = 9;
    opListSize = OPLISTSIZE - 1;
#ifdef _WIN32
    sZOpList = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	sZOpList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(sZOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create sZOpList\n", 0);
		exit(EXIT_FAILURE);
    }
    iZOpListLen = 0;
    iZOpListSize = ZLISTSIZE - 1;
    
    if(SettingManager->ui8FullMyINFOOption != 0) {
#ifdef _WIN32
        myInfos = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		myInfos = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(myInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create myInfos\n", 0);
    		exit(EXIT_FAILURE);
        }
        myInfosSize = MYINFOLISTSIZE - 1;
        
#ifdef _WIN32
        sZMyInfos = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		sZMyInfos = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(sZMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create sZMyInfos\n", 0);
    		exit(EXIT_FAILURE);
        }
        iZMyInfosSize = ZMYINFOLISTSIZE - 1;
    } else {
        myInfos = NULL;
        myInfosSize = 0;
        sZMyInfos = NULL;
        iZMyInfosSize = 0;
    }
    myInfosLen = 0;
    iZMyInfosLen = 0;
    
    if(SettingManager->ui8FullMyINFOOption != 2) {
#ifdef _WIN32
        myInfosTag = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		myInfosTag = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(myInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create myInfosTag\n", 0);
    		exit(EXIT_FAILURE);
        }
        myInfosTagSize = MYINFOLISTSIZE - 1;

#ifdef _WIN32
        sZMyInfosTag = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		sZMyInfosTag = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(sZMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create sZMyInfosTag\n", 0);
    		exit(EXIT_FAILURE);
        }
        iZMyInfosTagSize = ZMYINFOLISTSIZE - 1;
    } else {
        myInfosTag = NULL;
        myInfosTagSize = 0;
        sZMyInfosTag = NULL;
        iZMyInfosTagSize = 0;
    }
    myInfosTagLen = 0;
    iZMyInfosTagLen = 0;

    iChatMsgs = 0;
    iChatMsgsTick = 0;
    iChatLockFromTick = 0;
    bChatLocked = false;

#ifdef _WIN32
    userIPList = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, IPLISTSIZE);
#else
	userIPList = (char *)calloc(IPLISTSIZE, 1);
#endif
    if(userIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create userIPList\n", 0);
		exit(EXIT_FAILURE);
    }
    memcpy(userIPList, "$UserIP |", 9);
    userIPList[9] = '\0';
    userIPListLen = 9;
    userIPListSize = IPLISTSIZE - 1;

#ifdef _WIN32
    sZUserIPList = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	sZUserIPList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(sZUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create sZUserIPList\n", 0);
		exit(EXIT_FAILURE);
    }
    iZUserIPListLen = 0;
    iZUserIPListSize = ZLISTSIZE - 1;
}
//---------------------------------------------------------------------------

classUsers::~classUsers() {
    RecTime * next = RecTimeList;

    while(next != NULL) {
        RecTime * cur = next;
        next = cur->next;

#ifdef _WIN32
        if(cur->sNick != NULL) {
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sNick) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate cur->sNick in classUsers::~classUsers\n", 0);
            }
        }
#else
		free(cur->sNick);
#endif

        delete cur;
    }

#ifdef _WIN32
    if(nickList != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)nickList) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate nickList in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(nickList);
#endif

#ifdef _WIN32
    if(sZNickList != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sZNickList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sZNickList in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(sZNickList);
#endif

#ifdef _WIN32
    if(opList != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)opList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate opList in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(opList);
#endif

#ifdef _WIN32
    if(sZOpList != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sZOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sZOpList in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(sZOpList);
#endif

#ifdef _WIN32
    if(myInfos != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)myInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate myInfos in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(myInfos);
#endif

#ifdef _WIN32
    if(sZMyInfos != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sZMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sZMyInfos in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(sZMyInfos);
#endif

#ifdef _WIN32
    if(myInfosTag != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)myInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate myInfosTag in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(myInfosTag);
#endif

#ifdef _WIN32
    if(sZMyInfosTag != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sZMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sZMyInfosTag in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(sZMyInfosTag);
#endif

#ifdef _WIN32
    if(userIPList != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)userIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate userIPList in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(userIPList);
#endif

#ifdef _WIN32
    if(sZUserIPList != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sZUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate sZUserIPList in classUsers::~classUsers\n", 0);
        }
    }
#else
	free(sZUserIPList);
#endif

#ifdef _WIN32
    HeapDestroy(hRecvHeap);
    HeapDestroy(hSendHeap);
#endif
}
//---------------------------------------------------------------------------

void classUsers::AddUser(User * u) {
    if(llist == NULL) {
    	llist = u;
    	elist = u;
    } else {
        u->prev = elist;
        elist->next = u;
        elist = u;
    }
}
//---------------------------------------------------------------------------

void classUsers::RemUser(User * u) {
    if(u->prev == NULL) {
        if(u->next == NULL) {
            llist = NULL;
            elist = NULL;
        } else {
            u->next->prev = NULL;
            llist = u->next;
        }
    } else if(u->next == NULL) {
        u->prev->next = NULL;
        elist = u->prev;
    } else {
        u->prev->next = u->next;
        u->next->prev = u->prev;
    }
    #ifdef _DEBUG
        int iret = sprintf(msg, "# User %s removed from linked list.", u->sNick);
        if(CheckSprintf(iret, 1024, "classUsers::RemUser") == true) {
            Cout(msg);
        }
    #endif
}
//---------------------------------------------------------------------------

void classUsers::DisconnectAll() {
    uint32_t iCloseLoops = 0;

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 50000000;
#endif

    while(llist != NULL && iCloseLoops <= 100) {
        User *next = llist;
        while(next != NULL) {
            User *u = next;
    		next = u->next;
            if(((u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) == true || u->sbdatalen == 0) {
//              Memo("*** User " + string(u->Nick, u->NickLen) + " closed...");
                if(u->prev == NULL) {
                    if(u->next == NULL) {
                        llist = NULL;
                    } else {
                        u->next->prev = NULL;
                        llist = u->next;
                    }
                } else if(u->next == NULL) {
                    u->prev->next = NULL;
                } else {
                    u->prev->next = u->next;
                    u->next->prev = u->prev;
                }

#ifdef _WIN32
                shutdown(u->Sck, SD_SEND);
				closesocket(u->Sck);
#else
                shutdown(u->Sck, SHUT_RD);
				close(u->Sck);
#endif

				delete u;
            } else {
                UserTry2Send(u);
            }
        }
        iCloseLoops++;
#ifdef _WIN32
        ::Sleep(50);
#else
        nanosleep(&sleeptime, NULL);
#endif
    }

    User *next = llist;
    while(next != NULL) {
        User *u = next;
    	next = u->next;
#ifdef _WIN32
    	shutdown(u->Sck, SD_SEND);
		closesocket(u->Sck);
#else
    	shutdown(u->Sck, SHUT_RDWR);
		close(u->Sck);
#endif

		delete u;
	}
}
//---------------------------------------------------------------------------

void classUsers::Add2NickList(User * u) {
    // $NickList nick$$nick2$$|

    if(nickListSize < nickListLen+u->ui8NickLen+2) {
        char * pOldBuf = nickList;
#ifdef _WIN32
        nickList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, nickListSize+NICKLISTSIZE+1);
#else
		nickList = (char *)realloc(pOldBuf, nickListSize+NICKLISTSIZE+1);
#endif
        if(nickList == NULL) {
            nickList = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::Add2NickList for NickList\n", (uint64_t)(nickListSize+NICKLISTSIZE+1));

            return;
		}
        nickListSize += NICKLISTSIZE;
    }

    memcpy(nickList+nickListLen-1, u->sNick, u->ui8NickLen);
    nickListLen += (uint32_t)(u->ui8NickLen+2);

    nickList[nickListLen-3] = '$';
    nickList[nickListLen-2] = '$';
    nickList[nickListLen-1] = '|';
    nickList[nickListLen] = '\0';

    iZNickListLen = 0;

    if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        return;
    }

    if(opListSize < opListLen+u->ui8NickLen+2) {
        char * pOldBuf = opList;
#ifdef _WIN32
        opList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, opListSize+OPLISTSIZE+1);
#else
		opList = (char *)realloc(pOldBuf, opListSize+OPLISTSIZE+1);
#endif
        if(opList == NULL) {
            opList = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::Add2NickList for opList\n", (uint64_t)(opListSize+OPLISTSIZE+1));

            return;
        }
        opListSize += OPLISTSIZE;
    }

    memcpy(opList+opListLen-1, u->sNick, u->ui8NickLen);
    opListLen += (uint32_t)(u->ui8NickLen+2);

    opList[opListLen-3] = '$';
    opList[opListLen-2] = '$';
    opList[opListLen-1] = '|';
    opList[opListLen] = '\0';

    iZOpListLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::AddBot2NickList(char * Nick, const size_t &szNickLen, const bool &isOp) {
    // $NickList nick$$nick2$$|

    if(nickListSize < nickListLen+szNickLen+2) {
        char * pOldBuf = nickList;
#ifdef _WIN32
        nickList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, nickListSize+NICKLISTSIZE+1);
#else
		nickList = (char *)realloc(pOldBuf, nickListSize+NICKLISTSIZE+1);
#endif
        if(nickList == NULL) {
            nickList = pOldBuf;

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::AddBot2NickList for NickList\n", (uint64_t)(nickListSize+NICKLISTSIZE+1));

            return;
		}
        nickListSize += NICKLISTSIZE;
    }

    memcpy(nickList+nickListLen-1, Nick, szNickLen);
    nickListLen += (uint32_t)(szNickLen+2);

    nickList[nickListLen-3] = '$';
    nickList[nickListLen-2] = '$';
    nickList[nickListLen-1] = '|';
    nickList[nickListLen] = '\0';

    iZNickListLen = 0;

    if(isOp == false)
        return;

    if(opListSize < opListLen+szNickLen+2) {
        char * pOldBuf = opList;
#ifdef _WIN32
        opList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, opListSize+OPLISTSIZE+1);
#else
		opList = (char *)realloc(pOldBuf, opListSize+OPLISTSIZE+1);
#endif
        if(opList == NULL) {
            opList = pOldBuf;

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::AddBot2NickList for opList\n", (uint64_t)(opListSize+OPLISTSIZE+1));

            return;
        }
        opListSize += OPLISTSIZE;
    }

    memcpy(opList+opListLen-1, Nick, szNickLen);
    opListLen += (uint32_t)(szNickLen+2);

    opList[opListLen-3] = '$';
    opList[opListLen-2] = '$';
    opList[opListLen-1] = '|';
    opList[opListLen] = '\0';

    iZOpListLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::Add2OpList(User * u) {
    if(opListSize < opListLen+u->ui8NickLen+2) {
        char * pOldBuf = opList;
#ifdef _WIN32
        opList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, opListSize+OPLISTSIZE+1);
#else
		opList = (char *)realloc(pOldBuf, opListSize+OPLISTSIZE+1);
#endif
        if(opList == NULL) {
            opList = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::Add2OpList for opList\n", (uint64_t)(opListSize+OPLISTSIZE+1));

            return;
        }
        opListSize += OPLISTSIZE;
    }

    memcpy(opList+opListLen-1, u->sNick, u->ui8NickLen);
    opListLen += (uint32_t)(u->ui8NickLen+2);

    opList[opListLen-3] = '$';
    opList[opListLen-2] = '$';
    opList[opListLen-1] = '|';
    opList[opListLen] = '\0';

    iZOpListLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::DelFromNickList(char * Nick, const bool &isOp) {
    int m = sprintf(msg, "$%s$", Nick);
    if(CheckSprintf(m, 1024, "classUsers::DelFromNickList") == false) {
        return;
    }

    nickList[9] = '$';
    char *off = strstr(nickList, msg);
    nickList[9] = ' ';

    if(off != NULL) {
        memmove(off+1, off+(m+1), nickListLen-((off+m)-nickList));
        nickListLen -= m;
        iZNickListLen = 0;
    }

    if(!isOp) return;

    opList[7] = '$';
    off = strstr(opList, msg);
    opList[7] = ' ';

    if(off != NULL) {
        memmove(off+1, off+(m+1), opListLen-((off+m)-opList));
        opListLen -= m;
        iZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

void classUsers::DelFromOpList(char * Nick) {
    int m = sprintf(msg, "$%s$", Nick);
    if(CheckSprintf(m, 1024, "classUsers::DelFromOpList") == false) {
        return;
    }

    opList[7] = '$';
    char *off = strstr(opList, msg);
    opList[7] = ' ';

    if(off != NULL) {
        memmove(off+1, off+(m+1), opListLen-((off+m)-opList));
        opListLen -= m;
        iZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

// PPK ... check global mainchat flood and add to global queue
void classUsers::SendChat2All(User * cur, char * sData, const size_t &szChatLen, void * ptr) {
    UdpDebug->Broadcast(sData, szChatLen);

    if(ProfileMan->IsAllowed(cur, ProfileManager::NODEFLOODMAINCHAT) == false && 
        SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] != 0) {
        if(iChatMsgs == 0) {
			iChatMsgsTick = ui64ActualTick;
			iChatLockFromTick = ui64ActualTick;
            iChatMsgs = 0;
            bChatLocked = false;
		} else if((iChatMsgsTick+SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME]) < ui64ActualTick) {
			iChatMsgsTick = ui64ActualTick;
            iChatMsgs = 0;
        }

        iChatMsgs++;

        if(iChatMsgs > (uint16_t)SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES]) {
			iChatLockFromTick = ui64ActualTick;
            if(bChatLocked == false) {
                if(SettingManager->bBools[SETBOOL_DEFLOOD_REPORT] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                        if(CheckSprintf(imsgLen, 1024, "classUsers::SendChat2All1") == true) {
                            g_GlobalDataQueue->SingleItemStore(msg, imsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                        if(CheckSprintf(imsgLen, 1024, "classUsers::SendChat2All2") == true) {
                            g_GlobalDataQueue->AddQueueItem(msg, imsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                        }
                    }
                }
                bChatLocked = true;
            }
        }

        if(bChatLocked == true) {
            if((iChatLockFromTick+SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT]) > ui64ActualTick) {
                if(SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 1) {
                    return;
                } else if(SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 2) {
                    if(szChatLen > 64000) {
                        sData[64000] = '\0';
                    }

                	int iMsgLen = sprintf(g_sBuffer, "%s ", cur->sIP);
                	if(CheckSprintf(iMsgLen, g_szBufferSize, "classUsers::SendChat2All3") == true) {
                        memcpy(g_sBuffer+iMsgLen, sData, szChatLen);
                        iMsgLen += (uint32_t)szChatLen;
                        g_sBuffer[iMsgLen] = '\0';
                        g_GlobalDataQueue->AddQueueItem(g_sBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
                    }

                    return;
                }
            } else {
                bChatLocked = false;
            }
        }
    }

    if(ptr == NULL) {
        g_GlobalDataQueue->AddQueueItem(sData, szChatLen, NULL, 0, GlobalDataQueue::CMD_CHAT);
    } else {
        g_GlobalDataQueue->InsertQueueItem(sData, szChatLen, ptr, GlobalDataQueue::CMD_CHAT);
    }
}
//---------------------------------------------------------------------------

void classUsers::Add2MyInfos(User * u) {
    if(myInfosSize < myInfosLen+u->ui16MyInfoShortLen) {
        char * pOldBuf = myInfos;
#ifdef _WIN32
        myInfos = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, myInfosSize+MYINFOLISTSIZE+1);
#else
		myInfos = (char *)realloc(pOldBuf, myInfosSize+MYINFOLISTSIZE+1);
#endif
        if(myInfos == NULL) {
            myInfos = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::Add2MyInfos\n", (uint64_t)(myInfosSize+MYINFOLISTSIZE+1));

            return;
        }
        myInfosSize += MYINFOLISTSIZE;
    }

    memcpy(myInfos+myInfosLen, u->sMyInfoShort, u->ui16MyInfoShortLen);
    myInfosLen += u->ui16MyInfoShortLen;

    myInfos[myInfosLen] = '\0';

    iZMyInfosLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::DelFromMyInfos(User * u) {
	char *match = strstr(myInfos, u->sMyInfoShort+8);
    if(match != NULL) {
		match -= 8;
		memmove(match, match+u->ui16MyInfoShortLen, myInfosLen-((match+(u->ui16MyInfoShortLen-1))-myInfos));
        myInfosLen -= u->ui16MyInfoShortLen;
        iZMyInfosLen = 0;
    }
}
//---------------------------------------------------------------------------

void classUsers::Add2MyInfosTag(User * u) {
    if(myInfosTagSize < myInfosTagLen+u->ui16MyInfoLongLen) {
        char * pOldBuf = myInfosTag;
#ifdef _WIN32
        myInfosTag = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, myInfosTagSize+MYINFOLISTSIZE+1);
#else
		myInfosTag = (char *)realloc(pOldBuf, myInfosTagSize+MYINFOLISTSIZE+1);
#endif
        if(myInfosTag == NULL) {
            myInfosTag = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            UserClose(u);

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::Add2MyInfosTag\n", (uint64_t)(myInfosTagSize+MYINFOLISTSIZE+1));

            return;
        }
        myInfosTagSize += MYINFOLISTSIZE;
    }

    memcpy(myInfosTag+myInfosTagLen, u->sMyInfoLong, u->ui16MyInfoLongLen);
    myInfosTagLen += u->ui16MyInfoLongLen;

    myInfosTag[myInfosTagLen] = '\0';

    iZMyInfosTagLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::DelFromMyInfosTag(User * u) {
	char *match = strstr(myInfosTag, u->sMyInfoLong+8);
    if(match != NULL) {
		match -= 8;
        memmove(match, match+u->ui16MyInfoLongLen, myInfosTagLen-((match+(u->ui16MyInfoLongLen-1))-myInfosTag));
        myInfosTagLen -= u->ui16MyInfoLongLen;
        iZMyInfosTagLen = 0;
    }
}
//---------------------------------------------------------------------------

void classUsers::AddBot2MyInfos(char * MyInfo) {
	size_t len = strlen(MyInfo);
	if(myInfosTag != NULL) {
	    if(strstr(myInfosTag, MyInfo) == NULL ) {
            if(myInfosTagSize < myInfosTagLen+len) {
                char * pOldBuf = myInfosTag;
#ifdef _WIN32
                myInfosTag = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, myInfosTagSize+MYINFOLISTSIZE+1);
#else
				myInfosTag = (char *)realloc(pOldBuf, myInfosTagSize+MYINFOLISTSIZE+1);
#endif
                if(myInfosTag == NULL) {
                    myInfosTag = pOldBuf;

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for myInfosTag in classUsers::AddBot2MyInfos\n", (uint64_t)(myInfosTagSize+MYINFOLISTSIZE+1));

                    return;
                }
                myInfosTagSize += MYINFOLISTSIZE;
            }
        	memcpy(myInfosTag+myInfosTagLen, MyInfo, len);
            myInfosTagLen += (uint32_t)len;
            myInfosTag[myInfosTagLen] = '\0';
            iZMyInfosLen = 0;
        }
    }
    if(myInfos != NULL) {
    	if(strstr(myInfos, MyInfo) == NULL ) {
            if(myInfosSize < myInfosLen+len) {
                char * pOldBuf = myInfos;
#ifdef _WIN32
                myInfos = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, myInfosSize+MYINFOLISTSIZE+1);
#else
				myInfos = (char *)realloc(pOldBuf, myInfosSize+MYINFOLISTSIZE+1);
#endif
                if(myInfos == NULL) {
                    myInfos = pOldBuf;

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for myInfos in classUsers::AddBot2MyInfos\n", (uint64_t)(myInfosSize+MYINFOLISTSIZE+1));

                    return;
                }
                myInfosSize += MYINFOLISTSIZE;
            }
        	memcpy(myInfos+myInfosLen, MyInfo, len);
            myInfosLen += (uint32_t)len;
            myInfos[myInfosLen] = '\0';
            iZMyInfosTagLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void classUsers::DelBotFromMyInfos(char * MyInfo) {
	size_t len = strlen(MyInfo);
	if(myInfosTag) {
		char *match = strstr(myInfosTag,  MyInfo);
	    if(match) {
    		memmove(match, match+len, myInfosTagLen-((match+(len-1))-myInfosTag));
        	myInfosTagLen -= (uint32_t)len;
        	iZMyInfosTagLen = 0;
         }
    }
	if(myInfos) {
		char *match = strstr(myInfos,  MyInfo);
	    if(match) {
    		memmove(match, match+len, myInfosLen-((match+(len-1))-myInfos));
        	myInfosLen -= (uint32_t)len;
        	iZMyInfosLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void classUsers::Add2UserIP(User * cur) {
    int m = sprintf(msg,"$%s %s$", cur->sNick, cur->sIP);
    if(CheckSprintf(m, 1024, "classUsers::Add2UserIP") == false) {
        return;
    }

    if(userIPListSize < userIPListLen+m) {
        char * pOldBuf = userIPList;
#ifdef _WIN32
        userIPList = (char *)HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, userIPListSize+IPLISTSIZE+1);
#else
		userIPList = (char *)realloc(pOldBuf, userIPListSize+IPLISTSIZE+1);
#endif
        if(userIPList == NULL) {
            userIPList = pOldBuf;
            cur->ui32BoolBits |= User::BIT_ERROR;
            UserClose(cur);

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in classUsers::Add2UserIP\n", (uint64_t)(userIPListSize+IPLISTSIZE+1));

            return;
        }
        userIPListSize += IPLISTSIZE;
    }

    memcpy(userIPList+userIPListLen-1, msg+1, m-1);
    userIPListLen += m;

    userIPList[userIPListLen-2] = '$';
    userIPList[userIPListLen-1] = '|';
    userIPList[userIPListLen] = '\0';

    iZUserIPListLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::DelFromUserIP(User * cur) {
    int m = sprintf(msg,"$%s %s$", cur->sNick, cur->sIP);
    if(CheckSprintf(m, 1024, "classUsers::DelFromUserIP") == false) {
        return;
    }

    userIPList[7] = '$';
    char *off = strstr(userIPList, msg);
    userIPList[7] = ' ';
    
	if(off != NULL) {
        memmove(off+1, off+(m+1), userIPListLen-((off+m)-userIPList));
        userIPListLen -= m;
        iZUserIPListLen = 0;
    }
}
//---------------------------------------------------------------------------

void classUsers::Add2RecTimes(User * curUser) {
    time_t acc_time;
    time(&acc_time);

    if(ProfileMan->IsAllowed(curUser, ProfileManager::NOUSRSAMEIP) == true ||
        (acc_time-curUser->LoginTime) >= SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME]) {
        return;
    }

    RecTime * pNewRecTime = new RecTime();

	if(pNewRecTime == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewRecTime in classUsers::Add2RecTimes\n", 0);
        return;
	}

#ifdef _WIN32
    pNewRecTime->sNick = (char *)HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, curUser->ui8NickLen+1);
#else
	pNewRecTime->sNick = (char *)malloc(curUser->ui8NickLen+1);
#endif
	if(pNewRecTime->sNick == NULL) {
        delete pNewRecTime;

        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in classUsers::Add2RecTimes\n", (uint64_t)(curUser->ui8NickLen+1));

        return;
    }

    memcpy(pNewRecTime->sNick, curUser->sNick, curUser->ui8NickLen);
    pNewRecTime->sNick[curUser->ui8NickLen] = '\0';

	pNewRecTime->ui64DisConnTick = ui64ActualTick-(acc_time-curUser->LoginTime);
    pNewRecTime->ui32NickHash = curUser->ui32NickHash;

    memcpy(pNewRecTime->ui128IpHash, curUser->ui128IpHash, 16);

    pNewRecTime->prev = NULL;
    pNewRecTime->next = RecTimeList;

	if(RecTimeList != NULL) {
		RecTimeList->prev = pNewRecTime;
	}

	RecTimeList = pNewRecTime;
}
//---------------------------------------------------------------------------

bool classUsers::CheckRecTime(User * curUser) {
    RecTime * next = RecTimeList;

    while(next != NULL) {
        RecTime * cur = next;
        next = cur->next;

        // check expires...
        if(cur->ui64DisConnTick+SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME] <= ui64ActualTick) {
#ifdef _WIN32
            if(cur->sNick != NULL) {
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sNick) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate cur->sNick in classUsers::CheckRecTime\n", 0);
                }
            }
#else
			free(cur->sNick);
#endif

            if(cur->prev == NULL) {
                if(cur->next == NULL) {
                    RecTimeList = NULL;
                } else {
                    cur->next->prev = NULL;
                    RecTimeList = cur->next;
                }
            } else if(cur->next == NULL) {
                cur->prev->next = NULL;
            } else {
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
            }

            delete cur;
            continue;
        }

        if(cur->ui32NickHash == curUser->ui32NickHash && memcmp(cur->ui128IpHash, curUser->ui128IpHash, 16) == 0 && strcasecmp(cur->sNick, curUser->sNick) == 0) {
            int imsgLen = sprintf(msg, "<%s> %s %" PRIu64 " %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], LanguageManager->sTexts[LAN_PLEASE_WAIT], 
                (cur->ui64DisConnTick+SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME])-ui64ActualTick, LanguageManager->sTexts[LAN_SECONDS_BEFORE_RECONN]);
            if(CheckSprintf(imsgLen, 1024, "classUsers::CheckRecTime1") == true) {
                UserSendChar(curUser, msg, imsgLen);
            }
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

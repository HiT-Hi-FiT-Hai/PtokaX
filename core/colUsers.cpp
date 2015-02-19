/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
clsUsers * clsUsers::mPtr = NULL;
//---------------------------------------------------------------------------

clsUsers::RecTime::RecTime(const uint8_t * pIpHash) : ui64DisConnTick(0), ui32NickHash(0), pPrev(NULL), pNext(NULL), sNick(NULL) {
    memcpy(ui128IpHash, pIpHash, 16);
};
//---------------------------------------------------------------------------

clsUsers::clsUsers() : ui64ChatMsgsTick(0), ui64ChatLockFromTick(0), ui16ChatMsgs(0), pRecTimeList(NULL), pListE(NULL), bChatLocked(false), ui32MyInfosLen(0), ui32MyInfosSize(0), ui32ZMyInfosLen(0), ui32ZMyInfosSize(0), ui32MyInfosTagLen(0), ui32MyInfosTagSize(0), ui32ZMyInfosTagLen(0),
	ui32ZMyInfosTagSize(0), ui32NickListLen(0), ui32NickListSize(0), ui32ZNickListLen(0), ui32ZNickListSize(0), ui32OpListLen(0), ui32OpListSize(0), ui32ZOpListLen(0), ui32ZOpListSize(0), ui32UserIPListSize(0), ui32UserIPListLen(0), ui32ZUserIPListSize(0), ui32ZUserIPListLen(0),
    pNickList(NULL), pZNickList(NULL), pOpList(NULL), pZOpList(NULL), pUserIPList(NULL), pZUserIPList(NULL), pMyInfos(NULL), pZMyInfos(NULL), pMyInfosTag(NULL), pZMyInfosTag(NULL), pListS(NULL), ui16ActSearchs(0), ui16PasSearchs(0) {
    msg[0] = '\0';

#ifdef _WIN32
    clsServerManager::hRecvHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x20000, 0);
    clsServerManager::hSendHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);

    pNickList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, NICKLISTSIZE);
#else
	pNickList = (char *)calloc(NICKLISTSIZE, 1);
#endif
    if(pNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pNickList\n", 0);
		exit(EXIT_FAILURE);
    }
    memcpy(pNickList, "$NickList |", 11);
    pNickList[11] = '\0';
    ui32NickListLen = 11;
    ui32NickListSize = NICKLISTSIZE - 1;
#ifdef _WIN32
    pZNickList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	pZNickList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(pZNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pZNickList\n", 0);
		exit(EXIT_FAILURE);
    }
    ui32ZNickListLen = 0;
    ui32ZNickListSize = ZLISTSIZE - 1;
#ifdef _WIN32
    pOpList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, OPLISTSIZE);
#else
	pOpList = (char *)calloc(OPLISTSIZE, 1);
#endif
    if(pOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pOpList\n", 0);
		exit(EXIT_FAILURE);
    }
    memcpy(pOpList, "$OpList |", 9);
    pOpList[9] = '\0';
    ui32OpListLen = 9;
    ui32OpListSize = OPLISTSIZE - 1;
#ifdef _WIN32
    pZOpList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	pZOpList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(pZOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pZOpList\n", 0);
		exit(EXIT_FAILURE);
    }
    ui32ZOpListLen = 0;
    ui32ZOpListSize = ZLISTSIZE - 1;
    
    if(clsSettingManager::mPtr->ui8FullMyINFOOption != 0) {
#ifdef _WIN32
        pMyInfos = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		pMyInfos = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(pMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pMyInfos\n", 0);
    		exit(EXIT_FAILURE);
        }
        ui32MyInfosSize = MYINFOLISTSIZE - 1;
        
#ifdef _WIN32
        pZMyInfos = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		pZMyInfos = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(pZMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pZMyInfos\n", 0);
    		exit(EXIT_FAILURE);
        }
        ui32ZMyInfosSize = ZMYINFOLISTSIZE - 1;
    } else {
        pMyInfos = NULL;
        ui32MyInfosSize = 0;
        pZMyInfos = NULL;
        ui32ZMyInfosSize = 0;
    }
    ui32MyInfosLen = 0;
    ui32ZMyInfosLen = 0;
    
    if(clsSettingManager::mPtr->ui8FullMyINFOOption != 2) {
#ifdef _WIN32
        pMyInfosTag = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		pMyInfosTag = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(pMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pMyInfosTag\n", 0);
    		exit(EXIT_FAILURE);
        }
        ui32MyInfosTagSize = MYINFOLISTSIZE - 1;

#ifdef _WIN32
        pZMyInfosTag = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		pZMyInfosTag = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(pZMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create pZMyInfosTag\n", 0);
    		exit(EXIT_FAILURE);
        }
        ui32ZMyInfosTagSize = ZMYINFOLISTSIZE - 1;
    } else {
        pMyInfosTag = NULL;
        ui32MyInfosTagSize = 0;
        pZMyInfosTag = NULL;
        ui32ZMyInfosTagSize = 0;
    }
    ui32MyInfosTagLen = 0;
    ui32ZMyInfosTagLen = 0;

#ifdef _WIN32
    pUserIPList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, IPLISTSIZE);
#else
	pUserIPList = (char *)calloc(IPLISTSIZE, 1);
#endif
    if(pUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pUserIPList\n", 0);
		exit(EXIT_FAILURE);
    }
    memcpy(pUserIPList, "$UserIP |", 9);
    pUserIPList[9] = '\0';
    ui32UserIPListLen = 9;
    ui32UserIPListSize = IPLISTSIZE - 1;

#ifdef _WIN32
    pZUserIPList = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	pZUserIPList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(pZUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pZUserIPList\n", 0);
		exit(EXIT_FAILURE);
    }
    ui32ZUserIPListLen = 0;
    ui32ZUserIPListSize = ZLISTSIZE - 1;
}
//---------------------------------------------------------------------------

clsUsers::~clsUsers() {
    RecTime * cur = NULL,
        * next = pRecTimeList;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

#ifdef _WIN32
        if(cur->sNick != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sNick) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate cur->sNick in clsUsers::~clsUsers\n", 0);
            }
        }
#else
		free(cur->sNick);
#endif

        delete cur;
    }

#ifdef _WIN32
    if(pNickList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pNickList) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate pNickList in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pNickList);
#endif

#ifdef _WIN32
    if(pZNickList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZNickList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZNickList in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pZNickList);
#endif

#ifdef _WIN32
    if(pOpList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pOpList in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pOpList);
#endif

#ifdef _WIN32
    if(pZOpList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZOpList in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pZOpList);
#endif

#ifdef _WIN32
    if(pMyInfos != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pMyInfos in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pMyInfos);
#endif

#ifdef _WIN32
    if(pZMyInfos != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZMyInfos in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pZMyInfos);
#endif

#ifdef _WIN32
    if(pMyInfosTag != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pMyInfosTag in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pMyInfosTag);
#endif

#ifdef _WIN32
    if(pZMyInfosTag != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZMyInfosTag in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pZMyInfosTag);
#endif

#ifdef _WIN32
    if(pUserIPList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pUserIPList in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pUserIPList);
#endif

#ifdef _WIN32
    if(pZUserIPList != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pZUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate pZUserIPList in clsUsers::~clsUsers\n", 0);
        }
    }
#else
	free(pZUserIPList);
#endif

#ifdef _WIN32
    HeapDestroy(clsServerManager::hRecvHeap);
    HeapDestroy(clsServerManager::hSendHeap);
#endif
}
//---------------------------------------------------------------------------

void clsUsers::AddUser(User * u) {
    if(pListS == NULL) {
    	pListS = u;
    	pListE = u;
    } else {
        u->pPrev = pListE;
        pListE->pNext = u;
        pListE = u;
    }
}
//---------------------------------------------------------------------------

void clsUsers::RemUser(User * u) {
    if(u->pPrev == NULL) {
        if(u->pNext == NULL) {
            pListS = NULL;
            pListE = NULL;
        } else {
            u->pNext->pPrev = NULL;
            pListS = u->pNext;
        }
    } else if(u->pNext == NULL) {
        u->pPrev->pNext = NULL;
        pListE = u->pPrev;
    } else {
        u->pPrev->pNext = u->pNext;
        u->pNext->pPrev = u->pPrev;
    }
    #ifdef _DEBUG
        int iret = sprintf(msg, "# User %s removed from linked list.", u->sNick);
        if(CheckSprintf(iret, 1024, "clsUsers::RemUser") == true) {
            Cout(msg);
        }
    #endif
}
//---------------------------------------------------------------------------

void clsUsers::DisconnectAll() {
    uint32_t iCloseLoops = 0;

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 50000000;
#endif

    User * u = NULL, * next = NULL;

    while(pListS != NULL && iCloseLoops <= 100) {
        next = pListS;

        while(next != NULL) {
            u = next;
    		next = u->pNext;

            if(((u->ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) == true || u->ui32SendBufDataLen == 0) {
//              Memo("*** User " + string(u->Nick, u->NickLen) + " closed...");
                if(u->pPrev == NULL) {
                    if(u->pNext == NULL) {
                        pListS = NULL;
                    } else {
                        u->pNext->pPrev = NULL;
                        pListS = u->pNext;
                    }
                } else if(u->pNext == NULL) {
                    u->pPrev->pNext = NULL;
                } else {
                    u->pPrev->pNext = u->pNext;
                    u->pNext->pPrev = u->pPrev;
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
                u->Try2Send();
            }
        }
        iCloseLoops++;
#ifdef _WIN32
        ::Sleep(50);
#else
        nanosleep(&sleeptime, NULL);
#endif
    }

    next = pListS;

    while(next != NULL) {
        u = next;
    	next = u->pNext;

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

void clsUsers::Add2NickList(User * u) {
    // $NickList nick$$nick2$$|

    if(ui32NickListSize < ui32NickListLen+u->ui8NickLen+2) {
        char * pOldBuf = pNickList;
#ifdef _WIN32
        pNickList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#else
		pNickList = (char *)realloc(pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#endif
        if(pNickList == NULL) {
            pNickList = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            u->Close();

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::Add2NickList for NickList\n", (uint64_t)(ui32NickListSize+NICKLISTSIZE+1));

            return;
		}
        ui32NickListSize += NICKLISTSIZE;
    }

    memcpy(pNickList+ui32NickListLen-1, u->sNick, u->ui8NickLen);
    ui32NickListLen += (uint32_t)(u->ui8NickLen+2);

    pNickList[ui32NickListLen-3] = '$';
    pNickList[ui32NickListLen-2] = '$';
    pNickList[ui32NickListLen-1] = '|';
    pNickList[ui32NickListLen] = '\0';

    ui32ZNickListLen = 0;

    if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        return;
    }

    if(ui32OpListSize < ui32OpListLen+u->ui8NickLen+2) {
        char * pOldBuf = pOpList;
#ifdef _WIN32
        pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#else
		pOpList = (char *)realloc(pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#endif
        if(pOpList == NULL) {
            pOpList = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            u->Close();

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::Add2NickList for pOpList\n", (uint64_t)(ui32OpListSize+OPLISTSIZE+1));

            return;
        }
        ui32OpListSize += OPLISTSIZE;
    }

    memcpy(pOpList+ui32OpListLen-1, u->sNick, u->ui8NickLen);
    ui32OpListLen += (uint32_t)(u->ui8NickLen+2);

    pOpList[ui32OpListLen-3] = '$';
    pOpList[ui32OpListLen-2] = '$';
    pOpList[ui32OpListLen-1] = '|';
    pOpList[ui32OpListLen] = '\0';

    ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::AddBot2NickList(char * Nick, const size_t &szNickLen, const bool &isOp) {
    // $NickList nick$$nick2$$|

    if(ui32NickListSize < ui32NickListLen+szNickLen+2) {
        char * pOldBuf = pNickList;
#ifdef _WIN32
        pNickList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#else
		pNickList = (char *)realloc(pOldBuf, ui32NickListSize+NICKLISTSIZE+1);
#endif
        if(pNickList == NULL) {
            pNickList = pOldBuf;

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::AddBot2NickList for NickList\n", (uint64_t)(ui32NickListSize+NICKLISTSIZE+1));

            return;
		}
        ui32NickListSize += NICKLISTSIZE;
    }

    memcpy(pNickList+ui32NickListLen-1, Nick, szNickLen);
    ui32NickListLen += (uint32_t)(szNickLen+2);

    pNickList[ui32NickListLen-3] = '$';
    pNickList[ui32NickListLen-2] = '$';
    pNickList[ui32NickListLen-1] = '|';
    pNickList[ui32NickListLen] = '\0';

    ui32ZNickListLen = 0;

    if(isOp == false)
        return;

    if(ui32OpListSize < ui32OpListLen+szNickLen+2) {
        char * pOldBuf = pOpList;
#ifdef _WIN32
        pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#else
		pOpList = (char *)realloc(pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#endif
        if(pOpList == NULL) {
            pOpList = pOldBuf;

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::AddBot2NickList for pOpList\n", (uint64_t)(ui32OpListSize+OPLISTSIZE+1));

            return;
        }
        ui32OpListSize += OPLISTSIZE;
    }

    memcpy(pOpList+ui32OpListLen-1, Nick, szNickLen);
    ui32OpListLen += (uint32_t)(szNickLen+2);

    pOpList[ui32OpListLen-3] = '$';
    pOpList[ui32OpListLen-2] = '$';
    pOpList[ui32OpListLen-1] = '|';
    pOpList[ui32OpListLen] = '\0';

    ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::Add2OpList(User * u) {
    if(ui32OpListSize < ui32OpListLen+u->ui8NickLen+2) {
        char * pOldBuf = pOpList;
#ifdef _WIN32
        pOpList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#else
		pOpList = (char *)realloc(pOldBuf, ui32OpListSize+OPLISTSIZE+1);
#endif
        if(pOpList == NULL) {
            pOpList = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            u->Close();

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::Add2OpList for pOpList\n", (uint64_t)(ui32OpListSize+OPLISTSIZE+1));

            return;
        }
        ui32OpListSize += OPLISTSIZE;
    }

    memcpy(pOpList+ui32OpListLen-1, u->sNick, u->ui8NickLen);
    ui32OpListLen += (uint32_t)(u->ui8NickLen+2);

    pOpList[ui32OpListLen-3] = '$';
    pOpList[ui32OpListLen-2] = '$';
    pOpList[ui32OpListLen-1] = '|';
    pOpList[ui32OpListLen] = '\0';

    ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromNickList(char * Nick, const bool &isOp) {
    int m = sprintf(msg, "$%s$", Nick);
    if(CheckSprintf(m, 1024, "clsUsers::DelFromNickList") == false) {
        return;
    }

    pNickList[9] = '$';
    char *off = strstr(pNickList, msg);
    pNickList[9] = ' ';

    if(off != NULL) {
        memmove(off+1, off+(m+1), ui32NickListLen-((off+m)-pNickList));
        ui32NickListLen -= m;
        ui32ZNickListLen = 0;
    }

    if(!isOp) return;

    pOpList[7] = '$';
    off = strstr(pOpList, msg);
    pOpList[7] = ' ';

    if(off != NULL) {
        memmove(off+1, off+(m+1), ui32OpListLen-((off+m)-pOpList));
        ui32OpListLen -= m;
        ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::DelFromOpList(char * Nick) {
    int m = sprintf(msg, "$%s$", Nick);
    if(CheckSprintf(m, 1024, "clsUsers::DelFromOpList") == false) {
        return;
    }

    pOpList[7] = '$';
    char *off = strstr(pOpList, msg);
    pOpList[7] = ' ';

    if(off != NULL) {
        memmove(off+1, off+(m+1), ui32OpListLen-((off+m)-pOpList));
        ui32OpListLen -= m;
        ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

// PPK ... check global mainchat flood and add to global queue
void clsUsers::SendChat2All(User * cur, char * sData, const size_t &szChatLen, void * ptr) {
    clsUdpDebug::mPtr->Broadcast(sData, szChatLen);

    if(clsProfileManager::mPtr->IsAllowed(cur, clsProfileManager::NODEFLOODMAINCHAT) == false && 
        clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] != 0) {
        if(ui16ChatMsgs == 0) {
			ui64ChatMsgsTick = clsServerManager::ui64ActualTick;
			ui64ChatLockFromTick = clsServerManager::ui64ActualTick;
            ui16ChatMsgs = 0;
            bChatLocked = false;
		} else if((ui64ChatMsgsTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME]) < clsServerManager::ui64ActualTick) {
			ui64ChatMsgsTick = clsServerManager::ui64ActualTick;
            ui16ChatMsgs = 0;
        }

        ui16ChatMsgs++;

        if(ui16ChatMsgs > (uint16_t)clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES]) {
			ui64ChatLockFromTick = clsServerManager::ui64ActualTick;
            if(bChatLocked == false) {
                if(clsSettingManager::mPtr->bBools[SETBOOL_DEFLOOD_REPORT] == true) {
                    if(clsSettingManager::mPtr->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC],
                            clsLanguageManager::mPtr->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                        if(CheckSprintf(imsgLen, 1024, "clsUsers::SendChat2All1") == true) {
                            clsGlobalDataQueue::mPtr->SingleItemStore(msg, imsgLen, NULL, 0, clsGlobalDataQueue::SI_PM2OPS);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                        if(CheckSprintf(imsgLen, 1024, "clsUsers::SendChat2All2") == true) {
                            clsGlobalDataQueue::mPtr->AddQueueItem(msg, imsgLen, NULL, 0, clsGlobalDataQueue::CMD_OPS);
                        }
                    }
                }
                bChatLocked = true;
            }
        }

        if(bChatLocked == true) {
            if((ui64ChatLockFromTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT]) > clsServerManager::ui64ActualTick) {
                if(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 1) {
                    return;
                } else if(clsSettingManager::mPtr->i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 2) {
                    if(szChatLen > 64000) {
                        sData[64000] = '\0';
                    }

                	int iMsgLen = sprintf(clsServerManager::pGlobalBuffer, "%s ", cur->sIP);
                	if(CheckSprintf(iMsgLen, clsServerManager::szGlobalBufferSize, "clsUsers::SendChat2All3") == true) {
                        memcpy(clsServerManager::pGlobalBuffer+iMsgLen, sData, szChatLen);
                        iMsgLen += (uint32_t)szChatLen;
                        clsServerManager::pGlobalBuffer[iMsgLen] = '\0';
                        clsGlobalDataQueue::mPtr->AddQueueItem(clsServerManager::pGlobalBuffer, iMsgLen, NULL, 0, clsGlobalDataQueue::CMD_OPS);
                    }

                    return;
                }
            } else {
                bChatLocked = false;
            }
        }
    }

    if(ptr == NULL) {
        clsGlobalDataQueue::mPtr->AddQueueItem(sData, szChatLen, NULL, 0, clsGlobalDataQueue::CMD_CHAT);
    } else {
        clsGlobalDataQueue::mPtr->FillBlankQueueItem(sData, szChatLen, ptr);
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2MyInfos(User * u) {
    if(ui32MyInfosSize < ui32MyInfosLen+u->ui16MyInfoShortLen) {
        char * pOldBuf = pMyInfos;
#ifdef _WIN32
        pMyInfos = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#else
		pMyInfos = (char *)realloc(pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#endif
        if(pMyInfos == NULL) {
            pMyInfos = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            u->Close();

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::Add2MyInfos\n", (uint64_t)(ui32MyInfosSize+MYINFOLISTSIZE+1));

            return;
        }
        ui32MyInfosSize += MYINFOLISTSIZE;
    }

    memcpy(pMyInfos+ui32MyInfosLen, u->sMyInfoShort, u->ui16MyInfoShortLen);
    ui32MyInfosLen += u->ui16MyInfoShortLen;

    pMyInfos[ui32MyInfosLen] = '\0';

    ui32ZMyInfosLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromMyInfos(User * u) {
	char *match = strstr(pMyInfos, u->sMyInfoShort+8);
    if(match != NULL) {
		match -= 8;
		memmove(match, match+u->ui16MyInfoShortLen, ui32MyInfosLen-((match+(u->ui16MyInfoShortLen-1))-pMyInfos));
        ui32MyInfosLen -= u->ui16MyInfoShortLen;
        ui32ZMyInfosLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2MyInfosTag(User * u) {
    if(ui32MyInfosTagSize < ui32MyInfosTagLen+u->ui16MyInfoLongLen) {
        char * pOldBuf = pMyInfosTag;
#ifdef _WIN32
        pMyInfosTag = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#else
		pMyInfosTag = (char *)realloc(pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#endif
        if(pMyInfosTag == NULL) {
            pMyInfosTag = pOldBuf;
            u->ui32BoolBits |= User::BIT_ERROR;
            u->Close();

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::Add2MyInfosTag\n", (uint64_t)(ui32MyInfosTagSize+MYINFOLISTSIZE+1));

            return;
        }
        ui32MyInfosTagSize += MYINFOLISTSIZE;
    }

    memcpy(pMyInfosTag+ui32MyInfosTagLen, u->sMyInfoLong, u->ui16MyInfoLongLen);
    ui32MyInfosTagLen += u->ui16MyInfoLongLen;

    pMyInfosTag[ui32MyInfosTagLen] = '\0';

    ui32ZMyInfosTagLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromMyInfosTag(User * u) {
	char *match = strstr(pMyInfosTag, u->sMyInfoLong+8);
    if(match != NULL) {
		match -= 8;
        memmove(match, match+u->ui16MyInfoLongLen, ui32MyInfosTagLen-((match+(u->ui16MyInfoLongLen-1))-pMyInfosTag));
        ui32MyInfosTagLen -= u->ui16MyInfoLongLen;
        ui32ZMyInfosTagLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::AddBot2MyInfos(char * MyInfo) {
	size_t len = strlen(MyInfo);
	if(pMyInfosTag != NULL) {
	    if(strstr(pMyInfosTag, MyInfo) == NULL ) {
            if(ui32MyInfosTagSize < ui32MyInfosTagLen+len) {
                char * pOldBuf = pMyInfosTag;
#ifdef _WIN32
                pMyInfosTag = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#else
				pMyInfosTag = (char *)realloc(pOldBuf, ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#endif
                if(pMyInfosTag == NULL) {
                    pMyInfosTag = pOldBuf;

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for pMyInfosTag in clsUsers::AddBot2MyInfos\n", (uint64_t)(ui32MyInfosTagSize+MYINFOLISTSIZE+1));

                    return;
                }
                ui32MyInfosTagSize += MYINFOLISTSIZE;
            }
        	memcpy(pMyInfosTag+ui32MyInfosTagLen, MyInfo, len);
            ui32MyInfosTagLen += (uint32_t)len;
            pMyInfosTag[ui32MyInfosTagLen] = '\0';
            ui32ZMyInfosLen = 0;
        }
    }
    if(pMyInfos != NULL) {
    	if(strstr(pMyInfos, MyInfo) == NULL ) {
            if(ui32MyInfosSize < ui32MyInfosLen+len) {
                char * pOldBuf = pMyInfos;
#ifdef _WIN32
                pMyInfos = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#else
				pMyInfos = (char *)realloc(pOldBuf, ui32MyInfosSize+MYINFOLISTSIZE+1);
#endif
                if(pMyInfos == NULL) {
                    pMyInfos = pOldBuf;

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes for pMyInfos in clsUsers::AddBot2MyInfos\n", (uint64_t)(ui32MyInfosSize+MYINFOLISTSIZE+1));

                    return;
                }
                ui32MyInfosSize += MYINFOLISTSIZE;
            }
        	memcpy(pMyInfos+ui32MyInfosLen, MyInfo, len);
            ui32MyInfosLen += (uint32_t)len;
            pMyInfos[ui32MyInfosLen] = '\0';
            ui32ZMyInfosTagLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void clsUsers::DelBotFromMyInfos(char * MyInfo) {
	size_t len = strlen(MyInfo);
	if(pMyInfosTag) {
		char *match = strstr(pMyInfosTag,  MyInfo);
	    if(match) {
    		memmove(match, match+len, ui32MyInfosTagLen-((match+(len-1))-pMyInfosTag));
        	ui32MyInfosTagLen -= (uint32_t)len;
        	ui32ZMyInfosTagLen = 0;
         }
    }
	if(pMyInfos) {
		char *match = strstr(pMyInfos,  MyInfo);
	    if(match) {
    		memmove(match, match+len, ui32MyInfosLen-((match+(len-1))-pMyInfos));
        	ui32MyInfosLen -= (uint32_t)len;
        	ui32ZMyInfosLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2UserIP(User * cur) {
    int m = sprintf(msg,"$%s %s$", cur->sNick, cur->sIP);
    if(CheckSprintf(m, 1024, "clsUsers::Add2UserIP") == false) {
        return;
    }

    if(ui32UserIPListSize < ui32UserIPListLen+m) {
        char * pOldBuf = pUserIPList;
#ifdef _WIN32
        pUserIPList = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32UserIPListSize+IPLISTSIZE+1);
#else
		pUserIPList = (char *)realloc(pOldBuf, ui32UserIPListSize+IPLISTSIZE+1);
#endif
        if(pUserIPList == NULL) {
            pUserIPList = pOldBuf;
            cur->ui32BoolBits |= User::BIT_ERROR;
            cur->Close();

			AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsUsers::Add2UserIP\n", (uint64_t)(ui32UserIPListSize+IPLISTSIZE+1));

            return;
        }
        ui32UserIPListSize += IPLISTSIZE;
    }

    memcpy(pUserIPList+ui32UserIPListLen-1, msg+1, m-1);
    ui32UserIPListLen += m;

    pUserIPList[ui32UserIPListLen-2] = '$';
    pUserIPList[ui32UserIPListLen-1] = '|';
    pUserIPList[ui32UserIPListLen] = '\0';

    ui32ZUserIPListLen = 0;
}
//---------------------------------------------------------------------------

void clsUsers::DelFromUserIP(User * cur) {
    int m = sprintf(msg,"$%s %s$", cur->sNick, cur->sIP);
    if(CheckSprintf(m, 1024, "clsUsers::DelFromUserIP") == false) {
        return;
    }

    pUserIPList[7] = '$';
    char *off = strstr(pUserIPList, msg);
    pUserIPList[7] = ' ';
    
	if(off != NULL) {
        memmove(off+1, off+(m+1), ui32UserIPListLen-((off+m)-pUserIPList));
        ui32UserIPListLen -= m;
        ui32ZUserIPListLen = 0;
    }
}
//---------------------------------------------------------------------------

void clsUsers::Add2RecTimes(User * curUser) {
    time_t acc_time;
    time(&acc_time);

    if(clsProfileManager::mPtr->IsAllowed(curUser, clsProfileManager::NOUSRSAMEIP) == true ||
        (acc_time-curUser->tLoginTime) >= clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_RECONN_TIME]) {
        return;
    }

    RecTime * pNewRecTime = new (std::nothrow) RecTime(curUser->ui128IpHash);

	if(pNewRecTime == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewRecTime in clsUsers::Add2RecTimes\n", 0);
        return;
	}

#ifdef _WIN32
    pNewRecTime->sNick = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, curUser->ui8NickLen+1);
#else
	pNewRecTime->sNick = (char *)malloc(curUser->ui8NickLen+1);
#endif
	if(pNewRecTime->sNick == NULL) {
        delete pNewRecTime;

        AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in clsUsers::Add2RecTimes\n", (uint64_t)(curUser->ui8NickLen+1));

        return;
    }

    memcpy(pNewRecTime->sNick, curUser->sNick, curUser->ui8NickLen);
    pNewRecTime->sNick[curUser->ui8NickLen] = '\0';

	pNewRecTime->ui64DisConnTick = clsServerManager::ui64ActualTick-(acc_time-curUser->tLoginTime);
    pNewRecTime->ui32NickHash = curUser->ui32NickHash;

    pNewRecTime->pNext = pRecTimeList;

	if(pRecTimeList != NULL) {
		pRecTimeList->pPrev = pNewRecTime;
	}

	pRecTimeList = pNewRecTime;
}
//---------------------------------------------------------------------------

bool clsUsers::CheckRecTime(User * curUser) {
    RecTime * cur = NULL,
        * next = pRecTimeList;

    while(next != NULL) {
        cur = next;
        next = cur->pNext;

        // check expires...
        if(cur->ui64DisConnTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_RECONN_TIME] <= clsServerManager::ui64ActualTick) {
#ifdef _WIN32
            if(cur->sNick != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sNick) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate cur->sNick in clsUsers::CheckRecTime\n", 0);
                }
            }
#else
			free(cur->sNick);
#endif

            if(cur->pPrev == NULL) {
                if(cur->pNext == NULL) {
                    pRecTimeList = NULL;
                } else {
                    cur->pNext->pPrev = NULL;
                    pRecTimeList = cur->pNext;
                }
            } else if(cur->pNext == NULL) {
                cur->pPrev->pNext = NULL;
            } else {
                cur->pPrev->pNext = cur->pNext;
                cur->pNext->pPrev = cur->pPrev;
            }

            delete cur;
            continue;
        }

        if(cur->ui32NickHash == curUser->ui32NickHash && memcmp(cur->ui128IpHash, curUser->ui128IpHash, 16) == 0 && strcasecmp(cur->sNick, curUser->sNick) == 0) {
            int imsgLen = sprintf(msg, "<%s> %s %" PRIu64 " %s.|", clsSettingManager::mPtr->sPreTexts[clsSettingManager::SETPRETXT_HUB_SEC], clsLanguageManager::mPtr->sTexts[LAN_PLEASE_WAIT],
                (cur->ui64DisConnTick+clsSettingManager::mPtr->i16Shorts[SETSHORT_MIN_RECONN_TIME])-clsServerManager::ui64ActualTick, clsLanguageManager::mPtr->sTexts[LAN_SECONDS_BEFORE_RECONN]);
            if(CheckSprintf(imsgLen, 1024, "clsUsers::CheckRecTime1") == true) {
                curUser->SendChar(msg, imsgLen);
            }
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

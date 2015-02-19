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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GlobalDataQueue.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "colUsers.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
clsGlobalDataQueue * clsGlobalDataQueue::mPtr = NULL;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsGlobalDataQueue::clsGlobalDataQueue() : pCreatedGlobalQueues(NULL), pQueueItems(NULL), pSingleItems(NULL), bHaveItems(false) {
    msg[0] = '\0';

    // OpList buffer
#ifdef _WIN32
    OpListQueue.pBuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
#else
	OpListQueue.pBuffer = (char *)calloc(256, 1);
#endif
	if(OpListQueue.pBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create OpListQueue\n", 0);
		exit(EXIT_FAILURE);
	}
	OpListQueue.szLen = 0;
    OpListQueue.szSize = 255;
    
    // UserIP buffer
#ifdef _WIN32
    UserIPQueue.pBuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
#else
	UserIPQueue.pBuffer = (char *)calloc(256, 1);
#endif
	if(UserIPQueue.pBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create UserIPQueue\n", 0);
		exit(EXIT_FAILURE);
	}
    UserIPQueue.szLen = 0;
    UserIPQueue.szSize = 255;
	UserIPQueue.bHaveDollars = false;

    pNewQueueItems[0] = NULL;
    pNewQueueItems[1] = NULL;
    
    pNewSingleItems[0] = NULL;
    pNewSingleItems[1] = NULL;

    for(uint8_t ui8i = 0; ui8i < 144; ui8i++) {
        GlobalQueues[ui8i].szLen = 0;
        GlobalQueues[ui8i].szSize = 255;
        GlobalQueues[ui8i].szZlen = 0;
        GlobalQueues[ui8i].szZsize = 255;

#ifdef _WIN32
        GlobalQueues[ui8i].pBuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
        GlobalQueues[ui8i].pZbuffer = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
#else
        GlobalQueues[ui8i].pBuffer = (char *)calloc(256, 1);
        GlobalQueues[ui8i].pZbuffer = (char *)calloc(256, 1);
#endif
        if(GlobalQueues[ui8i].pBuffer == NULL || GlobalQueues[ui8i].pZbuffer == NULL) {
            AppendDebugLog("%s - [MEM] Cannot create GlobalQueues[ui8i]\n", 0);
            exit(EXIT_FAILURE);
        }

        GlobalQueues[ui8i].pNext = NULL;
        GlobalQueues[ui8i].bCreated = false;
        GlobalQueues[ui8i].bZlined = false;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

clsGlobalDataQueue::~clsGlobalDataQueue() {
#ifdef _WIN32
    if(OpListQueue.pBuffer != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)OpListQueue.pBuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate OpListQueue.pBuffer in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
        }
    }
#else
	free(OpListQueue.pBuffer);
#endif

#ifdef _WIN32
    if(UserIPQueue.pBuffer != NULL) {
        if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)UserIPQueue.pBuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate UserIPQueue.pBuffer in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
        }
    }
#else
	free(UserIPQueue.pBuffer);
#endif

    if(pNewSingleItems[0] != NULL) {
        SingleDataItem * pCur = NULL,
            * pNext = pNewSingleItems[0];

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

#ifdef _WIN32
            if(pCur->pData != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pData) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pData in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
                }
            }
#else
			free(pCur->pData);
#endif
            delete pCur;
		}
    }

    if(pSingleItems != NULL) {
        SingleDataItem * pCur = NULL,
            * pNext = pSingleItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

#ifdef _WIN32
            if(pCur->pData != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pData) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pData in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
                }
            }
#else
			free(pCur->pData);
#endif
            delete pCur;
		}
    }

    if(pNewQueueItems[0] != NULL) {
        QueueItem * pCur = NULL,
            * pNext = pNewQueueItems[0];

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

#ifdef _WIN32
            if(pCur->pCommand1 != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pCommand1) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pCommand1 in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
                }
            }
            if(pCur->pCommand2 != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pCommand2) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pCommand2 in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
                }
            }
#else
			free(pCur->pCommand1);
			free(pCur->pCommand2);
#endif
            delete pCur;
		}
    }

    if(pQueueItems != NULL) {
        QueueItem * pCur = NULL,
            * pNext = pQueueItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

#ifdef _WIN32
            if(pCur->pCommand1 != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pCommand1) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pCommand1 in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
                }
            }
            if(pCur->pCommand2 != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pCommand2) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pCommand2 in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
                }
            }
#else
			free(pCur->pCommand1);
			free(pCur->pCommand2);
#endif
            delete pCur;
		}
    }

    for(uint8_t ui8i = 0; ui8i < 144; ui8i++) {
#ifdef _WIN32
        if(GlobalQueues[ui8i].pBuffer != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)GlobalQueues[ui8i].pBuffer) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate GlobalQueues[ui8i].pBuffer in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
            }
        }

        if(GlobalQueues[ui8i].pZbuffer != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)GlobalQueues[ui8i].pZbuffer) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate GlobalQueues[ui8i].pZbuffer in clsGlobalDataQueue::~clsGlobalDataQueue\n", 0);
            }
        }
#else
		free(GlobalQueues[ui8i].pBuffer);
		free(GlobalQueues[ui8i].pZbuffer);
#endif
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::AddQueueItem(char * sCommand1, const size_t &szLen1, char * sCommand2, const size_t &szLen2, const uint8_t &ui8CmdType) {
    QueueItem * pNewItem = new (std::nothrow) QueueItem();
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem in clsGlobalDataQueue::AddQueueItem\n", 0);
    	return;
    }

#ifdef _WIN32
    pNewItem->pCommand1 = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen1+1);
#else
	pNewItem->pCommand1 = (char *)malloc(szLen1+1);
#endif
    if(pNewItem->pCommand1 == NULL) {
        delete pNewItem;

		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for pNewItem->pCommand1 in clsGlobalDataQueue::AddQueueItem\n", (uint64_t)(szLen1+1));

        return;
    }

    memcpy(pNewItem->pCommand1, sCommand1, szLen1);
    pNewItem->pCommand1[szLen1] = '\0';

	pNewItem->szLen1 = szLen1;

    if(sCommand2 != NULL) {
#ifdef _WIN32
        pNewItem->pCommand2 = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen2+1);
#else
        pNewItem->pCommand2 = (char *)malloc(szLen2+1);
#endif
        if(pNewItem->pCommand2 == NULL) {
#ifdef _WIN32
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pNewItem->pCommand1) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate pNewItem->pCommand1 in clsGlobalDataQueue::AddQueueItem\n", 0);
            }
#else
            free(pNewItem->pCommand1);
#endif
            delete pNewItem;

            AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for pNewItem->pCommand2 in clsGlobalDataQueue::AddQueueItem\n", (uint64_t)(szLen2+1));

            return;
        }

        memcpy(pNewItem->pCommand2, sCommand2, szLen2);
        pNewItem->pCommand2[szLen2] = '\0';

        pNewItem->szLen2 = szLen2;
    } else {
        pNewItem->pCommand2 = NULL;
        pNewItem->szLen2 = 0;
    }

	pNewItem->ui8CommandType = ui8CmdType;

	pNewItem->pNext = NULL;

    if(pNewQueueItems[0] == NULL) {
        pNewQueueItems[0] = pNewItem;
        pNewQueueItems[1] = pNewItem;
    } else {
        pNewQueueItems[1]->pNext = pNewItem;
        pNewQueueItems[1] = pNewItem;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// appends data to the OpListQueue
void clsGlobalDataQueue::OpListStore(char * sNick) {
    if(OpListQueue.szLen == 0) {
        OpListQueue.szLen = sprintf(OpListQueue.pBuffer, "$OpList %s$$|", sNick);
    } else {
        int iDataLen = sprintf(msg, "%s$$|", sNick);
        if(CheckSprintf(iDataLen, 128, "clsGlobalDataQueue::OpListStore2") == true) {
            if(OpListQueue.szSize < OpListQueue.szLen+iDataLen) {
                size_t szAllignLen = Allign256(OpListQueue.szLen+iDataLen);
                char * pOldBuf = OpListQueue.pBuffer;
#ifdef _WIN32
                OpListQueue.pBuffer = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
#else
				OpListQueue.pBuffer = (char *)realloc(pOldBuf, szAllignLen);
#endif
                if(OpListQueue.pBuffer == NULL) {
                    OpListQueue.pBuffer = pOldBuf;

                    AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::OpListStore\n", (uint64_t)szAllignLen);

                    return;
                }
                OpListQueue.szSize = (uint32_t)(szAllignLen-1);
            }
            memcpy(OpListQueue.pBuffer+OpListQueue.szLen-1, msg, iDataLen);
            OpListQueue.szLen += iDataLen-1;
            OpListQueue.pBuffer[OpListQueue.szLen] = '\0';
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// appends data to the UserIPQueue
void clsGlobalDataQueue::UserIPStore(User * pUser) {
    if(UserIPQueue.szLen == 0) {
        UserIPQueue.szLen = sprintf(UserIPQueue.pBuffer, "$UserIP %s %s|", pUser->sNick, pUser->sIP);
        UserIPQueue.bHaveDollars = false;
    } else {
        int iDataLen = sprintf(msg, "%s %s$$|", pUser->sNick, pUser->sIP);
        if(CheckSprintf(iDataLen, 128, "clsGlobalDataQueue::UserIPStore") == true) {
            if(UserIPQueue.bHaveDollars == false) {
                UserIPQueue.pBuffer[UserIPQueue.szLen-1] = '$';
                UserIPQueue.pBuffer[UserIPQueue.szLen] = '$';
                UserIPQueue.bHaveDollars = true;
                UserIPQueue.szLen += 2;
            }
            if(UserIPQueue.szSize < UserIPQueue.szLen+iDataLen) {
                size_t szAllignLen = Allign256(UserIPQueue.szLen+iDataLen);
                char * pOldBuf = UserIPQueue.pBuffer;
#ifdef _WIN32
                UserIPQueue.pBuffer = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
#else
				UserIPQueue.pBuffer = (char *)realloc(pOldBuf, szAllignLen);
#endif
                if(UserIPQueue.pBuffer == NULL) {
                    UserIPQueue.pBuffer = pOldBuf;

					AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::UserIPStore\n", (uint64_t)szAllignLen);

                    return;
                }
                UserIPQueue.szSize = (uint32_t)(szAllignLen-1);
            }
            memcpy(UserIPQueue.pBuffer+UserIPQueue.szLen-1, msg, iDataLen);
            UserIPQueue.szLen += iDataLen-1;
            UserIPQueue.pBuffer[UserIPQueue.szLen] = '\0';
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::PrepareQueueItems() {
    pQueueItems = pNewQueueItems[0];

    pNewQueueItems[0] = NULL;
    pNewQueueItems[1] = NULL;

    if(pQueueItems != NULL || OpListQueue.szLen != 0 || UserIPQueue.szLen != 0) {
        bHaveItems = true;
    }

    pSingleItems = pNewSingleItems[0];

    pNewSingleItems[0] = NULL;
    pNewSingleItems[1] = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::ClearQueues() {
    bHaveItems = false;

    if(pCreatedGlobalQueues != NULL) {
        GlobalQueue * pCur = NULL,
            * pNext = pCreatedGlobalQueues;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

            pCur->szLen = 0;
            pCur->szZlen = 0;
            pCur->pNext = NULL;
            pCur->bCreated = false;
            pCur->bZlined = false;
        }
    }

    pCreatedGlobalQueues = NULL;

    OpListQueue.pBuffer[0] = '\0';
    OpListQueue.szLen = 0;

    UserIPQueue.pBuffer[0] = '\0';
    UserIPQueue.szLen = 0;

    if(pQueueItems != NULL) {
        QueueItem * pCur = NULL,
            * pNext = pQueueItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

#ifdef _WIN32
            if(pCur->pCommand1 != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pCommand1) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pCommand1 in clsGlobalDataQueue::ClearQueues\n", 0);
                }
            }
            if(pCur->pCommand2 != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pCommand2) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pCommand2 in clsGlobalDataQueue::ClearQueues\n", 0);
                }
            }
#else
			free(pCur->pCommand1);
			free(pCur->pCommand2);
#endif
            delete pCur;
		}
    }

    pQueueItems = NULL;

    if(pSingleItems != NULL) {
        SingleDataItem * pCur = NULL,
            * pNext = pSingleItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

#ifdef _WIN32
            if(pCur->pData != NULL) {
                if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->pData) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->pData in clsGlobalDataQueue::ClearQueues\n", 0);
                }
            }
#else
			free(pCur->pData);
#endif
            delete pCur;
		}
    }

    pSingleItems = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::ProcessQueues(User * pUser) {
    uint32_t ui32QueueType = 0; // short myinfos
    uint16_t ui16QueueBits = 0;

    if(clsSettingManager::mPtr->ui8FullMyINFOOption == 1 && clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::SENDFULLMYINFOS)) {
        ui32QueueType = 1; // full myinfos
        ui16QueueBits |= BIT_LONG_MYINFO;
    }

    if(pUser->ui64SharedSize != 0) {
        if(((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) == true) {
            if(((pUser->ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) == true) {
                if(((pUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) == true) {
                    if(((pUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) == true) {
                        ui32QueueType += 6; // all searches both ipv4 and ipv6
                        ui16QueueBits |= BIT_ALL_SEARCHES_IPV64;
                    } else {
                        ui32QueueType += 14; // all searches ipv6 + active searches ipv4
                        ui16QueueBits |= BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4;
                    }
                } else {
                    if(((pUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) == true) {
                        ui32QueueType += 16; // active searches ipv6 + all searches ipv4
                        ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4;
                    } else {
                        ui32QueueType += 12; // active searches both ipv4 and ipv6
                        ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV64;
                    }
                }
            } else {
                if(((pUser->ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) == true) {
                    ui32QueueType += 4; // all searches ipv6 only
                    ui16QueueBits |= BIT_ALL_SEARCHES_IPV6;
                } else {
                    ui32QueueType += 10; // active searches ipv6 only
                    ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV6;
                }
            }
        } else {
            if(((pUser->ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) == true) {
                ui32QueueType += 2; // all searches ipv4 only
                ui16QueueBits |= BIT_ALL_SEARCHES_IPV4;
            } else {
                ui32QueueType += 8; // active searches ipv4 only
                ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV4;
            }
        }
    }

    if(((pUser->ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
        ui32QueueType += 14; // send hellos
        ui16QueueBits |= BIT_HELLO;
    }

    if(((pUser->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
        ui32QueueType += 28; // send operator data
        ui16QueueBits |= BIT_OPERATOR;
    }

    if(pUser->i32Profile != -1 && ((pUser->ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == true &&
        clsProfileManager::mPtr->IsAllowed(pUser, clsProfileManager::SENDALLUSERIP) == true) {
        ui32QueueType += 56; // send userips
        ui16QueueBits |= BIT_USERIP;
    }

    if(GlobalQueues[ui32QueueType].bCreated == false) {
        if(pQueueItems != NULL) {
            QueueItem * pCur = NULL,
                * pNext = pQueueItems;

            while(pNext != NULL) {
                pCur = pNext;
                pNext = pCur->pNext;

                switch(pCur->ui8CommandType) {
                    case CMD_HELLO:
                        if((ui16QueueBits & BIT_HELLO) == BIT_HELLO) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_MYINFO:
                        if((ui16QueueBits & BIT_LONG_MYINFO) == BIT_LONG_MYINFO) {
                            if(pCur->pCommand2 != NULL) {
                                AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand2, pCur->szLen2);
                            }
                            break;
                        }

                        if(pCur->pCommand1 != NULL) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }

                        break;
                    case CMD_OPS:
                        if((ui16QueueBits & BIT_OPERATOR) == BIT_OPERATOR) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_ACTIVE_SEARCH_V6:
                        if(((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) == false &&
                            ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV4) == BIT_ACTIVE_SEARCHES_IPV4) == false) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_ACTIVE_SEARCH_V64:
                        if(((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) == false &&
                            ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV4) == BIT_ACTIVE_SEARCHES_IPV4) == false) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        } else if(((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) == false &&
                            ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6) == BIT_ACTIVE_SEARCHES_IPV6) == false) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand2, pCur->szLen2);
                        }
                        break;
                    case CMD_ACTIVE_SEARCH_V4:
                        if(((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) == false &&
                            ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6) == BIT_ACTIVE_SEARCHES_IPV6) == false) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V6:
                        if((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 ||
                            (ui16QueueBits & BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) == BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V64:
                        if((ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6 ||
                            (ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4 || (ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) == BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4 ||
                            (ui16QueueBits & BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) == BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V4:
                        if((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 ||
                            (ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) == BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V4_ONLY:
                        if((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V6_ONLY:
                        if((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_CHAT:
                        if(pCur->pCommand1 != NULL) {
                            AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        }
                        break;
                    case CMD_HUBNAME:
                    case CMD_QUIT:
                    case CMD_LUA:
                        AddDataToQueue(GlobalQueues[ui32QueueType], pCur->pCommand1, pCur->szLen1);
                        break;
                    default:
                        break; // should not happen ;)
                }
            }
        }

        if(OpListQueue.szLen != 0) {
            AddDataToQueue(GlobalQueues[ui32QueueType], OpListQueue.pBuffer, OpListQueue.szLen);
        }

        if(UserIPQueue.szLen != 0 && (ui16QueueBits & BIT_USERIP) == BIT_USERIP) {
            AddDataToQueue(GlobalQueues[ui32QueueType], UserIPQueue.pBuffer, UserIPQueue.szLen);
        }

        GlobalQueues[ui32QueueType].bCreated = true;
        GlobalQueues[ui32QueueType].pNext = pCreatedGlobalQueues;
        pCreatedGlobalQueues = &GlobalQueues[ui32QueueType];
    }

    if(GlobalQueues[ui32QueueType].szLen == 0) {
        if(clsServerManager::ui8SrCntr == 0) {
            if(pUser->pCmdActive6Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->pCmdActive6Search);
                pUser->pCmdActive6Search = NULL;
            }

            if(pUser->pCmdActive4Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->pCmdActive4Search);
                pUser->pCmdActive4Search = NULL;
            }
        }

        return;
    }

    if(clsServerManager::ui8SrCntr == 0) {
		PrcsdUsrCmd * cmdActiveSearch = NULL;

		if((pUser->ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
			cmdActiveSearch = pUser->pCmdActive6Search;
			pUser->pCmdActive6Search = NULL;

			if(pUser->pCmdActive4Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->pCmdActive4Search);
				pUser->pCmdActive4Search = NULL;
			}
		} else {
			cmdActiveSearch = pUser->pCmdActive4Search;
			pUser->pCmdActive4Search = NULL;
		}

		if(cmdActiveSearch != NULL) {
			if(pUser->ui64SharedSize == 0) {
				User::DeletePrcsdUsrCmd(cmdActiveSearch);
			} else {
				if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
					if(GlobalQueues[ui32QueueType].bZlined == false) {
						GlobalQueues[ui32QueueType].bZlined = true;
						GlobalQueues[ui32QueueType].pZbuffer = clsZlibUtility::mPtr->CreateZPipe(GlobalQueues[ui32QueueType].pBuffer, GlobalQueues[ui32QueueType].szLen,
							GlobalQueues[ui32QueueType].pZbuffer, GlobalQueues[ui32QueueType].szZlen, GlobalQueues[ui32QueueType].szZsize);
					}

					if(GlobalQueues[ui32QueueType].szZlen != 0 && (GlobalQueues[ui32QueueType].szZlen <= (GlobalQueues[ui32QueueType].szLen-cmdActiveSearch->ui32Len))) {
						pUser->PutInSendBuf(GlobalQueues[ui32QueueType].pZbuffer, GlobalQueues[ui32QueueType].szZlen);
						clsServerManager::ui64BytesSentSaved += (GlobalQueues[ui32QueueType].szLen - GlobalQueues[ui32QueueType].szZlen);

						User::DeletePrcsdUsrCmd(cmdActiveSearch);

						return;
					}
				}

				uint32_t ui32SbLen = pUser->ui32SendBufDataLen;
				pUser->PutInSendBuf(GlobalQueues[ui32QueueType].pBuffer, GlobalQueues[ui32QueueType].szLen);

				// PPK ... check if adding of searchs not cause buffer overflow !
				if(pUser->ui32SendBufDataLen <= ui32SbLen) {
					ui32SbLen = 0;
				}

				pUser->RemFromSendBuf(cmdActiveSearch->sCommand, cmdActiveSearch->ui32Len, ui32SbLen);

				User::DeletePrcsdUsrCmd(cmdActiveSearch);

				return;
			}
		}
	}

    if(((pUser->ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
        if(GlobalQueues[ui32QueueType].bZlined == false) {
            GlobalQueues[ui32QueueType].bZlined = true;
            GlobalQueues[ui32QueueType].pZbuffer = clsZlibUtility::mPtr->CreateZPipe(GlobalQueues[ui32QueueType].pBuffer, GlobalQueues[ui32QueueType].szLen, GlobalQueues[ui32QueueType].pZbuffer,
                GlobalQueues[ui32QueueType].szZlen, GlobalQueues[ui32QueueType].szZsize);
        }

        if(GlobalQueues[ui32QueueType].szZlen != 0) {
            pUser->PutInSendBuf(GlobalQueues[ui32QueueType].pZbuffer, GlobalQueues[ui32QueueType].szZlen);
            clsServerManager::ui64BytesSentSaved += (GlobalQueues[ui32QueueType].szLen - GlobalQueues[ui32QueueType].szZlen);

            return;
        }
    } 

    pUser->PutInSendBuf(GlobalQueues[ui32QueueType].pBuffer, GlobalQueues[ui32QueueType].szLen);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::ProcessSingleItems(User * u) const {
    size_t szLen = 0, szWanted = 0;
    int iret = 0;

    SingleDataItem * pCur = NULL,
        * pNext = pSingleItems;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->pNext;

        if(pCur->pFromUser != u) {
            switch(pCur->ui8Type) {
                case SI_PM2ALL: { // send PM to ALL
                    szWanted = szLen+pCur->szDataLen+u->ui8NickLen+13;
                    if(clsServerManager::szGlobalBufferSize < szWanted) {
                        if(CheckAndResizeGlobalBuffer(szWanted) == false) {
							AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::ProcessSingleItems\n", (uint64_t)Allign128K(szWanted));
                            break;
                        }
                    }
                    iret = sprintf(clsServerManager::pGlobalBuffer+szLen, "$To: %s From: ", u->sNick);
                    szLen += iret;
                    CheckSprintf1(iret, szLen, clsServerManager::szGlobalBufferSize, "clsGlobalDataQueue::ProcessSingleItems1");

                    memcpy(clsServerManager::pGlobalBuffer+szLen, pCur->pData, pCur->szDataLen);
                    szLen += pCur->szDataLen;
                    clsServerManager::pGlobalBuffer[szLen] = '\0';

                    break;
                }
                case SI_PM2OPS: { // send PM only to operators
                    if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        szWanted = szLen+pCur->szDataLen+u->ui8NickLen+13;
                        if(clsServerManager::szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::ProcessSingleItems1\n", (uint64_t)Allign128K(szWanted));
								break;
                            }
                        }
                        iret = sprintf(clsServerManager::pGlobalBuffer+szLen, "$To: %s From: ", u->sNick);
                        szLen += iret;
                        CheckSprintf1(iret, szLen, clsServerManager::szGlobalBufferSize, "clsGlobalDataQueue::ProcessSingleItems2");

                        memcpy(clsServerManager::pGlobalBuffer+szLen, pCur->pData, pCur->szDataLen);
                        szLen += pCur->szDataLen;
                        clsServerManager::pGlobalBuffer[szLen] = '\0';
                    }
                    break;
                }
                case SI_OPCHAT: { // send OpChat only to allowed users...
                    if(clsProfileManager::mPtr->IsAllowed(u, clsProfileManager::ALLOWEDOPCHAT) == true) {
                        szWanted = szLen+pCur->szDataLen+u->ui8NickLen+13;
                        if(clsServerManager::szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::ProcessSingleItems2\n", (uint64_t)Allign128K(szWanted));
                                break;
                            }
                        }
                        iret = sprintf(clsServerManager::pGlobalBuffer+szLen, "$To: %s From: ", u->sNick);
                        szLen += iret;
                        CheckSprintf1(iret, szLen, clsServerManager::szGlobalBufferSize, "clsGlobalDataQueue::ProcessSingleItems3");

                        memcpy(clsServerManager::pGlobalBuffer+szLen, pCur->pData, pCur->szDataLen);
                        szLen += pCur->szDataLen;
                        clsServerManager::pGlobalBuffer[szLen] = '\0';
                    }
                    break;
                }
                case SI_TOPROFILE: { // send data only to given profile...
                    if(u->i32Profile == pCur->i32Profile) {
                        szWanted = szLen+pCur->szDataLen;
                        if(clsServerManager::szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::ProcessSingleItems3\n", (uint64_t)Allign128K(szWanted));
                                break;
                            }
                        }
                        memcpy(clsServerManager::pGlobalBuffer+szLen, pCur->pData, pCur->szDataLen);
                        szLen += pCur->szDataLen;
                        clsServerManager::pGlobalBuffer[szLen] = '\0';
                    }
                    break;
                }
                case SI_PM2PROFILE: { // send pm only to given profile...
                    if(u->i32Profile == pCur->i32Profile) {
                        szWanted = szLen+pCur->szDataLen+u->ui8NickLen+13;
                        if(clsServerManager::szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::ProcessSingleItems4\n", (uint64_t)Allign128K(szWanted));
                                break;
                            }
                        }
                        iret = sprintf(clsServerManager::pGlobalBuffer+szLen, "$To: %s From: ", u->sNick);
                        szLen += iret;
                        CheckSprintf1(iret, szLen, clsServerManager::szGlobalBufferSize, "clsGlobalDataQueue::ProcessSingleItems4");
                        memcpy(clsServerManager::pGlobalBuffer+szLen, pCur->pData, pCur->szDataLen);
                        szLen += pCur->szDataLen;
                        clsServerManager::pGlobalBuffer[szLen] = '\0';
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if(szLen != 0) {
        u->SendCharDelayed(clsServerManager::pGlobalBuffer, szLen);
    }

    ReduceGlobalBuffer();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::SingleItemStore(char * sData, const size_t &szDataLen, User * pFromUser, const int32_t &i32Profile, const uint8_t &ui8Type) {
    SingleDataItem * pNewItem = new (std::nothrow) SingleDataItem();
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem in clsGlobalDataQueue::SingleItemStore\n", 0);
    	return;
    }

    if(sData != NULL) {
#ifdef _WIN32
        pNewItem->pData = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szDataLen+1);
#else
		pNewItem->pData = (char *)malloc(szDataLen+1);
#endif
        if(pNewItem->pData == NULL) {
            delete pNewItem;

			AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes in clsGlobalDataQueue::SingleItemStore\n", (uint64_t)(szDataLen+1));

            return;
        }

        memcpy(pNewItem->pData, sData, szDataLen);
        pNewItem->pData[szDataLen] = '\0';
    } else {
        pNewItem->pData = NULL;
    }

	pNewItem->szDataLen = szDataLen;

	pNewItem->pFromUser = pFromUser;

	pNewItem->ui8Type = ui8Type;

	pNewItem->pPrev = NULL;
	pNewItem->pNext = NULL;

    pNewItem->i32Profile = i32Profile;

    if(pNewSingleItems[0] == NULL) {
        pNewSingleItems[0] = pNewItem;
        pNewSingleItems[1] = pNewItem;
    } else {
        pNewItem->pPrev = pNewSingleItems[1];
        pNewSingleItems[1]->pNext = pNewItem;
        pNewSingleItems[1] = pNewItem;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::SendFinalQueue() {
    if(pQueueItems != NULL) {
        QueueItem * pCur = NULL,
            * pNext = pQueueItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

            switch(pCur->ui8CommandType) {
                case CMD_OPS:
                case CMD_CHAT:
                case CMD_LUA:
                    AddDataToQueue(GlobalQueues[0], pCur->pCommand1, pCur->szLen1);
                    break;
                default:
                    break;
            }
        }
    }

    if(pNewQueueItems[0] != NULL) {
        QueueItem * pCur = NULL,
            * pNext = pNewQueueItems[0];

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->pNext;

            switch(pCur->ui8CommandType) {
                case CMD_OPS:
                case CMD_CHAT:
                case CMD_LUA:
                    AddDataToQueue(GlobalQueues[0], pCur->pCommand1, pCur->szLen1);
                    break;
                default:
                    break;
            }
        }
    }

    if(GlobalQueues[0].szLen == 0) {
        return;
    }

    GlobalQueues[0].pZbuffer = clsZlibUtility::mPtr->CreateZPipe(GlobalQueues[0].pBuffer, GlobalQueues[0].szLen, GlobalQueues[0].pZbuffer, GlobalQueues[0].szZlen, GlobalQueues[0].szZsize);

	User * pCur = NULL,
        * pNext = clsUsers::mPtr->pListS;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->pNext;

        if(GlobalQueues[0].szZlen != 0) {
            pCur->PutInSendBuf(GlobalQueues[0].pZbuffer, GlobalQueues[0].szZlen);
            clsServerManager::ui64BytesSentSaved += (GlobalQueues[0].szLen - GlobalQueues[0].szZlen);
        } else {
            pCur->PutInSendBuf(GlobalQueues[0].pBuffer, GlobalQueues[0].szLen);
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::AddDataToQueue(GlobalQueue &pQueue, char * sData, const size_t &szLen) {
    if(pQueue.szSize < (pQueue.szLen + szLen)) {
        size_t szAllignLen = Allign256(pQueue.szLen + szLen);
        char * pOldBuf = pQueue.pBuffer;

#ifdef _WIN32
        pQueue.pBuffer = (char *)HeapReAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
#else
		pQueue.pBuffer = (char *)realloc(pOldBuf, szAllignLen);
#endif
        if(pQueue.pBuffer == NULL) {
            pQueue.pBuffer = pOldBuf;

            AppendDebugLog("%s - [MEM] Cannot reallocate %" PRIu64 " bytes in clsGlobalDataQueue::AddDataToQueue\n", (uint64_t)szAllignLen);
            return;
        }

        pQueue.szSize = szAllignLen-1;
    }

    memcpy(pQueue.pBuffer+pQueue.szLen, sData, szLen);
    pQueue.szLen += szLen;
    pQueue.pBuffer[pQueue.szLen] = '\0';
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * clsGlobalDataQueue::GetLastQueueItem() {
    return pNewQueueItems[1];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * clsGlobalDataQueue::GetFirstQueueItem() {
    return pNewQueueItems[0];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * clsGlobalDataQueue::InsertBlankQueueItem(void * pAfterItem, const uint8_t &ui8CmdType) {
    QueueItem * pNewItem = new (std::nothrow) QueueItem();
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem in clsGlobalDataQueue::InsertBlankQueueItem\n", 0);
    	return NULL;
    }

    pNewItem->pCommand1 = NULL;
	pNewItem->szLen1 = 0;

    pNewItem->pCommand2 = NULL;
    pNewItem->szLen2 = 0;

	pNewItem->ui8CommandType = ui8CmdType;

    if(pAfterItem == pNewQueueItems[0]) {
        pNewItem->pNext = pNewQueueItems[0];
        pNewQueueItems[0] = pNewItem;
        return pNewItem;
    }

    QueueItem * pCur = NULL,
        * pNext = pNewQueueItems[0];

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->pNext;

        if(pCur == pAfterItem) {
            if(pCur->pNext == NULL) {
                pNewQueueItems[1] = pNewItem;
            }

            pNewItem->pNext = pCur->pNext;
            pCur->pNext = pNewItem;
            return pNewItem;
        }
    }

	pNewItem->pNext = NULL;

    if(pNewQueueItems[0] == NULL) {
        pNewQueueItems[0] = pNewItem;
        pNewQueueItems[1] = pNewItem;
    } else {
        pNewQueueItems[1]->pNext = pNewItem;
        pNewQueueItems[1] = pNewItem;
    }

    return pNewItem;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void clsGlobalDataQueue::FillBlankQueueItem(char * sCommand, const size_t &szLen, void * pQueueItem) {
#ifdef _WIN32
    ((QueueItem *)pQueueItem)->pCommand1 = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	((QueueItem *)pQueueItem)->pCommand1 = (char *)malloc(szLen+1);
#endif
    if(((QueueItem *)pQueueItem)->pCommand1 == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate %" PRIu64 " bytes for pNewItem->pCommand1 in clsGlobalDataQueue::FillBlankQueueItem\n", (uint64_t)(szLen+1));

        return;
    }

    memcpy(((QueueItem *)pQueueItem)->pCommand1, sCommand, szLen);
    ((QueueItem *)pQueueItem)->pCommand1[szLen] = '\0';

	((QueueItem *)pQueueItem)->szLen1 = szLen;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

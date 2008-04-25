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
#include "colUsers.h"
//---------------------------------------------------------------------------
#include "globalQueue.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
static const int MYINFOLISTSIZE = 1024*256;
static const int IPLISTSIZE = 1024*64;
//---------------------------------------------------------------------------
classUsers *colUsers = NULL;
//---------------------------------------------------------------------------

classUsers::classUsers() {
    llist = NULL;
    elist = NULL;

    RecTimeList = NULL;
    
    ui16ActSearchs = 0;
    ui16PasSearchs = 0;

    nickList = (char *) calloc(NICKLISTSIZE, 1);
    if(nickList == NULL) {
		AppendSpecialLog("Cannot create nickList!");
		return;
    }
    memcpy(nickList, "$NickList |", 11);
    nickList[11] = '\0';
    nickListLen = 11;
    nickListSize = NICKLISTSIZE - 1;
    
    sZNickList = (char *) calloc(ZLISTSIZE, 1);
    if(sZNickList == NULL) {
		AppendSpecialLog("Cannot create sZNickList!");
		return;
    }
    iZNickListLen = 0;
    iZNickListSize = ZLISTSIZE - 1;
    
    opList = (char *) calloc(OPLISTSIZE, 1);
    if(opList == NULL) {
		AppendSpecialLog("Cannot create opList!");
		return;
    }
    memcpy(opList, "$OpList |", 9);
    opList[9] = '\0';
    opListLen = 9;
    opListSize = OPLISTSIZE - 1;

    sZOpList = (char *) calloc(ZLISTSIZE, 1);
    if(sZOpList == NULL) {
		AppendSpecialLog("Cannot create sZOpList!");
		return;
    }
    iZOpListLen = 0;
    iZOpListSize = ZLISTSIZE - 1;
    
    if(SettingManager->ui8FullMyINFOOption != 0) {
        myInfos = (char *) calloc(MYINFOLISTSIZE, 1);
        if(myInfos == NULL) {
    		AppendSpecialLog("Cannot create myInfos!");
    		return;
        }
        myInfosSize = MYINFOLISTSIZE - 1;
        
        sZMyInfos = (char *) calloc(ZMYINFOLISTSIZE, 1);
        if(sZMyInfos == NULL) {
    		AppendSpecialLog("Cannot create sZMyInfos!");
    		return;
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
        myInfosTag = (char *) calloc(MYINFOLISTSIZE, 1);
        if(myInfosTag == NULL) {
    		AppendSpecialLog("Cannot create myInfosTag!");
    		return;
        }
        myInfosTagSize = MYINFOLISTSIZE - 1;

        sZMyInfosTag = (char *) calloc(ZMYINFOLISTSIZE, 1);
        if(sZMyInfosTag == NULL) {
    		AppendSpecialLog("Cannot create sZMyInfosTag!");
    		return;
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
   
    userIPList = (char *) calloc(IPLISTSIZE, 1);
    if(userIPList == NULL) {
		AppendSpecialLog("Cannot create userIPList!");
		return;
    }
    memcpy(userIPList, "$UserIP |", 9);
    userIPList[9] = '\0';
    userIPListLen = 9;
    userIPListSize = IPLISTSIZE - 1;
    
    sZUserIPList = (char *) calloc(ZLISTSIZE, 1);
    if(sZUserIPList == NULL) {
		AppendSpecialLog("Cannot create sZUserIPList!");
		return;
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

        if(cur->sNick != NULL) {
            free(cur->sNick);
        }

        delete cur;
    }

    if(nickList != NULL) {
        free(nickList);
        nickList = NULL;
    }

    if(sZNickList != NULL) {
        free(sZNickList);
        sZNickList = NULL;
    }

    if(opList != NULL) {
        free(opList);
        opList = NULL;
    }

    if(sZOpList != NULL) {
        free(sZOpList);
        sZOpList = NULL;
    }

    if(myInfos != NULL) {
        free(myInfos);
        myInfos = NULL;
    }

    if(sZMyInfos != NULL) {
        free(sZMyInfos);
        sZMyInfos = NULL;
    }

    if(myInfosTag != NULL) {
        free(myInfosTag);
        myInfosTag = NULL;
    }

    if(sZMyInfosTag != NULL) {
        free(sZMyInfosTag);
        sZMyInfosTag = NULL;
    }

    if(userIPList != NULL) {
        free(userIPList);
        userIPList = NULL;
    }

    if(sZUserIPList != NULL) {
        free(sZUserIPList);
        sZUserIPList = NULL;
    }
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
        int iret = sprintf(msg, "# User %s removed from linked list.", u->Nick);
        if(CheckSprintf(iret, 1024, "classUsers::RemUser") == true) {
            Cout(msg);
        }
    #endif
}
//---------------------------------------------------------------------------

void classUsers::DisconnectAll() {
    int iCloseLoops = 0;
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
                
                shutdown(u->Sck, SHUT_RDWR);
				close(u->Sck);

				delete u;
            } else {
                UserTry2Send(u);
            }
        }
        iCloseLoops++;
        usleep(50000);
    }

    User *next = llist;
    while(next != NULL) {
        User *u = next;
    	next = u->next;
    	shutdown(u->Sck, SHUT_RDWR);
		close(u->Sck);

		delete u;
	}
}
//---------------------------------------------------------------------------

// NICKLIST OPERATIONS
void classUsers::Add2NickList(char * Nick, const size_t &iNickLen, const bool &isOp) {
    // $NickList nick$$nick2$$|

    if(nickListSize < nickListLen+iNickLen+2) {
        nickList = (char *) realloc(nickList, nickListSize+NICKLISTSIZE+1);
        if(nickList == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(nickListSize)+"/"+string(nickListSize+NICKLISTSIZE+1)+
				" bytes of memory in classUsers::Add2NickList for NickList!";
			AppendSpecialLog(sDbgstr);
            return;
		}
        nickListSize += NICKLISTSIZE;
    }

    memcpy(nickList+nickListLen-1, Nick, iNickLen);
    nickListLen += iNickLen+2;

    nickList[nickListLen-3] = '$';
    nickList[nickListLen-2] = '$';
    nickList[nickListLen-1] = '|';
    nickList[nickListLen] = '\0';

    iZNickListLen = 0;

    if(isOp == false)
        return;

    if(opListSize < opListLen+iNickLen+2) {
        opList = (char *) realloc(opList, opListSize+OPLISTSIZE+1);
        if(opList == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(opListSize)+"/"+string(opListSize+OPLISTSIZE+1)+
				" bytes of memory in classUsers::Add2NickList for opList!";
            AppendSpecialLog(sDbgstr);
            return;
        }
        opListSize += OPLISTSIZE;
    }

    memcpy(opList+opListLen-1, Nick, iNickLen);
    opListLen += iNickLen+2;

    opList[opListLen-3] = '$';
    opList[opListLen-2] = '$';
    opList[opListLen-1] = '|';
    opList[opListLen] = '\0';

    iZOpListLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::Add2OpList(char * Nick, const size_t &iNickLen) {
    if(opListSize < opListLen+iNickLen+2) {
        opList = (char *) realloc(opList, opListSize+OPLISTSIZE+1);
        if(opList == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(opListSize)+"/"+string(opListSize+OPLISTSIZE+1)+
                " bytes of memory in classUsers::Add2OpList for opList!";
            AppendSpecialLog(sDbgstr);
            return;
        }
        opListSize += OPLISTSIZE;
    }

    memcpy(opList+opListLen-1, Nick, iNickLen);
    opListLen += iNickLen+2;

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
void classUsers::SendChat2All(User * cur, char * data, const int &iChatLen) {
    UdpDebug->Broadcast(data, iChatLen);

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

        if(iChatMsgs > SettingManager->iShorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES]) {
			iChatLockFromTick = ui64ActualTick;
            if(bChatLocked == false) {
                if(SettingManager->bBools[SETBOOL_DEFLOOD_REPORT] == true) {
                    if(SettingManager->bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
                        int imsgLen = sprintf(msg, "%s $<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                        if(CheckSprintf(imsgLen, 1024, "classUsers::SendChat2All1") == true) {
                            QueueDataItem *newItem = globalQ->CreateQueueDataItem(msg, imsgLen, NULL, 0, queue::PM2OPS);
                            globalQ->SingleItemsStore(newItem);
                        }
                    } else {
                        int imsgLen = sprintf(msg, "<%s> *** %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                            LanguageManager->sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                        if(CheckSprintf(imsgLen, 1024, "classUsers::SendChat2All2") == true) {
                            globalQ->OPStore(msg, imsgLen);
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
                    int iWantLen = iChatLen+17;
                    char *MSG = (char *) malloc(iChatLen+17);
                    if(MSG == NULL) {
						string sDbgstr = "[BUF] Cannot allocate "+string(iWantLen)+" bytes of memory in classUsers::SendChat2All! "+
							string(data, iChatLen);
                        AppendSpecialLog(sDbgstr);
                        return;
                    }
                	int iMsgLen = sprintf(MSG, "%s ", cur->IP);
                	if(CheckSprintf(iMsgLen, iWantLen, "classUsers::SendChat2All3") == true) {
                        memcpy(MSG+iMsgLen, data, iChatLen);
                        iMsgLen += iChatLen;
                        MSG[iMsgLen] = '\0';
                        globalQ->OPStore(MSG, iMsgLen);
                    }
                    
                    free(MSG);

                    return;
                }
            } else {
                bChatLocked = false;
            }
        }
    }

    globalQ->Store(data, iChatLen);
}
//---------------------------------------------------------------------------

void classUsers::Add2MyInfos(User * u) {
    if(myInfosSize < myInfosLen+u->iMyInfoLen) {
        myInfos = (char *) realloc(myInfos, myInfosSize+MYINFOLISTSIZE+1);
        if(myInfos == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(myInfosSize)+"/"+string(myInfosSize+MYINFOLISTSIZE+1)+
				" bytes of memory in classUsers::Add2MyInfos! "+string(u->MyInfo, u->iMyInfoLen);
			AppendSpecialLog(sDbgstr);
            return;
        }
        myInfosSize += MYINFOLISTSIZE;
    }

    memcpy(myInfos+myInfosLen, u->MyInfo, u->iMyInfoLen);
    myInfosLen += u->iMyInfoLen;

    myInfos[myInfosLen] = '\0';

    iZMyInfosLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::DelFromMyInfos(User * u) {
	char *match = strstr(myInfos, u->MyInfo+8);
    if(match != NULL) {
		match -= 8;
		memmove(match, match+u->iMyInfoLen, myInfosLen-((match+(u->iMyInfoLen-1))-myInfos));
        myInfosLen -= u->iMyInfoLen;
        iZMyInfosLen = 0;
    }
}
//---------------------------------------------------------------------------

void classUsers::Add2MyInfosTag(User * u) {
    if(myInfosTagSize < myInfosTagLen+u->iMyInfoTagLen) {
        myInfosTag = (char *) realloc(myInfosTag, myInfosTagSize+MYINFOLISTSIZE+1);
        if(myInfosTag == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(myInfosTagSize)+"/"+string(myInfosTagSize+MYINFOLISTSIZE+1)+
				" bytes of memory in classUsers::Add2MyInfosTag! "+string(u->MyInfoTag, u->iMyInfoTagLen);
			AppendSpecialLog(sDbgstr);
            return;
        }
        myInfosTagSize += MYINFOLISTSIZE;
    }

    memcpy(myInfosTag+myInfosTagLen, u->MyInfoTag, u->iMyInfoTagLen);
    myInfosTagLen += u->iMyInfoTagLen;

    myInfosTag[myInfosTagLen] = '\0';

    iZMyInfosTagLen = 0;
}
//---------------------------------------------------------------------------

void classUsers::DelFromMyInfosTag(User * u) {
	char *match = strstr(myInfosTag, u->MyInfoTag+8);
    if(match != NULL) {
		match -= 8;
        memmove(match, match+u->iMyInfoTagLen, myInfosTagLen-((match+(u->iMyInfoTagLen-1))-myInfosTag));
        myInfosTagLen -= u->iMyInfoTagLen;
        iZMyInfosTagLen = 0;
    }
}
//---------------------------------------------------------------------------

void classUsers::AddBot2MyInfos(char * MyInfo) {
	int len = strlen(MyInfo);
	if(myInfosTag != NULL) {
	    if(strstr(myInfosTag, MyInfo) == NULL ) {
            if(myInfosTagSize < myInfosTagLen+len) {
                myInfosTag = (char *) realloc(myInfosTag, myInfosTagSize+MYINFOLISTSIZE+1);
                if(myInfosTag == NULL) {
					string sDbgstr = "[BUF] Cannot reallocate "+string(myInfosTagSize)+"/"+string(myInfosTagSize+MYINFOLISTSIZE+1)+
						" bytes of memory for myInfosTag in classUsers::AddBot2MyInfos! "+string(MyInfo, len);
					AppendSpecialLog(sDbgstr);
                    return;
                }
                myInfosTagSize += MYINFOLISTSIZE;
            }
        	memcpy(myInfosTag+myInfosTagLen, MyInfo, len);
            myInfosTagLen += len;
            myInfosTag[myInfosTagLen] = '\0';
            iZMyInfosLen = 0;
        }
    }
    if(myInfos != NULL) {
    	if(strstr(myInfos, MyInfo) == NULL ) {
            if(myInfosSize < myInfosLen+len) {
                myInfos = (char *) realloc(myInfos, myInfosSize+MYINFOLISTSIZE+1);
                if(myInfos == NULL) {
					string sDbgstr = "[BUF] Cannot reallocate "+string(myInfosSize)+"/"+string(myInfosSize+MYINFOLISTSIZE+1)+
						" bytes of memory for myInfos in classUsers::AddBot2MyInfos! "+string(MyInfo, len);
					AppendSpecialLog(sDbgstr);
                    return;
                }
                myInfosSize += MYINFOLISTSIZE;
            }
        	memcpy(myInfos+myInfosLen, MyInfo, len);
            myInfosLen += len;
            myInfos[myInfosLen] = '\0';
            iZMyInfosTagLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void classUsers::DelBotFromMyInfos(char * MyInfo) {
	int len = strlen(MyInfo);
	if(myInfosTag) {
		char *match = strstr(myInfosTag,  MyInfo);
	    if(match) {
    		memmove(match, match+len, myInfosTagLen-((match+(len-1))-myInfosTag));
        	myInfosTagLen -= len;
        	iZMyInfosTagLen = 0;
         }
    }
	if(myInfos) {
		char *match = strstr(myInfos,  MyInfo);
	    if(match) {
    		memmove(match, match+len, myInfosLen-((match+(len-1))-myInfos));
        	myInfosLen -= len;
        	iZMyInfosLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void classUsers::Add2UserIP(User * cur) {
    int m = sprintf(msg,"$%s %s$", cur->Nick, cur->IP);
    if(CheckSprintf(m, 1024, "classUsers::Add2UserIP") == false) {
        return;
    }

    if(userIPListSize < userIPListLen+m) {
        userIPList = (char *) realloc(userIPList, userIPListSize+IPLISTSIZE+1);
        if(userIPList == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(userIPListSize)+"/"+string(userIPListSize+IPLISTSIZE+1)+
				" bytes of memory in classUsers::Add2UserIP!";
			AppendSpecialLog(sDbgstr);
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
    int m = sprintf(msg,"$%s %s$", cur->Nick, cur->IP);
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
    time_t acc_time; time(&acc_time);

    if(ProfileMan->IsAllowed(curUser, ProfileManager::NOUSRSAMEIP) == true || 
        (acc_time-curUser->LoginTime) >= SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME]) {
        return;
    }

    RecTime * cur = new RecTime();

	if(cur == NULL) {
		string sDbgstr = "[BUF] Cannot allocate RecTime in classUsers::Add2RecTimes!";
		AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
	}

    cur->sNick = (char *) malloc(curUser->NickLen+1);
	if(cur->sNick == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(curUser->NickLen+1)+" bytes of memory in classUsers::Add2RecTimes!";
        AppendSpecialLog(sDbgstr);
        exit(EXIT_FAILURE);
    }

    memcpy(cur->sNick, curUser->Nick, curUser->NickLen);
    cur->sNick[curUser->NickLen] = '\0';

	cur->ui64DisConnTick = ui64ActualTick-(acc_time-curUser->LoginTime);
    cur->ui32NickHash = curUser->ui32NickHash;
    cur->ui32IpHash = curUser->ui32IpHash;

    cur->prev = NULL;
    cur->next = RecTimeList;

	if(RecTimeList != NULL) {
		RecTimeList->prev = cur;
	}

	RecTimeList = cur;
}
//---------------------------------------------------------------------------

bool classUsers::CheckRecTime(User * curUser) {
    RecTime * next = RecTimeList;

    while(next != NULL) {
        RecTime * cur = next;
        next = cur->next;

        // check expires...
        if(cur->ui64DisConnTick+SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME] <= ui64ActualTick) {
            if(cur->sNick != NULL) {
                free(cur->sNick);
            }

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

        if(cur->ui32NickHash == curUser->ui32NickHash && cur->ui32IpHash == curUser->ui32IpHash &&
            strcasecmp(cur->sNick, curUser->Nick) == 0) {
            int imsgLen = sprintf(msg, "<%s> %s %llu %s.|", SettingManager->sPreTexts[SetMan::SETPRETXT_HUB_SEC], 
                LanguageManager->sTexts[LAN_PLEASE_WAIT], 
                (unsigned long long)(cur->ui64DisConnTick+SettingManager->iShorts[SETSHORT_MIN_RECONN_TIME])-ui64ActualTick, 
                LanguageManager->sTexts[LAN_SECONDS_BEFORE_RECONN]);
            if(CheckSprintf(imsgLen, 1024, "classUsers::CheckRecTime1") == true) {
                UserSendChar(curUser, msg, imsgLen);
            }
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

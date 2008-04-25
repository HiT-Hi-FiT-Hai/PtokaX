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
#ifndef colUsersH
#define colUsersH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
static const int NICKLISTSIZE = 1024*64;
static const int OPLISTSIZE = 1024*32;
static const int ZLISTSIZE = 1024*16;
static const int ZMYINFOLISTSIZE = 1024*128;
//---------------------------------------------------------------------------

class classUsers {
private:
    struct RecTime {
        uint64_t ui64DisConnTick;
        uint32_t ui32NickHash, ui32IpHash;
        RecTime *prev, *next;
        char * sNick;
    };

    uint64_t iChatMsgsTick, iChatLockFromTick;

    int iChatMsgs;

    RecTime * RecTimeList;

	User *elist;

    bool bChatLocked;

	char msg[1024];
public:
    unsigned int myInfosLen, myInfosSize, iZMyInfosLen, iZMyInfosSize;
    unsigned int myInfosTagLen, myInfosTagSize, iZMyInfosTagLen, iZMyInfosTagSize;
    unsigned int nickListLen, nickListSize, iZNickListLen, iZNickListSize;
    unsigned int opListLen, opListSize, iZOpListLen, iZOpListSize;
    unsigned int userIPListSize, userIPListLen, iZUserIPListSize, iZUserIPListLen;

    char *nickList, *sZNickList, *opList, *sZOpList, *userIPList, *sZUserIPList; 
    char *myInfos, *sZMyInfos, *myInfosTag, *sZMyInfosTag;

    User *llist;
    
    uint16_t ui16ActSearchs, ui16PasSearchs;

    classUsers();
    ~classUsers();

    void DisconnectAll();
    void AddUser(User * u);
    void RemUser(User * u);
    void Add2NickList(char * Nick, const size_t &iNickLen, const bool &isOp);
    void Add2OpList(char * Nick, const size_t &iNickLen);
    void DelFromNickList(char * Nick, const bool &isOp);
    void DelFromOpList(char * Nick);
    void SendChat2All(User * cur, char * data, const int &iChatLen);
	void Add2MyInfos(User * u);
	void DelFromMyInfos(User * u);
    void Add2MyInfosTag(User * u);
	void DelFromMyInfosTag(User * u);
    void AddBot2MyInfos(char * MyInfo);
	void DelBotFromMyInfos(char * MyInfo);
	void Add2UserIP(User * cur);
	void DelFromUserIP(User * cur);
	void Add2RecTimes(User * curUser);
	bool CheckRecTime(User * curUser);
};

//---------------------------------------------------------------------------
extern classUsers *colUsers;
//---------------------------------------------------------------------------

#endif

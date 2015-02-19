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
#ifndef colUsersH
#define colUsersH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
static const uint32_t NICKLISTSIZE = 1024*64;
static const uint32_t OPLISTSIZE = 1024*32;
static const uint32_t ZLISTSIZE = 1024*16;
static const uint32_t ZMYINFOLISTSIZE = 1024*128;
//---------------------------------------------------------------------------

class clsUsers {
private:
    struct RecTime {
        RecTime(const uint8_t * pIpHash);

        RecTime(const RecTime&);
        const RecTime& operator=(const RecTime&);

        uint64_t ui64DisConnTick;
        uint32_t ui32NickHash;
        RecTime * pPrev, * pNext;
        char * sNick;
        uint8_t ui128IpHash[16];
    };

    uint64_t ui64ChatMsgsTick, ui64ChatLockFromTick;

    uint16_t ui16ChatMsgs;

    RecTime * pRecTimeList;

	User * pListE;

    bool bChatLocked;

	char msg[1024];

    clsUsers(const clsUsers&);
    const clsUsers& operator=(const clsUsers&);
public:
    static clsUsers * mPtr;

    uint32_t ui32MyInfosLen, ui32MyInfosSize, ui32ZMyInfosLen, ui32ZMyInfosSize;
    uint32_t ui32MyInfosTagLen, ui32MyInfosTagSize, ui32ZMyInfosTagLen, ui32ZMyInfosTagSize;
    uint32_t ui32NickListLen, ui32NickListSize, ui32ZNickListLen, ui32ZNickListSize;
    uint32_t ui32OpListLen, ui32OpListSize, ui32ZOpListLen, ui32ZOpListSize;
    uint32_t ui32UserIPListSize, ui32UserIPListLen, ui32ZUserIPListSize, ui32ZUserIPListLen;

    char * pNickList, * pZNickList, * pOpList, * pZOpList, * pUserIPList, * pZUserIPList;
    char * pMyInfos, * pZMyInfos, * pMyInfosTag, * pZMyInfosTag;

    User * pListS;
    
    uint16_t ui16ActSearchs, ui16PasSearchs;

    clsUsers();
    ~clsUsers();

    void DisconnectAll();
    void AddUser(User * u);
    void RemUser(User * u);
    void Add2NickList(User * u);
    void AddBot2NickList(char * Nick, const size_t &szNickLen, const bool &isOp);
    void Add2OpList(User * u);
    void DelFromNickList(char * Nick, const bool &isOp);
    void DelFromOpList(char * Nick);
    void SendChat2All(User * cur, char * sData, const size_t &szChatLen, void * pToUser);
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

#endif

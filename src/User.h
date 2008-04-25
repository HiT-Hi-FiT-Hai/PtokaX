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
#ifndef UserH
#define UserH
//---------------------------------------------------------------------------

struct UserBan {
    UserBan(char * sMess, int iMessLen, uint32_t ui32Hash);
    ~UserBan();

    unsigned int iLen;
    uint32_t ui32NickHash;
    char * sMessage;
};
//---------------------------------------------------------------------------

struct LoginLogout {
    LoginLogout();
    ~LoginLogout();

    uint64_t logonClk;
    unsigned int iToCloseLoops, iUserConnectedLen;
    char *sLockUsrConn, *Password, *sKickMsg;
    UserBan *uBan;
};
//---------------------------------------------------------------------------

struct PrcsdUsrCmd {
    enum PrcsdCmdsIds {
        CTM_MCTM_RCTM_SR_TO,
        SUPPORTS,
        LOGINHELLO,
        GETPASS,
        CHAT,
        TO_OP_CHAT,
    };

    unsigned int iLen;
    char * sCommand;
    PrcsdUsrCmd *next;
    unsigned char cType;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
struct User; // needed for next struct, and next struct must be defined before user :-/
//---------------------------------------------------------------------------

struct PrcsdToUsrCmd {
    unsigned int iLen, iPmCount, iLoops, iToNickLen;
    char * sCommand, * ToNick;
    PrcsdToUsrCmd *next;
    User *To;
};
//---------------------------------------------------------------------------

struct User {
	User();
	~User();

    enum UserStates {
        STATE_SOCKET_ACCEPTED,
        STATE_KEY_OR_SUP,
        STATE_VALIDATE,
        STATE_VERSION_OR_MYPASS,
        STATE_GETNICKLIST_OR_MYINFO,
        STATE_ADDME,
        STATE_ADDME_1LOOP,
        STATE_ADDME_2LOOP,
        STATE_ADDED,
        STATE_CLOSING,
        STATE_REMME
    };

    //  u->ui32BoolBits |= BIT_PRCSD_MYINFO;   <- set to 1
    //  u->ui32BoolBits &= ~BIT_PRCSD_MYINFO;  <- set to 0
    //  (u->ui32BoolBits & BIT_PRCSD_MYINFO) == BIT_PRCSD_MYINFO    <- test if is 1/true
    enum UserBits {
    	BIT_HASHED                 = 0x1,
    	BIT_ERROR                  = 0x2,
    	BIT_OPERATOR               = 0x4,
    	BIT_GAGGED                 = 0x8,
    	BIT_GETNICKLIST            = 0x10,
    	BIT_ACTIVE                 = 0x20,
    	BIT_OLDHUBSTAG             = 0x40,
    	BIT_TEMP_OPERATOR          = 0x80,
    	BIT_PINGER                 = 0x100,
    	BIT_BIG_SEND_BUFFER        = 0x200,
    	BIT_HAVE_SUPPORTS          = 0x400,
    	BIT_HAVE_VALIDATENICK      = 0x800,
    	BIT_HAVE_VERSION           = 0x1000,
    	BIT_HAVE_BADTAG            = 0x2000,
    	BIT_HAVE_GETNICKLIST       = 0x4000,
    	BIT_HAVE_BOTINFO           = 0x8000,
    	BIT_HAVE_KEY               = 0x10000,
    	BIT_HAVE_SHARECOUNTED      = 0x20000,
    	BIT_SUPPORT_NOGETINFO      = 0x40000,
    	BIT_SUPPORT_USERCOMMAND    = 0x80000,
    	BIT_SUPPORT_NOHELLO        = 0x100000,
    	BIT_SUPPORT_QUICKLIST      = 0x200000,
    	BIT_SUPPORT_USERIP2        = 0x400000,
    	BIT_SUPPORT_ZPIPE          = 0x800000, 
    	BIT_PRCSD_MYINFO           = 0x1000000, 
    	BIT_RECV_FLOODER           = 0x2000000, 
    };

    uint64_t sharedSize;
	uint64_t ui64GetNickListsTick, ui64MyINFOsTick, ui64SearchsTick, ui64ChatMsgsTick;
    uint64_t ui64PMsTick, ui64SameSearchsTick, ui64SamePMsTick, ui64SameChatsTick;
    uint64_t iLastMyINFOSendTick, iLastNicklist, iReceivedPmTick, ui64ChatMsgsTick2;
    uint64_t ui64PMsTick2, ui64SearchsTick2, ui64MyINFOsTick2, ui64CTMsTick;
    uint64_t ui64CTMsTick2, ui64RCTMsTick, ui64RCTMsTick2, ui64SRsTick;
    uint64_t ui64SRsTick2, ui64RecvsTick, ui64RecvsTick2, ui64ChatIntMsgsTick;
    uint64_t ui64PMsIntTick, ui64SearchsIntTick;

    uint32_t ui32Recvs, ui32Recvs2;

    int Hubs, Slots, OLimit, LLimit, DLimit, iNormalHubs, iRegHubs, iOpHubs;
    int iState, iProfile;
    int iSendCalled, iRecvCalled, iReceivedPmCount, iSR, iDefloodWarnings;
//    socklen_t sin_len;
//    int PORT;

	int Sck;
//    sockaddr_in addr;

    time_t LoginTime;

    unsigned int NickLen, iMyInfoTagLen, iMyInfoLen, iMyInfoOldLen;
    unsigned int sendbuflen, recvbuflen, sbdatalen, rbdatalen;

    uint32_t ui32BoolBits;
    uint32_t ui32NickHash, ui32IpHash;

    char *sLastChat, *sLastPM, *sendbuf, *recvbuf, *sbplayhead, *sLastSearch;
    char *Nick, *Version, *MyInfo, *MyInfoOld, *Ver, Mode;
    char *MyInfoTag, *Client, *Tag, *Description, *Connection, MagicByte, *Email;
    
    LoginLogout *uLogInOut;

    PrcsdToUsrCmd *cmdToUserStrt, *cmdToUserEnd;

    PrcsdUsrCmd *cmdStrt, *cmdEnd, 
        *cmdASearch, *cmdPSearch;

    User *prev, *next, *hashtableprev, *hashtablenext, *hashiptableprev, *hashiptablenext;

    uint16_t ui16GetNickLists, ui16MyINFOs, ui16Searchs, ui16ChatMsgs, ui16PMs;
    uint16_t ui16SameSearchs, ui16LastSearchLen, ui16SamePMs, ui16LastPMLen;
    uint16_t ui16SameChatMsgs, ui16LastChatLen, ui16LastPmLines, ui16SameMultiPms; 
    uint16_t ui16LastChatLines, ui16SameMultiChats, ui16ChatMsgs2, ui16PMs2;
    uint16_t ui16Searchs2, ui16MyINFOs2, ui16CTMs, ui16CTMs2;
    uint16_t ui16RCTMs, ui16RCTMs2, ui16SRs, ui16SRs2;
    uint16_t ui16ChatIntMsgs, ui16PMsInt, ui16SearchsInt;

    uint8_t ui8IpLen, ui8ConnLen, ui8DescrLen, ui8EmailLen, ui8TagLen, ui8ClientLen, ui8VerLen;
    uint8_t ui8Country;

    char IP[16];
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
struct QzBuf; // for send queue
//---------------------------------------------------------------------------

void UserMakeLock(User * u);

bool UserDoRecv(User * u);

void UserSendChar(User * u, const char * cText, const int &iTextLen);
void UserSendCharDelayed(User * u, const char * cText);
void UserSendCharDelayed(User * u, const char * cText, const int &iTextLen);
void UserSendText(User * u, const string & sText);
void UserSendTextDelayed(User * u, const string & sText);
void UserSendQueue(User * u, QzBuf * Queue, bool bChckActSr = true);
bool UserPutInSendBuf(User * u, const char * Text, const int &iTxtLen);
bool UserTry2Send(User * u);

void UserSetIP(User * u, char * newIP);
void UserSetNick(User * u, char * newNick, const int &iNewNickLen);
void UserSetMyInfoTag(User * u, char * newInfoTag, const int &MyInfoTagLen);
void UserSetVersion(User * u, char * newVer);
void UserSetPasswd(User * u, char * newPass);
void UserSetLastChat(User * u, char * newData, const int &iLen);
void UserSetLastPM(User * u, char * newData, const int &iLen);
void UserSetLastSearch(User * u, char * newData, const int &iLen);
void UserSetKickMsg(User * u, char * kickmsg, int iLen = 0);

void UserClose(User * u, bool bNoQuit = false);

void UserAdd2Userlist(User * u);
void UserAddUserList(User * u);

void UserClearMyINFOTag(User * u);

void UserGenerateMyINFO(User * u);

void UserHasSuspiciousTag(User * curUser);

bool UserProcessRules(User * u);

void UserAddPrcsdCmd(User * u, unsigned char cType, char *sCommand, int iCommandLen, User * to, bool bIsPm = false);
//---------------------------------------------------------------------------

#endif

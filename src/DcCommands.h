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
#ifndef DcCommandsH
#define DcCommandsH
//---------------------------------------------------------------------------
struct User;
struct PassBf;
//---------------------------------------------------------------------------

class cDcCommands {
private:
    PassBf * PasswdBfCheck;
    char msg[1024];
    
	void BotINFO(User * curUser, char * sData, const unsigned int &iLen);
    void ConnectToMe(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck, const bool &bMulti);
	void GetINFO(User * curUser, char * sData, const unsigned int &iLen);
    bool GetNickList(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	void Key(User * curUser, char * sData, const unsigned int &iLen);
	void Kick(User * curUser, char * sData, const unsigned int &iLen);
    bool SearchDeflood(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck, const bool &bMulti);
    void Search(User * curUser, char * sData, int unsigned iLen, const bool &bCheck, const bool &bMulti);
    bool MyINFODeflood(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	bool MyINFO(User * curUser, char * sData, const unsigned int &iLen);
	void MyPass(User * curUser, char * sData, const unsigned int &iLen);
	void OpForceMove(User * curUser, char * sData, const unsigned int &iLen);
	void RevConnectToMe(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	void SR(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	void Supports(User * curUser, char * sData, const unsigned int &iLen);
    void To(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	void ValidateNick(User * curUser, char * sData, const unsigned int &iLen);
	void Version(User * curUser, char * sData, const unsigned int &iLen);
    bool ChatDeflood(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	void Chat(User * curUser, char * sData, const unsigned int &iLen, const bool &bCheck);
	void Close(User * curUser, char * sData, const unsigned int &iLen);
    
    void Unknown(User * curUser, char * sData, const unsigned int &iLen);
    
    bool ValidateUserNick(User * curUser, char * Nick, const unsigned int &iNickLen, const bool &ValidateNick);

	PassBf * Find(const uint32_t &hash);
	void Remove(PassBf * PassBfItem);

    void SendIncorrectIPMsg(User * curUser, char * sBadIP, const bool &bCTM);
protected:
public:
	cDcCommands();
    ~cDcCommands();

    void PreProcessData(User * curUser, char * sData, const bool &bCheck, const unsigned int &iLen);
    void ProcessCmds(User * curUser);

    void SRFromUDP(User * curUser, char * sData, const unsigned int &iLen);
    
	uint32_t iStatChat, iStatCmdUnknown, iStatCmdTo, iStatCmdMyInfo, iStatCmdSearch, iStatCmdSR, iStatCmdRevCTM;
	uint32_t iStatCmdOpForceMove, iStatCmdMyPass, iStatCmdValidate, iStatCmdKey, iStatCmdGetInfo, iStatCmdGetNickList;
	uint32_t iStatCmdConnectToMe, iStatCmdVersion, iStatCmdKick, iStatCmdSupports, iStatBotINFO, iStatZPipe;
    uint32_t iStatCmdMultiSearch, iStatCmdMultiConnectToMe, iStatCmdClose;
};

//---------------------------------------------------------------------------
extern cDcCommands *DcCommands;
//---------------------------------------------------------------------------

#endif

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
#ifndef HubCommandsH
#define HubCommandsH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class HubCommands {
private:
    char msg[1024];

    bool Ban(User * curUser, char * sCommand, bool fromPM, bool bFull);
    bool BanIp(User * curUser, char * sCommand, bool fromPM, bool bFull);
    bool NickBan(User * curUser, char * sNick, char * sReason, bool bFromPM);
    bool TempBan(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull);
    bool TempBanIp(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull);
    bool TempNickBan(User * curUser, char * sNick, char * sTime, const size_t &szTimeLen, char * sReason, bool bFromPM);
    bool RangeBan(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull);
    bool RangeTempBan(User * curUser, char * sCommand, const size_t &dlen, bool fromPM, bool bFull);
    bool RangeUnban(User * curUser, char * sCommand, bool fromPM);
    bool RangeUnban(User * curUser, char * sCommand, bool fromPM, unsigned char cType);

    void SendNoPermission(User * user, const bool &fromPM);
    int CheckFromPm(User * curUser, const bool &fromPM);
    static void UncountDeflood(User * curUser, const bool &fromPM);
public:
	HubCommands();
    ~HubCommands();

    bool DoCommand(User * curUser, char * sCommand, const size_t &szCmdLen, bool fromPM = false);
};

//---------------------------------------------------------------------------
extern HubCommands *HubCmds;
//---------------------------------------------------------------------------

#endif

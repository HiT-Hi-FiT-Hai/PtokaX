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
#ifndef HubCommandsH
#define HubCommandsH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class clsHubCommands {
private:
    static char msg[1024];

    static bool Ban(User * pUser, char * sCommand, bool bFromPM, bool bFull);
    static bool BanIp(User * pUser, char * sCommand, bool bFromPM, bool bFull);
    static bool NickBan(User * pUser, char * sNick, char * sReason, bool bFromPM);
    static bool TempBan(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull);
    static bool TempBanIp(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull);
    static bool TempNickBan(User * pUser, char * sNick, char * sTime, const size_t &szTimeLen, char * sReason, bool bFromPM, bool bNotNickBan = false);
    static bool RangeBan(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull);
    static bool RangeTempBan(User * pUser, char * sCommand, const size_t &dlen, bool bFromPM, bool bFull);
    static bool RangeUnban(User * pUser, char * sCommand, bool bFromPM);
    static bool RangeUnban(User * pUser, char * sCommand, bool bFromPM, unsigned char cType);

    static void SendNoPermission(User * pUser, const bool &bFromPM);
    static int CheckFromPm(User * pUser, const bool &bFromPM);
    static void UncountDeflood(User * pUser, const bool &bFromPM);
public:
    static bool DoCommand(User * pUser, char * sCommand, const size_t &szCmdLen, bool bFromPM = false);
};
//---------------------------------------------------------------------------

#endif

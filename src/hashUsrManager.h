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
#ifndef hashUsrManagerH
#define hashUsrManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class hashMan {
private:
    struct IpTableItem {
        IpTableItem *prev, *next;
        User * FirstUser;
        uint16_t ui16Count;
    };

    User *nicktable[65536];
    IpTableItem *iptable[65536];
public:
    hashMan();
    ~hashMan();

    void Add(User * u);
    void Remove(User * u);

    User * FindUser(char * sNick, const size_t &iNickLen);
    User * FindUser(User * u);
    User * FindUser(const uint32_t &ui32IpHash);

    uint32_t GetUserIpCount(User * u);
};

//---------------------------------------------------------------------------
extern hashMan *hashManager;
//---------------------------------------------------------------------------

#endif

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
#ifndef hashManH
#define hashManH
//---------------------------------------------------------------------------
struct BanItem;
struct RegUser;
struct User;
//---------------------------------------------------------------------------

class hashMan {
private:
    struct NickTableItem {
        BanItem * Bans;
        RegUser * Regs;
        User * Users;
    };

    struct IpTableBans {
		IpTableBans *prev, *next;
        BanItem * Bans;
    };

    struct IpTableUsers {
        IpTableUsers *prev, *next;
        User * Users;
        uint32_t ui32Count;
    };

    struct IpTableItem {
        IpTableBans * BanIPs;
        IpTableUsers * UserIPs;
    };

    NickTableItem *nicktable[65536];
    IpTableItem *iptable[65536];
public:
    hashMan();
    ~hashMan();

    void AddIP(BanItem *Ban);
    void RemoveIP(BanItem *Ban);

    void RemoveIPAll(const uint32_t &hash);
    void RemoveIPPermAll(const uint32_t &hash);
    void RemoveIPTempAll(const uint32_t &hash);

    void AddNick(BanItem *Ban);
    void RemoveNick(BanItem *Ban);

    void Add(RegUser * Reg);
    void Remove(RegUser * Reg);

    void Add(User * u);
    void Remove(User * u);

    BanItem* FindBanIP(User* u);
    BanItem* FindBanIP(const uint32_t &hash, const time_t &acc_time);
	BanItem* FindBanIPFull(const uint32_t &hash);
    BanItem* FindBanIPFull(const uint32_t &hash, const time_t &acc_time);
    BanItem* FindBanIPTemp(const uint32_t &hash, const time_t &acc_time);
    BanItem* FindBanIPPerm(const uint32_t &hash);
    BanItem* FindBanNick(User* u);
    BanItem* FindBanNick(char * sNick, const size_t &iNickLen);
    BanItem* FindBanNick(const uint32_t &hash, const time_t &acc_time, char * sNick);
    BanItem* FindBanNickTemp(char * sNick, const size_t &iNickLen);
    BanItem* FindBanNickTemp(const uint32_t &hash, const time_t &acc_time, char * sNick);
    BanItem* FindBanNickPerm(char * sNick, const size_t &iNickLen);
    BanItem* FindBanNickPerm(const uint32_t &hash, char * sNick);

    RegUser* FindReg(char * sNick, const size_t &iNickLen);
    RegUser* FindReg(User * u);
    RegUser* FindReg(uint32_t hash, char * sNick);

    User * FindUser(char * sNick, const size_t &iNickLen);
    User * FindUser(User * u);
    User * FindUser(const uint32_t &ui32IpHash);

    uint32_t GetUserIpCount(User * u);
    void RemoveAllUsers();
};

//---------------------------------------------------------------------------
extern hashMan *hashManager;
//---------------------------------------------------------------------------

#endif

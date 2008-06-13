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
#ifndef hashBanManagerH
#define hashBanManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct BanItem {
    time_t tempbanexpire;

    uint32_t ui32NickHash, ui32IpHash;

    char *sNick, *sReason, *sBy;

    BanItem *prev, *next;
    BanItem *hashnicktableprev, *hashnicktablenext;
    BanItem *hashiptableprev, *hashiptablenext;

    uint8_t ui8Bits;
    char sIp[16];

    BanItem(void);
    ~BanItem(void);
}; 
//---------------------------------------------------------------------------

struct RangeBanItem {
    time_t tempbanexpire;

    uint32_t ui32FromIpHash, ui32ToIpHash;

    char *sReason, *sBy;

    RangeBanItem *prev, *next;

    uint8_t ui8Bits;

    char sIpFrom[16], sIpTo[16] ;

    RangeBanItem(void);
    ~RangeBanItem(void);
};
//---------------------------------------------------------------------------

class hashBanMan {
private:
    struct IpTableItem {
        IpTableItem *prev, *next;
        BanItem * FirstBan;
    };

    uint32_t iSaveCalled;

    BanItem *nicktable[65536];
    IpTableItem *iptable[65536];
public:
    BanItem *TempBanListS, *TempBanListE;
    BanItem *PermBanListS, *PermBanListE;
    RangeBanItem *RangeBanListS, *RangeBanListE;

    enum BanBits {
        PERM        = 0x1,
        TEMP        = 0x2,
        FULL        = 0x4,
        IP          = 0x8,
        NICK        = 0x10
    };

    hashBanMan(void);
    ~hashBanMan(void);


    void Add(BanItem *Ban);
    void Add2Table(BanItem *Ban);
	void Add2NickTable(BanItem *Ban);
	void Add2IpTable(BanItem *Ban);
    void Rem(BanItem *Ban);
    void RemFromTable(BanItem *Ban);
    void RemFromNickTable(BanItem *Ban);
	void RemFromIpTable(BanItem *Ban);

	BanItem* Find(BanItem *Ban); // from gui
	void Remove(BanItem *Ban); // from gui

    void AddRange(RangeBanItem *RangeBan);
	void RemRange(RangeBanItem *RangeBan);

	RangeBanItem* FindRange(RangeBanItem *RangeBan); // from gui
	void RemoveRange(RangeBanItem *RangeBan); // from gui

    BanItem* FindNick(User* u);
    BanItem* FindIP(User* u);
    RangeBanItem* FindRange(User* u);

	BanItem* FindFull(const uint32_t &hash);
    BanItem* FindFull(const uint32_t &hash, const time_t &acc_time);
    RangeBanItem* FindFullRange(const uint32_t &hash, const time_t &acc_time);

    BanItem* FindNick(char * sNick, const size_t &iNickLen);
    BanItem* FindNick(const uint32_t &hash, const time_t &acc_time, char * sNick);
    BanItem* FindIP(const uint32_t &hash, const time_t &acc_time);
    RangeBanItem* FindRange(const uint32_t &hash, const time_t &acc_time);
    RangeBanItem* FindRange(const uint32_t &fromhash, const uint32_t &tohash, const time_t &acc_time);

    BanItem* FindTempNick(char * sNick, const size_t &iNickLen);
    BanItem* FindTempNick(const uint32_t &hash, const time_t &acc_time, char * sNick);
    BanItem* FindTempIP(const uint32_t &hash, const time_t &acc_time);
    
    BanItem* FindPermNick(char * sNick, const size_t &iNickLen);
    BanItem* FindPermNick(const uint32_t &hash, char * sNick);
    BanItem* FindPermIP(const uint32_t &hash);

    void Load(void);
    void Save(bool bForce = false);
    
    void CreateTemp(char * first, char * second, const uint32_t &iTime, const time_t &acc_time);
    void CreatePerm(char * first, char * second);

    void ClearTemp(void);
    void ClearPerm(void);
    void ClearRange(void);
    void ClearTempRange(void);
    void ClearPermRange(void);

    void Ban(User * u, const char * sReason, char * sBy, const bool &bFull);
    char BanIp(User * u, char * sIp, char * sReason, char * sBy, const bool &bFull);
    bool NickBan(User * u, char * sNick, char * sReason, char * sBy);
    
    void TempBan(User * u, const char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull);
    char TempBanIp(User * u, char * sIp, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull);
    bool NickTempBan(User * u, char * sNick, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime);
    
    bool Unban(char * sWhat);
    bool PermUnban(char * sWhat);
    bool TempUnban(char * sWhat);

    void RemoveAllIP(const uint32_t &hash);
    void RemovePermAllIP(const uint32_t &hash);
    void RemoveTempAllIP(const uint32_t &hash);

    bool RangeBan(char * sIpFrom, const uint32_t &ui32FromIpHash, char * sIpTo, const uint32_t &ui32ToIpHash, 
        char * sReason, char * sBy, const bool &bFull);
    bool RangeTempBan(char * sIpFrom, const uint32_t &ui32FromIpHash, char * sIpTo, const uint32_t &ui32ToIpHash,
        char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull);
        
    bool RangeUnban(const uint32_t &ui32FromIpHash, const uint32_t &ui32ToIpHash);
    bool RangeUnban(const uint32_t &ui32FromIpHash, const uint32_t &ui32ToIpHash, unsigned char cType);
};

//--------------------------------------------------------------------------- 
extern hashBanMan *hashBanManager;
//---------------------------------------------------------------------------

#endif

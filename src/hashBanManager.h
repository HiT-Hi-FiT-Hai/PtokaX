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
    
    void AddBan(BanItem *Ban);
    void AddBan2Table(BanItem *Ban);
    void RemBan(BanItem *Ban);
    void RemBanFromTable(BanItem *Ban);

	BanItem* FindBan(BanItem *Ban); // from gui
	void RemoveBan(BanItem *Ban); // from gui

    void AddRangeBan(RangeBanItem *RangeBan);
	void RemRangeBan(RangeBanItem *RangeBan);

	RangeBanItem* FindRangeBan(RangeBanItem *RangeBan); // from gui
	void RemoveRangeBan(RangeBanItem *RangeBan); // from gui

    RangeBanItem* FindRange(User* u);

    RangeBanItem* FindFullRangeBan(const uint32_t &hash, const time_t &acc_time);

    RangeBanItem* FindRangeBan(const uint32_t &hash, const time_t &acc_time);
    RangeBanItem* FindRangeBan(const uint32_t &fromhash, const uint32_t &tohash, const time_t &acc_time);
    
    void LoadBanList(void);
    void SaveBanList(bool bForce = false);
    
    void CreateTempBan(char * first, char * second, const uint32_t &iTime, const time_t &acc_time);
    void CreatePermBan(char * first, char * second);

    void ClearTempBan(void);
    void ClearPermBan(void);
    void ClearRangeBans(void);
    void ClearTempRangeBans(void);
    void ClearPermRangeBans(void);

    void Ban(User * u, const char * sReason, char * sBy, const bool &bFull);
    char BanIp(User * u, char * sIp, char * sReason, char * sBy, const bool &bFull);
    bool NickBan(User * u, char * sNick, char * sReason, char * sBy);
    
    void TempBan(User * u, const char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull);
    char TempBanIp(User * u, char * sIp, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime, const bool &bFull);
    bool NickTempBan(User * u, char * sNick, char * sReason, char * sBy, const uint32_t &minutes, const time_t &expiretime);
    
    bool Unban(char * sWhat);
    bool PermUnban(char * sWhat);
    bool TempUnban(char * sWhat);
    
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

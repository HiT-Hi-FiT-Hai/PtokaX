/*
 * PtokaX - hub server for Direct Connect peer to peer network.

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
#ifndef hashBanManagerH
#define hashBanManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct BanItem {
    time_t tTempBanExpire;

    uint32_t ui32NickHash;

    uint8_t ui128IpHash[16];

    char *sNick, *sReason, *sBy;

    BanItem * pPrev, * pNext;
    BanItem * pHashNickTablePrev, * pHashNickTableNext;
    BanItem * pHashIpTablePrev, * pHashIpTableNext;

    uint8_t ui8Bits;
    char sIp[40];

    BanItem();
    ~BanItem();

    BanItem(const BanItem&);
    const BanItem& operator=(const BanItem&);
}; 
//---------------------------------------------------------------------------

struct RangeBanItem {
    time_t tTempBanExpire;

    uint8_t ui128FromIpHash[16], ui128ToIpHash[16];

    char *sReason, *sBy;

    RangeBanItem * pPrev, * pNext;

    uint8_t ui8Bits;

    char sIpFrom[40], sIpTo[40] ;

    RangeBanItem();
    ~RangeBanItem();

    RangeBanItem(const RangeBanItem&);
    const RangeBanItem& operator=(const RangeBanItem&);
};
//---------------------------------------------------------------------------

class clsBanManager {
private:
    struct IpTableItem {
        IpTableItem() : pPrev(NULL), pNext(NULL), pFirstBan(NULL) { };

        IpTableItem(const IpTableItem&);
        const IpTableItem& operator=(const IpTableItem&);

        IpTableItem * pPrev, * pNext;
        BanItem * pFirstBan;
    };

    uint32_t ui32SaveCalled;

    BanItem * pNickTable[65536];
    IpTableItem * pIpTable[65536];

    clsBanManager(const clsBanManager&);
    const clsBanManager& operator=(const clsBanManager&);
public:
    static clsBanManager * mPtr;

    BanItem * pTempBanListS, * pTempBanListE;
    BanItem * pPermBanListS, * pPermBanListE;
    RangeBanItem * pRangeBanListS, * pRangeBanListE;

    enum BanBits {
        PERM        = 0x1,
        TEMP        = 0x2,
        FULL        = 0x4,
        IP          = 0x8,
        NICK        = 0x10
    };

    clsBanManager(void);
    ~clsBanManager(void);


    bool Add(BanItem * Ban);
    bool Add2Table(BanItem *Ban);
	void Add2NickTable(BanItem *Ban);
	bool Add2IpTable(BanItem *Ban);
    void Rem(BanItem * Ban, const bool &bFromGui = false);
    void RemFromTable(BanItem *Ban);
    void RemFromNickTable(BanItem *Ban);
	void RemFromIpTable(BanItem *Ban);

	BanItem* Find(BanItem *Ban); // from gui
	void Remove(BanItem *Ban); // from gui

    void AddRange(RangeBanItem *RangeBan);
	void RemRange(RangeBanItem *RangeBan, const bool &bFromGui = false);

	RangeBanItem* FindRange(RangeBanItem *RangeBan); // from gui
	void RemoveRange(RangeBanItem *RangeBan); // from gui

    BanItem* FindNick(User* u);
    BanItem* FindIP(User* u);
    RangeBanItem* FindRange(User* u);

	BanItem* FindFull(const uint8_t * ui128IpHash);
    BanItem* FindFull(const uint8_t * ui128IpHash, const time_t &acc_time);
    RangeBanItem* FindFullRange(const uint8_t * ui128IpHash, const time_t &acc_time);

    BanItem* FindNick(char * sNick, const size_t &szNickLen);
    BanItem* FindNick(const uint32_t &hash, const time_t &acc_time, char * sNick);
    BanItem* FindIP(const uint8_t * ui128IpHash, const time_t &acc_time);
    RangeBanItem* FindRange(const uint8_t * ui128IpHash, const time_t &acc_time);
    RangeBanItem* FindRange(const uint8_t * ui128FromHash, const uint8_t * ui128ToHash, const time_t &acc_time);

    BanItem* FindTempNick(char * sNick, const size_t &szNickLen);
    BanItem* FindTempNick(const uint32_t &hash, const time_t &acc_time, char * sNick);
    BanItem* FindTempIP(const uint8_t * ui128IpHash, const time_t &acc_time);
    
    BanItem* FindPermNick(char * sNick, const size_t &szNickLen);
    BanItem* FindPermNick(const uint32_t &hash, char * sNick);
    BanItem* FindPermIP(const uint8_t * ui128IpHash);

    void Load();
    void LoadXML();
    void Save(bool bForce = false);

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

    void RemoveAllIP(const uint8_t * ui128IpHash);
    void RemovePermAllIP(const uint8_t * ui128IpHash);
    void RemoveTempAllIP(const uint8_t * ui128IpHash);

    bool RangeBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const bool &bFull);
    bool RangeTempBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const uint32_t &minutes,
        const time_t &expiretime, const bool &bFull);
        
    bool RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash);
    bool RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash, unsigned char cType);
};
//---------------------------------------------------------------------------

#endif

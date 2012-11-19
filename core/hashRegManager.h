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
#ifndef hashRegManH
#define hashRegManH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct RegUser {
    char *sNick;
    char *sPass;
    time_t tLastBadPass;
    uint16_t iProfile;
    uint32_t ui32Hash;
    RegUser *prev, *next;
    RegUser *hashtableprev, *hashtablenext;
    unsigned char iBadPassCount;

    RegUser(char * Nick, char * Pass, const uint16_t &iProfile);
    ~RegUser(void);
}; 
//---------------------------------------------------------------------------

class hashRegMan {
private:
    RegUser *table[65536];

    void LoadXML();
public:
    RegUser *RegListS, *RegListE;

    hashRegMan(void);
    ~hashRegMan(void);

    bool AddNew(char * sNick, char * sPasswd, const uint16_t &iProfile);

    void Add(RegUser * Reg);
    void Add2Table(RegUser * Reg);
    static void ChangeReg(RegUser * pReg, char * sNewPasswd, const uint16_t &ui16NewProfile);
    void Delete(RegUser * pReg, const bool &bFromGui = false);
    void Rem(RegUser * Reg);
    void RemFromTable(RegUser * Reg);

    RegUser* Find(char * sNick, const size_t &szNickLen);
    RegUser* Find(User * u);
    RegUser* Find(uint32_t hash, char * sNick);

    void Load(void);
    void Save(void) const;
};

//--------------------------------------------------------------------------- 
extern hashRegMan *hashRegManager;
//---------------------------------------------------------------------------

#endif

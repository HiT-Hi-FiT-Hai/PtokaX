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
#ifndef hashRegManH
#define hashRegManH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct RegUser {
    char *sNick;
    char *sPass;
    time_t tLastBadPass;
    unsigned int iProfile;
    uint32_t ui32Hash;
    RegUser *prev, *next;
    RegUser *hashtableprev, *hashtablenext;
    unsigned char iBadPassCount;

    RegUser(char * Nick, char * Pass, const unsigned int &iProfile);
    ~RegUser(void);
}; 
//---------------------------------------------------------------------------

class hashRegMan {
private:
public:
    RegUser *RegListS, *RegListE;

    hashRegMan(void);
    ~hashRegMan(void);

    bool AddNewReg(char * sNick, char * sPasswd, const unsigned int &iProfile);

    void AddReg(RegUser * Reg);
    void RemReg(RegUser * Reg);

    void LoadRegList(void);
    void SaveRegList(void);
};

//--------------------------------------------------------------------------- 
extern hashRegMan *hashRegManager;
//---------------------------------------------------------------------------

#endif

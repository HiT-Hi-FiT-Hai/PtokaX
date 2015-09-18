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
#ifndef ResNickManagerH
#define ResNickManagerH
//---------------------------------------------------------------------------

class clsReservedNicksManager {
private:
    struct ReservedNick {
    	ReservedNick * pPrev, * pNext;

        char * sNick;

        uint32_t ui32Hash;

        bool bFromScript;

        ReservedNick();
        ~ReservedNick();

        ReservedNick(const ReservedNick&);
        const ReservedNick& operator=(const ReservedNick&);

        static ReservedNick * CreateReservedNick(const char * sNewNick, uint32_t ui32NickHash);
    };

	ReservedNick * pReservedNicks;

    clsReservedNicksManager(const clsReservedNicksManager&);
    const clsReservedNicksManager& operator=(const clsReservedNicksManager&);

	void Load();
	void Save();
	void LoadXML();
public:
    static clsReservedNicksManager * mPtr;

	clsReservedNicksManager();
	~clsReservedNicksManager();

    bool CheckReserved(const char * sNick, const uint32_t &hash) const;
    void AddReservedNick(const char * sNick, const bool &bFromScript = false);
    void DelReservedNick(char * sNick, const bool &bFromScript = false);
};
//---------------------------------------------------------------------------

#endif

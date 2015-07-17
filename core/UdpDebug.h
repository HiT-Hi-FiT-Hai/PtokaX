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
#ifndef UdpDebugH
#define UdpDebugH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class clsUdpDebug {
private:
	char * sDebugBuffer, * sDebugHead;

    struct UdpDbgItem {
        UdpDbgItem();
        ~UdpDbgItem();

        UdpDbgItem(const UdpDbgItem&);
        const UdpDbgItem& operator=(const UdpDbgItem&);

#ifdef _WIN32
        SOCKET s;
#else
		int s;
#endif
        sockaddr_storage sas_to;
        int sas_len;

        uint32_t ui32Hash;
        char * sNick;

        UdpDbgItem * pPrev, * pNext;
        bool bIsScript, bAllData;
    };

    clsUdpDebug(const clsUdpDebug&);
    const clsUdpDebug& operator=(const clsUdpDebug&);

	void CreateBuffer();
	void DeleteBuffer();
public:
    static clsUdpDebug * mPtr;

    UdpDbgItem * pDbgItemList;

	clsUdpDebug();
	~clsUdpDebug();

	void Broadcast(const char * msg, const size_t &szLen) const;
    void BroadcastFormat(const char * sFormatMsg, ...) const;
	bool New(User *u, const uint16_t &ui16Port);
	bool New(char * sIP, const uint16_t &ui16Port, const bool &bAllData, char * sScriptName);
	bool Remove(User * u);
	void Remove(char * sScriptName);
	bool CheckUdpSub(User * u, bool bSndMess = false) const;
	void Send(const char * sScriptName, char * sMsg, const size_t &szLen) const;
	void Cleanup();
	void UpdateHubName();
};
//---------------------------------------------------------------------------

#endif

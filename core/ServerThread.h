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
#ifndef ServerThreadH
#define ServerThreadH
//---------------------------------------------------------------------------

class ServerThread {
private:
    struct AntiConFlood {
        uint64_t ui64Time;

        AntiConFlood * pPrev, * pNext;

        int16_t ui16Hits;

        uint8_t ui128IpHash[16];

        explicit AntiConFlood(const uint8_t * pIpHash);

        AntiConFlood(const AntiConFlood&);
        const AntiConFlood& operator=(const AntiConFlood&);
    };

	AntiConFlood * pAntiFloodList;

#ifdef _WIN32
	HANDLE threadHandle;

	CRITICAL_SECTION csServerThread;

    SOCKET server;

    unsigned int threadId;

    uint32_t iSuspendTime;
#else
	pthread_t threadId;

	pthread_mutex_t mtxServerThread;

    int server;

	unsigned int iSuspendTime;
#endif

    int iAdressFamily;

	bool bTerminated;

	ServerThread(const ServerThread&);
	const ServerThread& operator=(const ServerThread&);
public:
    ServerThread * pPrev, * pNext;

    uint16_t ui16Port;

    bool bActive, bSuspended;

	ServerThread(const int &iAddrFamily, const uint16_t &ui16PortNumber);
	~ServerThread();

	void Resume();
	void Run();
	void Close();
	void WaitFor();
	bool Listen(bool bSilent = false);
#ifdef _WIN32
	bool isFlooder(const SOCKET &s, const sockaddr_storage &addr);
#else
	bool isFlooder(const int &s, const sockaddr_storage &addr);
#endif
	void RemoveConFlood(AntiConFlood * cur);
	void ResumeSck();
	void SuspendSck(const uint32_t &iTime);
};
//---------------------------------------------------------------------------

#endif

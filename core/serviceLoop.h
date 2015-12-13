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
#ifndef serviceLoopH
#define serviceLoopH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class clsServiceLoop {
private:
    uint64_t ui64LstUptmTck;

#ifdef _WIN32
    #ifdef _WIN_IOT
    	uint64_t ui64LastSecond;
    #endif
    CRITICAL_SECTION csAcceptQueue;
#else
	uint64_t ui64LastSecond;

	pthread_mutex_t mtxAcceptQueue;
#endif

    struct AcceptedSocket {
        sockaddr_storage addr;

        AcceptedSocket * pNext;

#ifdef _WIN32
        SOCKET s;
#else
		int s;
#endif

        AcceptedSocket();

        AcceptedSocket(const AcceptedSocket&);
        const AcceptedSocket& operator=(const AcceptedSocket&);
    };

	AcceptedSocket * pAcceptedSocketsS, * pAcceptedSocketsE;

	clsServiceLoop(const clsServiceLoop&);
	const clsServiceLoop& operator=(const clsServiceLoop&);

    static void AcceptUser(AcceptedSocket * AccptSocket);
protected:
public:
	double dLoggedUsers, dActualSrvLoopLogins;

#if defined(_WIN32) && !defined(_WIN_IOT)
    static UINT_PTR srvLoopTimer;
#else
	uint64_t ui64LastRegToHublist;
#endif

	static clsServiceLoop * mPtr;

    uint32_t ui32LastSendRest,  ui32SendRestsPeak,  ui32LastRecvRest,  ui32RecvRestsPeak,  ui32LoopsForLogins;

    bool bRecv;

	clsServiceLoop();
	~clsServiceLoop();

#ifdef _WIN32
	void AcceptSocket(const SOCKET &s, const sockaddr_storage &addr);
#else
	void AcceptSocket(const int &s, const sockaddr_storage &addr);
#endif
	void ReceiveLoop();
	void SendLoop();
	void Looper();
};
//---------------------------------------------------------------------------

#endif

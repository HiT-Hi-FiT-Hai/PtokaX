/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2014  Petr Kozelka, PPK at PtokaX dot org

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
    struct AcceptedSocket {
        AcceptedSocket();

        AcceptedSocket(const AcceptedSocket&);
        const AcceptedSocket& operator=(const AcceptedSocket&);

#ifdef _WIN32
        SOCKET s;
#else
		int s;
#endif

        sockaddr_storage addr;

        AcceptedSocket * pNext;
    };

    uint64_t ui64LstUptmTck;

#ifdef _WIN32
    CRITICAL_SECTION csAcceptQueue;
#else
	pthread_mutex_t mtxAcceptQueue;
#endif

	AcceptedSocket * pAcceptedSocketsS, * pAcceptedSocketsE;

#ifndef _WIN32
    int iSIG;

    siginfo_t info;

    sigset_t sst;

    struct timespec zerotime;
#endif

	char msg[1024];

	clsServiceLoop(const clsServiceLoop&);
	const clsServiceLoop& operator=(const clsServiceLoop&);

    void AcceptUser(AcceptedSocket * AccptSocket);
protected:
public:
    static clsServiceLoop * mPtr;

	clsServiceLoop();
	~clsServiceLoop();

#ifdef _WIN32
    static UINT_PTR srvLoopTimer;
#endif

    double dLoggedUsers, dActualSrvLoopLogins;

    uint32_t ui32LastSendRest,  ui32SendRestsPeak,  ui32LastRecvRest,  ui32RecvRestsPeak,  ui32LoopsForLogins;
    bool bRecv;

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

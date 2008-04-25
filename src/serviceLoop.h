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
#ifndef serviceLoopH
#define serviceLoopH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class theLoop {
private:
    struct AcceptedSocket {
        int s;
        sockaddr_in addr;
        socklen_t sin_len;
        AcceptedSocket *next;
        char IP[16];
    };

    uint64_t iLstUptmTck;

    pthread_mutex_t mtxAcceptQueue;

	AcceptedSocket *AcceptedSocketsS, *AcceptedSocketsE;

    int iSIG;

    siginfo_t info;

    sigset_t sst;

    struct timespec zerotime;

	char msg[1024];

    void AcceptUser(AcceptedSocket * AccptSocket);
protected:
public:
	theLoop();
	~theLoop();

    double dLoggedUsers, dActualSrvLoopLogins;

    unsigned int iLastSendRest, iSendRestsPeak, iLastRecvRest, iRecvRestsPeak, iLoopsForLogins;
    bool bRecv;

	void AcceptSocket(const int &s, const sockaddr_in &addr, const socklen_t &sin_len);
	void Terminate();
	void ReceiveLoop();
	void SendLoop();
	void Looper();
};
//---------------------------------------------------------------------------
extern theLoop *srvLoop;
//---------------------------------------------------------------------------

#endif

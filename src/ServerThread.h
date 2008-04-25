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
#ifndef ServerThreadH
#define ServerThreadH
//---------------------------------------------------------------------------

class ServerThread {
private:
    struct AntiConFlood {
        uint64_t Time;
        unsigned long addr;
        AntiConFlood *prev, *next;
        short hits;
    };

    int server;

    unsigned int iSuspendTime;

    pthread_t threadId;

    pthread_mutex_t mtxServerThread;

	AntiConFlood *AntiFloodList;
	
	bool bTerminated;
public:
    ServerThread *prev, *next;

    unsigned short usPort;

    bool bActive, bSuspended;

	ServerThread();
	~ServerThread();

	void Resume();
	void Run();
	void Close();
	void WaitFor();
	bool Listen(unsigned short port, bool bSilent = false);
	bool isFlooder(const int &s, const sockaddr_in &addr, const socklen_t &sin_len);
	void RemoveConFlood(AntiConFlood * cur);
	void ResumeSck();
	void SuspendSck(const unsigned int &iTime);
};
//---------------------------------------------------------------------------

#endif

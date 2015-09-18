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
#ifndef eventqueueH
#define eventqueueH
//---------------------------------------------------------------------------

class clsEventQueue {
private:
#ifdef _WIN32
	CRITICAL_SECTION csEventQueue;
#else
	pthread_mutex_t mtxEventQueue;
#endif

    struct event {
    	event * pPrev, * pNext;

        char * sMsg;

        uint8_t ui128IpHash[16];
        uint8_t ui8Id;

        event();

        event(const event&);
        const event& operator=(const event&);
    };

	event * pNormalE, * pThreadE;

	clsEventQueue(const clsEventQueue&);
	const clsEventQueue& operator=(const clsEventQueue&);
public:
    static clsEventQueue * mPtr;

	event * pNormalS, * pThreadS;

	enum {
        EVENT_RESTART, 
        EVENT_RSTSCRIPTS, 
        EVENT_RSTSCRIPT, 
        EVENT_STOPSCRIPT, 
        EVENT_STOP_SCRIPTING, 
        EVENT_SHUTDOWN, 
        EVENT_REGSOCK_MSG, 
        EVENT_SRVTHREAD_MSG, 
        EVENT_UDP_SR
	};

    clsEventQueue();
    ~clsEventQueue();

    void AddNormal(uint8_t ui8Id, char * sMsg);
    void AddThread(uint8_t ui8Id, char * sMsg, const sockaddr_storage * sas = NULL);
    void ProcessEvents();
};
//---------------------------------------------------------------------------

#endif

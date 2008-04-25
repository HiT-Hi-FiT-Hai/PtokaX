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
#ifndef eventqueueH
#define eventqueueH
//---------------------------------------------------------------------------

class eventq {
private:
    struct event {
        char * sMsg;

        event *prev, *next;

        uint32_t ui32Hash;
        uint8_t ui8Id;
    };

    event *NormalE, *ThreadE;

	pthread_mutex_t mtxEventQueue;
public:
	enum {
        EVENT_RESTART, 
        EVENT_RSTSCRIPTS, 
        EVENT_RSTSCRIPT, 
        EVENT_STOPSCRIPT, 
        EVENT_STOP_SCRIPTING, 
        EVENT_SHUTDOWN, 
        EVENT_REGSOCK_MSG, 
        EVENT_SRVTHREAD_MSG, 
        EVENT_UDP_SR, 
	};

    event *NormalS, *ThreadS;

    eventq();
    ~eventq();

    void AddNormal(uint8_t ui8Id, char * sMsg);
    void AddThread(uint8_t ui8Id, char * sMsg, const uint32_t &ui32Hash = 0);
    void ProcessEvents();
};

//---------------------------------------------------------------------------
extern eventq *eventqueue;
//---------------------------------------------------------------------------

#endif

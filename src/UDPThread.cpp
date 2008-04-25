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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#include "UDPThread.h"
//---------------------------------------------------------------------------
UDPRecvThread *UDPThread = NULL;
//---------------------------------------------------------------------------

UDPRecvThread::UDPRecvThread() { 
    threadId = 0;

	bTerminated = false;

    sockaddr_in sin;
    sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)atoi(SettingManager->sTexts[SETTXT_UDP_PORT]));

    if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && sHubIP[0] != '\0') {
        sin.sin_addr.s_addr = inet_addr(sHubIP);
    } else {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(sock == -1) {
		AppendLog("[ERR] UDP Socket creation error.");
    } else if(bind(sock, (sockaddr *)&sin, sizeof (sin)) == -1) {
		AppendLog("[ERR] UDP Socket bind error: "+string(ErrnoStr(errno))+" ("+string(errno)+")");
    }
}
//---------------------------------------------------------------------------

UDPRecvThread::~UDPRecvThread() {
    if(threadId != 0) {
        Close();
        WaitFor();
    }
}
//---------------------------------------------------------------------------

static void* ExecuteUDP(void* UDPThrd) {
	((UDPRecvThread *)UDPThrd)->Run();
	return 0;
}
//---------------------------------------------------------------------------

void UDPRecvThread::Resume() {
	int iRet = pthread_create(&threadId, NULL, ExecuteUDP, this);
	if(iRet != 0) {
		AppendSpecialLog("[ERR] Failed to create new UDPThread!");
    }
}
//---------------------------------------------------------------------------

void UDPRecvThread::Run() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

	while(bTerminated == false) {
		int len = recvfrom(sock, rcvbuf, 4095, 0, (sockaddr *)&addr, &addr_len);

        if(len < 5 || strncmp(rcvbuf, "$SR ", 4) != 0) {
            continue;
        }

        rcvbuf[len] = '\0';

        // added ip check, we don't want fake $SR causing kick of innocent user...
        uint32_t ui32Hash;
        char * sIP = inet_ntoa(addr.sin_addr);
        HashIP(sIP, strlen(sIP), ui32Hash);

        eventqueue->AddThread(eventq::EVENT_UDP_SR, rcvbuf, ui32Hash);
    }
}
//---------------------------------------------------------------------------

void UDPRecvThread::Close() {
	bTerminated = true;
	shutdown(sock, SHUT_RDWR);
	close(sock);
}
//---------------------------------------------------------------------------

void UDPRecvThread::WaitFor() {
	if(threadId != 0) {
		pthread_join(threadId, NULL);
        threadId = 0;
	}
}
//---------------------------------------------------------------------------

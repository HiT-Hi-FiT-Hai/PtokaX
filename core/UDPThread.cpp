/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2010  Petr Kozelka, PPK at PtokaX dot org

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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
UDPRecvThread *UDPThread = NULL;
//---------------------------------------------------------------------------

UDPRecvThread::UDPRecvThread() {
#ifdef _WIN32
    sock = INVALID_SOCKET;
#else
    sock = -1;
#endif

    rcvbuf[0] = '\0';

    threadId = 0;

#ifdef _WIN32
    threadHandle = INVALID_HANDLE_VALUE;
#endif

	bTerminated = false;
}

bool UDPRecvThread::Listen() {
    sockaddr_in sin;
    sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)atoi(SettingManager->sTexts[SETTXT_UDP_PORT]));

    if(SettingManager->bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && sHubIP[0] != '\0') {
#ifdef _WIN32
        sin.sin_addr.S_un.S_addr = inet_addr(sHubIP);
#else
		sin.sin_addr.s_addr = inet_addr(sHubIP);
#endif
    } else {
#ifdef _WIN32
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    }

#ifdef _WIN32
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sock == INVALID_SOCKET) {
#else
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(sock == -1) {
#endif
		AppendLog("[ERR] UDP Socket creation error.");
		return false;
#ifdef _WIN32
    } else if(bind(sock, (sockaddr *)&sin, sizeof (sin)) == SOCKET_ERROR) {
		AppendLog("[ERR] UDP Socket bind error: "+string(WSAGetLastError()));
#else
    } else if(bind(sock, (sockaddr *)&sin, sizeof (sin)) == -1) {
		AppendLog("[ERR] UDP Socket bind error: "+string(ErrnoStr(errno))+" ("+string(errno)+")");
#endif
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

UDPRecvThread::~UDPRecvThread() {
#ifdef _WIN32
    if(sock != INVALID_SOCKET) {
		closesocket(sock);

        sock = INVALID_SOCKET;
    }

    if(threadHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(threadHandle);
#else
    if(threadId != 0) {
        Close();
        WaitFor();
#endif
	}
}
//---------------------------------------------------------------------------

#ifdef _WIN32
unsigned __stdcall ExecuteUDP(void* UDPThrd) {
#else
static void* ExecuteUDP(void* UDPThrd) {
#endif
	((UDPRecvThread *)UDPThrd)->Run();
	return 0;
}
//---------------------------------------------------------------------------

void UDPRecvThread::Resume() {
#ifdef _WIN32
	threadHandle = (HANDLE)_beginthreadex(NULL, 0, ExecuteUDP, this, 0, &threadId);
	if(threadHandle == 0) {
#else
	int iRet = pthread_create(&threadId, NULL, ExecuteUDP, this);
	if(iRet != 0) {
#endif
		AppendSpecialLog("[ERR] Failed to create new UDPThread!");
    }
}
//---------------------------------------------------------------------------

void UDPRecvThread::Run() {
#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			try {
		#endif
	#endif
#endif
	sockaddr_in addr;
#ifdef _WIN32
	int addr_len = sizeof(addr);
#else
	socklen_t addr_len = sizeof(addr);
#endif

	while(bTerminated == false) {
		int len = recvfrom(sock, rcvbuf, 4095, 0, (sockaddr *)&addr, &addr_len);

		if(len < 5 || strncmp(rcvbuf, "$SR ", 4) != 0) {
			continue;
		}

		rcvbuf[len] = '\0';

		// added ip check, we don't want fake $SR causing kick of innocent user...
#ifdef _WIN32
		uint32_t ui32Hash = 16777216 * addr.sin_addr.S_un.S_un_b.s_b1 + 65536 * addr.sin_addr.S_un.S_un_b.s_b2
			+ 256 * addr.sin_addr.S_un.S_un_b.s_b3 + addr.sin_addr.S_un.S_un_b.s_b4;
#else
        uint32_t ui32Hash;
        char * sIP = inet_ntoa(addr.sin_addr);
        HashIP(sIP, strlen(sIP), ui32Hash);
#endif

            eventqueue->AddThread(eventq::EVENT_UDP_SR, rcvbuf, ui32Hash);
        }

#ifdef _WIN32
	#ifndef _SERVICE
		#ifndef _MSC_VER
			} catch(Exception &e) {
				AppendSpecialLog("[ERR] Exception in udp thread: " + string(e.Message.c_str(), e.Message.Length()));
				Application->ShowException(&e);
			} catch(...) {
				try {
					throw Exception("");
				}
				catch(Exception &exception) {
					Application->ShowException(&exception);
				}
			}
		#endif
	#endif
#endif
}
//---------------------------------------------------------------------------

void UDPRecvThread::Close() {
	bTerminated = true;
#ifdef _WIN32
	closesocket(sock);
#else
	shutdown(sock, SHUT_RDWR);
	close(sock);
#endif
}
//---------------------------------------------------------------------------

void UDPRecvThread::WaitFor() {
#ifdef _WIN32
    WaitForSingleObject(threadHandle, INFINITE);
#else
	if(threadId != 0) {
		pthread_join(threadId, NULL);
        threadId = 0;
	}
#endif
}
//---------------------------------------------------------------------------

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
#ifndef RegThreadH
#define RegThreadH
//---------------------------------------------------------------------------

class clsRegisterThread {
private:
    struct RegSocket {
        RegSocket();
        ~RegSocket();

        RegSocket(const RegSocket&);
        const RegSocket& operator=(const RegSocket&);

        uint64_t ui64TotalShare;

#ifdef _WIN32
        SOCKET sock;
#else
		int sock;
#endif

        uint32_t ui32RecvBufLen, ui32RecvBufSize, ui32SendBufLen, ui32TotalUsers;

        uint32_t ui32AddrLen;

		char * sAddress, * pRecvBuf, * pSendBuf, * pSendBufHead;

        RegSocket * pPrev, * pNext;
    };

#ifdef _WIN32
    unsigned int threadId;

    HANDLE threadHandle;
#else
	pthread_t threadId;
#endif

    RegSocket * pRegSockListS, * pRegSockListE;

    bool bTerminated;

    char sMsg[2048];

	clsRegisterThread(const clsRegisterThread&);
	const clsRegisterThread& operator=(const clsRegisterThread&);

	void AddSock(char * sAddress, const size_t &ui32Len);
	bool Receive(RegSocket * pSock);
    static void Add2SendBuf(RegSocket * pSock, char * sData);
    bool Send(RegSocket * pSock);
    void RemoveSock(RegSocket * pSock);
public:
    static clsRegisterThread * mPtr;

    uint32_t ui32BytesRead, ui32BytesSent;

	clsRegisterThread();
	~clsRegisterThread();

	void Setup(char * sAddresses, const uint16_t &ui16AddrsLen);
	void Resume();
	void Run();
	void Close();
	void WaitFor();
};
//---------------------------------------------------------------------------

#endif

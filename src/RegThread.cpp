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
#include "RegThread.h"
//---------------------------------------------------------------------------
RegThread *RegisterThread = NULL;
//---------------------------------------------------------------------------

RegThread::RegSocket::~RegSocket() {
    if(sAddress != NULL) {
        free(sAddress);
        sAddress = NULL;
    }

    if(sRecvBuf != NULL) {
        free(sRecvBuf);
        sRecvBuf = NULL;
    }

    if(sSendBuf != NULL) {
        free(sSendBuf);
        sSendBuf = NULL;
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
}
//---------------------------------------------------------------------------

RegThread::RegThread() {  
    bTerminated = false;

    threadId = 0;

    iBytesRead = 0;
    iBytesSent = 0;
       
    RegSockListS = NULL;
    RegSockListE = NULL;
}
//---------------------------------------------------------------------------

RegThread::~RegThread() {
    if(threadId != 0) {
        Close();
        WaitFor();
    }

	ui64BytesRead += (uint64_t)iBytesRead;
    ui64BytesSent += (uint64_t)iBytesSent;
    
    RegSocket *next = RegSockListS;
        
    while(next != NULL) {
        RegSocket *cur = next;
        next = cur->next;
                       
        delete cur;
    }
}
//---------------------------------------------------------------------------

void RegThread::Setup(char * sListAddresses, const uint16_t &ui16AddrsLen) {
    // parse all addresses and create individul RegSockets
    char *sAddresses = (char *) malloc(ui16AddrsLen+1);
    if(sAddresses == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(ui16AddrsLen+1)+
			" bytes of memory for sAddresses in RegThread::Setup!";
        AppendSpecialLog(sDbgstr);
        return;
    }

    memcpy(sAddresses, sListAddresses, ui16AddrsLen);
    sAddresses[ui16AddrsLen] = '\0';

    char *sAddress = sAddresses;

    for(uint16_t i = 0; i < ui16AddrsLen; i++) {
        if(sAddresses[i] == ';') {
            sAddresses[i] = '\0';
        } else if(i != ui16AddrsLen-1) {
            continue;
        } else {
            i++;
        }

        AddSock(sAddress, (sAddresses+i)-sAddress);
        sAddress = sAddresses+i+1;
    }

	free(sAddresses);
}
//---------------------------------------------------------------------------

void RegThread::AddSock(char * sAddress, const size_t &ui32Len) {
    RegSocket * newsock = new RegSocket();
    if(newsock == NULL) {
		string sDbgstr = "[BUF] Cannot allocate newsock in RegThread::AddSock!";
		AppendSpecialLog(sDbgstr);
    	return;
    }

    newsock->prev = NULL;
    newsock->next = NULL;

    // Initialise socket we want to use for connect
    if((newsock->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        int iMsgLen = sprintf(sMsg, "[REG] RegSock create error %d. (%s)", errno, newsock->sAddress);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::AddSock1") == true) {
			AppendSpecialLog(string(sMsg, iMsgLen));
        }
		delete newsock;

        return;
    }

    // Set the receive buffer
    int bufsize = 1024;
    if(setsockopt(newsock->sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
        int iMsgLen = sprintf(sMsg, "[REG] RegSock recv buff error %d. (%s)", errno, newsock->sAddress);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::AddSock2") == true) {
			AppendSpecialLog(string(sMsg, iMsgLen));
        }

        delete newsock;
        return;
    }

    // Set the send buffer
    bufsize = 2048;
    if(setsockopt(newsock->sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        int iMsgLen = sprintf(sMsg, "[REG] RegSock send buff error %d. (%s)", errno, newsock->sAddress);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::AddSock3") == true) {
			AppendSpecialLog(string(sMsg, iMsgLen));
        }

        delete newsock;
        return;
    }

    // Set non-blocking mode
    int oldFlag = fcntl(newsock->sock, F_GETFL, 0);
    if(fcntl(newsock->sock, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
        int iMsgLen = sprintf(sMsg, "[REG] RegSock non-block error %d. (%s)", errno, newsock->sAddress);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::AddSock4") == true) {
			AppendSpecialLog(string(sMsg, iMsgLen));
        }

        delete newsock;
        return;
    }

    newsock->sAddress = (char *) malloc(ui32Len+1);
    if(newsock->sAddress == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(ui32Len+1))+
			" bytes of memory for sAddress in RegThread::AddSock!";
		AppendSpecialLog(sDbgstr);
        return;
    }

    newsock->ui32AddrLen = (uint32_t)ui32Len;

    memcpy(newsock->sAddress, sAddress, newsock->ui32AddrLen);
    newsock->sAddress[newsock->ui32AddrLen] = '\0';
    
    newsock->sRecvBuf = (char *) malloc(256);
    if(newsock->sRecvBuf == NULL) {
		string sDbgstr = "[BUF] Cannot allocate 256 bytes of memory for sRecvBuf in RegThread::AddSock!";
		AppendSpecialLog(sDbgstr);
        return;
    }

    newsock->sSendBuf = NULL;
    newsock->sSendBufHead = NULL;

    newsock->iRecvBufLen = 0;
    newsock->iRecvBufSize = 255;
    newsock->iSendBufLen = 0;

    newsock->iTotalUsers = 0;
    newsock->iTotalShare = 0;

    if(RegSockListS == NULL) {
        newsock->prev = NULL;
        newsock->next = NULL;
        RegSockListS = newsock;
        RegSockListE = newsock;
    } else {
        newsock->prev = RegSockListE;
        newsock->next = NULL;
        RegSockListE->next = newsock;
        RegSockListE = newsock;
    }
}
//---------------------------------------------------------------------------

static void* ExecuteRegThread(void* RThread) {
	((RegThread *)RThread)->Run();
	return 0;
}
//---------------------------------------------------------------------------

void RegThread::Resume() {
    int iRet = pthread_create(&threadId, NULL, ExecuteRegThread, this);
    if(iRet != 0) {
		AppendSpecialLog("[ERR] Failed to create new RegisterThread!");
    }
}
//---------------------------------------------------------------------------

void RegThread::Run() {
    RegSocket *next = RegSockListS;
    while(bTerminated == false && next != NULL) {
        RegSocket *cur = next;
        next = cur->next;

        // IP address and port of the server we want to connect
        sockaddr_in target;
        target.sin_family = AF_INET;
        target.sin_port = htons(2501);

        char *port = strchr(cur->sAddress, ':');    
        if(port != NULL) {
            port[0] = '\0';
                int32_t iPort = atoi(port+1);
            if(iPort >= 0 && iPort <= 65535) {
                target.sin_port = htons((unsigned short)iPort);
            }
        }

        if(isIP(cur->sAddress, cur->ui32AddrLen) == true) {
            target.sin_addr.s_addr = inet_addr(cur->sAddress);
        } else {
            hostent *host = gethostbyname(cur->sAddress);
            if(host != NULL) {
                target.sin_addr.s_addr = *(unsigned long *)host->h_addr;
            } else {
                int iMsgLen = sprintf(sMsg, "[REG] RegSock resolve error (%s)", cur->sAddress);
                if(CheckSprintf(iMsgLen, 2048, "RegThread::Execute1") == true) {
                    eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
                }

                RemoveSock(cur);
                delete cur;
                continue;
            }
        }

        if(port != NULL) {
            port[0] = ':';
        }

        // Finally, time to connect ! ;)
        if(connect(cur->sock, (sockaddr *)&target, sizeof(target)) == -1) {
            if(errno != EINPROGRESS) {
                int iMsgLen = sprintf(sMsg, "[REG] RegSock connect error %s (%d). (%s)", ErrnoStr(errno), errno, cur->sAddress);
                if(CheckSprintf(iMsgLen, 2048, "RegThread::Execute2") == true) {
                    eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
                }

                RemoveSock(cur);
                delete cur;
                continue;
            }
        }

        usleep(1000);
    }

        uint16_t iLoops = 0;

    while(RegSockListS != NULL && bTerminated == false && iLoops < 4000) {   
        iLoops++;
  
        next = RegSockListS;
     
        while(next != NULL) {
            RegSocket *cur = next;
            next = cur->next;

            if(Receive(cur) == false) {
                RemoveSock(cur);
                delete cur;
                continue;
            }

            if(cur->iSendBufLen > 0) {
                if(Send(cur) == false) {
                    RemoveSock(cur);
                    delete cur;
                    continue;
                }
            }
        }
        usleep(75000);
    }

    next = RegSockListS;
    while(next != NULL) {
        RegSocket *cur = next;
        next = cur->next;

        if(bTerminated == false) {
            int iMsgLen = sprintf(sMsg, "[REG] RegSock timeout. (%s)", cur->sAddress);
            if(CheckSprintf(iMsgLen, 2048, "RegThread::Execute3") == true) {
                eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
            }
        }

        delete cur;
    }

    RegSockListS = NULL;
	RegSockListE = NULL;
}
//---------------------------------------------------------------------------

void RegThread::Close() {
    bTerminated = true;
}
//---------------------------------------------------------------------------

void RegThread::WaitFor() {
	if(threadId != 0) {
		pthread_join(threadId, NULL);
        threadId = 0;
	}
}
//---------------------------------------------------------------------------

bool RegThread::Receive(RegSocket * Sock) {
    if(Sock->sRecvBuf == NULL) {
        return true;
    }
    
	int32_t i32bytes = 0;
	if(ioctl(Sock->sock, FIONREAD, &i32bytes) == -1) {
	    int iMsgLen = sprintf(sMsg, "[REG] RegSock ioctl(FIONREAD) error %s (%d). (%s)", ErrnoStr(errno), errno, Sock->sAddress);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive0") == true) {
            eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
        }
        return false;
    }
    
    if(i32bytes == 0) {
        // we need to ry receive to catch connection error, or if user closed connection
        i32bytes = 16;
    } else if(i32bytes > 1024) {
        // receive max. 1024 bytes to receive buffer
        i32bytes = 1024;
    }

    if(Sock->iRecvBufSize < Sock->iRecvBufLen+i32bytes) {
        int iAllignLen = ((Sock->iRecvBufLen+i32bytes+1) & 0xFFFFFE00) + 0x200;
        if(iAllignLen > 2048) {
            int iMsgLen = sprintf(sMsg, "[REG] RegSock receive buffer overflow. (%s)", Sock->sAddress);
            if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive5") == true) {
                eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
            }
            return false;
        }

        Sock->sRecvBuf = (char *) realloc(Sock->sRecvBuf, iAllignLen);
        if(Sock->sRecvBuf == NULL) {
			string sDbgstr = "[BUF] Cannot reallocate "+string(i32bytes)+"/"+string(Sock->iRecvBufLen)+"/"+string(iAllignLen)+
				" bytes of memory for sRecvBuf in RegThread::Receive!";
			eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sDbgstr.c_str());
            return false;
        }

        Sock->iRecvBufSize = iAllignLen-1;
    }

    int iBytes = recv(Sock->sock, Sock->sRecvBuf+Sock->iRecvBufLen, Sock->iRecvBufSize-Sock->iRecvBufLen, 0);

    if(iBytes == -1) {
        if(errno != EAGAIN) {
			if(errno == ENOTCONN) {
				int iErr = 0;
            	socklen_t iErrLen = sizeof(iErr);
            	int iRet = getsockopt(Sock->sock, SOL_SOCKET, SO_ERROR, (char *) &iErr, &iErrLen);
				if(iRet == -1) {
					int iMsgLen = sprintf(sMsg, "[REG] RegSock getsockopt error %d. (%s)", iRet, Sock->sAddress);
					if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive1") == true) {
						eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
					}

					return false;
				} else if(iErr != 0) {
					int iMsgLen = sprintf(sMsg, "[REG] RegSock connect error %d. (%s)", iErr, Sock->sAddress);
					if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive2") == true) {
						eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
					}

					return false;
				}
				return true;
			}

            int iMsgLen = sprintf(sMsg, "[REG] RegSock recv error %s (%d). (%s)", ErrnoStr(errno), errno, Sock->sAddress);
            if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive3") == true) {
                eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
            }
            return false;
        } else {
            return true;
        }
    } else if(iBytes == 0) {
        int iMsgLen = sprintf(sMsg, "[REG] RegSock closed connection by server. (%s)", Sock->sAddress);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive4") == true) {
            eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
        }
        return false;
    }
    
    iBytesRead += iBytes;

    Sock->iRecvBufLen += iBytes;
    Sock->sRecvBuf[Sock->iRecvBufLen] = '\0';
    char *sBuffer = Sock->sRecvBuf;

    for(uint32_t i = 0; i < Sock->iRecvBufLen; i++) {
        if(Sock->sRecvBuf[i] == '|') {
            uint32_t iCommandLen = (uint32_t)(((Sock->sRecvBuf+i)-sBuffer)+1);
            if(strncmp(sBuffer, "$Lock ", 6) == 0) {
                sockaddr_in addr;
                socklen_t sin_len = sizeof(addr);

                if(getsockname(Sock->sock, (sockaddr *) &addr, &sin_len) == -1) {
                    int iMsgLen = sprintf(sMsg, "[REG] RegSock local port error %d. (%s)", errno, Sock->sAddress);
                    if(CheckSprintf(iMsgLen, 2048, "RegThread::Receive6") == true) {
                        eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
                    }
                    return false;
                }

                uint16_t lport =  (uint16_t)ntohs(addr.sin_port);
                char cMagic = (char) ((lport&0xFF)+((lport>>8)&0xFF));

                // strip the Lock data
                char *temp;
                if((temp = strchr(sBuffer+6, ' ')) != NULL) {
                    temp[0] = '\0';
                }

                // Compute the key
                memcpy(sMsg, "$Key ", 5);
                sMsg[5] = '\0';
                size_t iLen = temp-sBuffer;
                char v;

                // first make the crypting stuff
                for(size_t i = 6; i < iLen; i++) {
                    if(i == 6) {
                        v = sBuffer[i] ^ sBuffer[iLen] ^ sBuffer[iLen-1] ^ cMagic;
                    } else {
                        v = sBuffer[i] ^ sBuffer[i-1];
                    }

                    // Swap nibbles (0xF0 = 11110000, 0x0F = 00001111)
                    v = (char)(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));

                    switch(v) {
                        case 0:
                            strcat(sMsg, "/%DCN000%/");
                            break;
                        case 5:
                            strcat(sMsg, "/%DCN005%/");
                            break;
                        case 36:
                            strcat(sMsg, "/%DCN036%/");
                            break;
                        case 96:
                            strcat(sMsg, "/%DCN096%/");
                            break;
                        case 124:
                            strcat(sMsg, "/%DCN124%/");
                            break;
                        case 126:
                            strcat(sMsg, "/%DCN126%/");
                            break;
                        default:
                            strncat(sMsg, (char *)&v, 1);
                            break;
                    }
                }

                Sock->iTotalUsers = ui32Logged;
                Sock->iTotalShare = ui64TotalShare;

                strcat(sMsg, "|");
                SettingManager->GetText(SETTXT_HUB_NAME, sMsg);
                strcat(sMsg, "|");
                SettingManager->GetText(SETTXT_HUB_ADDRESS, sMsg);
                uint16_t iFirstPort = SettingManager->GetFirstPort();
                if(iFirstPort != 411) {
                    strcat(sMsg, ":");
                    strcat(sMsg, string(iFirstPort).c_str());
                }
                strcat(sMsg, "|");
                SettingManager->GetText(SETTXT_HUB_DESCRIPTION, sMsg);
                strcat(sMsg, "     |");
                iLen = strlen(sMsg);
                *(sMsg+iLen-4) = 'x';
                *(sMsg+iLen-3) = '.';
                *(sMsg+iLen-5) = 'p';
                *(sMsg+iLen-6) = '.';
                if(SettingManager->GetBool(SETBOOL_ANTI_MOGLO) == true) {
                    *(sMsg+iLen-2) = (char)160;
                }
                strcat(sMsg, string(Sock->iTotalUsers).c_str());
                strcat(sMsg, "|");
                strcat(sMsg, string(Sock->iTotalShare).c_str());
                strcat(sMsg, "|");
                Add2SendBuf(Sock, sMsg);

                free(Sock->sRecvBuf);
                Sock->sRecvBuf = NULL;
                Sock->iRecvBufLen = 0;
                Sock->iRecvBufSize = 0;
                return true;
            }
            sBuffer += iCommandLen;
        }
    }

    Sock->iRecvBufLen -= (uint32_t)(sBuffer-Sock->sRecvBuf);

    if(Sock->iRecvBufLen == 0) {
        Sock->sRecvBuf[0] = '\0';
    } else if(Sock->iRecvBufLen != 1) {
        memmove(Sock->sRecvBuf, sBuffer, Sock->iRecvBufLen);
        Sock->sRecvBuf[Sock->iRecvBufLen] = '\0';
    } else {
        if(sBuffer[0] == '|') {
            Sock->sRecvBuf[0] = '\0';
            Sock->iRecvBufLen = 0;
        } else {
            Sock->sRecvBuf[0] = sBuffer[0];
            Sock->sRecvBuf[1] = '\0';
        }
    }
    return true;
}
//---------------------------------------------------------------------------

void RegThread::Add2SendBuf(RegSocket * Sock, char * sData) {
    size_t iLen = strlen(sData);
    
    Sock->sSendBuf = (char *) malloc(iLen+1);
    if(Sock->sSendBuf == NULL) {
        string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(iLen+1))+
			" bytes of memory for sSendBuf in RegThread::Add2SendBuf!";
        eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sDbgstr.c_str());
        return;
    }

    Sock->sSendBufHead = Sock->sSendBuf;

    memcpy(Sock->sSendBuf, sData, iLen);
    Sock->iSendBufLen = (uint32_t)iLen;
    Sock->sSendBuf[Sock->iSendBufLen] = '\0';
}
//---------------------------------------------------------------------------

bool RegThread::Send(RegSocket * Sock) {
    // compute length of unsent data
    uint32_t iOffset = (uint32_t)(Sock->sSendBufHead - Sock->sSendBuf);
    int iLen = Sock->iSendBufLen - iOffset;

    int iBytes = send(Sock->sock, Sock->sSendBufHead, iLen, 0);

    if(iBytes == -1) {
        if(errno != EAGAIN) {
            int iMsgLen = sprintf(sMsg, "[REG] RegSock send error %s (%d). (%s)", ErrnoStr(errno), errno, Sock->sAddress);
            if(CheckSprintf(iMsgLen, 2048, "RegThread::Send1") == true) {
                eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
            }
            return false;
        } else {
            return true;
        }
    }

    iBytesSent += iBytes;

	if(iBytes < iLen) {
        Sock->sSendBufHead += iBytes;
		return true;
	} else {
        int iMsgLen = sprintf(sMsg, "[REG] Hub is registered on %s hublist (Users: %u, Share: %" PRIu64 ")", 
            Sock->sAddress, ui32Logged, ui64TotalShare);
        if(CheckSprintf(iMsgLen, 2048, "RegThread::Send2") == true) {
            eventqueue->AddThread(eventq::EVENT_REGSOCK_MSG, sMsg);
        }
        // Here is only one send, and when is all send then is time to delete this sock... false do it for us ;)
		return false;
	}
}
//---------------------------------------------------------------------------

void RegThread::RemoveSock(RegSocket * Sock) {
    if(Sock->prev == NULL) {
        if(Sock->next == NULL) {
            RegSockListS = NULL;
            RegSockListE = NULL;
        } else {
            Sock->next->prev = NULL;
            RegSockListS = Sock->next;
        }
    } else if(Sock->next == NULL) {
        Sock->prev->next = NULL;
        RegSockListE = Sock->prev;
    } else {
        Sock->prev->next = Sock->next;
        Sock->next->prev = Sock->prev;
    }
}
//---------------------------------------------------------------------------

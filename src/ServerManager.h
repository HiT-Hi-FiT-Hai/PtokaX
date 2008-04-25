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
#ifndef ServerManagerH
#define ServerManagerH
//---------------------------------------------------------------------------
class ServerThread;
//---------------------------------------------------------------------------

void ServerInitialize();

void ServerFinalStop(const bool &bFromServiceLoop);
void ServerResumeAccepts();
void ServerSuspendAccepts(const unsigned int &iTime);
void ServerUpdateAutoRegState();

bool ServerStart();
void ServerUpdateServers();
void ServerStop();
void ServerFinalClose();
//---------------------------------------------------------------------------
extern double CpuUsage[60], cpuUsage;
extern uint64_t ui64ActualTick, ui64TotalShare;
extern uint64_t ui64BytesRead, ui64BytesSent, ui64BytesSentSaved;
extern uint64_t iLastBytesRead, iLastBytesSent;
extern uint32_t ui32Joins, ui32Parts, ui32Logged, ui32Peak;
extern unsigned long UploadSpeed[60], DownloadSpeed[60];
extern unsigned long iActualBytesRead, iActualBytesSent;
extern unsigned long iAverageBytesRead, iAverageBytesSent;
extern ServerThread *ServersS;
extern time_t starttime;
extern unsigned int iMins, iHours, iDays;
extern bool bServerRunning, bServerTerminated, bIsRestart, bIsClose, bDaemon;
extern char sHubIP[16];
extern uint8_t ui8SrCntr, ui8MinTick;
//--------------------------------------------------------------------------- 

#endif

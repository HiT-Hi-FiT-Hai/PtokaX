/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2011  Petr Kozelka, PPK at PtokaX dot org

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
#ifndef stdincH
#define stdincH
//---------------------------------------------------------------------------
#ifndef _WIN32
	#define _REENTRANT 1
	#define __STDC_FORMAT_MACROS 1
#endif
//---------------------------------------------------------------------------
#include <stdlib.h>
#ifdef _WIN32
	#include <malloc.h>
#endif
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#ifdef _WIN32
	#include <dos.h>
	#ifndef _MSC_VER
		#include <dir.h>
	#else
		#pragma warning(disable: 4996) // Deprecated stricmp
	
		typedef signed __int8 int8_t;
		typedef signed __int16 int16_t;
		typedef signed __int32 int32_t;
		typedef signed __int64 int64_t;
	
		typedef unsigned __int8 uint8_t;
		typedef unsigned __int16 uint16_t;
		typedef unsigned __int32 uint32_t;
		typedef unsigned __int64 uint64_t;
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#ifdef _BUILD_GUI
		#include <commctrl.h>
		#include <RichEdit.h>
		#include <Windowsx.h>
	#endif
#endif
#include <locale.h>
#include <time.h>
#ifdef _WIN32
	#include <process.h>
#else
	#include <unistd.h>
	#include <errno.h>
	#include <dirent.h>
	#include <fcntl.h>
	#include <inttypes.h>
	#include <limits.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <signal.h>
	#include <sys/ioctl.h>
	#if defined (__SVR4) && defined (__sun)
	   #include <sys/filio.h>
	#endif
	#include <sys/resource.h> 
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/stat.h>
	#include <sys/utsname.h>
	#include <syslog.h>
#endif
#include <tinyxml.h>
#include <tinystr.h>
#ifdef _WIN32
	#include <psapi.h>
	#include <io.h>
	#include <Iphlpapi.h>
#endif
#include "pxstring.h"
//---------------------------------------------------------------------------
#define PtokaXVersionString "0.4.1.2"
#define BUILD_NUMBER "200"
#define sErrOutOfMem "[ERR] Out of memory..."

#ifdef _WIN32
    #define NEW_LINE_CHARS "\r\n"
#else
    #define NEW_LINE_CHARS "\n"
#endif

#ifdef _BUILD_GUI
    extern HINSTANCE g_hInstance;
    extern HWND g_hWndActiveDialog;
#endif
//---------------------------------------------------------------------------

#endif

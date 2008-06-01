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
#ifndef stdincH
#define stdincH
//---------------------------------------------------------------------------
#define _REENTRANT 1
#define __STDC_FORMAT_MACROS 1
//---------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
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
#include <sys/resource.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <tinyxml.h>
#include <tinystr.h>
#include "pxstring.h"
//---------------------------------------------------------------------------
#define PtokaXVersionString "0.4.1.0"
#define sErrOutOfMem "[ERR] Out of memory..."
//---------------------------------------------------------------------------

#endif

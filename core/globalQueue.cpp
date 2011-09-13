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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "globalQueue.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "ProfileManager.h"
#include "serviceLoop.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
//---------------------------------------------------------------------------
	#ifndef _MSC_VER
		#pragma package(smart_init)
	#endif
#endif
//---------------------------------------------------------------------------
globalqueue *globalQ = NULL;
//---------------------------------------------------------------------------

globalqueue::globalqueue() {
    msg[0] = '\0';

    bActive = true;

    bHaveQueue = false;

    bHaveIP = false;
    
    // active queues
    bHaveaQueue = false;
    bHaveaS = false;
    bHaveaH = false;
    bHaveaA = false;
    bHaveaP = false;
    bHaveaI = false;
    bHaveaStrp = false;
    bHaveaFull = false;
    bHaveaOp = false;

    //pasive queues
    bHavebQueue = false;
    bHavebS = false;
    bHavebH = false;
    bHavebA = false;
    bHavebP = false;
    bHavebI = false;
    bHavebStrp = false;
    bHavebFull = false;
    bHavebOp = false;

    // OpList buffer
#ifdef _WIN32
    OpListQueue.buffer = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
	OpListQueue.buffer = (char *) calloc(256, 1);
#endif
	if(OpListQueue.buffer == NULL) {
		AppendSpecialLog("Cannot create global output oplistqueue!");
		return;
	}
	OpListQueue.len = 0;
    OpListQueue.size = 255;
    
    // UserIP buffer
#ifdef _WIN32
    UserIPQueue.buffer = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
	UserIPQueue.buffer = (char *) calloc(256, 1);
#endif
	if(UserIPQueue.buffer == NULL) {
		AppendSpecialLog("Cannot create global output useripqueue!");
		return;
	}
    UserIPQueue.len = 0;
    UserIPQueue.size = 255;
	UserIPQueue.bHaveDollars = false;

    SingleItemsQueueS = NULL;

    // active	
	SingleItemsQueueaS = NULL;
    SingleItemsQueueaE = NULL;
    
    //pasive
	SingleItemsQueuebS = NULL;
    SingleItemsQueuebE = NULL;
    
    // active
    Qa = new QzBuf(); ActQa = new QzBuf(); PasQa = new QzBuf(); OpQa = new QzBuf(); OpActQa = new QzBuf(); OpPasQa = new QzBuf();
    HlStrpQa = new QzBuf(); HlStrpActQa = new QzBuf(); HlStrpPasQa = new QzBuf(); HlFullQa = new QzBuf(); HlFullActQa = new QzBuf(); HlFullPasQa = new QzBuf();
    HlStrpOpQa = new QzBuf(); HlStrpOpActQa = new QzBuf(); HlStrpOpPasQa = new QzBuf(); HlFullOpQa = new QzBuf(); HlFullOpActQa = new QzBuf(); HlFullOpPasQa = new QzBuf();
    HlStrpIpQa = new QzBuf(); HlStrpIpActQa = new QzBuf(); HlStrpIpPasQa = new QzBuf(); HlFullIpQa = new QzBuf(); HlFullIpActQa = new QzBuf(); HlFullIpPasQa = new QzBuf();
    HlStrpIpOpQa = new QzBuf(); HlStrpIpOpActQa = new QzBuf(); HlStrpIpOpPasQa = new QzBuf(); HlFullIpOpQa = new QzBuf(); HlFullIpOpActQa = new QzBuf(); HlFullIpOpPasQa = new QzBuf();
    StrpQa = new QzBuf(); StrpActQa = new QzBuf(); StrpPasQa = new QzBuf(); FullQa = new QzBuf(); FullActQa = new QzBuf(); FullPasQa = new QzBuf();
    StrpOpQa = new QzBuf(); StrpOpActQa = new QzBuf(); StrpOpPasQa = new QzBuf(); FullOpQa = new QzBuf(); FullOpActQa = new QzBuf(); FullOpPasQa = new QzBuf();
    StrpIpQa = new QzBuf(); StrpIpActQa = new QzBuf(); StrpIpPasQa = new QzBuf(); FullIpQa = new QzBuf(); FullIpActQa = new QzBuf(); FullIpPasQa = new QzBuf();
    StrpIpOpQa = new QzBuf(); StrpIpOpActQa = new QzBuf(); StrpIpOpPasQa = new QzBuf(); FullIpOpQa = new QzBuf(); FullIpOpActQa = new QzBuf(); FullIpOpPasQa = new QzBuf();

    // pasive
    Qb = new QzBuf(); ActQb = new QzBuf(); PasQb = new QzBuf(); OpQb = new QzBuf(); OpActQb = new QzBuf(); OpPasQb = new QzBuf();
    HlStrpQb = new QzBuf(); HlStrpActQb = new QzBuf(); HlStrpPasQb = new QzBuf(); HlFullQb = new QzBuf(); HlFullActQb = new QzBuf(); HlFullPasQb = new QzBuf();
    HlStrpOpQb = new QzBuf(); HlStrpOpActQb = new QzBuf(); HlStrpOpPasQb = new QzBuf(); HlFullOpQb = new QzBuf(); HlFullOpActQb = new QzBuf(); HlFullOpPasQb = new QzBuf();
    HlStrpIpQb = new QzBuf(); HlStrpIpActQb = new QzBuf(); HlStrpIpPasQb = new QzBuf(); HlFullIpQb = new QzBuf(); HlFullIpActQb = new QzBuf(); HlFullIpPasQb = new QzBuf();
    HlStrpIpOpQb = new QzBuf(); HlStrpIpOpActQb = new QzBuf(); HlStrpIpOpPasQb = new QzBuf(); HlFullIpOpQb = new QzBuf(); HlFullIpOpActQb = new QzBuf(); HlFullIpOpPasQb = new QzBuf();
    StrpQb = new QzBuf(); StrpActQb = new QzBuf(); StrpPasQb = new QzBuf(); FullQb = new QzBuf(); FullActQb = new QzBuf(); FullPasQb = new QzBuf();
    StrpOpQb = new QzBuf(); StrpOpActQb = new QzBuf(); StrpOpPasQb = new QzBuf(); FullOpQb = new QzBuf(); FullOpActQb = new QzBuf(); FullOpPasQb = new QzBuf();
    StrpIpQb = new QzBuf(); StrpIpActQb = new QzBuf(); StrpIpPasQb = new QzBuf(); FullIpQb = new QzBuf(); FullIpActQb = new QzBuf(); FullIpPasQb = new QzBuf();
    StrpIpOpQb = new QzBuf(); StrpIpOpActQb = new QzBuf(); StrpIpOpPasQb = new QzBuf(); FullIpOpQb = new QzBuf(); FullIpOpActQb = new QzBuf(); FullIpOpPasQb = new QzBuf();
    
    // active 
    allqzbufsa[0] = Qa; allqzbufsa[1] = ActQa; allqzbufsa[2] = PasQa; allqzbufsa[3] = OpQa; allqzbufsa[4] = OpActQa; allqzbufsa[5] =  OpPasQa;
    allqzbufsa[6] = HlStrpQa; allqzbufsa[7] = HlStrpActQa; allqzbufsa[8] = HlStrpPasQa; allqzbufsa[9] = HlFullQa; allqzbufsa[10] = HlFullActQa; allqzbufsa[11] = HlFullPasQa;
    allqzbufsa[12] = HlStrpOpQa; allqzbufsa[13] = HlStrpOpActQa; allqzbufsa[14] = HlStrpOpPasQa; allqzbufsa[15] = HlFullOpQa; allqzbufsa[16] = HlFullOpActQa; allqzbufsa[17] = HlFullOpPasQa;
    allqzbufsa[18] = HlStrpIpQa; allqzbufsa[19] = HlStrpIpActQa; allqzbufsa[20] = HlStrpIpPasQa; allqzbufsa[21] = HlFullIpQa; allqzbufsa[22] = HlFullIpActQa; allqzbufsa[23] = HlFullIpPasQa;
    allqzbufsa[24] = HlStrpIpOpQa; allqzbufsa[25] = HlStrpIpOpActQa; allqzbufsa[26] = HlStrpIpOpPasQa; allqzbufsa[27] = HlFullIpOpQa; allqzbufsa[28] = HlFullIpOpActQa; allqzbufsa[29] = HlFullIpOpPasQa;
    allqzbufsa[30] = StrpQa; allqzbufsa[31] = StrpActQa; allqzbufsa[32] = StrpPasQa; allqzbufsa[33] = FullQa; allqzbufsa[34] = FullActQa; allqzbufsa[35] = FullPasQa;
    allqzbufsa[36] = StrpOpQa; allqzbufsa[37] = StrpOpActQa; allqzbufsa[38] = StrpOpPasQa; allqzbufsa[39] = FullOpQa; allqzbufsa[40] = FullOpActQa; allqzbufsa[41] = FullOpPasQa;
    allqzbufsa[42] = StrpIpQa; allqzbufsa[43] = StrpIpActQa; allqzbufsa[44] = StrpIpPasQa; allqzbufsa[45] = FullIpQa; allqzbufsa[46] = FullIpActQa; allqzbufsa[47] = FullIpPasQa;
    allqzbufsa[48] = StrpIpOpQa; allqzbufsa[49] = StrpIpOpActQa; allqzbufsa[50] = StrpIpOpPasQa; allqzbufsa[51] = FullIpOpQa; allqzbufsa[52] = FullIpOpActQa; allqzbufsa[53] = FullIpOpPasQa;

    // pasive
    allqzbufsb[0] = Qb; allqzbufsb[1] = ActQb; allqzbufsb[2] = PasQb; allqzbufsb[3] = OpQb; allqzbufsb[4] = OpActQb; allqzbufsb[5] =  OpPasQb;
    allqzbufsb[6] = HlStrpQb; allqzbufsb[7] = HlStrpActQb; allqzbufsb[8] = HlStrpPasQb; allqzbufsb[9] = HlFullQb; allqzbufsb[10] = HlFullActQb; allqzbufsb[11] = HlFullPasQb;
    allqzbufsb[12] = HlStrpOpQb; allqzbufsb[13] = HlStrpOpActQb; allqzbufsb[14] = HlStrpOpPasQb; allqzbufsb[15] = HlFullOpQb; allqzbufsb[16] = HlFullOpActQb; allqzbufsb[17] = HlFullOpPasQb;
    allqzbufsb[18] = HlStrpIpQb; allqzbufsb[19] = HlStrpIpActQb; allqzbufsb[20] = HlStrpIpPasQb; allqzbufsb[21] = HlFullIpQb; allqzbufsb[22] = HlFullIpActQb; allqzbufsb[23] = HlFullIpPasQb;
    allqzbufsb[24] = HlStrpIpOpQb; allqzbufsb[25] = HlStrpIpOpActQb; allqzbufsb[26] = HlStrpIpOpPasQb; allqzbufsb[27] = HlFullIpOpQb; allqzbufsb[28] = HlFullIpOpActQb; allqzbufsb[29] = HlFullIpOpPasQb;
    allqzbufsb[30] = StrpQb; allqzbufsb[31] = StrpActQb; allqzbufsb[32] = StrpPasQb; allqzbufsb[33] = FullQb; allqzbufsb[34] = FullActQb; allqzbufsb[35] = FullPasQb;
    allqzbufsb[36] = StrpOpQb; allqzbufsb[37] = StrpOpActQb; allqzbufsb[38] = StrpOpPasQb; allqzbufsb[39] = FullOpQb; allqzbufsb[40] = FullOpActQb; allqzbufsb[41] = FullOpPasQb;
    allqzbufsb[42] = StrpIpQb; allqzbufsb[43] = StrpIpActQb; allqzbufsb[44] = StrpIpPasQb; allqzbufsb[45] = FullIpQb; allqzbufsb[46] = FullIpActQb; allqzbufsb[47] = FullIpPasQb;
    allqzbufsb[48] = StrpIpOpQb; allqzbufsb[49] = StrpIpOpActQb; allqzbufsb[50] = StrpIpOpPasQb; allqzbufsb[51] = FullIpOpQb; allqzbufsb[52] = FullIpOpActQb; allqzbufsb[53] = FullIpOpPasQb;
    
    // active    
    hlqzbufsa[0] = HlStrpQa; hlqzbufsa[1] = HlStrpActQa; hlqzbufsa[2] = HlStrpPasQa; hlqzbufsa[3] = HlFullQa; hlqzbufsa[4] = HlFullActQa; hlqzbufsa[5] = HlFullPasQa;
    hlqzbufsa[6] = HlStrpOpQa; hlqzbufsa[7] = HlStrpOpActQa; hlqzbufsa[8] = HlStrpOpPasQa; hlqzbufsa[9] = HlFullOpQa; hlqzbufsa[10] = HlFullOpActQa; hlqzbufsa[11] = HlFullOpPasQa;
    hlqzbufsa[12] = HlStrpIpQa; hlqzbufsa[13] = HlStrpIpActQa; hlqzbufsa[14] = HlStrpIpPasQa; hlqzbufsa[15] = HlFullIpQa; hlqzbufsa[16] = HlFullIpActQa; hlqzbufsa[17] = HlFullIpPasQa;
    hlqzbufsa[18] = HlStrpIpOpQa; hlqzbufsa[19] = HlStrpIpOpActQa; hlqzbufsa[20] = HlStrpIpOpPasQa; hlqzbufsa[21] = HlFullIpOpQa; hlqzbufsa[22] = HlFullIpOpActQa; hlqzbufsa[23] = HlFullIpOpPasQa;

    // pasive
    hlqzbufsb[0] = HlStrpQb; hlqzbufsb[1] = HlStrpActQb; hlqzbufsb[2] = HlStrpPasQb; hlqzbufsb[3] = HlFullQb; hlqzbufsb[4] = HlFullActQb; hlqzbufsb[5] = HlFullPasQb;
    hlqzbufsb[6] = HlStrpOpQb; hlqzbufsb[7] = HlStrpOpActQb; hlqzbufsb[8] = HlStrpOpPasQb; hlqzbufsb[9] = HlFullOpQb; hlqzbufsb[10] = HlFullOpActQb; hlqzbufsb[11] = HlFullOpPasQb;
    hlqzbufsb[12] = HlStrpIpQb; hlqzbufsb[13] = HlStrpIpActQb; hlqzbufsb[14] = HlStrpIpPasQb; hlqzbufsb[15] = HlFullIpQb; hlqzbufsb[16] = HlFullIpActQb; hlqzbufsb[17] = HlFullIpPasQb;
    hlqzbufsb[18] = HlStrpIpOpQb; hlqzbufsb[19] = HlStrpIpOpActQb; hlqzbufsb[20] = HlStrpIpOpPasQb; hlqzbufsb[21] = HlFullIpOpQb; hlqzbufsb[22] = HlFullIpOpActQb; hlqzbufsb[23] = HlFullIpOpPasQb;
    
    // active
    srchqzbufsa[0] = ActQa; srchqzbufsa[1] = PasQa; srchqzbufsa[2] = OpActQa; srchqzbufsa[3] = OpPasQa; srchqzbufsa[4] = HlStrpActQa; srchqzbufsa[5] = HlStrpPasQa;
    srchqzbufsa[6] = HlFullActQa; srchqzbufsa[7] = HlFullPasQa; srchqzbufsa[8] = HlStrpOpActQa; srchqzbufsa[9] = HlStrpOpPasQa; srchqzbufsa[10] = HlFullOpActQa; srchqzbufsa[11] = HlFullOpPasQa;
    srchqzbufsa[12] = HlStrpIpActQa; srchqzbufsa[13] = HlStrpIpPasQa; srchqzbufsa[14] = HlFullIpActQa; srchqzbufsa[15] = HlFullIpPasQa; srchqzbufsa[16] = HlStrpIpOpActQa; srchqzbufsa[17] = HlStrpIpOpPasQa;
    srchqzbufsa[18] = HlFullIpOpActQa; srchqzbufsa[19] = HlFullIpOpPasQa; srchqzbufsa[20] = StrpActQa; srchqzbufsa[21] = StrpPasQa; srchqzbufsa[22] = FullActQa; srchqzbufsa[23] = FullPasQa;
    srchqzbufsa[24] = StrpOpActQa; srchqzbufsa[25] = StrpOpPasQa; srchqzbufsa[26] = FullOpActQa; srchqzbufsa[27] = FullOpPasQa; srchqzbufsa[28] = StrpIpActQa; srchqzbufsa[29] = StrpIpPasQa;
    srchqzbufsa[30] = FullIpActQa; srchqzbufsa[31] = FullIpPasQa; srchqzbufsa[32] = StrpIpOpActQa; srchqzbufsa[33] = StrpIpOpPasQa; srchqzbufsa[34] = FullIpOpActQa; srchqzbufsa[35] = FullIpOpPasQa;

    // pasive
    srchqzbufsb[0] = ActQb; srchqzbufsb[1] = PasQb; srchqzbufsb[2] = OpActQb; srchqzbufsb[3] = OpPasQb; srchqzbufsb[4] = HlStrpActQb; srchqzbufsb[5] = HlStrpPasQb;
    srchqzbufsb[6] = HlFullActQb; srchqzbufsb[7] = HlFullPasQb; srchqzbufsb[8] = HlStrpOpActQb; srchqzbufsb[9] = HlStrpOpPasQb; srchqzbufsb[10] = HlFullOpActQb; srchqzbufsb[11] = HlFullOpPasQb;
    srchqzbufsb[12] = HlStrpIpActQb; srchqzbufsb[13] = HlStrpIpPasQb; srchqzbufsb[14] = HlFullIpActQb; srchqzbufsb[15] = HlFullIpPasQb; srchqzbufsb[16] = HlStrpIpOpActQb; srchqzbufsb[17] = HlStrpIpOpPasQb;
    srchqzbufsb[18] = HlFullIpOpActQb; srchqzbufsb[19] = HlFullIpOpPasQb; srchqzbufsb[20] = StrpActQb; srchqzbufsb[21] = StrpPasQb; srchqzbufsb[22] = FullActQb; srchqzbufsb[23] = FullPasQb;
    srchqzbufsb[24] = StrpOpActQb; srchqzbufsb[25] = StrpOpPasQb; srchqzbufsb[26] = FullOpActQb; srchqzbufsb[27] = FullOpPasQb; srchqzbufsb[28] = StrpIpActQb; srchqzbufsb[29] = StrpIpPasQb;
    srchqzbufsb[30] = FullIpActQb; srchqzbufsb[31] = FullIpPasQb; srchqzbufsb[32] = StrpIpOpActQb; srchqzbufsb[33] = StrpIpOpPasQb; srchqzbufsb[34] = FullIpOpActQb; srchqzbufsb[35] = FullIpOpPasQb;
    
    // active    
    actqzbufsa[0] = ActQa; actqzbufsa[1] = OpActQa; actqzbufsa[2] = HlStrpActQa; actqzbufsa[3] = HlFullActQa; actqzbufsa[4] = HlStrpOpActQa; actqzbufsa[5] = HlFullOpActQa;
    actqzbufsa[6] = HlStrpIpActQa; actqzbufsa[7] = HlFullIpActQa; actqzbufsa[8] = HlStrpIpOpActQa; actqzbufsa[9] = HlFullIpOpActQa; actqzbufsa[10] = StrpActQa; actqzbufsa[11] = FullActQa;
    actqzbufsa[12] = StrpOpActQa; actqzbufsa[13] = FullOpActQa; actqzbufsa[14] = StrpIpActQa; actqzbufsa[15] = FullIpActQa; actqzbufsa[16] = StrpIpOpActQa; actqzbufsa[17] = FullIpOpActQa;

    // pasive
    actqzbufsb[0] = ActQb; actqzbufsb[1] = OpActQb; actqzbufsb[2] = HlStrpActQb; actqzbufsb[3] = HlFullActQb; actqzbufsb[4] = HlStrpOpActQb; actqzbufsb[5] = HlFullOpActQb;
    actqzbufsb[6] = HlStrpIpActQb; actqzbufsb[7] = HlFullIpActQb; actqzbufsb[8] = HlStrpIpOpActQb; actqzbufsb[9] = HlFullIpOpActQb; actqzbufsb[10] = StrpActQb; actqzbufsb[11] = FullActQb;
    actqzbufsb[12] = StrpOpActQb; actqzbufsb[13] = FullOpActQb; actqzbufsb[14] = StrpIpActQb; actqzbufsb[15] = FullIpActQb; actqzbufsb[16] = StrpIpOpActQb; actqzbufsb[17] = FullIpOpActQb;

    // active           
    strpqzbufsa[0] = HlStrpQa; strpqzbufsa[1] = HlStrpActQa; strpqzbufsa[2] = HlStrpPasQa; strpqzbufsa[3] = HlStrpOpQa; strpqzbufsa[4] = HlStrpOpActQa; strpqzbufsa[5] = HlStrpOpPasQa;
    strpqzbufsa[6] = HlStrpIpQa; strpqzbufsa[7] = HlStrpIpActQa; strpqzbufsa[8] = HlStrpIpPasQa; strpqzbufsa[9] = HlStrpIpOpQa; strpqzbufsa[10] = HlStrpIpOpActQa; strpqzbufsa[11] = HlStrpIpOpPasQa;
    strpqzbufsa[12] = StrpQa; strpqzbufsa[13] = StrpActQa; strpqzbufsa[14] = StrpPasQa; strpqzbufsa[15] = StrpOpQa; strpqzbufsa[16] = StrpOpActQa; strpqzbufsa[17] = StrpOpPasQa;
    strpqzbufsa[18] = StrpIpQa; strpqzbufsa[19] = StrpIpActQa; strpqzbufsa[20] = StrpIpPasQa; strpqzbufsa[21] = StrpIpOpQa; strpqzbufsa[22] = StrpIpOpActQa; strpqzbufsa[23] = StrpIpOpPasQa;

    // pasive
    strpqzbufsb[0] = HlStrpQb; strpqzbufsb[1] = HlStrpActQb; strpqzbufsb[2] = HlStrpPasQb; strpqzbufsb[3] = HlStrpOpQb; strpqzbufsb[4] = HlStrpOpActQb; strpqzbufsb[5] = HlStrpOpPasQb;
    strpqzbufsb[6] = HlStrpIpQb; strpqzbufsb[7] = HlStrpIpActQb; strpqzbufsb[8] = HlStrpIpPasQb; strpqzbufsb[9] = HlStrpIpOpQb; strpqzbufsb[10] = HlStrpIpOpActQb; strpqzbufsb[11] = HlStrpIpOpPasQb;
    strpqzbufsb[12] = StrpQb; strpqzbufsb[13] = StrpActQb; strpqzbufsb[14] = StrpPasQb; strpqzbufsb[15] = StrpOpQb; strpqzbufsb[16] = StrpOpActQb; strpqzbufsb[17] = StrpOpPasQb;
    strpqzbufsb[18] = StrpIpQb; strpqzbufsb[19] = StrpIpActQb; strpqzbufsb[20] = StrpIpPasQb; strpqzbufsb[21] = StrpIpOpQb; strpqzbufsb[22] = StrpIpOpActQb; strpqzbufsb[23] = StrpIpOpPasQb;

    // active        
    fullqzbufsa[0] = HlFullQa; fullqzbufsa[1] = HlFullActQa; fullqzbufsa[2] = HlFullPasQa; fullqzbufsa[3] = HlFullOpQa; fullqzbufsa[4] = HlFullOpActQa; fullqzbufsa[5] = HlFullOpPasQa;
    fullqzbufsa[6] = HlFullIpQa; fullqzbufsa[7] = HlFullIpActQa; fullqzbufsa[8] = HlFullIpPasQa; fullqzbufsa[9] = HlFullIpOpQa; fullqzbufsa[10] = HlFullIpOpActQa; fullqzbufsa[11] = HlFullIpOpPasQa;
    fullqzbufsa[12] = FullQa; fullqzbufsa[13] = FullActQa; fullqzbufsa[14] = FullPasQa; fullqzbufsa[15] = FullOpQa; fullqzbufsa[16] = FullOpActQa; fullqzbufsa[17] = FullOpPasQa;
    fullqzbufsa[18] = FullIpQa; fullqzbufsa[19] = FullIpActQa; fullqzbufsa[20] = FullIpPasQa; fullqzbufsa[21] = FullIpOpQa; fullqzbufsa[22] = FullIpOpActQa; fullqzbufsa[23] = FullIpOpPasQa;

    // pasive
    fullqzbufsb[0] = HlFullQb; fullqzbufsb[1] = HlFullActQb; fullqzbufsb[2] = HlFullPasQb; fullqzbufsb[3] = HlFullOpQb; fullqzbufsb[4] = HlFullOpActQb; fullqzbufsb[5] = HlFullOpPasQb;
    fullqzbufsb[6] = HlFullIpQb; fullqzbufsb[7] = HlFullIpActQb; fullqzbufsb[8] = HlFullIpPasQb; fullqzbufsb[9] = HlFullIpOpQb; fullqzbufsb[10] = HlFullIpOpActQb; fullqzbufsb[11] = HlFullIpOpPasQb;
    fullqzbufsb[12] = FullQb; fullqzbufsb[13] = FullActQb; fullqzbufsb[14] = FullPasQb; fullqzbufsb[15] = FullOpQb; fullqzbufsb[16] = FullOpActQb; fullqzbufsb[17] = FullOpPasQb;
    fullqzbufsb[18] = FullIpQb; fullqzbufsb[19] = FullIpActQb; fullqzbufsb[20] = FullIpPasQb; fullqzbufsb[21] = FullIpOpQb; fullqzbufsb[22] = FullIpOpActQb; fullqzbufsb[23] = FullIpOpPasQb;

    // active
    opqzbufsa[0] = OpQa; opqzbufsa[1] = OpActQa; opqzbufsa[2] = OpPasQa; opqzbufsa[3] = HlStrpOpQa; opqzbufsa[4] = HlStrpOpActQa; opqzbufsa[5] = HlStrpOpPasQa;
    opqzbufsa[6] = HlFullOpQa; opqzbufsa[7] = HlFullOpActQa; opqzbufsa[8] = HlFullOpPasQa; opqzbufsa[9] = HlStrpIpOpQa; opqzbufsa[10] = HlStrpIpOpActQa; opqzbufsa[11] = HlStrpIpOpPasQa;
    opqzbufsa[12] = HlFullIpOpQa; opqzbufsa[13] = HlFullIpOpActQa; opqzbufsa[14] = HlFullIpOpPasQa; opqzbufsa[15] = StrpOpQa; opqzbufsa[16] = StrpOpActQa; opqzbufsa[17] = StrpOpPasQa;
    opqzbufsa[18] = FullOpQa; opqzbufsa[19] = FullOpActQa; opqzbufsa[20] = FullOpPasQa; opqzbufsa[21] = StrpIpOpQa; opqzbufsa[22] = StrpIpOpActQa; opqzbufsa[23] = StrpIpOpPasQa;
    opqzbufsa[24] = FullIpOpQa; opqzbufsa[25] = FullIpOpActQa; opqzbufsa[26] = FullIpOpPasQa;

    // pasive
    opqzbufsb[0] = OpQb; opqzbufsb[1] = OpActQb; opqzbufsb[2] = OpPasQb; opqzbufsb[3] = HlStrpOpQb; opqzbufsb[4] = HlStrpOpActQb; opqzbufsb[5] = HlStrpOpPasQb;
    opqzbufsb[6] = HlFullOpQb; opqzbufsb[7] = HlFullOpActQb; opqzbufsb[8] = HlFullOpPasQb; opqzbufsb[9] = HlStrpIpOpQb; opqzbufsb[10] = HlStrpIpOpActQb; opqzbufsb[11] = HlStrpIpOpPasQb;
    opqzbufsb[12] = HlFullIpOpQb; opqzbufsb[13] = HlFullIpOpActQb; opqzbufsb[14] = HlFullIpOpPasQb; opqzbufsb[15] = StrpOpQb; opqzbufsb[16] = StrpOpActQb; opqzbufsb[17] = StrpOpPasQb;
    opqzbufsb[18] = FullOpQb; opqzbufsb[19] = FullOpActQb; opqzbufsb[20] = FullOpPasQb; opqzbufsb[21] = StrpIpOpQb; opqzbufsb[22] = StrpIpOpActQb; opqzbufsb[23] = StrpIpOpPasQb;
    opqzbufsb[24] = FullIpOpQb; opqzbufsb[25] = FullIpOpActQb; opqzbufsb[26] = FullIpOpPasQb;

    // active        
    ipqzbufsa[0] = HlStrpIpQa; ipqzbufsa[1] = HlStrpIpActQa; ipqzbufsa[2] = HlStrpIpPasQa; ipqzbufsa[3] = HlFullIpQa; ipqzbufsa[4] = HlFullIpActQa; ipqzbufsa[5] = HlFullIpPasQa;
    ipqzbufsa[6] = HlStrpIpOpQa; ipqzbufsa[7] = HlStrpIpOpActQa; ipqzbufsa[8] = HlStrpIpOpPasQa; ipqzbufsa[9] = HlFullIpOpQa; ipqzbufsa[10] = HlFullIpOpActQa; ipqzbufsa[11] = HlFullIpOpPasQa;
    ipqzbufsa[12] = StrpIpQa; ipqzbufsa[13] = StrpIpActQa; ipqzbufsa[14] = StrpIpPasQa; ipqzbufsa[15] = FullIpQa; ipqzbufsa[16] = FullIpActQa; ipqzbufsa[17] = FullIpPasQa;
    ipqzbufsa[18] = StrpIpOpQa; ipqzbufsa[19] = StrpIpOpActQa; ipqzbufsa[20] = StrpIpOpPasQa; ipqzbufsa[21] = FullIpOpQa; ipqzbufsa[22] = FullIpOpActQa; ipqzbufsa[23] = FullIpOpPasQa;

    // pasive
    ipqzbufsb[0] = HlStrpIpQb; ipqzbufsb[1] = HlStrpIpActQb; ipqzbufsb[2] = HlStrpIpPasQb; ipqzbufsb[3] = HlFullIpQb; ipqzbufsb[4] = HlFullIpActQb; ipqzbufsb[5] = HlFullIpPasQb;
    ipqzbufsb[6] = HlStrpIpOpQb; ipqzbufsb[7] = HlStrpIpOpActQb; ipqzbufsb[8] = HlStrpIpOpPasQb; ipqzbufsb[9] = HlFullIpOpQb; ipqzbufsb[10] = HlFullIpOpActQb; ipqzbufsb[11] = HlFullIpOpPasQb;
    ipqzbufsb[12] = StrpIpQb; ipqzbufsb[13] = StrpIpActQb; ipqzbufsb[14] = StrpIpPasQb; ipqzbufsb[15] = FullIpQb; ipqzbufsb[16] = FullIpActQb; ipqzbufsb[17] = FullIpPasQb;
    ipqzbufsb[18] = StrpIpOpQb; ipqzbufsb[19] = StrpIpOpActQb; ipqzbufsb[20] = StrpIpOpPasQb; ipqzbufsb[21] = FullIpOpQb; ipqzbufsb[22] = FullIpOpActQb; ipqzbufsb[23] = FullIpOpPasQb;

    for(uint8_t i = 0; i < 54; i++) {
        if(allqzbufsa[i] == NULL) {
#ifdef _WIN32
			string sDbgstr = "Memory allocation of qzbufs failed! "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#else
			string sDbgstr = "Memory allocation of qzbufs failed!";
#endif
        	AppendSpecialLog(sDbgstr);
        	exit(EXIT_FAILURE);
        }
        
        // active
        QzBuf *zqueue = allqzbufsa[i];
        zqueue->buffer = NULL;
        zqueue->zbuffer = NULL;
        
        // pasive
        zqueue = allqzbufsb[i];
        zqueue->buffer = NULL;
        zqueue->zbuffer = NULL;
    }
    
    for(uint8_t i = 0; i < 54; i++) {
        // active
        QzBuf *zqueue = allqzbufsa[i];
#ifdef _WIN32
        zqueue->buffer = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
		zqueue->buffer = (char *) calloc(256, 1);
#endif
    	if(zqueue->buffer == NULL) {
			AppendSpecialLog("Cannot create global output zqueuea"+string(i)+" buffer!");
            return;
    	}
    	zqueue->len = 0;
        zqueue->size = 255;
#ifdef _WIN32
        zqueue->zbuffer = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 32);
#else
		zqueue->zbuffer = (char *) calloc(32, 1);
#endif
    	if(zqueue->zbuffer == NULL) {
			AppendSpecialLog("Cannot create global output zqueuea"+string(i)+" zbuffer!");
            return;
    	}
        zqueue->zlen = 0;
        zqueue->zsize = 31;
        zqueue->zlined = false;
        
        // pasive
        zqueue = allqzbufsb[i];
#ifdef _WIN32
        zqueue->buffer = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 256);
#else
		zqueue->buffer = (char *) calloc(256, 1);
#endif
    	if(zqueue->buffer == NULL) {
			AppendSpecialLog("Cannot create global output zqueueb"+string(i)+" buffer!");
            return;
    	}
    	zqueue->len = 0;
        zqueue->size = 255;
#ifdef _WIN32
        zqueue->zbuffer = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, 32);
#else
		zqueue->zbuffer = (char *) calloc(32, 1);
#endif
    	if(zqueue->zbuffer == NULL) {
			AppendSpecialLog("Cannot create global output zqueueb"+string(i)+" zbuffer!");
            return;
    	}
        zqueue->zlen = 0;
        zqueue->zsize = 31;
        zqueue->zlined = false;
    }
}
//---------------------------------------------------------------------------

globalqueue::~globalqueue() {
#ifdef _WIN32
    if(OpListQueue.buffer != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)OpListQueue.buffer) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate OpListQueue.buffer in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(OpListQueue.buffer);
#endif

#ifdef _WIN32
    if(UserIPQueue.buffer != NULL) {
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)UserIPQueue.buffer) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate UserIPQueue.buffer in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
			AppendSpecialLog(sDbgstr);
        }
    }
#else
	free(UserIPQueue.buffer);
#endif

    if(SingleItemsQueueaS != NULL) {
        QueueDataItem *next = SingleItemsQueueaS;
        while(next != NULL) {
            QueueDataItem *cur = next;
            next = cur->next;

#ifdef _WIN32
            if(cur->sData != NULL) {
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sData) == 0) {
					string sDbgstr = "[BUF] Cannot deallocate cur->sData in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
                }
            }
#else
			free(cur->sData);
#endif
            delete cur;
		}
    }

    if(SingleItemsQueuebS != NULL) {
        QueueDataItem *next = SingleItemsQueuebS;
        while(next != NULL) {
            QueueDataItem *cur = next;
            next = cur->next;

#ifdef _WIN32
            if(cur->sData != NULL) {
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sData) == 0) {
					string sDbgstr = "[BUF] Cannot deallocate cur->sData1 in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
                }
            }
#else
			free(cur->sData);
#endif
            delete cur;
		}
    }
        
    for(uint8_t i = 0; i < 54; i++) {
        // active
        QzBuf *zqueue = allqzbufsa[i];

#ifdef _WIN32
        if(zqueue->buffer != NULL) {
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)zqueue->buffer) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate zqueue->buffer in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
        }
#else
		free(zqueue->buffer);
#endif

#ifdef _WIN32
        if(zqueue->zbuffer != NULL) {
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)zqueue->zbuffer) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate zqueue->zbuffer in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
        }
#else
		free(zqueue->zbuffer);
#endif
        delete zqueue;

        // pasive
        zqueue = allqzbufsb[i];

#ifdef _WIN32
        if(zqueue->buffer != NULL) {
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)zqueue->buffer) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate zqueue->buffer1 in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
				AppendSpecialLog(sDbgstr);
            }
        }
#else
		free(zqueue->buffer);
#endif

#ifdef _WIN32
        if(zqueue->zbuffer != NULL) {
            if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)zqueue->zbuffer) == 0) {
				string sDbgstr = "[BUF] Cannot deallocate zqueue->zbuffer1 in globalqueue::~globalqueue! "+string((uint32_t)GetLastError())+" "+
					string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
                AppendSpecialLog(sDbgstr);
            }
        }
#else
		free(zqueue->zbuffer);
#endif
        delete zqueue;
	}
}
//---------------------------------------------------------------------------

void globalqueue::Store(char * sData) {
    size_t iLen = strlen(sData);
    Store(sData, iLen);
}
//---------------------------------------------------------------------------

// appends data to all global output queues
void globalqueue::Store(char * sData, const size_t &iDataLen) {
    QzBuf **allqzbufs;
    
    if(bActive) {
        bHaveaS = true;
        allqzbufs = allqzbufsa;
    } else {
        bHavebS = true;
        allqzbufs = allqzbufsb;
    }
    
    for(uint8_t i = 0; i < 54; i++) {
        QzBuf *zqueue = allqzbufs[i];
        if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
            if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::Store! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
    	memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
    	zqueue->len += (uint32_t)iDataLen;
    	zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

void globalqueue::HStore(char * sData) {
    size_t iLen = strlen(sData);
    HStore(sData, iLen);
}
//---------------------------------------------------------------------------

// appends data to all hello global output queues
void globalqueue::HStore(char * sData, const size_t &iDataLen) {
    QzBuf **hlqzbufs;
    
    if(bActive) {
        bHaveaH = true;
        hlqzbufs = hlqzbufsa;
    } else {
        bHavebH = true;
        hlqzbufs = hlqzbufsb;
    }
    
    for(uint8_t i = 0; i < 24; i++) {
        QzBuf *zqueue = hlqzbufs[i];
        if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
            if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::HStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
        memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += (uint32_t)(iDataLen);
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to all active global output queues
void globalqueue::AStore(char * sData, const size_t &iDataLen) {
    QzBuf **srchqzbufs;
    
    if(bActive) {
        bHaveaA = true;
        srchqzbufs = srchqzbufsa;
    } else {
        bHavebA = true;
        srchqzbufs = srchqzbufsb;
    }
    
    for(uint8_t i = 0; i < 36; i++) {
        QzBuf *zqueue = srchqzbufs[i];
      	if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
           	if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::AStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
    	memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += iDataLen;
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to all passive global output queues
void globalqueue::PStore(char * sData, const size_t &iDataLen) {
    QzBuf **actqzbufs;
    
    if(bActive) {
        bHaveaP = true;
        actqzbufs = actqzbufsa;
    } else {
        bHavebP = true;
        actqzbufs = actqzbufsb;
    }
    
    for(uint8_t i = 0; i < 18; i++) {
        QzBuf *zqueue = actqzbufs[i];
      	if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
           	if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::PStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
    	memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += iDataLen;
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to all userlist global output queues
void globalqueue::InfoStore(char * sData, const size_t &iDataLen) {
    QzBuf **allqzbufs;
    
    if(bActive) {
        bHaveaI = true;
        allqzbufs = allqzbufsa;
    } else {
        bHavebI = true;
        allqzbufs = allqzbufsb;
    }
    
    for(uint8_t i = 6; i < 54; i++) {
        QzBuf *zqueue = allqzbufs[i];
       	if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
            if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::InfoStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
        memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += (uint32_t)iDataLen;
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to all striped myinfo global output queues
void globalqueue::StrpInfoStore(char * sData, const size_t &iDataLen) {
    QzBuf **strpqzbufs;
    
    if(bActive) {
        bHaveaStrp = true;
        strpqzbufs = strpqzbufsa;
    } else {
        bHavebStrp = true;
        strpqzbufs = strpqzbufsb;
    }
    
    for(uint8_t i = 0; i < 24; i++) {
        QzBuf *zqueue = strpqzbufs[i];
       	if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
            if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::StrpInfoStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
        memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += iDataLen;
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to all full myinfo global output queues
void globalqueue::FullInfoStore(char * sData, const size_t &iDataLen) {
    QzBuf **fullqzbufs;
    
    if(bActive) {
        bHaveaFull = true;
        fullqzbufs = fullqzbufsa;
    } else {
        bHavebFull = true;
        fullqzbufs = fullqzbufsb;
    }
    
    for(uint8_t i = 0; i < 24; i++) {
        QzBuf *zqueue = fullqzbufs[i];
       	if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
            if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::FullInfoStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
        memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += iDataLen;
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to all OP global output queues
void globalqueue::OPStore(char * sData, const size_t &iDataLen) {
    QzBuf **opqzbufs;
    
    if(bActive) {
        bHaveaOp = true;
        opqzbufs = opqzbufsa;
    } else {
        bHavebOp = true;
        opqzbufs = opqzbufsb;
    }
    
    for(uint8_t i = 0; i < 27; i++) {
        QzBuf *zqueue = opqzbufs[i];
        if(zqueue->size < zqueue->len+iDataLen) {
            size_t iAllignLen = Allign1024(zqueue->len+iDataLen);
            char * oldbuf = zqueue->buffer;
#ifdef _WIN32
            zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
			zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
            if(zqueue->buffer == NULL) {
				string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)iDataLen)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
					" bytes of memory in globalqueue::OPStore! "+string(sData, iDataLen);
#ifdef _WIN32
				sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                free(oldbuf);
#endif
				AppendSpecialLog(sDbgstr);
                return;
            }
            zqueue->size = (uint32_t)(iAllignLen-1);
        }
        memcpy(zqueue->buffer+zqueue->len, sData, iDataLen);
        zqueue->len += (uint32_t)iDataLen;
        zqueue->buffer[zqueue->len] = '\0';
    }
}
//---------------------------------------------------------------------------

// appends data to the OpList queue
void globalqueue::OpListStore(char * sNick) {
    if(OpListQueue.len == 0) {
        OpListQueue.len = sprintf(OpListQueue.buffer, "$OpList %s$$|", sNick);
    } else {
        int iDataLen = sprintf(msg, "%s$$|", sNick);
        if(CheckSprintf(iDataLen, 128, "globalqueue::OpListStore2") == true) {
            if(OpListQueue.size < OpListQueue.len+iDataLen) {
                size_t iAllignLen = Allign256(OpListQueue.len+iDataLen);
                char * oldbuf = OpListQueue.buffer;
#ifdef _WIN32
                OpListQueue.buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
				OpListQueue.buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
                if(OpListQueue.buffer == NULL) {
					string sDbgstr = "[BUF] Cannot reallocate "+string(iDataLen)+"/"+string((uint64_t)OpListQueue.size)+"/"+string((uint64_t)iAllignLen)+
						" bytes of memory in globalqueue::OpListStore! "+string(msg, iDataLen);
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
                    AppendSpecialLog(sDbgstr);
                    return;
                }
                OpListQueue.size = (uint32_t)(iAllignLen-1);
            }
            memcpy(OpListQueue.buffer+OpListQueue.len-1, msg, iDataLen);
            OpListQueue.len += iDataLen-1;
            OpListQueue.buffer[OpListQueue.len] = '\0';
        }
    }
}
//---------------------------------------------------------------------------

// appends data to the userip queue
void globalqueue::UserIPStore(User * curUser) {
    if(UserIPQueue.len == 0) {
        UserIPQueue.len = sprintf(UserIPQueue.buffer, "$UserIP %s %s|", curUser->sNick, curUser->sIP);
        UserIPQueue.bHaveDollars = false;
    } else {
        int iDataLen = sprintf(msg, "%s %s$$|", curUser->sNick, curUser->sIP);
        if(CheckSprintf(iDataLen, 128, "globalqueue::UserIPStore2") == true) {
            if(UserIPQueue.bHaveDollars == false) {
                UserIPQueue.buffer[UserIPQueue.len-1] = '$';
                UserIPQueue.buffer[UserIPQueue.len] = '$';
                UserIPQueue.bHaveDollars = true;
                UserIPQueue.len += 2;
            }
            if(UserIPQueue.size < UserIPQueue.len+iDataLen) {
                size_t iAllignLen = Allign256(UserIPQueue.len+iDataLen);
                char * oldbuf = UserIPQueue.buffer;
#ifdef _WIN32
                UserIPQueue.buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
				UserIPQueue.buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
                if(UserIPQueue.buffer == NULL) {
					string sDbgstr = "[BUF] Cannot reallocate "+string(iDataLen)+"/"+string((uint64_t)UserIPQueue.size)+"/"+string((uint64_t)iAllignLen)+
						" bytes of memory in globalqueue::UserIPStore! "+string(msg, iDataLen);
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    return;
                }
                UserIPQueue.size = (uint32_t)(iAllignLen-1);
            }
            memcpy(UserIPQueue.buffer+UserIPQueue.len-1, msg, iDataLen);
            UserIPQueue.len += iDataLen-1;
            UserIPQueue.buffer[UserIPQueue.len] = '\0';
        }
    }
    bHaveIP = true;
}
//---------------------------------------------------------------------------

void globalqueue::SingleItemsStore(QueueDataItem * NewItem) {
    if(bActive) {
        if(SingleItemsQueueaS == NULL) {
            SingleItemsQueueaS = NewItem;
            SingleItemsQueueaE = NewItem;
        } else {
            NewItem->prev = SingleItemsQueueaE;
            SingleItemsQueueaE = NewItem;
            NewItem->prev->next = NewItem;
        }
    } else {
        if(SingleItemsQueuebS == NULL) {
            SingleItemsQueuebS = NewItem;
            SingleItemsQueuebE = NewItem;
        } else {
            NewItem->prev = SingleItemsQueuebE;
            SingleItemsQueuebE = NewItem;
            NewItem->prev->next = NewItem;
        }
    }
}
//---------------------------------------------------------------------------

void globalqueue::FinalizeQueues() {
    if(OpListQueue.len != 0) {
        InfoStore(OpListQueue.buffer, OpListQueue.len);
        OpListQueue.len = 0;
        OpListQueue.buffer[0] = '\0';
    }

    QzBuf **ipqzbufs;

    if(bActive) {       
        bActive = false;

        ipqzbufs = ipqzbufsa;

        SingleItemsQueueS = SingleItemsQueueaS;

        bHaveaQueue = bHaveaS || bHaveaH || bHaveaA || bHaveaP || bHaveaI || bHaveaStrp || bHaveaFull || bHaveaOp || bHaveIP;
        bHaveQueue = bHaveaQueue;
    } else {       
        bActive = true;

        ipqzbufs = ipqzbufsb;

        SingleItemsQueueS = SingleItemsQueuebS;

        bHavebQueue = bHavebS || bHavebH || bHavebA || bHavebP || bHavebI || bHavebStrp || bHavebFull || bHavebOp || bHaveIP;
        bHaveQueue = bHavebQueue;
    }

    if(UserIPQueue.len != 0) {
        for(uint8_t i = 0; i < 24; i++) {
            QzBuf *zqueue = ipqzbufs[i];
            if(zqueue->size < zqueue->len+UserIPQueue.len) {
                size_t iAllignLen = Allign1024(zqueue->len+UserIPQueue.len);
                char * oldbuf = zqueue->buffer;
#ifdef _WIN32
                zqueue->buffer = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
				zqueue->buffer = (char *) realloc(oldbuf, iAllignLen);
#endif
                if(zqueue->buffer == NULL) {
					string sDbgstr = "[BUF] Cannot reallocate "+string((uint64_t)UserIPQueue.len)+"/"+string((uint64_t)zqueue->len)+"/"+string((uint64_t)iAllignLen)+
						" bytes of memory in globalqueue::FinalizeQueues!";
#ifdef _WIN32
					sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
#else
                    free(oldbuf);
#endif
					AppendSpecialLog(sDbgstr);
                    return;
                }
                zqueue->size = (uint32_t)(iAllignLen-1);
            }
            memcpy(zqueue->buffer+zqueue->len, UserIPQueue.buffer, UserIPQueue.len);
            zqueue->len += UserIPQueue.len;
            zqueue->buffer[zqueue->len] = '\0';
        }
        UserIPQueue.len = 0;
        UserIPQueue.buffer[0] = '\0';
    }
}
//---------------------------------------------------------------------------

void globalqueue::ClearQueues() {
    bHaveQueue = false;
    bHaveIP = false;
    
    if(bActive) {
        SingleItemsQueuebS = NULL;
        SingleItemsQueuebE = NULL;
        for(uint8_t i = 0; i < 54; i++) {
            QzBuf *zqueue = allqzbufsb[i];
            if(zqueue->len != 0) {
                zqueue->len = 0;
                zqueue->buffer[0] = '\0';
            }
            if(zqueue->zbuffer != NULL) {
                zqueue->zlen = 0;
                zqueue->zbuffer[0] = '\0';
            }
            zqueue->zlined = false;
        }
        bHavebQueue = false;
        bHavebS = false;
        bHavebH = false;
        bHavebA = false;
        bHavebP = false;
        bHavebI = false;
        bHavebStrp = false;
        bHavebFull = false;
        bHavebOp = false;
    } else {
        SingleItemsQueueaS = NULL;
        SingleItemsQueueaE = NULL;
        for(uint8_t i = 0; i < 54; i++) {
            QzBuf *zqueue = allqzbufsa[i];
            if(zqueue->len != 0) {
                zqueue->len = 0;
                zqueue->buffer[0] = '\0';
            }
            if(zqueue->zbuffer != NULL) {
                zqueue->zlen = 0;
                zqueue->zbuffer[0] = '\0';
            }
            zqueue->zlined = false;
        }
        bHaveaQueue = false;
        bHaveaS = false;
        bHaveaH = false;
        bHaveaA = false;
        bHaveaP = false;
        bHaveaI = false;
        bHaveaStrp = false;
        bHaveaFull = false;
        bHaveaOp = false;
    }
        
    if(SingleItemsQueueS) {
        QueueDataItem *next = SingleItemsQueueS;
        while(next != NULL) {
            QueueDataItem *cur = next;
            next = cur->next;

#ifdef _WIN32
            if(cur->sData != NULL) {
                if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->sData) == 0) {
					string sDbgstr = "[BUF] Cannot deallocate cur->sData in globalqueue::ClearQueues! "+string((uint32_t)GetLastError())+" "+
						string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
					AppendSpecialLog(sDbgstr);
                }
            }
#else
			free(cur->sData);
#endif
            delete cur;
		}
    }
    SingleItemsQueueS = NULL;
}
//---------------------------------------------------------------------------

void globalqueue::ProcessQueues(User * u) {
    bool bHaveH, bHaveA, bHaveP, bHaveFull, bHaveOp;
    struct QzBuf *Q, *ActQ, *PasQ, *OpQ, *OpActQ, *OpPasQ,
        *HlStrpQ, *HlStrpActQ, *HlStrpPasQ, *HlFullQ, *HlFullActQ, *HlFullPasQ,
        *HlStrpOpQ, *HlStrpOpActQ, *HlStrpOpPasQ, *HlFullOpQ, *HlFullOpActQ, *HlFullOpPasQ,
        *HlStrpIpQ, *HlStrpIpActQ, *HlStrpIpPasQ, *HlFullIpQ, *HlFullIpActQ, *HlFullIpPasQ,
        *HlStrpIpOpQ, *HlStrpIpOpActQ, *HlStrpIpOpPasQ, *HlFullIpOpQ, *HlFullIpOpActQ, *HlFullIpOpPasQ,
        *StrpQ, *StrpActQ, *StrpPasQ, *FullQ, *FullActQ, *FullPasQ,
        *StrpOpQ, *StrpOpActQ, *StrpOpPasQ, *FullOpQ, *FullOpActQ, *FullOpPasQ,
        *StrpIpQ, *StrpIpActQ, *StrpIpPasQ, *FullIpQ, *FullIpActQ, *FullIpPasQ,
        *StrpIpOpQ, *StrpIpOpActQ, *StrpIpOpPasQ, *FullIpOpQ, *FullIpOpActQ, *FullIpOpPasQ;

    if(bActive) {
        bHaveH = bHavebH;
        bHaveA = bHavebA;
        bHaveP = bHavebP;
        bHaveFull = bHavebFull;
        bHaveOp = bHavebOp;

        Q = Qb; ActQ = ActQb; PasQ = PasQb; OpQ = OpQb; OpActQ = OpActQb; OpPasQ = OpPasQb;
        HlStrpQ = HlStrpQb; HlStrpActQ = HlStrpActQb; HlStrpPasQ = HlStrpPasQb; HlFullQ = HlFullQb; HlFullActQ = HlFullActQb; HlFullPasQ = HlFullPasQb;
        HlStrpOpQ = HlStrpOpQb; HlStrpOpActQ = HlStrpOpActQb; HlStrpOpPasQ = HlStrpOpPasQb; HlFullOpQ = HlFullOpQb; HlFullOpActQ = HlFullOpActQb; HlFullOpPasQ = HlFullOpPasQb;
        HlStrpIpQ = HlStrpIpQb; HlStrpIpActQ = HlStrpIpActQb; HlStrpIpPasQ = HlStrpIpPasQb; HlFullIpQ = HlFullIpQb; HlFullIpActQ = HlFullIpActQb; HlFullIpPasQ = HlFullIpPasQb;
        HlStrpIpOpQ = HlStrpIpOpQb; HlStrpIpOpActQ = HlStrpIpOpActQb; HlStrpIpOpPasQ = HlStrpIpOpPasQb; HlFullIpOpQ = HlFullIpOpQb; HlFullIpOpActQ = HlFullIpOpActQb; HlFullIpOpPasQ = HlFullIpOpPasQb;
        StrpQ = StrpQb; StrpActQ = StrpActQb; StrpPasQ = StrpPasQb; FullQ = FullQb; FullActQ = FullActQb; FullPasQ = FullPasQb;
        StrpOpQ = StrpOpQb; StrpOpActQ = StrpOpActQb; StrpOpPasQ = StrpOpPasQb; FullOpQ = FullOpQb; FullOpActQ = FullOpActQb; FullOpPasQ = FullOpPasQb;
        StrpIpQ = StrpIpQb; StrpIpActQ = StrpIpActQb; StrpIpPasQ = StrpIpPasQb; FullIpQ = FullIpQb; FullIpActQ = FullIpActQb; FullIpPasQ = FullIpPasQb;
        StrpIpOpQ = StrpIpOpQb; StrpIpOpActQ = StrpIpOpActQb; StrpIpOpPasQ = StrpIpOpPasQb; FullIpOpQ = FullIpOpQb; FullIpOpActQ = FullIpOpActQb; FullIpOpPasQ = FullIpOpPasQb;
    } else {
        bHaveH = bHaveaH;
        bHaveA = bHaveaA;
        bHaveP = bHaveaP;
        bHaveFull = bHaveaFull;
        bHaveOp = bHaveaOp;

        Q = Qa; ActQ = ActQa; PasQ = PasQa; OpQ = OpQa; OpActQ = OpActQa; OpPasQ = OpPasQa;
        HlStrpQ = HlStrpQa; HlStrpActQ = HlStrpActQa; HlStrpPasQ = HlStrpPasQa; HlFullQ = HlFullQa; HlFullActQ = HlFullActQa; HlFullPasQ = HlFullPasQa;
        HlStrpOpQ = HlStrpOpQa; HlStrpOpActQ = HlStrpOpActQa; HlStrpOpPasQ = HlStrpOpPasQa; HlFullOpQ = HlFullOpQa; HlFullOpActQ = HlFullOpActQa; HlFullOpPasQ = HlFullOpPasQa;
        HlStrpIpQ = HlStrpIpQa; HlStrpIpActQ = HlStrpIpActQa; HlStrpIpPasQ = HlStrpIpPasQa; HlFullIpQ = HlFullIpQa; HlFullIpActQ = HlFullIpActQa; HlFullIpPasQ = HlFullIpPasQa;
        HlStrpIpOpQ = HlStrpIpOpQa; HlStrpIpOpActQ = HlStrpIpOpActQa; HlStrpIpOpPasQ = HlStrpIpOpPasQa; HlFullIpOpQ = HlFullIpOpQa; HlFullIpOpActQ = HlFullIpOpActQa; HlFullIpOpPasQ = HlFullIpOpPasQa;
        StrpQ = StrpQa; StrpActQ = StrpActQa; StrpPasQ = StrpPasQa; FullQ = FullQa; FullActQ = FullActQa; FullPasQ = FullPasQa;
        StrpOpQ = StrpOpQa; StrpOpActQ = StrpOpActQa; StrpOpPasQ = StrpOpPasQa; FullOpQ = FullOpQa; FullOpActQ = FullOpActQa; FullOpPasQ = FullOpPasQa;
        StrpIpQ = StrpIpQa; StrpIpActQ = StrpIpActQa; StrpIpPasQ = StrpIpPasQa; FullIpQ = FullIpQa; FullIpActQ = FullIpActQa; FullIpPasQ = FullIpPasQa;
        StrpIpOpQ = StrpIpOpQa; StrpIpOpActQ = StrpIpOpActQa; StrpIpOpPasQ = StrpIpOpPasQa; FullIpOpQ = FullIpOpQa; FullIpOpActQ = FullIpOpActQa; FullIpOpPasQ = FullIpOpPasQa;
    }
        
    if(u->ui64SharedSize == 0 || (bHaveA == false && bHaveP == false)) {
        if(((u->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == true) {
            UserAddUserList(u);
            u->ui32BoolBits &= ~User::BIT_GETNICKLIST;
            if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                UserSendQueue(u, OpQ, false);
            } else {
                UserSendQueue(u, Q, false);
            }
        } else {
            if(bHaveH == true && ((u->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                if(bHaveFull == true && ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS)) {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlFullIpOpQ, false);
                        } else {
                            UserSendQueue(u, HlFullOpQ, false);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlFullIpQ, false);
                        } else {
                            UserSendQueue(u, HlFullQ, false);
                        }
                    }
                } else {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlStrpIpOpQ, false);
                        } else {
                            UserSendQueue(u, HlStrpOpQ, false);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlStrpIpQ, false);
                        } else {
                            UserSendQueue(u, HlStrpQ, false);
                        }
                    }
                }
            } else {
                if(bHaveFull == true && ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS)) {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, FullIpOpQ, false);
                        } else {
                            UserSendQueue(u, FullOpQ, false);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, FullIpQ, false);
                        } else {
                            UserSendQueue(u, FullQ, false);
                        }
                    }
                } else {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, StrpIpOpQ, false);
                        } else {
                            UserSendQueue(u, StrpOpQ, false);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, StrpIpQ, false);
                        } else {
                            UserSendQueue(u, StrpQ, false);
                        }
                    }
                }
            }
        }
    } else if(((u->ui32BoolBits & User::BIT_ACTIVE) == User::BIT_ACTIVE) == true && bHaveP == true) {
        if(((u->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == true) {
            UserAddUserList(u);
            u->ui32BoolBits &= ~User::BIT_GETNICKLIST;
            if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                UserSendQueue(u, OpActQ);
            } else {
                UserSendQueue(u, ActQ);
            }
        } else {
            if(bHaveH == true && ((u->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                if(bHaveFull == true && ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS)) {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlFullIpOpActQ);
                        } else {
                            UserSendQueue(u, HlFullOpActQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlFullIpActQ);
                        } else {
                            UserSendQueue(u, HlFullActQ);
                        }
                    }
                } else {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlStrpIpOpActQ);
                        } else {
                            UserSendQueue(u, HlStrpOpActQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlStrpIpActQ);
                        } else {
                            UserSendQueue(u, HlStrpActQ);
                        }
                    }
                }
            } else {
                if(bHaveFull == true && ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS)) {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, FullIpOpActQ);
                        } else {
                            UserSendQueue(u, FullOpActQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, FullIpActQ);
                        } else {
                            UserSendQueue(u, FullActQ);
                        }
                    }
                } else {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, StrpIpOpActQ);
                        } else {
                            UserSendQueue(u, StrpOpActQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, StrpIpActQ);
                        } else {
                            UserSendQueue(u, StrpActQ);
                        }
                    }
                }
            }
        }
    } else {
        if(((u->ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == true) {
            UserAddUserList(u);
            u->ui32BoolBits &= ~User::BIT_GETNICKLIST;
            if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                UserSendQueue(u, OpPasQ);
            } else {
                UserSendQueue(u, PasQ);
            }
        } else {
            if(bHaveH == true && ((u->ui32BoolBits & User::BIT_SUPPORT_NOHELLO) == User::BIT_SUPPORT_NOHELLO) == false) {
                if(bHaveFull == true && ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS)) {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlFullIpOpPasQ);
                        } else {
                            UserSendQueue(u, HlFullOpPasQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlFullIpPasQ);
                        } else {
                            UserSendQueue(u, HlFullPasQ);
                        }
                    }
                } else {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlStrpIpOpPasQ);
                        } else {
                            UserSendQueue(u, HlStrpOpPasQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, HlStrpIpPasQ);
                        } else {
                            UserSendQueue(u, HlStrpPasQ);
                        }
                    }
                }
            } else {
                if(bHaveFull == true && ProfileMan->IsAllowed(u, ProfileManager::SENDFULLMYINFOS)) {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, FullIpOpPasQ);
                        } else {
                            UserSendQueue(u, FullOpPasQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, FullIpPasQ);
                        } else {
                            UserSendQueue(u, FullPasQ);
                        }
                    }
                } else {
                    if(bHaveOp == true && ((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true &&
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, StrpIpOpPasQ);
                        } else {
                            UserSendQueue(u, StrpOpPasQ);
                        }
                    } else {
                        if(bHaveIP == true && ProfileMan->IsAllowed(u, ProfileManager::SENDALLUSERIP) == true && 
                            ((u->ui32BoolBits & User::BIT_SUPPORT_USERIP2) == User::BIT_SUPPORT_USERIP2) == true) {
                            UserSendQueue(u, StrpIpPasQ);
                        } else {
                            UserSendQueue(u, StrpPasQ);
                        }
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void globalqueue::ProcessSingleItems(User * u) {
    char *MSG = NULL;
    int MSGLen = 0;
    size_t MSGSize = 0;
    QueueDataItem *qdinxt = SingleItemsQueueS;

    while(qdinxt != NULL) {
        QueueDataItem *qdicur = qdinxt;
        qdinxt = qdicur->next;
        if(qdicur->FromUser != u) {
            switch(qdicur->iType) {
                case globalqueue::PM2ALL: { // send PM to ALL
                    if(MSGSize < MSGLen+qdicur->iDataLen+u->ui8NickLen+13) {
                        size_t iAllignLen = Allign1024(MSGLen+qdicur->iDataLen+u->ui8NickLen+13);
                        char * oldbuf = MSG;
                        if(MSG == NULL) {
#ifdef _WIN32
                            MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
							MSG = (char *) malloc(iAllignLen);
#endif
                        } else {
#ifdef _WIN32
                            MSG = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
							MSG = (char *) realloc(oldbuf, iAllignLen);
#endif
						}
                        if(MSG == NULL) {
							string sDbgstr = "[BUF] "+string(u->sNick,u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+") Cannot (re)allocate "+
                                string((uint64_t)qdicur->iDataLen)+"/"+string((uint64_t)MSGLen)+"/"+string((uint64_t)iAllignLen)+
                                " bytes of memory in globalqueue::ProcessSingleItems! "+string(qdicur->sData, qdicur->iDataLen);
#ifdef _WIN32
							sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
#ifdef _WIN32
                            if(oldbuf != NULL) {
                                HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
                            }
#else
                            free(oldbuf);
#endif

							AppendSpecialLog(sDbgstr);
                            return;
                        }
                        MSGSize = (uint32_t)(iAllignLen-1);
                    }
                    int iret = sprintf(MSG+MSGLen, "$To: %s From: ", u->sNick);
                    MSGLen += iret;
                    CheckSprintf1(iret, MSGLen, MSGSize, "globalqueue::ProcessSingleItems1");
                    memcpy(MSG+MSGLen, qdicur->sData, qdicur->iDataLen);
                    MSGLen += (int)qdicur->iDataLen;
                    MSG[MSGLen] = '\0';
                    break;
                }
                case globalqueue::PM2OPS: { // send PM only to operators
                    if(((u->ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        if(MSGSize < MSGLen+qdicur->iDataLen+u->ui8NickLen+13) {
                            size_t iAllignLen = Allign1024(MSGLen+qdicur->iDataLen+u->ui8NickLen+13);
                            char * oldbuf = MSG;
                            if(MSG == NULL) {
#ifdef _WIN32
                                MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
								MSG = (char *) malloc(iAllignLen);
#endif
                            } else {
#ifdef _WIN32
                                MSG = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
								MSG = (char *) realloc(oldbuf, iAllignLen);
#endif
                            }
                            if(MSG == NULL) {
								string sDbgstr = "[BUF] "+string(u->sNick,u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+") Cannot (re)allocate "+
                                    string((uint64_t)qdicur->iDataLen)+"/"+string((uint64_t)MSGLen)+"/"+string((uint64_t)iAllignLen)+
                                    " bytes of memory in globalqueue::ProcessSingleItems1! "+string(qdicur->sData, qdicur->iDataLen);
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
#ifdef _WIN32
                                if(oldbuf != NULL) {
                                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
                                }
#else
                                free(oldbuf);
#endif

								AppendSpecialLog(sDbgstr);
								return;
                            }
                            MSGSize = (uint32_t)(iAllignLen-1);
                        }
                        int iret = sprintf(MSG+MSGLen, "$To: %s From: ", u->sNick);
                        MSGLen += iret;
                        CheckSprintf1(iret, MSGLen, MSGSize, "globalqueue::ProcessSingleItems2");
                        memcpy(MSG+MSGLen, qdicur->sData, qdicur->iDataLen);
                        MSGLen += (int)qdicur->iDataLen;
                        MSG[MSGLen] = '\0';
                    }
                    break;
                }
                case globalqueue::OPCHAT: { // send OpChat only to allowed users...
                    if(ProfileMan->IsAllowed(u, ProfileManager::ALLOWEDOPCHAT) == true) {
                        if(MSGSize < MSGLen+qdicur->iDataLen+u->ui8NickLen+13) {
                            size_t iAllignLen = Allign1024(MSGLen+qdicur->iDataLen+u->ui8NickLen+13);
                            char * oldbuf = MSG;
                            if(MSG == NULL) {
#ifdef _WIN32
                                MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
								MSG = (char *) malloc(iAllignLen);
#endif
                            } else {
#ifdef _WIN32
                                MSG = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
								MSG = (char *) realloc(oldbuf, iAllignLen);
#endif
                            }
                            if(MSG == NULL) {
								string sDbgstr = "[BUF] "+string(u->sNick,u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+") Cannot (re)allocate "+
                                    string((uint64_t)qdicur->iDataLen)+"/"+string((uint64_t)MSGLen)+"/"+string((uint64_t)iAllignLen)+
                                    " bytes of memory in globalqueue::ProcessSingleItems2! "+string(qdicur->sData, qdicur->iDataLen);
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
#ifdef _WIN32
                                if(oldbuf != NULL) {
                                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
                                }
#else
                                free(oldbuf);
#endif

								AppendSpecialLog(sDbgstr);
                                return;
                            }
                            MSGSize = (uint32_t)(iAllignLen-1);
                        }
                        int iret = sprintf(MSG+MSGLen, "$To: %s From: ", u->sNick);
                        MSGLen += iret;
                        CheckSprintf1(iret, MSGLen, MSGSize, "globalqueue::ProcessSingleItems3");
                        memcpy(MSG+MSGLen, qdicur->sData, qdicur->iDataLen);
                        MSGLen += (int)qdicur->iDataLen;
                        MSG[MSGLen] = '\0';
                    }
                    break;
                }
                case globalqueue::TOPROFILE: { // send data only to given profile...
                    if(u->iProfile == qdicur->iProfile) {
                        if(MSGSize < MSGLen+qdicur->iDataLen) {
                            size_t iAllignLen = Allign1024(MSGLen+qdicur->iDataLen+1);
                            char * oldbuf = MSG;
                            if(MSG == NULL) {
#ifdef _WIN32
                                MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
								MSG = (char *) malloc(iAllignLen);
#endif
                            } else {
#ifdef _WIN32
                                MSG = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
								MSG = (char *) realloc(oldbuf, iAllignLen);
#endif
                            }
                            if(MSG == NULL) {
								string sDbgstr = "[BUF] "+string(u->sNick,u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+") Cannot (re)allocate "+
                                    string((uint64_t)qdicur->iDataLen)+"/"+string((uint64_t)MSGLen)+"/"+string((uint64_t)iAllignLen)+
                                    " bytes of memory in globalqueue::ProcessSingleItems3! "+string(qdicur->sData, qdicur->iDataLen);
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
#ifdef _WIN32
                                if(oldbuf != NULL) {
                                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
                                }
#else
                                free(oldbuf);
#endif

								AppendSpecialLog(sDbgstr);
                                return;
                            }
                            MSGSize = (uint32_t)(iAllignLen-1);
                        }
                        memcpy(MSG+MSGLen, qdicur->sData, qdicur->iDataLen);
                        MSGLen += (int)qdicur->iDataLen;
                        MSG[MSGLen] = '\0';
                    }
                    break;
                }
                case globalqueue::PM2PROFILE: { // send pm only to given profile...
                    if(u->iProfile == qdicur->iProfile) {
                        if(MSGSize < MSGLen+qdicur->iDataLen+u->ui8NickLen+13) {
                            size_t iAllignLen = Allign1024(MSGLen+qdicur->iDataLen+u->ui8NickLen+13);
                            char * oldbuf = MSG;
                            if(MSG == NULL) {
#ifdef _WIN32
                                MSG = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, iAllignLen);
#else
								MSG = (char *) malloc(iAllignLen);
#endif
                            } else {
#ifdef _WIN32
                                MSG = (char *) HeapReAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, iAllignLen);
#else
								MSG = (char *) realloc(oldbuf, iAllignLen);
#endif
                            }
                            if(MSG == NULL) {
								string sDbgstr = "[BUF] "+string(u->sNick,u->ui8NickLen)+" ("+string(u->sIP, u->ui8IpLen)+") Cannot (re)allocate "+
                                    string((uint64_t)qdicur->iDataLen)+"/"+string((uint64_t)MSGLen)+"/"+string((uint64_t)iAllignLen)+
                                    " bytes of memory in globalqueue::ProcessSingleItems4! "+string(qdicur->sData, qdicur->iDataLen);
#ifdef _WIN32
								sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
#ifdef _WIN32
                                if(oldbuf != NULL) {
                                    HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf);
                                }
#else
                                free(oldbuf);
#endif

								AppendSpecialLog(sDbgstr);
                                return;
                            }
                            MSGSize = (uint32_t)(iAllignLen-1);
                        }
                        int iret = sprintf(MSG+MSGLen, "$To: %s From: ", u->sNick);
                        MSGLen += iret;
                        CheckSprintf1(iret, MSGLen, MSGSize, "globalqueue::ProcessSingleItems4");
                        memcpy(MSG+MSGLen, qdicur->sData, qdicur->iDataLen);
                        MSGLen += (int)qdicur->iDataLen;
                        MSG[MSGLen] = '\0';
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    if(MSG != NULL) {
        UserSendCharDelayed(u, MSG, MSGLen);
#ifdef _WIN32
        if(HeapFree(hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)MSG) == 0) {
			string sDbgstr = "[BUF] Cannot deallocate MSG in globalqueue::ProcessSingleItems! "+string((uint32_t)GetLastError())+" "+
				string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0));
            AppendSpecialLog(sDbgstr);
        }
#else
		free(MSG);
#endif
    }
}
//---------------------------------------------------------------------------

void globalqueue::SendGlobalQ() {
    if(Qa->len != 0) {
		User *nxt = colUsers->llist;
        while(nxt != NULL) {
        	User *u = nxt;
        	nxt = u->next;
        	UserSendQueue(u, globalQ->Qa, false);
        }
    }

    if(Qb->len != 0) {
		User *nxt = colUsers->llist;
        while(nxt != NULL) {
        	User *u = nxt;
        	nxt = u->next;
        	UserSendQueue(u, globalQ->Qb, false);
        }
    }
}
//---------------------------------------------------------------------------
QueueDataItem * globalqueue::CreateQueueDataItem(char * data, const size_t &idatalen, User * fromuser, const int32_t &iProfile, const uint32_t &type) {
    QueueDataItem * newitem = new QueueDataItem();
    if(newitem == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate newitem in globalqueue::CreateQueueDataItem!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	return NULL;
    }

    if(data != NULL) {
#ifdef _WIN32
        newitem->sData = (char *) HeapAlloc(hPtokaXHeap, HEAP_NO_SERIALIZE, idatalen+1);
#else
		newitem->sData = (char *) malloc(idatalen+1);
#endif
        if(newitem->sData == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string((uint64_t)(idatalen+1))+
				" bytes of memory in globalqueue::CreateQueueDataItem!";
#ifdef _WIN32
            sDbgstr += " "+string(HeapValidate(hPtokaXHeap, HEAP_NO_SERIALIZE, 0))+GetMemStat();
#endif
			AppendSpecialLog(sDbgstr);
            return newitem;
        }   
        memcpy(newitem->sData, data, idatalen);
        newitem->sData[idatalen] = '\0';
    } else {
        newitem->sData = NULL;
    }

	newitem->iDataLen = (uint32_t)idatalen;

	newitem->FromUser = fromuser;

	newitem->iType = type;

	newitem->prev = NULL;
	newitem->next = NULL;

    newitem->iProfile = iProfile;

	return newitem;
}
//---------------------------------------------------------------------------

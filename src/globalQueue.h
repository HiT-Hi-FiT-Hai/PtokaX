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
#ifndef globalQueueH
#define globalQueueH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

// zbuffer
struct QzBuf {
	size_t len, size, zlen, zsize;
	char *buffer, *zbuffer;
	bool zlined;
};
//-----------------------------------------------------------------------------

// PPK ... single items like opchat messages
struct QueueDataItem {
    size_t iDataLen;
    uint32_t iType;
    int32_t iProfile;
    char *sData;
    QueueDataItem *prev, *next;
    User *FromUser;
};
//-----------------------------------------------------------------------------

class queue {
private:
    // buffer
    struct QBuf {
    	size_t len, size;
    	char *buffer;
    };
    
    // PPK ... buffer for userip
    struct QdBuf {
    	size_t len, size;
    	char *buffer;
    	bool bHaveDollars;
    };

    QueueDataItem *SingleItemsQueueaS, *SingleItemsQueueaE,
        *SingleItemsQueuebS, *SingleItemsQueuebE;
    
    QzBuf *allqzbufsa[54];
    QzBuf *hlqzbufsa[24];
    QzBuf *srchqzbufsa[36];
    QzBuf *actqzbufsa[18];
    QzBuf *strpqzbufsa[24];
    QzBuf *fullqzbufsa[24];
    QzBuf *opqzbufsa[27];
    QzBuf *ipqzbufsa[24];
    
    QzBuf *allqzbufsb[54];
    QzBuf *hlqzbufsb[24];
    QzBuf *srchqzbufsb[36];
    QzBuf *actqzbufsb[18];
    QzBuf *strpqzbufsb[24];
    QzBuf *fullqzbufsb[24];
    QzBuf *opqzbufsb[27];
    QzBuf *ipqzbufsb[24];
    
	struct QBuf OpListQueue;
	struct QdBuf UserIPQueue;
	
	struct QzBuf *Qa, *ActQa, *PasQa, *OpQa, *OpActQa, *OpPasQa,
        *HlStrpQa, *HlStrpActQa, *HlStrpPasQa, *HlFullQa, *HlFullActQa, *HlFullPasQa,
        *HlStrpOpQa, *HlStrpOpActQa, *HlStrpOpPasQa, *HlFullOpQa, *HlFullOpActQa, *HlFullOpPasQa,
        *HlStrpIpQa, *HlStrpIpActQa, *HlStrpIpPasQa, *HlFullIpQa, *HlFullIpActQa, *HlFullIpPasQa,
        *HlStrpIpOpQa, *HlStrpIpOpActQa, *HlStrpIpOpPasQa, *HlFullIpOpQa, *HlFullIpOpActQa, *HlFullIpOpPasQa,
        *StrpQa, *StrpActQa, *StrpPasQa, *FullQa, *FullActQa, *FullPasQa,
        *StrpOpQa, *StrpOpActQa, *StrpOpPasQa, *FullOpQa, *FullOpActQa, *FullOpPasQa,
        *StrpIpQa, *StrpIpActQa, *StrpIpPasQa, *FullIpQa, *FullIpActQa, *FullIpPasQa,
        *StrpIpOpQa, *StrpIpOpActQa, *StrpIpOpPasQa, *FullIpOpQa, *FullIpOpActQa, *FullIpOpPasQa,
        *Qb, *ActQb, *PasQb, *OpQb, *OpActQb, *OpPasQb,
        *HlStrpQb, *HlStrpActQb, *HlStrpPasQb, *HlFullQb, *HlFullActQb, *HlFullPasQb,
        *HlStrpOpQb, *HlStrpOpActQb, *HlStrpOpPasQb, *HlFullOpQb, *HlFullOpActQb, *HlFullOpPasQb,
        *HlStrpIpQb, *HlStrpIpActQb, *HlStrpIpPasQb, *HlFullIpQb, *HlFullIpActQb, *HlFullIpPasQb,
        *HlStrpIpOpQb, *HlStrpIpOpActQb, *HlStrpIpOpPasQb, *HlFullIpOpQb, *HlFullIpOpActQb, *HlFullIpOpPasQb,
        *StrpQb, *StrpActQb, *StrpPasQb, *FullQb, *FullActQb, *FullPasQb,
        *StrpOpQb, *StrpOpActQb, *StrpOpPasQb, *FullOpQb, *FullOpActQb, *FullOpPasQb,
        *StrpIpQb, *StrpIpActQb, *StrpIpPasQb, *FullIpQb, *FullIpActQb, *FullIpPasQb,
        *StrpIpOpQb, *StrpIpOpActQb, *StrpIpOpPasQb, *FullIpOpQb, *FullIpOpActQb, *FullIpOpPasQb;

    bool bActive, bHaveIP,
        bHaveaQueue, bHaveaS, bHaveaH, bHaveaA, bHaveaP, bHaveaI, bHaveaStrp, bHaveaFull, bHaveaOp,
        bHavebQueue, bHavebS, bHavebH, bHavebA, bHavebP, bHavebI, bHavebStrp, bHavebFull, bHavebOp;
    
    char msg[128];
public:
    enum {
        PM2ALL,
        PM2OPS,
        OPCHAT, 
        TOPROFILE, 
        PM2PROFILE
    };

    QueueDataItem *SingleItemsQueueS;

    bool bHaveQueue;

    queue();
    ~queue();

    void Store(char * sData);
    void Store(char * sData, const size_t &iDataLen);
    void HStore(char * sData);
    void HStore(char * sData, const size_t &iDataLen);
    void AStore(char * sData, const size_t &iDataLen);
    void PStore(char * sData, const size_t &iDataLen);
    void InfoStore(char * sData, const size_t &iDataLen);
    void StrpInfoStore(char * sData, const size_t &iDataLen);
    void FullInfoStore(char * sData, const size_t &iDataLen);
    void OPStore(char * sData, const size_t &iDataLen);
    void OpListStore(char * sNick);
    void UserIPStore(User * curUser);
    void SingleItemsStore(QueueDataItem * NewItem);
    void FinalizeQueues();
    void ClearQueues();
    void ProcessQueues(User * u);
    void ProcessSingleItems(User * u);
    void SendGlobalQ();
    QueueDataItem * CreateQueueDataItem(char * data, const size_t &idatalen, User * fromuser, const int32_t &iProfile, const uint32_t &type);
};

//-----------------------------------------------------------------------------
extern queue *globalQ;
//-----------------------------------------------------------------------------

#endif

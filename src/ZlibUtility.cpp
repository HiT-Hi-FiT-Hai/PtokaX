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
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#include "utility.h"
//---------------------------------------------------------------------------
#include <zlib.h>
//---------------------------------------------------------------------------
static const uint32_t ZBUFFERLEN = 1024*256;
static const uint32_t ZMINLEN = 100;
//---------------------------------------------------------------------------
clsZlibUtility *ZlibUtility = NULL;
//---------------------------------------------------------------------------

clsZlibUtility::clsZlibUtility() {
	// allocate buffer for zlib
	sZbuffer = (char *) calloc(ZBUFFERLEN, 1);
	if(sZbuffer == NULL) {
		string sDbgstr = "[BUF] Cannot allocate "+string(ZBUFFERLEN)+
			" bytes of memory for sZbuffer in clsZlibclsZlibUtility!";
        AppendSpecialLog(sDbgstr);
		exit(EXIT_FAILURE);
	}
	memcpy(sZbuffer, "$ZOn|", 5);
    uiZbufferSize = ZBUFFERLEN;
}
//---------------------------------------------------------------------------
	
clsZlibUtility::~clsZlibUtility() {
    if(sZbuffer != NULL) {
        free(sZbuffer);
        sZbuffer = NULL;
    }
}
//---------------------------------------------------------------------------

char * clsZlibUtility::CreateZPipe(const char *sInData, const size_t &sInDataSize, uint32_t &iOutDataLen) {
    // prepare Zbuffer
    if(uiZbufferSize < sInDataSize + 128) {
        uiZbufferSize += ZBUFFERLEN;
        while(uiZbufferSize < sInDataSize + 128) {
            uiZbufferSize += ZBUFFERLEN;
        }
        
        sZbuffer = (char *) realloc(sZbuffer, uiZbufferSize);
        if(sZbuffer == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(uiZbufferSize)+
				" bytes of memory for sZbuffer in clsZlibCreateZPipe!";
			AppendSpecialLog(sDbgstr);
            uiZbufferSize = 0;
            iOutDataLen = 0;
            return sZbuffer;
        }
    }
    
    z_stream stream;

    // init zlib struct
    memset(&stream, 0 , sizeof(stream));

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_BEST_COMPRESSION);

    stream.next_in  = (Bytef*)sInData;
    stream.avail_in = (uint32_t)sInDataSize;

    stream.next_out = (Bytef*)sZbuffer+5;
    stream.avail_out = (uint32_t)(uiZbufferSize-5);

    // compress
    if(deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        AppendSpecialLog("[ERR] deflate error!");
        iOutDataLen = 0;
        return sZbuffer;
    }
    
    iOutDataLen = stream.total_out+5;

    // cleanup zlib
    deflateEnd(&stream);

    if(iOutDataLen >= sInDataSize) {
        iOutDataLen = 0;
        return sZbuffer;
    }
   
    return sZbuffer;
}
//---------------------------------------------------------------------------

char * clsZlibUtility::CreateZPipe(char *sInData, const size_t &sInDataSize, char *sOutData, size_t &iOutDataLen, size_t &iOutDataSize) {
    if(sInDataSize < ZMINLEN)
        return sOutData;

    // prepare Zbuffer
    if(uiZbufferSize < sInDataSize + 128) {
        uiZbufferSize += ZBUFFERLEN;
        while(uiZbufferSize < sInDataSize + 128) {
            uiZbufferSize += ZBUFFERLEN;
        }
        
        sZbuffer = (char *) realloc(sZbuffer, uiZbufferSize);
        if(sZbuffer == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(uiZbufferSize)+
				" bytes of memory for sZbuffer in clsZlibCreateZPipe1!";
			AppendSpecialLog(sDbgstr);
            uiZbufferSize = 0;
            iOutDataLen = 0;
            return sOutData;
        }
    }
    
    z_stream stream;

    // init zlib struct
    memset(&stream, 0 , sizeof(stream));

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_BEST_COMPRESSION);

    stream.next_in  = (Bytef*)sInData;
    stream.avail_in = (uint32_t)sInDataSize;

    stream.next_out = (Bytef*)sZbuffer+5;
    stream.avail_out = (uint32_t)(uiZbufferSize-5);

    // compress
    if(deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        AppendSpecialLog("[ERR] deflate error!");
        return sOutData;
    }
    
    iOutDataLen = stream.total_out+5;

    // cleanup zlib
    deflateEnd(&stream);

    if(iOutDataLen >= sInDataSize) {
        iOutDataLen = 0;
        return sOutData;
    }
    
    // prepare out buffer
    if(iOutDataSize < iOutDataLen) {
        iOutDataSize = (uint32_t)(Allign1024(iOutDataLen)-1);
        sOutData = (char *) realloc(sOutData, iOutDataSize+1);
        if(sOutData == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(iOutDataSize+1)+
				" bytes of memory for sOutData in clsZlibCreateZPipe1!";
			AppendSpecialLog(sDbgstr);
            return sOutData;
        }
    }
    
    memcpy(sOutData, sZbuffer, iOutDataLen);
    
    return sOutData;
}
//---------------------------------------------------------------------------

char * clsZlibUtility::CreateZPipe(char *sInData, const unsigned int &sInDataSize, char *sOutData, unsigned int &iOutDataLen, unsigned int &iOutDataSize, unsigned int iOutDataIncrease) {
    if(sInDataSize < ZMINLEN)
        return sOutData;

    // prepare Zbuffer
    if(uiZbufferSize < sInDataSize + 128) {
        uiZbufferSize += ZBUFFERLEN;
        while(uiZbufferSize < sInDataSize + 128) {
            uiZbufferSize += ZBUFFERLEN;
        }
        
        sZbuffer = (char *) realloc(sZbuffer, uiZbufferSize);
        if(sZbuffer == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(uiZbufferSize)+
				" bytes of memory for sZbuffer in clsZlibCreateZPipe2!";
			AppendSpecialLog(sDbgstr);
            uiZbufferSize = 0;
            iOutDataLen = 0;
            return sOutData;
        }
    }
    
    z_stream stream;

    // init zlib struct
    memset(&stream, 0 , sizeof (stream));

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_BEST_COMPRESSION);

    stream.next_in  = (Bytef*)sInData;
    stream.avail_in = sInDataSize;

    stream.next_out = (Bytef*)sZbuffer+5;
    stream.avail_out = (uint32_t)(uiZbufferSize-5);

    // compress
    if(deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        AppendSpecialLog("[ERR] deflate error!");
        return sOutData;
    }
    
    iOutDataLen = stream.total_out+5;

    // cleanup zlib
    deflateEnd(&stream);

    if(iOutDataLen >= sInDataSize) {
        iOutDataLen = 0;
        return sOutData;
    }
    
    // prepare out buffer
    if(iOutDataSize < iOutDataLen) {
        iOutDataSize += iOutDataIncrease;
        while(iOutDataSize < iOutDataLen) {
            iOutDataSize += iOutDataIncrease;
        }
        
        sOutData = (char *) realloc(sOutData, iOutDataSize+1);
        if(sOutData == NULL) {
			string sDbgstr = "[BUF] Cannot allocate "+string(iOutDataSize+1)+
            	" bytes of memory for sOutData in clsZlibCreateZPipe2!";
			AppendSpecialLog(sDbgstr);
            return sOutData;
        }
    }
    
    memcpy(sOutData, sZbuffer, iOutDataLen);
   
    return sOutData;
}
//---------------------------------------------------------------------------

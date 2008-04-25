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
#ifndef zlibutilityH
#define zlibutilityH
//---------------------------------------------------------------------------

class clsZlibUtility {
private:
    unsigned int uiZbufferSize;
    char *sZbuffer;
public:
	clsZlibUtility();
	~clsZlibUtility();
	
    char * CreateZPipe(const char *sInData, unsigned int sInDataSize, unsigned int &iOutDataLen);
    char * CreateZPipe(char *sInData, unsigned int sInDataSize, char *sOutData, unsigned int &iOutDataLen, unsigned int &iOutDataSize);
    char * CreateZPipe(char *sInData, unsigned int sInDataSize, char *sOutData, unsigned int &iOutDataLen, unsigned int &iOutDataSize, unsigned int iOutDataIncrease);
};
//---------------------------------------------------------------------------
extern clsZlibUtility *ZlibUtility;
//--------------------------------------------------------------------------- 

#endif

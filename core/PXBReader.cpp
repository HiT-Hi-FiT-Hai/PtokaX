/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2015  Petr Kozelka, PPK at PtokaX dot org

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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "PXBReader.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PXBReader::PXBReader() : pFile(NULL), pActualPosition(NULL), szRemainingSize(0), ui8AllocatedSize(0), bFullRead(false), pItemDatas(NULL), ui16ItemLengths(NULL), sItemIdentifiers(NULL), ui8ItemValues(NULL) {
	// ...
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
PXBReader::~PXBReader() {
#ifdef _WIN32
        if(pItemDatas != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, pItemDatas) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate pItemDatas in PXBReader::~PXBReader\n");
            }
        }
#else
		free(pItemDatas);
#endif

#ifdef _WIN32
        if(ui16ItemLengths != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui16ItemLengths) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate ui16ItemLengths in PXBReader::~PXBReader\n");
            }
        }
#else
		free(ui16ItemLengths);
#endif

#ifdef _WIN32
        if(sItemIdentifiers != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sItemIdentifiers) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate sItemIdentifiers in PXBReader::~PXBReader\n");
            }
        }
#else
		free(sItemIdentifiers);
#endif

#ifdef _WIN32
        if(ui8ItemValues != NULL) {
            if(HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ui8ItemValues) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate ui8ItemValues in PXBReader::~PXBReader\n");
            }
        }
#else
		free(ui8ItemValues);
#endif

    if(pFile != NULL) {
        fclose(pFile);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileRead(const char * sFilename, const uint8_t &ui8SubItems) {
	if(PrepareArrays(ui8SubItems) == false) {
		return false;
	}

    pFile = fopen(sFilename, "rb");

    if(pFile == NULL) {
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    long lFileLen = ftell(pFile);

    if(lFileLen <= 0) {
        return false;
    }

    fseek(pFile, 0, SEEK_SET);

    szRemainingSize = 131072;

    if((size_t)lFileLen < szRemainingSize) {
        szRemainingSize = lFileLen;

        bFullRead = true;
    }

    if(fread(clsServerManager::pGlobalBuffer, 1, szRemainingSize, pFile) != szRemainingSize) {
        return false;
    }

    pActualPosition = clsServerManager::pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::ReadNextFilePart() {
    memmove(clsServerManager::pGlobalBuffer, pActualPosition, szRemainingSize);

    size_t szReadSize = fread(clsServerManager::pGlobalBuffer + szRemainingSize, 1, 131072 - szRemainingSize, pFile);

    if(szReadSize != (131072 - szRemainingSize)) {
        bFullRead = true;
    }

    pActualPosition = clsServerManager::pGlobalBuffer;
    szRemainingSize += szReadSize;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::ReadNextItem(const uint16_t * sExpectedIdentificators, const uint8_t &ui8ExpectedSubItems, const uint8_t &ui8ExtraSubItems/* = 0*/) {
    if(szRemainingSize == 0) {
        return false;
    }

    memset(pItemDatas, 0, ui8AllocatedSize*sizeof(void *));
    memset(ui16ItemLengths, 0, ui8AllocatedSize*sizeof(uint16_t));

    if(szRemainingSize < 4) {
        if(bFullRead == true) {
            return false;
        } else { // read next part of file
            ReadNextFilePart();

            if(szRemainingSize < 4) {
                return false;
            }
        }
    }

    uint32_t ui32ItemSize = ntohl(*((uint32_t *)pActualPosition));

    if(ui32ItemSize > szRemainingSize) {
        if(bFullRead == true) {
            return false;
        } else { // read next part of file
            ReadNextFilePart();

            if(ui32ItemSize > szRemainingSize) {
                return false;
            }
        }
    }

    pActualPosition += 4;
    szRemainingSize -= 4;
    ui32ItemSize -= 4;

    uint8_t ui8ActualItem = 0;

    uint16_t ui16SubItemSize = 0;

    while(ui32ItemSize > 0) {
        ui16SubItemSize = ntohs(*((uint16_t *)pActualPosition));

        if(ui16SubItemSize > ui32ItemSize) {
            return false;
        }

        if(ui8ActualItem < ui8ExpectedSubItems && sExpectedIdentificators[ui8ActualItem] == *((uint16_t *)(pActualPosition+2))) {
            ui16ItemLengths[ui8ActualItem] = (ui16SubItemSize - 4);
            pItemDatas[ui8ActualItem] = (pActualPosition + 4);

            ui8ActualItem++;
        } else { // for compatibility with newer versions...
            for(uint8_t ui8i = 0; ui8i < (ui8ExpectedSubItems + ui8ExtraSubItems); ui8i++) {
                if(sExpectedIdentificators[ui8i] == *((uint16_t *)(pActualPosition+2))) {
                    ui16ItemLengths[ui8i] = (ui16SubItemSize - 4);
                    pItemDatas[ui8i] = (pActualPosition + 4);

                    ui8ActualItem++;
                }
            }
        }

        pActualPosition += ui16SubItemSize;
        szRemainingSize -= ui16SubItemSize;
        ui32ItemSize -= ui16SubItemSize;
    }

    if(ui8ActualItem != ui8ExpectedSubItems) {
        return false;
    } else {
        return true;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileSave(const char * sFilename, const uint8_t &ui8Size) {
	if(PrepareArrays(ui8Size) == false) {
		return false;
	}

    pFile = fopen(sFilename, "wb");

    if(pFile == NULL) {
        return false;
    }

    szRemainingSize = 131072;

    pActualPosition = clsServerManager::pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::WriteNextItem(const uint32_t &ui32Length, const uint8_t &ui8SubItems) {
    uint32_t ui32ItemLength = ui32Length + 4 + (4 * ui8SubItems);

    if(ui32ItemLength > szRemainingSize) {
        fwrite(clsServerManager::pGlobalBuffer, 1, pActualPosition-clsServerManager::pGlobalBuffer, pFile);
        pActualPosition = clsServerManager::pGlobalBuffer;
        szRemainingSize = 131072;
    }

    (*((uint32_t *)pActualPosition)) = htonl(ui32ItemLength);
    pActualPosition += 4;
    szRemainingSize -= 4;

    for(uint8_t ui8i = 0; ui8i < ui8SubItems; ui8i++) {
        (*((uint16_t *)(pActualPosition))) = htons(ui16ItemLengths[ui8i] + 4);

        pActualPosition[2] = sItemIdentifiers[(ui8i*2)];
        pActualPosition[3] = sItemIdentifiers[(ui8i*2)+1];

        switch(ui8ItemValues[ui8i]) {
            case PXB_BYTE:
                pActualPosition[4] = (pItemDatas[ui8i] == 0 ? '0' : '1');
                break;
            case PXB_TWO_BYTES:
                (*((uint16_t *)(pActualPosition+4))) = htons(*((uint16_t *)pItemDatas[ui8i]));
                break;
            case PXB_FOUR_BYTES:
                (*((uint32_t *)(pActualPosition+4))) = htonl(*((uint32_t *)pItemDatas[ui8i]));
                break;
            case PXB_EIGHT_BYTES:
            	(*((uint64_t *)(pActualPosition+4))) = htobe64(*((uint64_t *)pItemDatas[ui8i]));
            	break;
            case PXB_STRING:
                memcpy(pActualPosition+4, pItemDatas[ui8i], ui16ItemLengths[ui8i]);
                break;
            default:
                break;
        }

        pActualPosition += ui16ItemLengths[ui8i] + 4;
        szRemainingSize -= ui16ItemLengths[ui8i] + 4;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::WriteRemaining() {
    if((pActualPosition-clsServerManager::pGlobalBuffer) > 0) {
        fwrite(clsServerManager::pGlobalBuffer, 1, pActualPosition-clsServerManager::pGlobalBuffer, pFile);
    }

    fclose(pFile);
    pFile = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::PrepareArrays(const uint8_t &ui8Size) {
#ifdef _WIN32
    pItemDatas = (void **)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*sizeof(void *));
#else
	pItemDatas = (void **)calloc(ui8Size, sizeof(void *));
#endif
    if(pItemDatas == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pItemDatas in PXBReader::PrepareArrays\n");
		return false;
    }

#ifdef _WIN32
    ui16ItemLengths = (uint16_t *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*sizeof(uint16_t));
#else
	ui16ItemLengths = (uint16_t *)calloc(ui8Size, sizeof(uint16_t));
#endif
    if(ui16ItemLengths == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create ui16ItemLengths in PXBReader::PrepareArrays\n");
		return false;
    }

#ifdef _WIN32
    sItemIdentifiers = (char *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*(sizeof(char)*2));
#else
	sItemIdentifiers = (char *)calloc(ui8Size, sizeof(char)*2);
#endif
    if(sItemIdentifiers == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create sItemIdentifiers in PXBReader::PrepareArrays\n");
		return false;
    }

#ifdef _WIN32
    ui8ItemValues = (uint8_t *)HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*sizeof(ui8ItemValues));
#else
	ui8ItemValues = (uint8_t *)calloc(ui8Size, sizeof(ui8ItemValues));
#endif
    if(ui8ItemValues == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create ui8ItemValues in PXBReader::PrepareArrays\n");
		return false;
    }

	ui8AllocatedSize = ui8Size;
	return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

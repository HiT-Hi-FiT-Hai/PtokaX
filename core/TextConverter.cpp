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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "TextConverter.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TextConverter * TextConverter::mPtr = NULL;
#ifdef _WIN32
	static wchar_t wcTempBuf[2048];
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

TextConverter::TextConverter() {
#ifndef _WIN32
	if(clsSettingManager::mPtr->sTexts[SETTXT_ENCODING] == NULL) {
		AppendLog("TextConverter failed to initialize - TextEncoding not set!");
		exit(EXIT_FAILURE);
	}

	iconvUtfCheck = iconv_open("utf-8", "utf-8");
	if(iconvUtfCheck == (iconv_t)-1) {
		AppendLog("TextConverter iconv_open for iconvUtfCheck failed!");
		exit(EXIT_FAILURE);
	}

	iconvAsciiToUtf = iconv_open("utf-8//TRANSLIT//IGNORE", clsSettingManager::mPtr->sTexts[SETTXT_ENCODING]);
	if(iconvAsciiToUtf == (iconv_t)-1) {
		AppendLog("TextConverter iconv_open for iconvAsciiToUtf failed!");
		exit(EXIT_FAILURE);
	}
#endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
TextConverter::~TextConverter() {
#ifndef _WIN32
	iconv_close(iconvUtfCheck);
	iconv_close(iconvAsciiToUtf);
#endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
bool TextConverter::CheckUtf8Validity(char * sInput, const uint8_t &ui8InputLen, char * /*sOutput*/, const uint8_t &/*ui8OutputSize*/) {
	if(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sInput, ui8InputLen, NULL, 0) == 0) {
		return false;
#else
bool TextConverter::CheckUtf8Validity(char * sInput, const uint8_t &ui8InputLen, char * sOutput, const uint8_t &ui8OutputSize) {
	char * sInBuf = sInput;
	size_t szInbufLeft = ui8InputLen;

	char * sOutBuf = sOutput;
	size_t szOutbufLeft = ui8OutputSize-1;

	size_t szRet = iconv(iconvUtfCheck, &sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
	if(szRet == (size_t)-1) {
		return false;
#endif
	} else {
		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

size_t TextConverter::CheckUtf8AndConvert(char * sInput, const uint8_t &ui8InputLen, char * sOutput, const uint8_t &ui8OutputSize) {
#ifdef _WIN32
	if(CheckUtf8Validity(sInput, ui8InputLen, sOutput, ui8OutputSize) == true) {
		memcpy(sOutput, sInput, ui8InputLen);
		sOutput[ui8InputLen] = '\0';

		return ui8InputLen;
	}

	int iMtoWRegLen = MultiByteToWideChar(CP_ACP, 0, sInput, ui8InputLen, NULL, 0);
	if(iMtoWRegLen == 0) {
		sOutput[0] = '\0';
		return 0;
	}

	wchar_t * wcTemp = 	wcTempBuf;
	if(iMtoWRegLen > 2048) {
		wcTemp = (wchar_t *)::HeapAlloc(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, iMtoWRegLen * sizeof(wchar_t));
		if(wcTemp == NULL) {	
			sOutput[0] = '\0';
			return 0;
		}
	}

	if(::MultiByteToWideChar(CP_ACP, 0, sInput, ui8InputLen, wcTemp, iMtoWRegLen) == 0) {
		if(wcTemp != wcTempBuf) {
			::HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
		}
				
		sOutput[0] = '\0';
		return 0;
	}

	int iWtoMRegLen = ::WideCharToMultiByte(CP_UTF8, 0, wcTemp, iMtoWRegLen, NULL, 0, NULL, NULL);
	if(iWtoMRegLen == 0) {
		if(wcTemp != wcTempBuf) {
			::HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
		}
		sOutput[0] = '\0';
		return 0;
	}

	if(iWtoMRegLen > (ui8OutputSize-1)) {
		iWtoMRegLen = ::WideCharToMultiByte(CP_UTF8, 0, wcTemp, --iMtoWRegLen, NULL, 0, NULL, NULL);

		while(iWtoMRegLen > (ui8OutputSize-1) && iMtoWRegLen > 0) {
			iWtoMRegLen = ::WideCharToMultiByte(CP_UTF8, 0, wcTemp, --iMtoWRegLen, NULL, 0, NULL, NULL);
		}

		if(iMtoWRegLen == 0) {
			if(wcTemp != wcTempBuf) {
				::HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
			}

			sOutput[0] = '\0';
			return 0;
		}
	}

	if(::WideCharToMultiByte(CP_UTF8, 0, wcTemp, iMtoWRegLen, sOutput, iWtoMRegLen, NULL, NULL) == 0) {
		if(wcTemp != wcTempBuf) {
			::HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
		}

		sOutput[0] = '\0';
		return 0;
	}

	if(wcTemp != wcTempBuf) {
		::HeapFree(clsServerManager::hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
	}

	sOutput[iWtoMRegLen] = '\0';
	return iWtoMRegLen;
#else
	if(CheckUtf8Validity(sInput, ui8InputLen, sOutput, ui8OutputSize) == true) {
		sOutput[ui8InputLen] = '\0';
		return ui8InputLen;
	}

	char * sInBuf = sInput;
	size_t szInbufLeft = ui8InputLen;

	char * sOutBuf = sOutput;
	size_t szOutbufLeft = ui8OutputSize-1;

	size_t szRet = iconv(iconvAsciiToUtf, &sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
	if(szRet == (size_t)-1) {
		if(errno == E2BIG) {
			clsUdpDebug::mPtr->Broadcast("[LOG] TextConverter::DoIconv iconv E2BIG for param: "+string(sInput, ui8InputLen));
		} else if(errno == EILSEQ) {
			sInBuf++;
			szInbufLeft--;

			while(szInbufLeft != 0) {
				szRet = iconv(iconvAsciiToUtf, &sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
				if(szRet == (size_t)-1) {
					if(errno == E2BIG) {
						clsUdpDebug::mPtr->Broadcast("[LOG] TextConverter::DoIconv iconv E2BIG in EILSEQ for param: "+string(sInput, ui8InputLen));
					} else if(errno == EINVAL) {
						clsUdpDebug::mPtr->Broadcast("[LOG] TextConverter::DoIconv iconv EINVAL in EILSEQ for param: "+string(sInput, ui8InputLen));
						sOutput[0] = '\0';
						return 0;
					} else if(errno == EILSEQ) {
						sInBuf++;
						szInbufLeft--;

						continue;
					}
				}
			}

			if(szOutbufLeft == size_t(ui8OutputSize-1)) {
				clsUdpDebug::mPtr->Broadcast("[LOG] TextConverter::DoIconv iconv EILSEQ for param: "+string(sInput, ui8InputLen));
				sOutput[0] = '\0';
				return 0;
			}
		} else if(errno == EINVAL) {
			clsUdpDebug::mPtr->Broadcast("[LOG] TextConverter::DoIconv iconv EINVAL for param: "+string(sInput, ui8InputLen));
			sOutput[0] = '\0';
			return 0;
		}
	}

	sOutput[(ui8OutputSize-szOutbufLeft)-1] = '\0';
	return (ui8OutputSize-szOutbufLeft)-1;
#endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

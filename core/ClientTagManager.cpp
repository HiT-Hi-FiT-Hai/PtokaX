/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
 * Copyright (C) 2004-2012  Petr Kozelka, PPK at PtokaX dot org

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
#include "ClientTagManager.h"
//---------------------------------------------------------------------------
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
ClientTagMan *ClientTagManager = NULL;
//---------------------------------------------------------------------------

ClientTagMan::ClientTagMan() {
    cliTags = (ClientTag *) calloc(256, sizeof(ClientTag));
    if(cliTags == NULL) {
    	string sDbgstr = "[BUF] Cannot allocate cliTags!";
#ifdef _WIN32
		sDbgstr += " "+string(HeapValidate(GetProcessHeap, 0, 0))+GetMemStat();
#endif
		AppendSpecialLog(sDbgstr);
    	exit(EXIT_FAILURE);
    }

    Load();
}
//---------------------------------------------------------------------------
	
ClientTagMan::~ClientTagMan() {
    free(cliTags);
    cliTags = NULL;
}
//---------------------------------------------------------------------------

void ClientTagMan::Load() {
#ifdef _WIN32
	TiXmlDocument doc((PATH+"\\cfg\\ClientTags.xml").c_str());
#else
	TiXmlDocument doc((PATH+"/cfg/ClientTags.xml").c_str());
#endif
    if(doc.LoadFile()) {
        TiXmlHandle cfg(&doc);
        TiXmlNode *clienttags = cfg.FirstChild("ClientTags").Node();
		if(clienttags != NULL) {
        	uint32_t i = 0;
			TiXmlNode *child = NULL;
			while((child = clienttags->IterateChildren(child)) != NULL) {
				TiXmlNode *clienttag = child->FirstChild("ClientTag");

				if(clienttag == NULL ||
                    (clienttag = clienttag->FirstChild()) == NULL) {
					continue;
				}

                const char *patt = clienttag->Value();
                
				if((clienttag = child->FirstChild("ClientName")) == NULL ||
                    (clienttag = clienttag->FirstChild()) == NULL) {
					continue;
				}

                const char *name = clienttag->Value();
                
				strcpy(cliTags[i].TagPatt, patt);
				cliTags[i].PattLen = strlen(cliTags[i].TagPatt);
				strcpy(cliTags[i].CliName, name);
				i++;
            }
        }
    } else {
#ifdef _WIN32
		TiXmlDocument doc((PATH+"\\cfg\\ClientTags.xml").c_str());
#else
		TiXmlDocument doc((PATH+"/cfg/ClientTags.xml").c_str());
#endif
		doc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
		TiXmlElement clienttags("ClientTags");
		const char* ClientPatts[] = { "++", "DCGUI", "DC", "oDC", "DC:PRO", "QuickDC", "LDC++", "R2++", "Goofy++", "PWDC++", "BDC++", "zK++" };
		const char* ClientNames[] = { "DC++", "Valknut", "NMDC2", "oDC", "DC:PRO", "QuickDC", "LDC++", "R2++", "Goofy++", "PWDC++", "BDC++", "zK++" };
		for(uint8_t i = 0; i < 12; i++) {
			TiXmlElement clientpatt("ClientTag");
			clientpatt.InsertEndChild(TiXmlText(ClientPatts[i]));

			TiXmlElement clientname("ClientName");
			clientname.InsertEndChild(TiXmlText(ClientNames[i]));

			TiXmlElement clienttag("Client");
			clienttag.InsertEndChild(clientpatt);
			clienttag.InsertEndChild(clientname);

			clienttags.InsertEndChild(clienttag);

			strcpy(cliTags[i].TagPatt, ClientPatts[i]);
			cliTags[i].PattLen = strlen(cliTags[i].TagPatt);
			strcpy(cliTags[i].CliName, ClientNames[i]);
		}
		doc.InsertEndChild(clienttags);
		doc.SaveFile();
    }
}
//---------------------------------------------------------------------------

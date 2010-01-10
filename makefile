#*******************************************************************************
#
# Makefile for PtokaX.
#
#*******************************************************************************

#*******************************************************************************
# Compiler
#*******************************************************************************
CXX = g++

#*******************************************************************************
# Debug flags
#*******************************************************************************
CXXFLAGS = -g -Wall

#*******************************************************************************
# Release flags
#*******************************************************************************
#CXXFLAGS = -O -Wall

#*******************************************************************************
# Include
#*******************************************************************************
INCLUDE = -Itinyxml -I/usr/include -I/usr/local/include -I/usr/include/lua5.1 -I/usr/pkg/include -I/usr/include/lua -I/usr/include/lua/5.1

#*******************************************************************************
# Binary to create
#
# Lua in Debian is lua5.1. Other known names are lua-5.1, lua51 and lua.
# If you have Lua lib with another name than lua5.1 change it in -llua5.1.
#
# In case when you don't have Lua as dynamic library (in debian liblua5.1.so)
# but as static library (liblua.a default when you compile Lua from sources)
# then remove -llua5.1 and after tinyxml/tinyxml.a add /usr/local/lib/liblua.a (default path when is Lua compiled from sources).
#*******************************************************************************
PtokaX: $(CURDIR)/obj/ClientTagManager.o $(CURDIR)/obj/colUsers.o $(CURDIR)/obj/DcCommands.o $(CURDIR)/obj/DeFlood.o $(CURDIR)/obj/eventqueue.o $(CURDIR)/obj/globalQueue.o $(CURDIR)/obj/hashBanManager.o $(CURDIR)/obj/hashUsrManager.o \
  $(CURDIR)/obj/hashRegManager.o $(CURDIR)/obj/HubCommands.o $(CURDIR)/obj/IP2Country.o $(CURDIR)/obj/LanguageManager.o $(CURDIR)/obj/LuaBanManLib.o $(CURDIR)/obj/LuaCoreLib.o $(CURDIR)/obj/LuaIP2CountryLib.o $(CURDIR)/obj/LuaProfManLib.o \
  $(CURDIR)/obj/LuaRegManLib.o $(CURDIR)/obj/LuaScript.o $(CURDIR)/obj/LuaScriptManager.o $(CURDIR)/obj/LuaScriptManLib.o $(CURDIR)/obj/LuaSetManLib.o $(CURDIR)/obj/LuaTmrManLib.o $(CURDIR)/obj/LuaUDPDbgLib.o $(CURDIR)/obj/ProfileManager.o \
  $(CURDIR)/obj/PtokaX.o $(CURDIR)/obj/pxstring.o $(CURDIR)/obj/RegThread.o $(CURDIR)/obj/ResNickManager.o $(CURDIR)/obj/ServerManager.o $(CURDIR)/obj/ServerThread.o $(CURDIR)/obj/serviceLoop.o $(CURDIR)/obj/SettingManager.o \
  $(CURDIR)/obj/TextFileManager.o $(CURDIR)/obj/UdpDebug.o $(CURDIR)/obj/UDPThread.o $(CURDIR)/obj/User.o $(CURDIR)/obj/utility.o $(CURDIR)/obj/ZlibUtility.o
	$(CXX) -lpthread -lz -llua5.1 -lrt -o PtokaX \
        $(CURDIR)/obj/ClientTagManager.o $(CURDIR)/obj/colUsers.o $(CURDIR)/obj/DcCommands.o $(CURDIR)/obj/DeFlood.o $(CURDIR)/obj/eventqueue.o $(CURDIR)/obj/globalQueue.o $(CURDIR)/obj/hashBanManager.o $(CURDIR)/obj/hashUsrManager.o \
        $(CURDIR)/obj/hashRegManager.o $(CURDIR)/obj/HubCommands.o $(CURDIR)/obj/IP2Country.o $(CURDIR)/obj/LanguageManager.o $(CURDIR)/obj/LuaBanManLib.o $(CURDIR)/obj/LuaCoreLib.o $(CURDIR)/obj/LuaIP2CountryLib.o \
        $(CURDIR)/obj/LuaProfManLib.o $(CURDIR)/obj/LuaRegManLib.o $(CURDIR)/obj/LuaScript.o $(CURDIR)/obj/LuaScriptManager.o $(CURDIR)/obj/LuaScriptManLib.o $(CURDIR)/obj/LuaSetManLib.o $(CURDIR)/obj/LuaTmrManLib.o \
        $(CURDIR)/obj/LuaUDPDbgLib.o $(CURDIR)/obj/ProfileManager.o $(CURDIR)/obj/PtokaX.o $(CURDIR)/obj/pxstring.o $(CURDIR)/obj/RegThread.o $(CURDIR)/obj/ResNickManager.o $(CURDIR)/obj/ServerManager.o $(CURDIR)/obj/ServerThread.o \
        $(CURDIR)/obj/serviceLoop.o $(CURDIR)/obj/SettingManager.o $(CURDIR)/obj/TextFileManager.o $(CURDIR)/obj/UdpDebug.o $(CURDIR)/obj/UDPThread.o $(CURDIR)/obj/User.o $(CURDIR)/obj/utility.o $(CURDIR)/obj/ZlibUtility.o \
        $(CURDIR)/tinyxml/tinyxml.a

#*******************************************************************************
# Files to compile
#*******************************************************************************
$(CURDIR)/obj/ClientTagManager.o: $(CURDIR)/src/ClientTagManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h \
  $(CURDIR)/src/ClientTagManager.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/ClientTagManager.cpp -o $(CURDIR)/obj/ClientTagManager.o

$(CURDIR)/obj/colUsers.o: $(CURDIR)/src/colUsers.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/colUsers.h $(CURDIR)/src/globalQueue.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/ServerManager.h \
  $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/colUsers.cpp -o $(CURDIR)/obj/colUsers.o

$(CURDIR)/obj/DcCommands.o: $(CURDIR)/src/DcCommands.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/DcCommands.h $(CURDIR)/src/colUsers.h \
  $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashBanManager.h $(CURDIR)/src/hashRegManager.h $(CURDIR)/src/hashUsrManager.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ProfileManager.h \
  $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h \
  $(CURDIR)/src/utility.h $(CURDIR)/src/ZlibUtility.h $(CURDIR)/src/DeFlood.h $(CURDIR)/src/HubCommands.h $(CURDIR)/src/IP2Country.h \
  $(CURDIR)/src/ResNickManager.h $(CURDIR)/src/TextFileManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/DcCommands.cpp -o $(CURDIR)/obj/DcCommands.o

$(CURDIR)/obj/DeFlood.o: $(CURDIR)/src/DeFlood.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashBanManager.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h \
  $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/DeFlood.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/DeFlood.cpp -o $(CURDIR)/obj/DeFlood.o

$(CURDIR)/obj/eventqueue.o: $(CURDIR)/src/eventqueue.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/eventqueue.h \
  $(CURDIR)/src/DcCommands.h $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ServerManager.h \
  $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h \
  $(CURDIR)/src/RegThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/eventqueue.cpp -o $(CURDIR)/obj/eventqueue.o

$(CURDIR)/obj/globalQueue.o: $(CURDIR)/src/globalQueue.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/globalQueue.h \
  $(CURDIR)/src/colUsers.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/serviceLoop.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/globalQueue.cpp -o $(CURDIR)/obj/globalQueue.o

$(CURDIR)/obj/hashBanManager.o: $(CURDIR)/src/hashBanManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/hashBanManager.h \
  $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/hashBanManager.cpp -o $(CURDIR)/obj/hashBanManager.o

$(CURDIR)/obj/hashUsrManager.o: $(CURDIR)/src/hashUsrManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/hashUsrManager.h \
  $(CURDIR)/src/hashBanManager.h $(CURDIR)/src/hashRegManager.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/hashUsrManager.cpp -o $(CURDIR)/obj/hashUsrManager.o

$(CURDIR)/obj/hashRegManager.o: $(CURDIR)/src/hashRegManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/hashRegManager.h \
  $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/ProfileManager.h \
  $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/hashRegManager.cpp -o $(CURDIR)/obj/hashRegManager.o

$(CURDIR)/obj/HubCommands.o: $(CURDIR)/src/HubCommands.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/colUsers.h \
  $(CURDIR)/src/DcCommands.h $(CURDIR)/src/eventqueue.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashBanManager.h \
  $(CURDIR)/src/hashRegManager.h $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h \
  $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/serviceLoop.h \
  $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/HubCommands.h \
  $(CURDIR)/src/IP2Country.h $(CURDIR)/src/LuaScript.h $(CURDIR)/src/TextFileManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/HubCommands.cpp -o $(CURDIR)/obj/HubCommands.o

$(CURDIR)/obj/IP2Country.o: $(CURDIR)/src/IP2Country.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/IP2Country.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/IP2Country.cpp -o $(CURDIR)/obj/IP2Country.o

$(CURDIR)/obj/LanguageManager.o: $(CURDIR)/src/LanguageManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LanguageXml.h \
  $(CURDIR)/src/LanguageStrings.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/SettingManager.h \
  $(CURDIR)/src/SettingIds.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LanguageManager.cpp -o $(CURDIR)/obj/LanguageManager.o

$(CURDIR)/obj/LuaBanManLib.o: $(CURDIR)/src/LuaBanManLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaBanManLib.h $(CURDIR)/src/hashBanManager.h $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LuaScriptManager.h \
  $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaBanManLib.cpp -o $(CURDIR)/obj/LuaBanManLib.o

$(CURDIR)/obj/LuaCoreLib.o: $(CURDIR)/src/LuaCoreLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h $(CURDIR)/src/LuaCoreLib.h \
  $(CURDIR)/src/colUsers.h $(CURDIR)/src/eventqueue.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashBanManager.h $(CURDIR)/src/hashUsrManager.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ServerManager.h \
  $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/IP2Country.h \
  $(CURDIR)/src/ResNickManager.h $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaCoreLib.cpp -o $(CURDIR)/obj/LuaCoreLib.o

$(CURDIR)/obj/LuaIP2CountryLib.o: $(CURDIR)/src/LuaIP2CountryLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaIP2CountryLib.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/IP2Country.h \
  $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaIP2CountryLib.cpp -o $(CURDIR)/obj/LuaIP2CountryLib.o

$(CURDIR)/obj/LuaProfManLib.o: $(CURDIR)/src/LuaProfManLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaProfManLib.h $(CURDIR)/src/ProfileManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaProfManLib.cpp -o $(CURDIR)/obj/LuaProfManLib.o

$(CURDIR)/obj/LuaRegManLib.o: $(CURDIR)/src/LuaRegManLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaRegManLib.h $(CURDIR)/src/colUsers.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashRegManager.h $(CURDIR)/src/hashUsrManager.h \
  $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h \
  $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaRegManLib.cpp -o $(CURDIR)/obj/LuaRegManLib.o

$(CURDIR)/obj/LuaScript.o: $(CURDIR)/src/LuaScript.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h $(CURDIR)/src/colUsers.h \
  $(CURDIR)/src/eventqueue.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LanguageManager.h \
  $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h \
  $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h $(CURDIR)/src/IP2Country.h \
  $(CURDIR)/src/LuaCoreLib.h $(CURDIR)/src/LuaBanManLib.h $(CURDIR)/src/LuaIP2CountryLib.h $(CURDIR)/src/LuaProfManLib.h \
  $(CURDIR)/src/LuaRegManLib.h $(CURDIR)/src/LuaScriptManLib.h $(CURDIR)/src/LuaSetManLib.h $(CURDIR)/src/LuaTmrManLib.h \
  $(CURDIR)/src/LuaUDPDbgLib.h $(CURDIR)/src/ResNickManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaScript.cpp -o $(CURDIR)/obj/LuaScript.o

$(CURDIR)/obj/LuaScriptManager.o: $(CURDIR)/src/LuaScriptManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/User.h \
  $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaScriptManager.cpp -o $(CURDIR)/obj/LuaScriptManager.o

$(CURDIR)/obj/LuaScriptManLib.o: $(CURDIR)/src/LuaScriptManLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaScriptManLib.h $(CURDIR)/src/eventqueue.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaScriptManLib.cpp -o $(CURDIR)/obj/LuaScriptManLib.o

$(CURDIR)/obj/LuaSetManLib.o: $(CURDIR)/src/LuaSetManLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaSetManLib.h $(CURDIR)/src/eventqueue.h $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LuaScriptManager.h \
  $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaSetManLib.cpp -o $(CURDIR)/obj/LuaSetManLib.o

$(CURDIR)/obj/LuaTmrManLib.o: $(CURDIR)/src/LuaTmrManLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaTmrManLib.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/scrtmrinc.h $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaTmrManLib.cpp -o $(CURDIR)/obj/LuaTmrManLib.o

$(CURDIR)/obj/LuaUDPDbgLib.o: $(CURDIR)/src/LuaUDPDbgLib.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LuaInc.h \
  $(CURDIR)/src/LuaUDPDbgLib.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/utility.h $(CURDIR)/src/LuaScript.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/LuaUDPDbgLib.cpp -o $(CURDIR)/obj/LuaUDPDbgLib.o

$(CURDIR)/obj/ProfileManager.o: $(CURDIR)/src/ProfileManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/ProfileManager.h \
  $(CURDIR)/src/colUsers.h $(CURDIR)/src/hashRegManager.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h \
  $(CURDIR)/src/ServerManager.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/ProfileManager.cpp -o $(CURDIR)/obj/ProfileManager.o

$(CURDIR)/obj/PtokaX.o: $(CURDIR)/src/PtokaX-nix.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h \
  $(CURDIR)/src/regtmrinc.h $(CURDIR)/src/scrtmrinc.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/PtokaX-nix.cpp -o $(CURDIR)/obj/PtokaX.o

$(CURDIR)/obj/pxstring.o: $(CURDIR)/src/pxstring.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/pxstring.cpp -o $(CURDIR)/obj/pxstring.o

$(CURDIR)/obj/RegThread.o: $(CURDIR)/src/RegThread.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/eventqueue.h \
  $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/utility.h $(CURDIR)/src/RegThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/RegThread.cpp -o $(CURDIR)/obj/RegThread.o

$(CURDIR)/obj/ResNickManager.o: $(CURDIR)/src/ResNickManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/ResNickManager.h \
  $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/ResNickManager.cpp -o $(CURDIR)/obj/ResNickManager.o

$(CURDIR)/obj/ServerManager.o: $(CURDIR)/src/ServerManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/ServerManager.h \
  $(CURDIR)/src/colUsers.h $(CURDIR)/src/DcCommands.h $(CURDIR)/src/eventqueue.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashBanManager.h \
  $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/hashRegManager.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h \
  $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/regtmrinc.h $(CURDIR)/src/serviceLoop.h $(CURDIR)/src/SettingManager.h \
  $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/utility.h $(CURDIR)/src/ZlibUtility.h $(CURDIR)/src/ClientTagManager.h \
  $(CURDIR)/src/HubCommands.h $(CURDIR)/src/IP2Country.h $(CURDIR)/src/RegThread.h $(CURDIR)/src/ResNickManager.h $(CURDIR)/src/ServerThread.h \
  $(CURDIR)/src/TextFileManager.h $(CURDIR)/src/UDPThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/ServerManager.cpp -o $(CURDIR)/obj/ServerManager.o

$(CURDIR)/obj/ServerThread.o: $(CURDIR)/src/ServerThread.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/eventqueue.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/serviceLoop.h \
  $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/utility.h $(CURDIR)/src/ServerThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/ServerThread.cpp -o $(CURDIR)/obj/ServerThread.o

$(CURDIR)/obj/serviceLoop.o: $(CURDIR)/src/serviceLoop.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/serviceLoop.h \
  $(CURDIR)/src/colUsers.h $(CURDIR)/src/eventqueue.h $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashBanManager.h $(CURDIR)/src/hashUsrManager.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/regtmrinc.h \
  $(CURDIR)/src/scrtmrinc.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/User.h \
  $(CURDIR)/src/utility.h $(CURDIR)/src/ZlibUtility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/serviceLoop.cpp -o $(CURDIR)/obj/serviceLoop.o

$(CURDIR)/obj/SettingManager.o: $(CURDIR)/src/SettingManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/SettingXml.h \
  $(CURDIR)/src/SettingDefaults.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/colUsers.h \
  $(CURDIR)/src/globalQueue.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/LuaScriptManager.h \
  $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h $(CURDIR)/src/ResNickManager.h \
  $(CURDIR)/src/ServerThread.h $(CURDIR)/src/TextFileManager.h $(CURDIR)/src/UDPThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/SettingManager.cpp -o $(CURDIR)/obj/SettingManager.o

$(CURDIR)/obj/TextFileManager.o: $(CURDIR)/src/TextFileManager.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h \
  $(CURDIR)/src/TextFileManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/User.h $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/TextFileManager.cpp -o $(CURDIR)/obj/TextFileManager.o

$(CURDIR)/obj/UdpDebug.o: $(CURDIR)/src/UdpDebug.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/LanguageManager.h \
  $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/User.h \
  $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/UdpDebug.cpp -o $(CURDIR)/obj/UdpDebug.o

$(CURDIR)/obj/UDPThread.o: $(CURDIR)/src/UDPThread.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/eventqueue.h \
  $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h $(CURDIR)/src/SettingIds.h $(CURDIR)/src/utility.h $(CURDIR)/src/UDPThread.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/UDPThread.cpp -o $(CURDIR)/obj/UDPThread.o

$(CURDIR)/obj/User.o: $(CURDIR)/src/User.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/User.h $(CURDIR)/src/colUsers.h $(CURDIR)/src/DcCommands.h \
  $(CURDIR)/src/globalQueue.h $(CURDIR)/src/hashUsrManager.h $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h \
  $(CURDIR)/src/LuaScriptManager.h $(CURDIR)/src/ProfileManager.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h \
  $(CURDIR)/src/SettingIds.h $(CURDIR)/src/utility.h $(CURDIR)/src/UdpDebug.h $(CURDIR)/src/ZlibUtility.h $(CURDIR)/src/ClientTagManager.h \
  $(CURDIR)/src/DeFlood.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/User.cpp -o $(CURDIR)/obj/User.o

$(CURDIR)/obj/utility.o: $(CURDIR)/src/utility.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/utility.h $(CURDIR)/src/hashBanManager.h \
  $(CURDIR)/src/LanguageManager.h $(CURDIR)/src/LanguageIds.h $(CURDIR)/src/ServerManager.h $(CURDIR)/src/SettingManager.h \
  $(CURDIR)/src/SettingIds.h $(CURDIR)/src/UdpDebug.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/utility.cpp -o $(CURDIR)/obj/utility.o

$(CURDIR)/obj/ZlibUtility.o: $(CURDIR)/src/ZlibUtility.cpp $(CURDIR)/src/stdinc.h $(CURDIR)/src/pxstring.h $(CURDIR)/src/ZlibUtility.h \
  $(CURDIR)/src/utility.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $(CURDIR)/src/ZlibUtility.cpp -o $(CURDIR)/obj/ZlibUtility.o

#*******************************************************************************
# Cleanup
#*******************************************************************************
clean:
	-rm $(CURDIR)/obj/ClientTagManager.o $(CURDIR)/obj/colUsers.o $(CURDIR)/obj/DcCommands.o $(CURDIR)/obj/DeFlood.o $(CURDIR)/obj/eventqueue.o $(CURDIR)/obj/globalQueue.o $(CURDIR)/obj/hashBanManager.o $(CURDIR)/obj/hashUsrManager.o \
        $(CURDIR)/obj/hashRegManager.o $(CURDIR)/obj/HubCommands.o $(CURDIR)/obj/IP2Country.o $(CURDIR)/obj/LanguageManager.o $(CURDIR)/obj/LuaBanManLib.o $(CURDIR)/obj/LuaCoreLib.o $(CURDIR)/obj/LuaIP2CountryLib.o \
        $(CURDIR)/obj/LuaProfManLib.o $(CURDIR)/obj/LuaRegManLib.o $(CURDIR)/obj/LuaScript.o $(CURDIR)/obj/LuaScriptManager.o $(CURDIR)/obj/LuaScriptManLib.o $(CURDIR)/obj/LuaSetManLib.o $(CURDIR)/obj/LuaTmrManLib.o \
        $(CURDIR)/obj/LuaUDPDbgLib.o $(CURDIR)/obj/ProfileManager.o $(CURDIR)/obj/PtokaX.o $(CURDIR)/obj/pxstring.o $(CURDIR)/obj/RegThread.o $(CURDIR)/obj/ResNickManager.o $(CURDIR)/obj/ServerManager.o $(CURDIR)/obj/ServerThread.o \
        $(CURDIR)/obj/serviceLoop.o $(CURDIR)/obj/SettingManager.o $(CURDIR)/obj/TextFileManager.o $(CURDIR)/obj/UdpDebug.o $(CURDIR)/obj/UDPThread.o $(CURDIR)/obj/User.o $(CURDIR)/obj/utility.o $(CURDIR)/obj/ZlibUtility.o PtokaX

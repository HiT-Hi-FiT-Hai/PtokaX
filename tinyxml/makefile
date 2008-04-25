#*******************************************************************************
#
# Makefile for TinyXml static lib (archive).
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
# Archive to create
#*******************************************************************************
tinyxml.a: $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o
	$(AR) $(ARFLAGS) tinyxml.a $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o

#*******************************************************************************
# Files to compile
#*******************************************************************************
$(CURDIR)/obj/tinystr.o: $(CURDIR)/src/tinystr.cpp $(CURDIR)/src/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/src/tinystr.cpp -o $(CURDIR)/obj/tinystr.o

$(CURDIR)/obj/tinyxml.o: $(CURDIR)/src/tinyxml.cpp $(CURDIR)/src/tinyxml.h $(CURDIR)/src/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/src/tinyxml.cpp -o $(CURDIR)/obj/tinyxml.o

$(CURDIR)/obj/tinyxmlerror.o: $(CURDIR)/src/tinyxmlerror.cpp $(CURDIR)/src/tinyxml.h $(CURDIR)/src/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/src/tinyxmlerror.cpp -o $(CURDIR)/obj/tinyxmlerror.o

$(CURDIR)/obj/tinyxmlparser.o: $(CURDIR)/src/tinyxmlparser.cpp $(CURDIR)/src/tinyxml.h $(CURDIR)/src/tinystr.h
	$(CXX) $(CXXFLAGS) -c $(CURDIR)/src/tinyxmlparser.cpp -o $(CURDIR)/obj/tinyxmlparser.o

#*******************************************************************************
# Cleanup
#*******************************************************************************
clean:
	-rm $(CURDIR)/obj/tinystr.o $(CURDIR)/obj/tinyxml.o $(CURDIR)/obj/tinyxmlerror.o $(CURDIR)/obj/tinyxmlparser.o tinyxml.a

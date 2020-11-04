NMS_ROOT=/home/app/nextmove
CFIXOBJ=$(NMS_ROOT)/CaffeineFix/C++/CaffeineFix.o

OE_DIR = /home/app/openeye
OELIBS = -loeiupac -loechem -loesystem -loeplatform
OECXXFLAGS = -I$(OE_DIR)/toolkits/include
OELFLAGS = -L$(OE_DIR)/toolkits/lib

CXX = /usr/bin/g++
CXXFLAGS = -O3 -W -Wall -pedantic \
	-I$(NMS_ROOT)/Extract \
	-I$(NMS_ROOT)/CaffeineFix/include \
	-I$(NMS_ROOT)/CaffeineFix/C++ \
	-I/home/jjyang/projects/CADR/data
LFLAGS = 
LIBS = -lz
RM = rm -rf

APPS =			TestCaffeineFix TestCaffeineFix2 \
			correct correct_drugnames MedTermsCorrect \
			fuzzymatch_molnames

all:			$(APPS)

.cpp.o:
	$(CXX) $(CXXFLAGS) \
	-c $<
.o:     $@.o
	$(CXX) $(LFLAGS) -o $@ $@.o $(CFIXOBJ) $(LIBS)
.cpp:
	@make $@.o
	@make $@

TestCaffeineFix:	TestCaffeineFix.o $(CFIXOBJ)
	$(CXX) $(LFLAGS) -o TestCaffeineFix $^ $(LIBS)

TestCaffeineFix.o:	TestCaffeineFix.cpp
	$(CXX) $(CXXFLAGS) -c TestCaffeineFix.cpp

TestCaffeineFix2:	TestCaffeineFix2.o $(CFIXOBJ)
	$(CXX) $(LFLAGS) -o TestCaffeineFix2 $^ $(LIBS)

TestCaffeineFix2.o:	TestCaffeineFix2.cpp
	$(CXX) $(CXXFLAGS) -c TestCaffeineFix2.cpp

correct:	correct.o $(CFIXOBJ)
	$(CXX) $(LFLAGS) -o correct $^ $(LIBS)

correct.o:	correct.cpp
	$(CXX) $(CXXFLAGS) -c correct.cpp

MedTermsCorrect: MedTermsCorrect.o engine.o normalize.o html.o
	$(CXX) $(LFLAGS) -o $@ $@.o \
	engine.o normalize.o html.o \
	$(LIBS)

fuzzymatch_molnames.o:	fuzzymatch_molnames.cpp
	$(CXX) $(CXXFLAGS) $(OECXXFLAGS) -c fuzzymatch_molnames.cpp

fuzzymatch_molnames: fuzzymatch_molnames.o
	$(CXX) $(LFLAGS) $(OELFLAGS) -o $@ $@.o \
	$(LIBS) $(CFIXOBJ) $(OELIBS) -lpthread

clean:
	$(RM) $(APPS) *.o

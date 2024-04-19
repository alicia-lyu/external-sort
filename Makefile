CPPOPT=-g -Og -D_DEBUG
# -O2 -Os -Ofast
# -fprofile-generate -fprofile-use
CPPFLAGS=$(CPPOPT) -Wall -ansi -pedantic -std=c++17 -DVERBOSEL1
# TODO: Add flag to turn on duplicate removal
DEBUGFLAGS=-DCMAKE_BUILD_TYPE=Debug -DLLDB_EXPORT_ALL_SYMBOLS=ON -std=c++17
# -Wparentheses -Wno-unused-parameter -Wformat-security
# -fno-rtti -std=c++11 -std=c++98
TIMESTAMP=$(shell date +%Y-%m-%d-%H-%M-%S)
# documents and scripts
DOCS=Tasks.md InformationSession.md
SCRS=

# headers and code sources
HDRS=	defs.h params.h\
		Iterator.h Scan.h Filter.h Sort.h \
		utils.h Buffer.h Witness.h TournamentTree.h, SortedRecordRenderer.h \
		ExternalRenderer.h ExternalRun.h Verify.h Remove.h
SRCS=	defs.cpp Assert.cpp Test.cpp \
		Iterator.cpp Scan.cpp Filter.cpp Sort.cpp \
		utils.cpp Buffer.cpp Witness.cpp TournamentTree.cpp SortedRecordRenderer.cpp \
		ExternalRenderer.cpp ExternalRun.cpp Verify.cpp Remove.cpp

# compilation targets
OBJS=	defs.o Assert.o Test.o \
		Iterator.o Scan.o Filter.o Sort.o \
		utils.o Buffer.o Witness.o TournamentTree.o SortedRecordRenderer.o \
		ExternalRenderer.o ExternalRun.o Verify.o Remove.o

# RCS assists
REV=-q -f
MSG=no message

# default target
#
Test.exe : Makefile $(OBJS)
	g++ $(CPPFLAGS) -o Test.exe $(OBJS)

LOG_DIR=./logs/${TIMESTAMP}
LOG_FILE=$(LOG_DIR)/trace

$(LOG_DIR) : 
	mkdir -p $(LOG_DIR)

./inputs/ : 
	mkdir -p ./inputs/

./spills/pass0: 
	mkdir -p ./spills/pass0

./spills/pass1:
	mkdir -p ./spills/pass1

./spills/pass2:
	mkdir -p ./spills/pass2

test : Test.exe Makefile $(LOG_DIR) ./inputs/
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 7 -s 20 -o $(LOG_FILE) -d >> $(LOG_FILE)

dup : Test.exe Makefile $(LOG_DIR) ./inputs/
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 4000 -s 2 -o $(LOG_FILE) -d >> $(LOG_FILE)

# Small test plan: Memory size = 100 KB, SSD page size = 2 KB
# 50 pages per memory, 98 KB per memory run

# 200 KB data, 3 initial runs, 1 pass
external: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 10000 -s 20 -o $(LOG_FILE) >> $(LOG_FILE)

external-lldb: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./Test.exe -c 10000 -s 20 -o $(LOG_FILE)

# 8 MB data, 82 initial runs, 2 pass-1 runs, 2 pass
external-2: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 40000 -s 200 -o $(LOG_FILE) >> $(LOG_FILE)

external-2-lldb: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	lldb -- ./Test.exe -c 40000 -s 200 -o $(LOG_FILE)

# TODO: external-sort on HDD: 100 MB can be divided into 200 pages of 500KB each

# TODO: Add more test cases 10^3 * 50 (50M), 10^3 * 125 (125M), 10^5 * 120 (12 G), 10^6 * 120 (120 G) (rows, record size) and sample input by TA

lldb : Test.exe $(LOG_DIR)
	lldb -- ./Test.exe -c 7 -s 20 -o $(LOG_FILE)

# TODO: We eventually need a ExternalSort.exe target
ExternalSort.exe: Makefile ExternalSort.cpp
	g++ $(CPPFLAGS) -o ExternalSort.exe ExternalSort.cpp
# Where, 
# "-c" gives the total number of records 
# "-s" is the individual record size 
# "-o" is the trace of your program run 
# ./ExternalSort.exe -c 120 -s 1000 -o trace0.txt  (Example values)

$(OBJS) : Makefile defs.h
Test.o : Iterator.h Scan.h Filter.h Sort.h utils.h Buffer.h Witness.h TournamentTree.h SortedRecordRenderer.h Verify.h
Iterator.o Scan.o Filter.o Sort.o utils.o Buffer.o Witness.o Verify.o: Iterator.h Buffer.h
Scan.o : Scan.h
Filter.o : Filter.h
Sort.o : Sort.h
utils.o: utils.h
Witness.o: Witness.h
Verify.o: Verify.h
Remove.o: Remove.h
TournamentTree.o: TournamentTree.h
SortedRecordRenderer.o: SortedRecordRenderer.h
ExternalRun.o: ExternalRun.h
ExternalRenderer.o: ExternalRenderer.h ExternalRun.h SortedRecordRenderer.h

list : Makefile
	echo Makefile $(HDRS) $(SRCS) $(DOCS) $(SCRS) > list
count : list
	@wc `cat list`

ci :
	ci $(REV) -m"$(MSG)" $(HDRS) $(SRCS) $(DOCS) $(SCRS)
	ci -l $(REV) -m"$(MSG)" Makefile
co :
	co $(REV) -l $(HDRS) $(SRCS) $(DOCS) $(SCRS)

clean :
	@rm -f $(OBJS) Test.exe Test.exe.stackdump trace

temp.exe : temp.cpp
	g++ $(CPPFLAGS) -o temp.exe temp.cpp

temp.out : temp.exe
	echo $(TIMESTAMP) > temp.out
	./temp.exe >> temp.out

TestTTHDRS=	TournamentTree.h defs.h
TestTTSRCS=	TournamentTree.cpp defs.cpp
TestTTOBJS=	TournamentTree.o utils.o Buffer.o defs.o

TestTT.exe : TestTournamentTree.cpp $(TestTTOBJS) $(TestTTHDRS) $(TestTTSRCS)
	g++ $(CPPFLAGS) -o TestTT.exe TestTournamentTree.cpp $(TestTTOBJS)

TestTT.out : TestTT.exe
	echo $(TIMESTAMP) > TestTT.out
	./TestTT.exe >> TestTT.out

lldbTT : TestTournamentTree.cpp $(TestTTOBJS) $(TestTTHDRS) $(TestTTSRCS)
	g++ $(DEBUGFLAGS) -o TestTT.exe TestTournamentTree.cpp $(TestTTOBJS)
	lldb -- ./TestTT.exe
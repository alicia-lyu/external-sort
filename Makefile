CPPOPT=-g -Og -D_DEBUG
# -O2 -Os -Ofast
# -fprofile-generate -fprofile-use
CPPFLAGS=$(CPPOPT) -Wall -ansi -pedantic -std=c++17 -DVERBOSEL1
DEBUGFLAGS=-DCMAKE_BUILD_TYPE=Debug -DLLDB_EXPORT_ALL_SYMBOLS=ON -std=c++17
# -Wparentheses -Wno-unused-parameter -Wformat-security
# -fno-rtti -std=c++11 -std=c++98
TIMESTAMP=$(shell date +%Y-%m-%d-%H-%M-%S)
# documents and scripts
DOCS=Tasks.md InformationSession.md
SCRS=

# headers and code sources
HDRS=	defs.h params.h\
		Iterator.h Scan.h Sort.h \
		utils.h Buffer.h Witness.h TournamentTree.h, SortedRecordRenderer.h \
		ExternalRenderer.h ExternalRun.h Verify.h Remove.h Metrics.h \
		InMemoryRenderer.h Output.h GracefulRenderer.h
SRCS=	defs.cpp Assert.cpp Test.cpp \
		Iterator.cpp Scan.cpp Sort.cpp \
		utils.cpp Buffer.cpp Witness.cpp TournamentTree.cpp SortedRecordRenderer.cpp \
		ExternalRenderer.cpp ExternalRun.cpp Verify.cpp Remove.cpp Metrics.cpp \
		InMemoryRenderer.cpp Output.cpp GracefulRenderer.cpp

# compilation targets
OBJS=	defs.o Assert.o Test.o \
		Iterator.o Scan.o Sort.o \
		utils.o Buffer.o Witness.o TournamentTree.o SortedRecordRenderer.o \
		ExternalRenderer.o ExternalRun.o Verify.o Remove.o Metrics.o \
		InMemoryRenderer.o Output.o GracefulRenderer.o

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
	rm -rf ./logs/*
	mkdir -p $(LOG_DIR)

./inputs/ : 
	mkdir -p ./inputs/
	rm -rf ./inputs/*

./spills/pass0:
	mkdir -p ./spills/pass0

./spills/pass1:
	mkdir -p ./spills/pass1

./spills/pass2:
	mkdir -p ./spills/pass2

clean-spills:
	rm -rf ./spills/*

test : Test.exe Makefile $(LOG_DIR) ./inputs/
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 7 -s 20 -t $(LOG_FILE) -d insort

testinput : Test.exe Makefile $(LOG_DIR) ./inputs/
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 20 -s 1023 -i input_table -t $(LOG_FILE) -d insort
	diff output_table output.txt

insort : Test.exe Makefile $(LOG_DIR) ./inputs/
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 4000 -s 2 -t $(LOG_FILE) -d insort

instream : Test.exe Makefile $(LOG_DIR) ./inputs/
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 4000 -s 2 -t $(LOG_FILE) -d instream

# Small test plan: Memory size = 100 KB, SSD page size = 2 KB
# 50 pages per memory, 98 KB per memory run

# 200 KB data, 3 initial runs, 1 pass
external: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 10000 -s 20 -t $(LOG_FILE)

external-lldb: Test.exe Makefile ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./Test.exe -c 10000 -s 20

# 8 MB data, 82 initial runs, 2 pass-1 runs
external-2: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 40000 -s 200 -t $(LOG_FILE)

external-2-lldb: Test.exe Makefile ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	lldb -- ./Test.exe -c 40000 -s 200

# 100 KB data
graceful: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 5000 -s 20 -t $(LOG_FILE)

graceful-lldb: Test.exe Makefile ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./Test.exe -c 5000 -s 20

# 5 MB data, 52 initial runs, 2 pass-1 run (only 1 pass-2 run with graceful degradation)
optimized-merge: Test.exe Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./Test.exe -c 25000 -s 200 -t $(LOG_FILE)

optimized-merge-lldb: Test.exe Makefile ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./Test.exe -c 25000 -s 200

# TODO: external-sort on HDD: 100 MB can be divided into 200 pages of 500KB each

# TODO: Add more test cases 10^3 * 50 (50M), 10^3 * 125 (125M), 10^5 * 120 (12 G), 10^6 * 120 (120 G) (rows, record size) and sample input by TA

lldb : Test.exe
	lldb -- ./Test.exe -c 7 -s 20

# TODO: We eventually need a ExternalSort.exe target
ExternalSort.exe: Makefile ExternalSort.cpp
	g++ $(CPPFLAGS) -o ExternalSort.exe ExternalSort.cpp
# Where, 
# "-c" gives the total number of records 
# "-s" is the individual record size 
# "-o" is the trace of your program run 
# ./ExternalSort.exe -c 120 -s 1000 -o trace0.txt  (Example values)

$(OBJS) : Makefile defs.h
Test.o : Iterator.h Scan.h Sort.h utils.h Buffer.h Witness.h TournamentTree.h SortedRecordRenderer.h Verify.h
Iterator.o Scan.o Sort.o utils.o Buffer.o Witness.o Verify.o TournamentTree.o SortedRecordRenderer.o: Iterator.h Buffer.h params.h utils.h
Scan.o : Scan.h 
Sort.o : Sort.h
utils.o: utils.h
Witness.o: Witness.h
Verify.o: Verify.h
Remove.o: Remove.h
TournamentTree.o: TournamentTree.h
Metrics.o: Metrics.h
Output.o: Output.h
SortedRecordRenderer.o: SortedRecordRenderer.h
ExternalRun.o: ExternalRun.h
ExternalRenderer.o: ExternalRenderer.h ExternalRun.h SortedRecordRenderer.h
InMemoryRenderer.o: InMemoryRenderer.h SortedRecordRenderer.h
GracefulRenderer.o: GracefulRenderer.h InMemoryRenderer.h ExternalRun.h

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
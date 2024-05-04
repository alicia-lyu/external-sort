CPPOPT=-g -Og -D_DEBUG
# -O2 -Os -Ofast
# -fprofile-generate -fprofile-use
CPPFLAGS=$(CPPOPT) -Wall -ansi -pedantic -std=c++17 -DPRODUCTION
DEBUGFLAGS=-DCMAKE_BUILD_TYPE=Debug -DLLDB_EXPORT_ALL_SYMBOLS=ON -std=c++17
# -Wparentheses -Wno-unused-parameter -Wformat-security
# -fno-rtti -std=c++11 -std=c++98
TIMESTAMP=$(shell date +%Y-%m-%d-%H-%M-%S)
# documents and scripts
DOCS=Tasks.md InformationSession.md
SCRS=
TARGET=ExternalSort.exe

# headers and code sources
HDRS=	defs.h params.h\
		Iterator.h Scan.h Sort.h \
		utils.h Buffer.h Witness.h TournamentTree.h, SortedRecordRenderer.h \
		ExternalRenderer.h ExternalRun.h Verify.h Remove.h Metrics.h \
		InMemoryRenderer.h GracefulRenderer.h ExternalSorter.h
SRCS=	defs.cpp Assert.cpp Test.cpp \
		Iterator.cpp Scan.cpp Sort.cpp \
		utils.cpp Buffer.cpp Witness.cpp TournamentTree.cpp SortedRecordRenderer.cpp \
		ExternalRenderer.cpp ExternalRun.cpp Verify.cpp Remove.cpp Metrics.cpp \
		InMemoryRenderer.cpp GracefulRenderer.cpp ExternalSorter.cpp

# compilation targets
OBJS=	defs.o Assert.o Test.o \
		Iterator.o Scan.o Sort.o \
		utils.o Buffer.o Witness.o TournamentTree.o SortedRecordRenderer.o \
		ExternalRenderer.o ExternalRun.o Verify.o Remove.o Metrics.o \
		InMemoryRenderer.o GracefulRenderer.o ExternalSorter.o

# RCS assists
REV=-q -f
MSG=no message

# default target
#
$(TARGET) : $(OBJS)
	g++ $(CPPFLAGS) -o $(TARGET) $(OBJS)

LOG_DIR=./logs/${TIMESTAMP}
LOG_FILE=$(LOG_DIR)/trace

$(LOG_DIR) : 
	rm -rf ./logs/*
	mkdir -p $(LOG_DIR)

clean-inputs:
	rm -rf ./inputs/*

./inputs/ : clean-inputs
	mkdir -p ./inputs/

./spills/pass0: clean-spills
	mkdir -p ./spills/pass0

./spills/pass1: clean-spills
	mkdir -p ./spills/pass1

./spills/pass2: clean-spills
	mkdir -p ./spills/pass2

clean-spills:
	rm -rf ./spills/*

test : $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 7 -s 20 -t $(LOG_FILE) -d insort

testinput : $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 20 -s 1023 -i input_table -t $(LOG_FILE)
	# diff output_table output.txt

insort : $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 4000 -s 2 -t $(LOG_FILE) -d insort

instream : $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 4000 -s 2 -t $(LOG_FILE) -d instream

# Memory size = 100 MB, SSD page size = 20 KB
# 200--5000 pages can fit in memory
# Memory run size = 100 MB
# Pass 1 run: 20 GB -- 500 GB

# 200 KB data, 3 initial runs, 1 pass
external: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 10000 -s 20 -t $(LOG_FILE)

external-lldb: $(TARGET) Makefile ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./$(TARGET) -c 10000 -s 20

external-1: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 40000 -s 200 -t $(LOG_FILE)

external-2: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 40000 -s 1250 -t $(LOG_FILE)

external-2-lldb: $(TARGET) Makefile ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	lldb -- ./$(TARGET) -c 40000 -s 1250

200m: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 160000 -s 1250 -t $(LOG_FILE)

# 1 GB data: 800000 * 1250
1g: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	date +%s > time
	./$(TARGET) -c 800000 -s 1250 -t $(LOG_FILE)
	date +%s >> time
	./calculateTime.sh time

1g-lldb: $(TARGET) Makefile ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	lldb -- ./$(TARGET) -c 800000 -s 1250

10g: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	date +%s > time
	./$(TARGET) -c 12500000 -s 800 -t $(LOG_FILE)
	date +%s >> time
	./calculateTime.sh time

# 120 GB data
120g: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	date +%s > time
	./$(TARGET) -c 125000000 -s 960 -t $(LOG_FILE)
	date +%s >> time
	./calculateTime.sh time

# 30 GB data
30g: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1 ./spills/pass2
	echo $(TIMESTAMP) > $(LOG_FILE)
	date +%s > time
	./$(TARGET) -c 30000000 -s 1000 -t $(LOG_FILE)
	date +%s >> time
	./calculateTime.sh time

# 120 MB data
graceful: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 1000000 -s 120 -t $(LOG_FILE)

graceful-lldb: $(TARGET) Makefile ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./$(TARGET) -c 5000 -s 20

# 5 MB data, 52 initial runs, 2 pass-1 run (only 1 pass-2 run with graceful degradation)
optimized-merge: $(TARGET) Makefile $(LOG_DIR) ./inputs/ ./spills/pass0 ./spills/pass1
	echo $(TIMESTAMP) > $(LOG_FILE)
	./$(TARGET) -c 25000 -s 200 -t $(LOG_FILE)

optimized-merge-lldb: $(TARGET) Makefile ./inputs/ ./spills/pass0 ./spills/pass1
	lldb -- ./$(TARGET) -c 25000 -s 200

# TODO: external-sort on HDD: 100 MB can be divided into 200 pages of 500KB each

# TODO: Add more test cases 10^3 * 50 (50M), 10^3 * 125 (125M), 10^5 * 120 (12 G), 10^6 * 120 (120 G) (rows, record size) and sample input by TA

lldb : $(TARGET)
	lldb -- ./$(TARGET) -c 7 -s 20

# Where, 
# "-c" gives the total number of records 
# "-s" is the individual record size 
# "-o" is the trace of your program run 
# ./ExternalSort.exe -c 120 -s 1000 -o trace0.txt  (Example values)

$(OBJS) : Makefile defs.h
Test.o : Iterator.h Scan.h Sort.h utils.h Buffer.h Witness.h TournamentTree.h SortedRecordRenderer.h Verify.h Remove.h Metrics.h GracefulRenderer.h ExternalRenderer.h ExternalRun.h
Iterator.o Scan.o Sort.o utils.o Buffer.o Witness.o TournamentTree.o SortedRecordRenderer.o Verify.o Remove.o Metrics.o GracefulRenderer.o ExternalRenderer.o ExternalRun.o: Iterator.h Buffer.h params.h utils.h
Scan.o : Scan.h 
Sort.o : Sort.h TournamentTree.h SortedRecordRenderer.h
utils.o: utils.h
Witness.o: Witness.h
Verify.o: Verify.h
Remove.o: Remove.h
TournamentTree.o: TournamentTree.h
Metrics.o: Metrics.h
SortedRecordRenderer.o: SortedRecordRenderer.h
ExternalRun.o: ExternalRun.h
ExternalRenderer.o: ExternalRenderer.h ExternalRun.h SortedRecordRenderer.h TournamentTree.h
InMemoryRenderer.o: InMemoryRenderer.h SortedRecordRenderer.h TournamentTree.h
GracefulRenderer.o: GracefulRenderer.h InMemoryRenderer.h ExternalRun.h TournamentTree.h
ExternalSorter.o: ExternalRenderer.h utils.h

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
	@rm -f $(OBJS) $(TARGET) $(TARGET).stackdump trace

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
CPPOPT=-g -Og -D_DEBUG
# -O2 -Os -Ofast
# -fprofile-generate -fprofile-use
CPPFLAGS=$(CPPOPT) -Wall -ansi -pedantic -std=c++17
# -Wparentheses -Wno-unused-parameter -Wformat-security
# -fno-rtti -std=c++11 -std=c++98
TIMESTAMP=$(shell date +%Y-%m-%d-%H-%M-%S)
# documents and scripts
DOCS=Tasks.txt
SCRS=

# headers and code sources
HDRS=	defs.h \
		Iterator.h Scan.h Filter.h Sort.h
SRCS=	defs.cpp Assert.cpp Test.cpp \
		Iterator.cpp Scan.cpp Filter.cpp Sort.cpp

# compilation targets
OBJS=	defs.o Assert.o Test.o \
		Iterator.o Scan.o Filter.o Sort.o \
		utils.o Row.o Witness.o

# RCS assists
REV=-q -f
MSG=no message

# default target
#
Test.exe : Makefile $(OBJS)
	g++ $(CPPFLAGS) -o Test.exe $(OBJS)

OUTPUT_DIR=./logs/${TIMESTAMP}
OUTPUT_FILE=$(OUTPUT_DIR)/trace

$(OUTPUT_DIR) : 
	mkdir -p $(OUTPUT_DIR)

$(OUTPUT_FILE) : Test.exe Makefile $(OUTPUT_DIR)
	echo $(TIMESTAMP) > $(OUTPUT_FILE)
	./Test.exe -c 7 -s 20 -o $(OUTPUT_FILE) >> $(OUTPUT_FILE)
	# @size -t Test.exe $(OBJS) | sort -r >> $(OUTPUT_FILE)

test: $(OUTPUT_FILE)

ExternalSort.exe: Makefile ExternalSort.cpp
	g++ $(CPPFLAGS) -o ExternalSort.exe ExternalSort.cpp
# Where, 
# "-c" gives the total number of records 
# "-s" is the individual record size 
# "-o" is the trace of your program run 
# ./ExternalSort.exe -c 120 -s 1000 -o trace0.txt  (Example values)

$(OBJS) : Makefile defs.h
Test.o : Iterator.h Scan.h Filter.h Sort.h utils.h Row.h Witness.h
Iterator.o Scan.o Filter.o Sort.o utils.o Row.o Witness.o : Iterator.h Row.h
Scan.o : Scan.h
Filter.o : Filter.h
Sort.o : Sort.h
utils.o: utils.h
Witness.o: Witness.h

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
# the app target
TARGET=bin/cmovies

# Compiler
CC=gcc
# optimization
OPT=-O0
# warnings
WARN=-Wall
# pedantic
PED=-pedantic
# standard
STD=-std=c99
# pthread
PTHREAD=-pthread

CCFLAGS=$(OPT) $(WARN) $(PED) $(STD) $(PTHREAD) -pipe

SQLITE=-lsqlite3

# linker
LD=gcc

LDFLAGS=$(PTHREAD) $(SQLITE) #-export-dynamic

OBJS= bin/dbutils.o bin/log.o bin/cmovies.o 

all: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(LDFLAGS)

bin/dbutils.o: src/dbutils.c
	$(CC) -c $(CCFLAGS) src/dbutils.c $(SQLITE) -o bin/dbutils.o

bin/log.o: src/log.c
	$(CC) -c $(CCFLAGS) src/log.c -o bin/log.o

bin/cmovies.o: src/cmovies.c
	$(CC) -c $(CCFLAGS) src/cmovies.c $(SQLITE) -o bin/cmovies.o

clean:
	rm -f bin/*.o $(TARGET)
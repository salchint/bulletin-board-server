IDIR =.
CC=g++
CFLAGS=-I$(IDIR) -Wall -pedantic -std=c++17

ODIR=obj
LDIR =../lib

LIBS=-lm -pthread

_DEPS = AutoLock.h         CmdUser.h  ConnectionQueue.h  BBServException.h  Config.h   InConnection.h     ThreadPool.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = Config.o  ConnectionQueue.o  InConnection.o  main.o  ThreadPool.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bbserv: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
	touch bbserv


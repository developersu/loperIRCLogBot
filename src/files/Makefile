CC = gcc
CFLAGS = -Wall -DIN_BUILDING_LIBIRC -g -O2 -O3 -DENABLE_THREADS -D_REENTRANT
AR=ar cr
RANLIB=ranlib
INCLUDES=-I../include
DESTDIR=
APIVERSION = 1

all: libircclient.o
	$(AR) libircclient.a libircclient.o
	$(RANLIB) libircclient.a

install:
	-mkdir -p ./include
	cp ../include/libircclient.h ./include/libircclient.h
	cp ../include/libirc_errors.h ./include/libirc_errors.h
	cp ../include/libirc_events.h  ./include/libirc_events.h 
	cp ../include/libirc_options.h  ./include/libirc_options.h 
	cp ../include/libirc_rfcnumeric.h  ./include/libirc_rfcnumeric.h 

clean:
	rm -rf ./include libircclient.a libircclient.o

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


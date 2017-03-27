# Compiler
CC=gcc
# Flags
#CFLAGS=-lircclient
#CFLAGS=-static libircclient.a
MKDIR_P = mkdir -p

all: lib loperIRCLogBot

lib:
	cp ./files/* ./libircclient-1.9/src/
	patch -p0 -i ./files/patchFile
	$(MAKE) -C  ./libircclient-1.9/src/ || exit 1;
	cp -r ./libircclient-1.9/include/ ./libircclient
	cp ./libircclient-1.9/src/libircclient.a .

loperIRCLogBot: loperIRCLogBot.c
	$(MKDIR_P) ./bin
#	$(CC) $(CFLAGS) loperIRCLogBot.c -o ./bin/loperIRCLogBot
	$(CC) loperIRCLogBot.c libircclient.a -o ./bin/loperIRCLogBot


clean:
	rm -rf ./bin/*.o ./bin/loperIRCLogBot ./libircclient libircclient.a  ./libircclient-1.9/src/libircclient.o ./libircclient-1.9/src/libircclient.a

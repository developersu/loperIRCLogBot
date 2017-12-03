# Compiler
CC=gcc
# Flags
#	CFLAGS=-lircclient
#	CFLAGS=-static libircclient.a
MKDIR_P = mkdir -p
APP_NAME = loperIRCLogBot
SYSTEMD_DETECT = shell cat /proc/1/comm|grep systemd

all: lib $(APP_NAME)

lib:
	cp ./src/files/* ./src/libircclient-1.9/src/
#	patch -p0 -i ./files/patchFile
	$(MAKE) -C  ./src/libircclient-1.9/src/ || exit 1;
	cp -r ./src/libircclient-1.9/include/ ./src/libircclient
	cp ./src/libircclient-1.9/src/libircclient.a ./src

loperIRCLogBot: ./src/$(APP_NAME).c
	$(MKDIR_P) ./bin
#	$(CC) $(CFLAGS) $(APP_NAME).c -o ./bin/loperIRCLogBot
	$(CC) -std=gnu89 ./src/$(APP_NAME).c ./src/libircclient.a -o ./bin/$(APP_NAME)


clean:
	rm -rf ./bin/*.o \
		./bin/loperIRCLogBot \
		./src/libircclient \
		./src/libircclient.a  \
		./src/libircclient-1.9/src/libircclient.o \
		./src/libircclient-1.9/src/libircclient.a \
		./src/libircclient-1.9/src/config.h \
		./src/libircclient-1.9/src/Makefile
install: 
	install ./bin/loperIRCLogBot /usr/bin
	$(MKDIR_P) /etc/$(APP_NAME)
ifeq ($(SYSTEMD_DETECT),'systemd')
	cp ./src/files/loperIRCLogBot.service /etc/systemd/system/loperIRCLogBot.service 
else
	@echo "* Not a systemD distro. Skipping unit installation."
endif

uninstall: 
	rm /usr/bin/loperIRCLogBot
	rmdir /etc/$(APP_NAME)
ifeq ($(SYSTEMD_DETECT),'systemd')
	rm -rf /etc/systemd/system/loperIRCLogBot.service
endif

include ../../make.defs

COBJS=../common/timer.o ../common/malloc.o ../common/socket.o ../common/lock.o ../common/db.o ../char/int_storage.o ../char/inter.o ../char/int_party.o ../char/int_guild.o
MOBJS=../common/socket.o ../common/timer.o ../common/db.o ../common/lock.o ../common/malloc.o ../char/int_guild.o ../char/int_party.o ../char/int_storage.o ../char/inter.o
all:	adduser itemfrob mapfrob marriage-info eathena-monitor
adduser: adduser.o ../common/socket.o

itemfrob: itemfrob.o ${COBJS}
mapfrob: mapfrob.o ${COBJS}
marriage-info: marriage-info.o ${MOBJS}
eathena-monitor: eathena-monitor.c
	$(CC) $(CFLAGS) -o eathena-monitor eathena-monitor.c

clean:
	rm -f adduser itemfrob mapfrob marriage-info eathena-monitor
	rm -f *.exe
	rm -f *.o

include ../../make.defs

OBJS =../common/timer.o ../common/socket.o ../common/db.o ../common/lock.o ../char/inter.o ../char/int_guild.o ../char/int_party.o ../char/int_storage.o
all:	adduser itemfrob mapfrob marriage-info eathena-monitor
adduser: adduser.o ../common/socket.o

itemfrob: itemfrob.o ${OBJS}
mapfrob: mapfrob.o ${OBJS}
marriage-info: marriage-info.o ${OBJS}
eathena-monitor: eathena-monitor.c

clean:
	rm -f adduser itemfrob mapfrob marriage-info eathena-monitor
	rm -f *.exe
	rm -f *.o

include ../../make.defs

BDIR=..
COBJS=${BDIR}/common/timer.o ${BDIR}/common/malloc.o ${BDIR}/common/socket.o ${BDIR}/common/lock.o ${BDIR}/common/db.o ${BDIR}/char/int_storage.o ${BDIR}/char/inter.o ${BDIR}/char/int_party.o ${BDIR}/char/int_guild.o
MOBJS=${BDIR}/common/socket.o ${BDIR}/common/timer.o ${BDIR}/common/db.o ${BDIR}/common/lock.o ${BDIR}/common/malloc.o ${BDIR}/char/int_guild.o ${BDIR}/char/int_party.o ${BDIR}/char/int_storage.o ${BDIR}/char/inter.o

all:	${COBJS} ${MOBJS}
	$(CC) $(CFLAGS) -o adduser adduser.c $(BDIR)/common/socket.o
	$(CC) $(CFLAGS) -I ${BDIR}/char -I ${BDIR}/common itemfrob.c -o itemfrob ${COBJS}
	$(CC) $(CFLAGS) -I ${BDIR}/char -I ${BDIR}/common mapfrob.c -o mapfrob ${COBJS}
	$(CC) $(CFLAGS) -I ${BDIR}/char -I ${BDIR}/common -I ${BDIR}/map -I ${BDIR}/login marriage-info.c -o marriage-info ${MOBJS}
	$(CC) $(CFLAGS) -o ${BDIR}/../eathena-monitor eathena-monitor.c

clean:
	rm -f adduser itemfrob mapfrob marriage-info ${BDIR}/../eathena-monitor
	rm -f *.exe

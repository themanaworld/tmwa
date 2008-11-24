CC=gcc -m32 -O0 -g
BDIR=..
COBJS=${BDIR}/common/timer.o ${BDIR}/common/malloc.o ${BDIR}/common/socket.o ${BDIR}/common/lock.o ${BDIR}/common/db.o ${BDIR}/char/int_pet.o ${BDIR}/char/int_storage.o ${BDIR}/char/inter.o ${BDIR}/char/int_party.o ${BDIR}/char/int_guild.o

all:
	$(CC) -o adduser adduser.c
	$(CC) -I ${BDIR}/char -I ${BDIR}/common itemfrob.c -o itemfrob ${COBJS}
	$(CC) -I ${BDIR}/char -I ${BDIR}/common mapfrob.c -o mapfrob ${COBJS}

clean:
	rm -f adduser itemfrob mapfrop
	rm -f *.exe

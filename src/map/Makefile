include ../../make.defs

all: txt

txt: obj map-server

obj:
	mkdir obj

COMMON_OBJ = ../common/core.o ../common/socket.o ../common/timer.o ../common/grfio.o ../common/db.o ../common/lock.o ../common/nullpo.o ../common/malloc.o ../common/mt_rand.o ../common/md5calc.o
LIBS = -lz -lm

map-server: obj/tmw.o obj/magic-interpreter-lexer.o obj/magic-interpreter-parser.o obj/magic-interpreter-base.o obj/magic-expr.o obj/magic-stmt.o obj/magic.o obj/map.o obj/chrif.o obj/clif.o obj/pc.o obj/npc.o obj/chat.o obj/path.o obj/itemdb.o obj/mob.o obj/script.o obj/storage.o obj/skill.o obj/skill-pools.o obj/atcommand.o obj/battle.o obj/intif.o obj/trade.o obj/party.o obj/guild.o $(COMMON_OBJ)
	$(CC) -o ../../$@ $^ $(LIBS)

obj/%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

magic-interpreter-lexer.c: magic-interpreter.l
	$(LEX) -o magic-interpreter-lexer.c magic-interpreter.l

magic-interpreter-parser.c: magic-interpreter-parser.y
	$(BISON) -v -d -o magic-interpreter-parser.c magic-interpreter-parser.y

obj/magic-interpreter-lexer.o: magic-interpreter-lexer.c magic-interpreter-parser.c magic-expr.h magic-interpreter.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/magic-interpreter-parser.o: magic-interpreter-parser.c magic-expr.h magic-interpreter.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/magic-interpreter-base.o: magic-interpreter-base.c magic-expr-eval.h magic-interpreter-aux.h magic-expr.h magic-interpreter.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/magic-expr.o: magic-expr.c magic-expr-eval.h magic-interpreter-aux.h magic-expr.h magic-interpreter.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/magic-stmt.o: magic-stmt.c magic-expr-eval.h magic-interpreter-aux.h magic-expr.h magic-interpreter.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/magic.o: magic.c magic.h magic-expr.h magic-interpreter.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/map.o: map.c map.h chrif.h clif.h npc.h pc.h mob.h chat.h skill.h itemdb.h storage.h party.h atcommand.h ../common/core.h ../common/timer.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/chrif.o: chrif.c map.h battle.h chrif.h clif.h intif.h pc.h npc.h ../common/socket.h ../common/timer.h ../common/mmo.h
obj/clif.o: magic.h clif.c map.h chrif.h clif.h mob.h intif.h pc.h npc.h itemdb.h chat.h script.h storage.h party.h guild.h atcommand.h atcommand.h ../common/socket.h ../common/timer.h ../common/mmo.h ../common/version.h tmw.h
obj/pc.o: pc.c map.h clif.h intif.h pc.h npc.h mob.h itemdb.h battle.h skill.h script.h party.h guild.h trade.h storage.h chat.h ../common/timer.h ../common/mmo.h ../common/db.h
obj/npc.o: npc.c map.h npc.h clif.h pc.h script.h mob.h itemdb.h battle.h ../common/db.h ../common/timer.h ../common/mmo.h
obj/chat.o: chat.c map.h clif.h pc.h chat.h ../common/db.h ../common/mmo.h
obj/path.o: path.c map.h battle.h ../common/mmo.h
obj/itemdb.o: itemdb.c map.h battle.h itemdb.h ../common/db.h ../common/grfio.h ../common/mmo.h
obj/mob.o: mob.c map.h clif.h intif.h pc.h mob.h skill.h battle.h npc.h itemdb.h ../common/timer.h ../common/socket.h ../common/mmo.h
obj/script.o: script.c itemdb.h map.h pc.h mob.h clif.h intif.h npc.h script.h storage.h skill.h battle.h ../common/timer.h ../common/socket.h ../common/db.h ../common/mmo.h ../common/lock.h
obj/storage.o: storage.c itemdb.h pc.h clif.h intif.h storage.h guild.h ../common/mmo.h ../common/db.h
obj/skill.o: skill.c skill.h map.h clif.h pc.h mob.h battle.h itemdb.h script.h ../common/timer.h ../common/mmo.h
obj/skill-pools.o: skill-pools.c skill.h map.h clif.h pc.h mob.h battle.h itemdb.h script.h ../common/timer.h ../common/mmo.h
obj/atcommand.o: atcommand.c atcommand.h itemdb.h pc.h map.h skill.h clif.h mob.h intif.h battle.h storage.h guild.h ../common/socket.h ../common/timer.h ../common/mmo.h
obj/battle.o: battle.c battle.h skill.h map.h mob.h pc.h guild.h ../common/timer.h ../common/mmo.h
obj/intif.o: intif.c intif.h chrif.h clif.h party.h guild.h storage.h map.h battle.h ../common/socket.h ../common/mmo.h
obj/trade.o: trade.c trade.h clif.h itemdb.h map.h pc.h npc.h ../common/mmo.h
obj/party.o: party.c party.h clif.h intif.h pc.h map.h battle.h ../common/db.h ../common/socket.h ../common/timer.h ../common/mmo.h
obj/tmw.o: tmw.c tmw.h map.h clif.h

clean:
	rm -rf *.o ../../map-server obj

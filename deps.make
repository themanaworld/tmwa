${BUILD_DIR}/webserver/main.o: src/webserver/main.c
${BUILD_DIR}/webserver/logs.o: src/webserver/logs.c
${BUILD_DIR}/webserver/generate.o: src/webserver/generate.c
${BUILD_DIR}/webserver/parse.o: src/webserver/parse.c
${BUILD_DIR}/webserver/pages/about.o: src/webserver/pages/about.c
${BUILD_DIR}/webserver/pages/notdone.o: src/webserver/pages/notdone.c
${BUILD_DIR}/webserver/pages/sample.o: src/webserver/pages/sample.c
${BUILD_DIR}/webserver/htmlstyle.o: src/webserver/htmlstyle.c
${BUILD_DIR}/common/mt_rand.o: src/common/mt_rand.c src/common/mt_rand.h \
 src/common/sanity.h
${BUILD_DIR}/common/timer.o: src/common/timer.c src/common/timer.h \
 src/common/sanity.h src/common/utils.h
${BUILD_DIR}/common/nullpo.o: src/common/nullpo.c src/common/nullpo.h \
 src/common/sanity.h
${BUILD_DIR}/common/lock.o: src/common/lock.c src/common/lock.h \
 src/common/socket.h src/common/sanity.h
${BUILD_DIR}/common/grfio.o: src/common/grfio.c src/common/utils.h \
 src/common/grfio.h src/common/mmo.h src/common/socket.h \
 src/common/sanity.h
${BUILD_DIR}/common/md5calc.o: src/common/md5calc.c src/common/md5calc.h \
 src/common/sanity.h src/common/mt_rand.h
${BUILD_DIR}/common/dbtest.o: src/common/dbtest.c src/common/db.h \
 src/common/sanity.h
${BUILD_DIR}/common/db.o: src/common/db.c src/common/db.h \
 src/common/sanity.h src/common/utils.h
${BUILD_DIR}/common/socket.o: src/common/socket.c src/common/mmo.h \
 src/common/utils.h src/common/socket.h src/common/sanity.h
${BUILD_DIR}/common/core.o: src/common/core.c src/common/core.h \
 src/common/socket.h src/common/sanity.h src/common/timer.h \
 src/common/version.h src/common/mt_rand.h src/common/nullpo.h
${BUILD_DIR}/login/login.o: src/login/login.c src/login/../common/core.h \
 src/login/../common/socket.h src/login/../common/sanity.h \
 src/login/../common/timer.h src/login/login.h src/login/../common/mmo.h \
 src/login/../common/utils.h src/login/../common/version.h \
 src/login/../common/db.h src/login/../common/lock.h \
 src/login/../common/mt_rand.h src/login/../common/md5calc.h
${BUILD_DIR}/char/int_guild.o: src/char/int_guild.c src/char/inter.h \
 src/char/int_guild.h src/char/int_storage.h src/char/../common/mmo.h \
 src/char/../common/utils.h src/char/char.h src/char/../common/socket.h \
 src/char/../common/sanity.h src/char/../common/db.h \
 src/char/../common/lock.h
${BUILD_DIR}/char/inter.o: src/char/inter.c src/char/../common/mmo.h \
 src/char/../common/utils.h src/char/char.h src/char/../common/socket.h \
 src/char/../common/sanity.h src/char/../common/timer.h \
 src/char/../common/db.h src/char/inter.h src/char/int_party.h \
 src/char/int_guild.h src/char/int_storage.h src/char/../common/lock.h
${BUILD_DIR}/char/int_party.o: src/char/int_party.c src/char/inter.h \
 src/char/int_party.h src/char/../common/mmo.h src/char/../common/utils.h \
 src/char/char.h src/char/../common/socket.h src/char/../common/sanity.h \
 src/char/../common/db.h src/char/../common/lock.h
${BUILD_DIR}/char/int_storage.o: src/char/int_storage.c \
 src/char/../common/mmo.h src/char/../common/utils.h \
 src/char/../common/socket.h src/char/../common/sanity.h \
 src/char/../common/db.h src/char/../common/lock.h src/char/char.h \
 src/char/inter.h src/char/int_storage.h src/char/int_guild.h
${BUILD_DIR}/char/char.o: src/char/char.c src/char/../common/core.h \
 src/char/../common/socket.h src/char/../common/sanity.h \
 src/char/../common/timer.h src/char/../common/mmo.h \
 src/char/../common/utils.h src/char/../common/version.h \
 src/char/../common/lock.h src/char/char.h src/char/inter.h \
 src/char/int_guild.h src/char/int_party.h src/char/int_storage.h
${BUILD_DIR}/tool/itemfrob.o: src/tool/itemfrob.c \
 src/tool/../common/mmo.h src/tool/../common/utils.h \
 src/tool/../char/char.c src/tool/../char/../common/core.h \
 src/tool/../char/../common/socket.h src/tool/../char/../common/sanity.h \
 src/tool/../char/../common/timer.h src/tool/../char/../common/mmo.h \
 src/tool/../char/../common/version.h src/tool/../char/../common/lock.h \
 src/tool/../char/char.h src/tool/../char/inter.h \
 src/tool/../char/int_guild.h src/tool/../char/int_party.h \
 src/tool/../char/int_storage.h
${BUILD_DIR}/tool/eathena-monitor.o: src/tool/eathena-monitor.c
${BUILD_DIR}/tool/adduser.o: src/tool/adduser.c
${BUILD_DIR}/tool/marriage-info.o: src/tool/marriage-info.c \
 src/tool/../login/login.h src/tool/../common/mmo.h \
 src/tool/../common/utils.h src/tool/../char/char.c \
 src/tool/../char/../common/core.h src/tool/../char/../common/socket.h \
 src/tool/../char/../common/sanity.h src/tool/../char/../common/timer.h \
 src/tool/../char/../common/mmo.h src/tool/../char/../common/version.h \
 src/tool/../char/../common/lock.h src/tool/../char/char.h \
 src/tool/../char/inter.h src/tool/../char/int_guild.h \
 src/tool/../char/int_party.h src/tool/../char/int_storage.h
${BUILD_DIR}/tool/skillfrob.o: src/tool/skillfrob.c \
 src/tool/../common/mmo.h src/tool/../common/utils.h \
 src/tool/../char/char.c src/tool/../char/../common/core.h \
 src/tool/../char/../common/socket.h src/tool/../char/../common/sanity.h \
 src/tool/../char/../common/timer.h src/tool/../char/../common/mmo.h \
 src/tool/../char/../common/version.h src/tool/../char/../common/lock.h \
 src/tool/../char/char.h src/tool/../char/inter.h \
 src/tool/../char/int_guild.h src/tool/../char/int_party.h \
 src/tool/../char/int_storage.h
${BUILD_DIR}/tool/convert.o: src/tool/convert.c
${BUILD_DIR}/tool/mapfrob.o: src/tool/mapfrob.c src/tool/../common/mmo.h \
 src/tool/../common/utils.h src/tool/../char/char.c \
 src/tool/../char/../common/core.h src/tool/../char/../common/socket.h \
 src/tool/../char/../common/sanity.h src/tool/../char/../common/timer.h \
 src/tool/../char/../common/mmo.h src/tool/../char/../common/version.h \
 src/tool/../char/../common/lock.h src/tool/../char/char.h \
 src/tool/../char/inter.h src/tool/../char/int_guild.h \
 src/tool/../char/int_party.h src/tool/../char/int_storage.h
${BUILD_DIR}/map/itemdb.o: src/map/itemdb.c src/map/../common/db.h \
 src/map/../common/sanity.h src/map/../common/grfio.h \
 src/map/../common/nullpo.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/timer.h src/map/battle.h \
 src/map/itemdb.h src/map/script.h src/map/pc.h \
 src/map/../common/socket.h src/map/../common/mt_rand.h
${BUILD_DIR}/map/magic.o: src/map/magic.c src/map/magic-interpreter.h \
 src/map/../common/nullpo.h src/map/../common/sanity.h src/map/battle.h \
 src/map/chat.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/timer.h \
 src/map/../common/db.h src/map/chrif.h src/map/clif.h src/map/storage.h \
 src/map/intif.h src/map/itemdb.h src/map/magic.h src/map/mob.h \
 src/map/npc.h src/map/pc.h src/map/party.h src/map/script.h \
 src/map/skill.h src/map/trade.h src/map/../common/socket.h
${BUILD_DIR}/map/guild.o: src/map/guild.c src/map/guild.h \
 src/map/storage.h src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/db.h src/map/../common/sanity.h \
 src/map/../common/timer.h src/map/../common/socket.h \
 src/map/../common/nullpo.h src/map/battle.h src/map/npc.h src/map/pc.h \
 src/map/map.h src/map/mob.h src/map/intif.h src/map/clif.h src/map/tmw.h
${BUILD_DIR}/map/mob.o: src/map/mob.c src/map/../common/timer.h \
 src/map/../common/sanity.h src/map/../common/socket.h \
 src/map/../common/db.h src/map/../common/nullpo.h \
 src/map/../common/mt_rand.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/clif.h src/map/storage.h \
 src/map/intif.h src/map/pc.h src/map/mob.h src/map/guild.h \
 src/map/itemdb.h src/map/skill.h src/map/magic.h src/map/battle.h \
 src/map/party.h src/map/npc.h
${BUILD_DIR}/map/storage.o: src/map/storage.c src/map/../common/db.h \
 src/map/../common/sanity.h src/map/../common/nullpo.h src/map/storage.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/chrif.h \
 src/map/itemdb.h src/map/map.h src/map/../common/timer.h src/map/clif.h \
 src/map/intif.h src/map/pc.h src/map/guild.h src/map/battle.h \
 src/map/atcommand.h
${BUILD_DIR}/map/path.o: src/map/path.c src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/db.h src/map/battle.h src/map/../common/nullpo.h
${BUILD_DIR}/map/magic-interpreter-parser.o: \
 src/map/magic-interpreter-parser.c src/map/magic-interpreter.h \
 src/map/../common/nullpo.h src/map/../common/sanity.h src/map/battle.h \
 src/map/chat.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/timer.h \
 src/map/../common/db.h src/map/chrif.h src/map/clif.h src/map/storage.h \
 src/map/intif.h src/map/itemdb.h src/map/magic.h src/map/mob.h \
 src/map/npc.h src/map/pc.h src/map/party.h src/map/script.h \
 src/map/skill.h src/map/trade.h src/map/../common/socket.h \
 src/map/magic-expr.h src/map/magic-interpreter-aux.h
${BUILD_DIR}/map/skill.o: src/map/skill.c src/map/../common/timer.h \
 src/map/../common/sanity.h src/map/../common/nullpo.h \
 src/map/../common/mt_rand.h src/map/magic.h src/map/clif.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/../common/db.h \
 src/map/storage.h src/map/intif.h src/map/battle.h src/map/itemdb.h \
 src/map/mob.h src/map/party.h src/map/pc.h src/map/script.h \
 src/map/skill.h src/map/../common/socket.h
${BUILD_DIR}/map/magic-interpreter-lexer.o: \
 src/map/magic-interpreter-lexer.c src/map/magic-interpreter.h \
 src/map/../common/nullpo.h src/map/../common/sanity.h src/map/battle.h \
 src/map/chat.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/timer.h \
 src/map/../common/db.h src/map/chrif.h src/map/clif.h src/map/storage.h \
 src/map/intif.h src/map/itemdb.h src/map/magic.h src/map/mob.h \
 src/map/npc.h src/map/pc.h src/map/party.h src/map/script.h \
 src/map/skill.h src/map/trade.h src/map/../common/socket.h \
 src/map/magic-interpreter-parser.h
${BUILD_DIR}/map/magic-stmt.o: src/map/magic-stmt.c \
 src/map/magic-interpreter.h src/map/../common/nullpo.h \
 src/map/../common/sanity.h src/map/battle.h src/map/chat.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/../common/db.h src/map/chrif.h \
 src/map/clif.h src/map/storage.h src/map/intif.h src/map/itemdb.h \
 src/map/magic.h src/map/mob.h src/map/npc.h src/map/pc.h src/map/party.h \
 src/map/script.h src/map/skill.h src/map/trade.h \
 src/map/../common/socket.h src/map/magic-expr.h \
 src/map/magic-interpreter-aux.h src/map/magic-expr-eval.h
${BUILD_DIR}/map/chat.o: src/map/chat.c src/map/../common/db.h \
 src/map/../common/sanity.h src/map/../common/nullpo.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/clif.h src/map/storage.h src/map/pc.h \
 src/map/chat.h src/map/npc.h
${BUILD_DIR}/map/skill-pools.o: src/map/skill-pools.c \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/nullpo.h src/map/../common/mt_rand.h src/map/magic.h \
 src/map/clif.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/db.h src/map/storage.h \
 src/map/intif.h src/map/battle.h src/map/itemdb.h src/map/mob.h \
 src/map/party.h src/map/pc.h src/map/script.h src/map/skill.h \
 src/map/../common/socket.h
${BUILD_DIR}/map/trade.o: src/map/trade.c src/map/clif.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/db.h src/map/storage.h src/map/itemdb.h \
 src/map/trade.h src/map/pc.h src/map/npc.h src/map/battle.h \
 src/map/../common/nullpo.h
${BUILD_DIR}/map/party.o: src/map/party.c src/map/party.h \
 src/map/../common/db.h src/map/../common/sanity.h \
 src/map/../common/timer.h src/map/../common/socket.h \
 src/map/../common/nullpo.h src/map/pc.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/battle.h \
 src/map/intif.h src/map/clif.h src/map/storage.h src/map/skill.h \
 src/map/magic.h src/map/tmw.h
${BUILD_DIR}/map/npc.o: src/map/npc.c src/map/../common/nullpo.h \
 src/map/../common/sanity.h src/map/../common/timer.h src/map/battle.h \
 src/map/clif.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/db.h src/map/storage.h \
 src/map/intif.h src/map/itemdb.h src/map/mob.h src/map/npc.h \
 src/map/pc.h src/map/script.h src/map/skill.h src/map/magic.h \
 src/map/../common/socket.h
${BUILD_DIR}/map/magic-interpreter-base.o: \
 src/map/magic-interpreter-base.c src/map/magic.h src/map/clif.h \
 src/map/map.h src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/db.h src/map/storage.h src/map/intif.h \
 src/map/magic-interpreter.h src/map/../common/nullpo.h src/map/battle.h \
 src/map/chat.h src/map/chrif.h src/map/itemdb.h src/map/mob.h \
 src/map/npc.h src/map/pc.h src/map/party.h src/map/script.h \
 src/map/skill.h src/map/trade.h src/map/../common/socket.h \
 src/map/magic-expr.h src/map/magic-interpreter-aux.h
${BUILD_DIR}/map/pc.o: src/map/pc.c src/map/../common/socket.h \
 src/map/../common/sanity.h src/map/../common/timer.h \
 src/map/../common/db.h src/map/../common/nullpo.h \
 src/map/../common/mt_rand.h src/map/atcommand.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/battle.h \
 src/map/chat.h src/map/chrif.h src/map/clif.h src/map/storage.h \
 src/map/guild.h src/map/intif.h src/map/itemdb.h src/map/mob.h \
 src/map/npc.h src/map/party.h src/map/pc.h src/map/script.h \
 src/map/skill.h src/map/magic.h src/map/trade.h
${BUILD_DIR}/map/tmw.o: src/map/tmw.c src/map/tmw.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/db.h src/map/../common/socket.h \
 src/map/../common/version.h src/map/../common/nullpo.h \
 src/map/atcommand.h src/map/battle.h src/map/chat.h src/map/chrif.h \
 src/map/clif.h src/map/storage.h src/map/guild.h src/map/intif.h \
 src/map/itemdb.h src/map/magic.h src/map/mob.h src/map/npc.h \
 src/map/party.h src/map/pc.h src/map/script.h src/map/skill.h \
 src/map/trade.h
${BUILD_DIR}/map/intif.o: src/map/intif.c src/map/../common/nullpo.h \
 src/map/../common/sanity.h src/map/../common/socket.h \
 src/map/../common/timer.h src/map/battle.h src/map/chrif.h \
 src/map/clif.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/db.h src/map/storage.h \
 src/map/guild.h src/map/intif.h src/map/party.h src/map/pc.h
${BUILD_DIR}/map/magic-expr.o: src/map/magic-expr.c src/map/magic-expr.h \
 src/map/magic-interpreter.h src/map/../common/nullpo.h \
 src/map/../common/sanity.h src/map/battle.h src/map/chat.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/timer.h src/map/../common/db.h src/map/chrif.h \
 src/map/clif.h src/map/storage.h src/map/intif.h src/map/itemdb.h \
 src/map/magic.h src/map/mob.h src/map/npc.h src/map/pc.h src/map/party.h \
 src/map/script.h src/map/skill.h src/map/trade.h \
 src/map/../common/socket.h src/map/magic-interpreter-aux.h \
 src/map/magic-expr-eval.h src/map/../common/mt_rand.h
${BUILD_DIR}/map/battle.o: src/map/battle.c src/map/battle.h \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/nullpo.h src/map/clif.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/../common/db.h \
 src/map/storage.h src/map/guild.h src/map/itemdb.h src/map/mob.h \
 src/map/pc.h src/map/skill.h src/map/magic.h src/map/intif.h \
 src/map/../common/socket.h src/map/../common/mt_rand.h
${BUILD_DIR}/map/script.o: src/map/script.c src/map/../common/socket.h \
 src/map/../common/sanity.h src/map/../common/timer.h \
 src/map/../common/lock.h src/map/../common/mt_rand.h src/map/atcommand.h \
 src/map/map.h src/map/../common/mmo.h src/map/../common/utils.h \
 src/map/../common/db.h src/map/battle.h src/map/chat.h src/map/chrif.h \
 src/map/clif.h src/map/storage.h src/map/guild.h src/map/intif.h \
 src/map/itemdb.h src/map/mob.h src/map/npc.h src/map/party.h \
 src/map/pc.h src/map/script.h src/map/skill.h src/map/magic.h
${BUILD_DIR}/map/clif.o: src/map/clif.c src/map/../common/socket.h \
 src/map/../common/sanity.h src/map/../common/timer.h \
 src/map/../common/version.h src/map/../common/nullpo.h \
 src/map/../common/md5calc.h src/map/../common/mt_rand.h \
 src/map/atcommand.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/../common/db.h src/map/battle.h \
 src/map/chat.h src/map/chrif.h src/map/clif.h src/map/storage.h \
 src/map/guild.h src/map/intif.h src/map/itemdb.h src/map/magic.h \
 src/map/mob.h src/map/npc.h src/map/party.h src/map/pc.h \
 src/map/script.h src/map/skill.h src/map/tmw.h src/map/trade.h
${BUILD_DIR}/map/chrif.o: src/map/chrif.c src/map/../common/socket.h \
 src/map/../common/sanity.h src/map/../common/timer.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/../common/db.h \
 src/map/battle.h src/map/chrif.h src/map/clif.h src/map/storage.h \
 src/map/intif.h src/map/npc.h src/map/pc.h src/map/../common/nullpo.h \
 src/map/itemdb.h
${BUILD_DIR}/map/atcommand.o: src/map/atcommand.c \
 src/map/../common/socket.h src/map/../common/sanity.h \
 src/map/../common/timer.h src/map/../common/nullpo.h \
 src/map/../common/mt_rand.h src/map/atcommand.h src/map/map.h \
 src/map/../common/mmo.h src/map/../common/utils.h src/map/../common/db.h \
 src/map/battle.h src/map/clif.h src/map/storage.h src/map/chrif.h \
 src/map/guild.h src/map/intif.h src/map/itemdb.h src/map/mob.h \
 src/map/npc.h src/map/pc.h src/map/party.h src/map/script.h \
 src/map/skill.h src/map/magic.h src/map/trade.h src/map/../common/core.h \
 src/map/tmw.h
${BUILD_DIR}/map/map.o: src/map/map.c src/map/../common/core.h \
 src/map/../common/timer.h src/map/../common/sanity.h \
 src/map/../common/db.h src/map/../common/grfio.h \
 src/map/../common/mt_rand.h src/map/map.h src/map/../common/mmo.h \
 src/map/../common/utils.h src/map/chrif.h src/map/clif.h \
 src/map/storage.h src/map/intif.h src/map/npc.h src/map/pc.h \
 src/map/mob.h src/map/chat.h src/map/itemdb.h src/map/skill.h \
 src/map/magic.h src/map/trade.h src/map/party.h src/map/battle.h \
 src/map/script.h src/map/guild.h src/map/atcommand.h \
 src/map/../common/nullpo.h src/map/../common/socket.h
${BUILD_DIR}/ladmin/ladmin.o: src/ladmin/ladmin.c \
 src/ladmin/../common/core.h src/ladmin/../common/socket.h \
 src/ladmin/../common/sanity.h src/ladmin/ladmin.h \
 src/ladmin/../common/version.h src/ladmin/../common/mmo.h \
 src/ladmin/../common/utils.h src/ladmin/../common/md5calc.h

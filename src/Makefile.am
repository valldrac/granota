SUBDIRS = afl exploitable libgranota
bin_PROGRAMS = granota
pkglibexec_SCRIPTS = crashdbg.sh
granota_SOURCES = bandit.c bandit.h bitwise.c bitwise.h common.h critbit.c critbit.h debug.h fuzz.c fuzz.h ga.c ga.h granota.c jkiss.c jkiss.h nulist.h progname.c progname.h random.c random.h strerr.c strerr.h summary.h term.h util.c util.h gtrcommon/files.h gtrcommon/input.h gtrcommon/shmem.c gtrcommon/shmem.h gtrcommon/semop.h gtrcommon/target.h
granota_CPPFLAGS = -I$(top_srcdir)/src/gtrcommon -DPKGLIBDIR=\"$(pkglibdir)\"
granota_LDADD = -lpthread -lrt -lcrypto
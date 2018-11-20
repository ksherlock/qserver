#for use with iix/make(1)

CC = occ
CHTYP = iix chtyp
COPYFORK = iix copyfork
LDFLAGS =
LDLIBS = 
CFLAGS	+=  $(DEFINES) -v #-O
OBJS	= qserver.a macroman.a common.a config.a tools.a

qserver: $(OBJS)  qserver.r
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(COPYFORK) qserver.r $@ -r
	$(CHTYP) -t nda $@
	

common.a: common.c macroman.h
config.a: config.c qserver.h
macroman.a: macroman.c macroman.h
qserver.a: qserver.c qserver.h
qserver.r: qserver.rez qserver.h
tools.a : tools.c

clean:
	$(RM) *.a *.root *.r

clobber: clean

%.a: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.r: %.rez
	$(CC) $< -o $@
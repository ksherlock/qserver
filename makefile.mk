#for use with GNO/dmake(1)

CFLAGS	+=  $(DEFINES) -v #-O
OBJS	= qserver.o macroman.o common.o config.o

qserver: $(OBJS)  qserver.r
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	chtyp -t nda $@
	copyfork qserver.r $@ -r
	

common.o: common.c macroman.h
config.o: config.c qserver.h
macroman.o: macroman.c macroman.h
qserver.o: qserver.c qserver.h
qserver.r: qserver.rez qserver.h


clean:
	$(RM) *.o *.root *.r

clobber: clean
	$(RM) -f qserver

#for use iwith dmake(1)
# for compiling

CFLAGS	+=  $(DEFINES) -v #-O
LDLIBS += -l /usr/local/lib/libk
OBJS	= qserver.o macroman.o common.o config.o
CFLAGS += -I /usr/local/include/

qserver: $(OBJS)  qserver.r
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	chtyp -t nda $@
	copyfork qserver.r $@ -r
	

qserver.o: qserver.c qserver.h
qserver.r: qserver.rez qserver.h


clean:
	$(RM) *.o *.root *.r

clobber: clean
	$(RM) -f qotdd

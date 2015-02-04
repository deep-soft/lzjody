CC=gcc
AR=ar
CFLAGS=-O3 -g
#CFLAGS=-Og -g3
BUILD_CFLAGS=-std=gnu99 -I. -D_FILE_OFFSET_BITS=64 -pipe -Wall -pedantic
LDFLAGS=-L.
LDLIBS=

prefix=${DESTDIR}/usr
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
mandir=${prefix}/man
datarootdir=${prefix}/share
datadir=${datarootdir}
sysconfdir=${prefix}/etc

# Use POSIX threads by default, but allow the user to override them
ifndef NOTHREADS
LDFLAGS += -lpthread
BUILD_CFLAGS += -DTHREADED
endif

ifdef DEBUG
BUILD_CFLAGS += -DDEBUG -g
endif

all: lzjody lzjody.static test

lzjody.static: liblzjody.a lzjody_util.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(BUILD_CFLAGS) -o lzjody.static lzjody_util.o liblzjody.a

lzjody: liblzjody.so lzjody_util.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(BUILD_CFLAGS) -llzjody -o lzjody lzjody_util.o

liblzjody.so: lzjody.c byteplane_xfrm.c
	$(CC) -c $(BUILD_CFLAGS) -fPIC $(CFLAGS) -o byteplane_xfrm_shared.o byteplane_xfrm.c
	$(CC) -c $(BUILD_CFLAGS) -fPIC $(CFLAGS) -o lzjody_shared.o lzjody.c
	$(CC) -shared -o liblzjody.so lzjody_shared.o byteplane_xfrm_shared.o

liblzjody.a: lzjody.c byteplane_xfrm.c
	$(CC) -c $(BUILD_CFLAGS) $(CFLAGS) byteplane_xfrm.c
	$(CC) -c $(BUILD_CFLAGS) $(CFLAGS) lzjody.c
	$(AR) rcs liblzjody.a lzjody.o byteplane_xfrm.o

#manual:
#	gzip -9 < lzjody.8 > lzjody.8.gz

.c.o:
	$(CC) -c $(BUILD_CFLAGS) $(CFLAGS) $<

clean:
	rm -f *.o *.a *~ .*un~ lzjody lzjody*.static *.so* debug.log *.?.gz log.test.* out.*

distclean:
	rm -f *.o *.a *~ .*un~ lzjody lzjody*.static *.so* debug.log *.?.gz log.test.* out.* *.pkg.tar.*

install: all
	install -D -o root -g root -m 0755 lzjody $(bindir)/lzjody
	install -D -o root -g root -m 0755 liblzjody.so $(libdir)/liblzjody.so
	install -D -o root -g root -m 0644 liblzjody.a $(libdir)/liblzjody.a
	install -D -o root -g root -m 0644 lzjody.h $(includedir)/lzjody.h
#	install -D -o root -g root -m 0644 lzjody.8.gz $(mandir)/man8/lzjody.8.gz

test: lzjody.static
	./test.sh

package:
	+./chroot_build.sh

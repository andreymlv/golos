# golos - speak with your friends
# See LICENSE file for copyright and license details.

include config.mk

CLIENTSRC = client.c
SERVERSRC = server.c
CLIENTOBJ = ${CLIENTSRC:.c=.o}
SERVEROBJ = ${SERVERSRC:.c=.o}

all: options client server

options:
	@echo golos build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${CLIENTOBJ}: config.mk

${SERVEROBJ}: config.mk

client: ${CLIENTOBJ}
	${CC} -o $@ ${CLIENTOBJ} ${LDFLAGS}

server: ${SERVEROBJ}
	${CC} -o $@ ${SERVEROBJ} ${LDFLAGS}

clean:
	rm -f client server ${CLIENTOBJ} ${SERVEROBJ} golos-${VERSION}.tar.gz

dist: clean
	mkdir -p golos-${VERSION}
	cp -R LICENSE Makefile README.md config.mk ${CLIENTSRC} ${SERVERSRC}\
		golos-${VERSION} util.h deps/miniaudio/miniaudio.h
	tar -cf golos-${VERSION}.tar golos-${VERSION}
	gzip golos-${VERSION}.tar
	rm -rf golos-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f client ${DESTDIR}${PREFIX}/bin/golos-client
	cp -f server ${DESTDIR}${PREFIX}/bin/golos-server
	chmod 755 ${DESTDIR}${PREFIX}/bin/golos-client
	chmod 755 ${DESTDIR}${PREFIX}/bin/golos-server

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/golos-client
	rm -f ${DESTDIR}${PREFIX}/bin/golos-server

clangd: clean
	bear -- make

.PHONY: all options clean dist install uninstall

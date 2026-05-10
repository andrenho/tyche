# user overwritable variables

PREFIX=/usr/local

# version

VERSION_MAJOR=0
VERSION_MINOR=1

VERSION=${VERSION_MAJOR}.${VERSION_MINOR}

#
# generic targets
#

all: tyche libtyche.a libtyche.so.${VERSION}

check: tyche-test
	./tyche-test

clean:
	rm -f tyche libtyche.a libtyche.so* tyche-test src/tyche.o src/tests.o lib/vm.o

install: tyche libtyche.a libtyche.so.${VERSION}
	install libtyche.a libtyche.so.${VERSION} ${PREFIX}/lib
	install tyche ${PREFIX}/bin
	ln -s libfoo.so.${VERSION} libfoo.so.${VERSION_MAJOR}
	ln -s libfoo.so.${VERSION_MAJOR} libfoo.so

uninstall:
	rm -f ${PREFIX}/lib/libtyche.* ${PREFIX}/bin/tyche

#
# flags/options
#

CFLAGS=-fPIC
LIBS=

#
# executable files
#

tyche: src/tyche.o libtyche.a
	$(CC) -o $@ $^ ${LIBS}

tyche-test: src/tests.o libtyche.a
	$(CC) -o $@ $^ ${LIBS}

libtyche.a: lib/vm.o
	ar rcs $@ $^

libtyche.so.${VERSION}: lib/vm.o
	$(CC) -shared -o $@ -Wl,-soname,libfoo.so.${VERSION_MAJOR} $^
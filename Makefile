#
# user overwritable variables
#

# install prefix
PREFIX=/usr/local

#
# internal flags/options
#

# version

VERSION_MAJOR=0
VERSION_MINOR=1

VERSION=${VERSION_MAJOR}.${VERSION_MINOR}

# add compiler-specific warnings

IS_CLANG := $(shell $(CC) -dM -E - < /dev/null | grep -c __clang__)
WARNINGS=@config/WARNINGS
ADD_DBG_FLAGS=
ifeq ($(IS_CLANG),1)
  WARNINGS += @config/WARNINGS_CLANG
else
  WARNINGS += @config/WARNINGS_GCC
  ADD_DBG_FLAGS=-fanalyzer
endif

# debug and release flags

DEBUG_CFLAGS=-O0 -ggdb3 ${WARNINGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined \
  -fno-sanitize-recover=all -fstack-protector-strong -fstack-clash-protection -fno-common ${ADD_DBG_FLAGS} \
  -DCHECK_TYCHE_BUGS=1
DEBUG_LDFLAGS=-fsanitize=address -fsanitize=undefined

# apple clang doesn't support -fsanitize=leak
UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
  DEBUG_CFLAGS += -fsanitize=leak
  DEBUG_LDFLAGS += -fsanitize=leak
endif

RELEASE_CFLAGS=-O3 -flto=auto -march=native -mtune=native -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -fstack-protector-strong
RELEASE_LDFLAGS=-flto=auto

CFLAGS+=-std=c99 -D_GNU_SOURCE -fPIC -fvisibility=hidden -isystem lib/contrib -MMD -MP
LDFLAGS+=


#
# generic targets
#

all: tyche libtyche.a libtyche.so.${VERSION}

check:
	$(MAKE) clean
	$(MAKE) tyche-test
	./tyche-test

clean:
	rm -f tyche libtyche.a libtyche.so* tyche-test **/*.o **/*.d

install: tyche libtyche.a libtyche.so.${VERSION} lib/tyche.h
	install -m 644 libtyche.a libtyche.so.${VERSION} ${PREFIX}/lib
	install tyche ${PREFIX}/bin
	install -m 644 lib/tyche.h ${PREFIX}/include
	ln -s ${PREFIX}/lib/libtyche.so.${VERSION} ${PREFIX}/lib/libtyche.so.${VERSION_MAJOR}
	ln -s ${PREFIX}/lib/libtyche.so.${VERSION_MAJOR} ${PREFIX}/lib/libtyche.so

uninstall:
	rm -f ${PREFIX}/lib/libtyche.* ${PREFIX}/bin/tyche ${PREFIX}/include/tyche.h

.PHONY: all check clean install uninstall

#
# intermediate rules
#

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

#
# executable files
#

LIB_SRC=lib/value.o lib/stack.o lib/heap.o lib/vm.o

tyche: CFLAGS += ${RELEASE_CFLAGS}
tyche: LDFLAGS += ${RELEASE_LDFLAGS}
tyche: src/tyche.o libtyche.a
	$(CC) -o $@ $^ ${LDFLAGS}
	strip $@

tyche-test: CFLAGS += ${DEBUG_CFLAGS}
tyche-test: LDFLAGS += ${DEBUG_LDFLAGS}
tyche-test: test/tests.o libtyche.a
	$(CC) -o $@ $^ ${LDFLAGS} -I../lib

libtyche.a: ${LIB_SRC}
	ar rcs $@ $^

libtyche.so.${VERSION}: LDFLAGS += ${RELEASE_LDFLAGS}
libtyche.so.${VERSION}: ${LIB_SRC}
	$(CC) -shared -o $@ -Wl,-soname,libfoo.so.${VERSION_MAJOR} $^ ${LDFLAGS}

-include $(LIB_SRC:.o=.d)
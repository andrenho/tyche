#
# user overwritable variables
#

# add debugging info to library/executable (slower)
DEBUG ?= 0

# install prefix
PREFIX ?= /usr/local

# add functions to debug assembly to console
DEBUG_ASSEMBLY ?= 0

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
RELEASE_LDFLAGS=-flto=auto -s

CFLAGS+=-std=c99 -D_GNU_SOURCE -fPIC -fvisibility=hidden -isystem lib/contrib -isystem src/contrib -isystem test/contrib -MMD -MP \
	-DVERSION_MINOR=${VERSION_MINOR} -DVERSION_MAJOR=${VERSION_MAJOR} -Ilib
LDFLAGS+=-lm

ifeq ($(DEBUG_ASSEMBLY),1)
  CFLAGS += -DDEBUG_ASSEMBLY
endif

# adjust CFLAGS/LDFLAGS

ifeq ($(DEBUG),1)
  CFLAGS+=${DEBUG_CFLAGS}
  LDFLAGS+=${DEBUG_LDFLAGS}
else
  CFLAGS+=${RELEASE_CFLAGS}
  LDFLAGS+=${RELEASE_LDFLAGS}
endif

#
# generic targets
#

all: tyche libtyche.a libtyche.so.${VERSION}

check: tyche-test-as tyche-test-vm tyche-test-op tyche-test-compiler
	./tyche-test-as
	./tyche-test-vm
	./tyche-test-op
	./tyche-test-compiler

clean:
	find . -name '*.[od]' -delete
	rm -f tyche libtyche.a libtyche.so* tyche-test-* \
		lib/instructions/instructions.h lib/instructions/instructions.c

install: tyche libtyche.a libtyche.so.${VERSION} lib/tyche.h
	install -m 644 libtyche.a libtyche.so.${VERSION} ${PREFIX}/lib
	install tyche ${PREFIX}/bin
	install -m 644 lib/tyche.h ${PREFIX}/include
	ln -s ${PREFIX}/lib/libtyche.so.${VERSION} ${PREFIX}/lib/libtyche.so.${VERSION_MAJOR}
	ln -s ${PREFIX}/lib/libtyche.so.${VERSION_MAJOR} ${PREFIX}/lib/libtyche.so

uninstall:
	rm -f ${PREFIX}/lib/libtyche.* ${PREFIX}/bin/tyche ${PREFIX}/include/tyche.h

package: clean
	tar -cjf tyche-${VERSION}.tar.bz2 config doc lib src test LICENSE Makefile README.md

.PHONY: all check clean install uninstall package

#
# custom instructions for code generation
#

lib/instructions/instructions.h: lib/instructions/gen-inst.sh
	cd lib/instructions && ./gen-inst.sh

#
# executable files
#

LIB_SRC=lib/value.o lib/stack.o lib/array.o lib/table.o lib/heap.o lib/vm.o lib/expr.o \
 	lib/code.o lib/utils.o lib/assembler/assembly.o lib/assembler/assembler.o lib/assembler/adj_labels.o \
 	lib/assembler/bytecode.o lib/instructions/instructions.o \
 	lib/compiler/lexer.o

$(LIB_SRC:.o=.c) test/tests-as.c test/tests-vm.c: lib/instructions/instructions.h

tyche: src/tyche.o libtyche.a
	$(CC) -o $@ $^ ${LDFLAGS}

tyche-test-as: lib/instructions/instructions.h
tyche-test-as: test/tests-as.o libtyche.a
	$(CC) -o $@ test/tests-as.o libtyche.a ${LDFLAGS} -I../lib

tyche-test-vm: lib/instructions/instructions.h
tyche-test-vm: test/tests-vm.o libtyche.a
	$(CC) -o $@ test/tests-vm.o libtyche.a ${LDFLAGS} -I../lib

tyche-test-op: lib/instructions/instructions.h
tyche-test-op: test/tests-op.o libtyche.a
	$(CC) -o $@ test/tests-op.o libtyche.a ${LDFLAGS} -I../lib

tyche-test-compiler: lib/instructions/instructions.h
tyche-test-compiler: test/tests-compiler.o libtyche.a
	$(CC) -o $@ test/tests-compiler.o libtyche.a ${LDFLAGS} -I../lib

libtyche.a: ${LIB_SRC}
	ar rcs $@ $^

libtyche.so.${VERSION}: ${LIB_SRC}
	$(CC) -shared -o $@ -Wl,-soname,libfoo.so.${VERSION_MAJOR} $^ ${LDFLAGS}

-include $(LIB_SRC:.o=.d)
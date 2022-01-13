CC=gcc

# uncomment to compile in 32bits mode (require gcc-*-multilib packages
# on Debian/Ubuntu)
#HOST32= -m32

CFLAGS+= $(HOST32) -Wall -Werror -std=c99 -g -D_GNU_SOURCE
CFLAGS+= -DDEBUG
# pour tester avec ls
CFLAGS+= -fPIC
LDFLAGS= $(HOST32)
TESTS+=test_init
PROGRAMS=memshell $(TESTS)

.PHONY: clean all test_ls tests tests/valgrind

all: $(PROGRAMS)
	for file in $(TESTS);do ./$$file; done

%.o: %.c
	$(CC) -c $(CFLAGS) -MMD -MF .$@.deps -o $@ $<

# dépendences des binaires
$(PROGRAMS) libmalloc.so: %: mem.o common.o

-include $(wildcard .*.deps)

# seconde partie du sujet
libmalloc.so: malloc_stub.o
	$(CC) -shared -Wl,-soname,$@ $^ -o $@

memshell: memshell.c mem.o common.o
	$(CC) mem.o common.o memshell.c -o memshell

test_ls: libmalloc.so
	LD_PRELOAD=./libmalloc.so ls

# Valgrind tests

tests: tests/general tests/valgrind

tests/general: tests/general.c mem.o common.o
	$(CC) $(CFLAGS) -o $@ $^

tests/valgrind_leak: tests/valgrind_leak.c malloc_stub.o mem.o common.o
	$(CC) $(CFLAGS) -o $@ $^

tests/valgrind_no_leak: tests/valgrind_no_leak.c malloc_stub.o malloc_stub.o mem.o common.o
	$(CC) $(CFLAGS) -o $@ $^

GREEN='\033[0;32m'
RESET='\033[0m'
tests/valgrind: tests/valgrind_leak tests/valgrind_no_leak
	! valgrind --leak-check=full --error-exitcode=1 tests/valgrind_leak
	@echo $(GREEN)"✓ The control test successfully leaked memory according to valgrind"$(RESET)
	valgrind --leak-check=full --error-exitcode=1 tests/valgrind_no_leak
	@echo $(GREEN)"✓ The correct program didn't leak memory according to valgrind"$(RESET)

# nettoyage
clean:
	$(RM) *.o $(PROGRAMS) libmalloc.so .*.deps tests/link_test

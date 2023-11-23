CC := gcc
TEST_CC ?= clang

CC_FLAGS := -O3 -Wall -Werror -march=native

# Note: by default, we do not enable address sanitizer
MEMORY_FLAGS := -fsanitize=address -fno-omit-frame-pointer

LIBINCLUDE := -Iinclude/
DRIVERINCLUDE := -Itest/

LD_FLAGS := -fPIC -shared

TEST_FLAGS := -O0

DRIVERSRC := $(wildcard *.c)
DRIVEROBJ := $(patsubst %.c, %.o, $(DRIVERSRC))
DRIVER := driver

LIBSRC := $(wildcard lib/*.c)
LIBOBJ := $(patsubst %.c, %.o, $(LIBSRC))
LIB := libtest.so

TESTSRC := $(wildcard test/*.c)
TESTOBJ := $(patsubst %.c, %.o, $(TESTSRC))
TESTLIB := libtestfunc.so

.PHONE: all clean

all: $(DRIVER)

$(DRIVER): $(LIB) $(TESTLIB) $(DRIVEROBJ)
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(DRIVERINCLUDE) -o $@ $^ -L. -ltestfunc -ltest

$(DRIVEROBJ): %.o:%.c
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(DRIVERINCLUDE) -c $< -o $@

$(LIB): $(LIBOBJ)
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(LD_FLAGS) -lpthread -o $@ $^

$(LIBOBJ): %.o:%.c
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(LD_FLAGS) -c $< -o $@

$(TESTLIB): $(TESTOBJ)
	$(TEST_CC) $(TEST_FLAGS) $(LD_FLAGS) -o $@ $^

$(TESTOBJ): %.o:%.c
	$(TEST_CC) $(TEST_FLAGS) -c $< -o $@

clean: 
	rm -rf $(DRIVER)
	rm -rf $(DRIVEROBJ)
	rm -rf $(LIB)
	rm -rf $(LIBOBJ)
	rm -rf $(TESTLIB)
	rm -rf $(TESTOBJ)

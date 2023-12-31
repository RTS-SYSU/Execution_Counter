CC := gcc
TEST_CC ?= gcc

MODE ?= Release

ifeq ($(MODE), Release)
CC_FLAGS := -O3 -Wall -Werror -march=native -fPIC
else ifeq ($(MODE), Debug)
CC_FLAGS := -O0 -g -fPIC -Wall -Werror -march=native # -fsanitize=address -fno-omit-frame-pointer
endif

# Note: by default, we do not enable address sanitizer
MEMORY_FLAGS := -fsanitize=address -fno-omit-frame-pointer

LIBINCLUDE := -Iinclude/ -Ilib/json/include/
DRIVERINCLUDE := -Itest/

# Only gcc support -fno-gnu-unique
ifeq ($(CC), gcc)
LD_FLAGS := -shared -fno-gnu-unique
else
LD_FLAGS := -shared
endif

DEFAULT_FLAGS := -fPIC
TEST_FLAGS := -O0

DRIVERSRC := $(wildcard *.c)
DRIVEROBJ := $(patsubst %.c, %.o, $(DRIVERSRC))
DRIVER := driver

FRAMESRC := $(wildcard lib/framework/*.c)
FRAMEOBJ := $(patsubst %.c, %.o, $(FRAMESRC))
FRAMELIB := libtest.so

TESTSRC := $(wildcard test/*.c)
TESTOBJ := $(patsubst %.c, %.o, $(TESTSRC))
TESTLIB := libtestfunc.so

JSONLIB := libjson.so

.PHONE: all clean

all: $(DRIVER)

$(DRIVER): $(DRIVEROBJ) $(JSONLIB) $(FRAMELIB) $(TESTLIB)
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(DRIVERINCLUDE) -o $@ $< -ldl -lc -L. -ltestfunc -ltest -ljson

$(DRIVEROBJ): %.o:%.c
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(DRIVERINCLUDE) -c $< -o $@

$(FRAMELIB): $(FRAMEOBJ)
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(LD_FLAGS) -lpthread -ldl -lc -L. -ljson -o $@ $^

$(FRAMEOBJ): %.o:%.c
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) -c $< -o $@

$(TESTLIB): $(TESTOBJ)
	$(TEST_CC) $(DEFAULT_FLAGS) $(TEST_FLAGS) $(LD_FLAGS) -o $@ $^

$(TESTOBJ): %.o:%.c
	$(TEST_CC) $(DEFAULT_FLAGS) $(TEST_FLAGS) -c $< -o $@

$(JSONLIB):
	$(MAKE) -C lib/json lib
	@ln -s lib/json/libjson.so libjson.so

clean: 
	rm -rf $(DRIVER)
	rm -rf $(DRIVEROBJ)
	rm -rf $(FRAMELIB)
	rm -rf $(FRAMEOBJ)
	rm -rf $(TESTLIB)
	rm -rf $(TESTOBJ)
	rm -rf $(JSONLIB)
	$(MAKE) -C lib/json clean

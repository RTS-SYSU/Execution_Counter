CC := gcc
TEST_CC ?= gcc

# Add support for Cross-Compilation
CROSS_COMPILE ?=

# For Additional Flags
ADD_FLAGS ?=

CC := $(CROSS_COMPILE)$(CC)
TEST_CC := $(CROSS_COMPILE)$(TEST_CC)

MODE ?= Release
SHARED := -fPIC -shared

ifeq ($(MODE), Release)
	CC_FLAGS := -O3 -Wall -Werror
else ifeq ($(MODE), Debug)
	CC_FLAGS := -O0 -g -fPIC -Wall -Werror
endif

# Add $ADD_FLAGS to CC_FLAGS
CC_FLAGS += $(ADD_FLAGS)

ifeq ($(CROSS_COMPILE), )
	CC_FLAGS += -march=native
endif

# Note: by default, we do not enable address sanitizer
MEMORY_FLAGS := -fsanitize=address -fno-omit-frame-pointer

LIBINCLUDE := -Iinclude/ -Ilib/json/include/
DRIVERINCLUDE := -Itest/

# Only gcc support -fno-gnu-unique
ifeq ($(CC), gcc)
	LD_FLAGS := -fno-gnu-unique
else
	LD_FLAGS := 
endif

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

.PHONE: all clean test rebuild

all: $(DRIVER)

test: $(TESTLIB)

$(DRIVER): $(DRIVEROBJ) $(JSONLIB) $(FRAMELIB)
	$(CC) $(CC_FLAGS) $(LIBINCLUDE) $(DRIVERINCLUDE) -o $@ $< -ldl -lc $(FRAMELIB) $(JSONLIB)

$(DRIVEROBJ): %.o:%.c
	$(CC) $(CC_FLAGS) $(SHARED) $(LIBINCLUDE) $(DRIVERINCLUDE) -c $< -o $@

$(FRAMELIB): $(FRAMEOBJ)
	$(CC) $(CC_FLAGS) $(SHARED) $(LIBINCLUDE) $(LD_FLAGS) -lpthread -ldl -lc -L. -ljson -o $@ $^

$(FRAMEOBJ): %.o:%.c
	$(CC) $(CC_FLAGS) $(SHARED) $(LIBINCLUDE) -c $< -o $@

$(TESTLIB): $(TESTOBJ)
	$(TEST_CC) $(SHARED) $(TEST_FLAGS) $(LD_FLAGS) -o $@ $^

$(TESTOBJ): %.o:%.c
	$(TEST_CC) $(SHARED) $(TEST_FLAGS) -c $< -o $@

$(JSONLIB):
	$(MAKE) -C lib/json lib CC="$(CC)"
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

rebuild: clean all
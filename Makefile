CC := gcc
CC_FLAGS := -O0 -Wall -Werror

SRC_C_FILE := $(wildcard *.c)
TARGET := $(patsubst %.c, %, $(SRC_C_FILE))

.PHONE: all clean

all: $(TARGET)

$(TARGET): %:%.c
	$(CC) $(CC_FLAGS) $< -o $@

clean: 
	rm -rf $(TARGET)
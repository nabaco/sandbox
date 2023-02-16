# Copied from https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

#TARGET_EXEC ?= client

BUILD_DIR ?= ./build
SRC_DIRS ?= ../../

SRCS := $(shell find . -type "f" -name "*.cpp" -or -name "*.c" -or -name "*.s")
OBJS := $(SRCS:%.c=%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP
CFLAGS ?= -Werror -Wall -Wextra
LDFLAGS ?= -lpthread

all: $(OBJS)

# c source
%.c.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	$(RM) -r *.o *.d

-include $(DEPS)

MKDIR_P ?= mkdir -p

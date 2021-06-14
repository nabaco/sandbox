# Copied from https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

#TARGET_EXEC ?= client

BUILD_DIR ?= ./build
SRC_DIRS ?= ../../

SRCS := $(shell find . -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP
LDFLAGS ?= -lpthread

all: $(OBJS)

# c source
$(OBJS): $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY: clean

clean:
	$(RM) -r *.o *.d

-include $(DEPS)

MKDIR_P ?= mkdir -p

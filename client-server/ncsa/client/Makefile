# Copied and adapted from https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

TARGET_EXEC ?= client

BUILD_DIR ?= ./build
CMN_DIR ?= ../common

SRCS := $(shell find . -name "*.cpp" -or -name "*.c" -or -name "*.s")
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CMN_SRCS := $(shell find $(CMN_DIR) -name "*.cpp" -or -name "*.c" -or -name "*.s")
CMN_OBJS := $(CMN_SRCS:%.c=%.o)
CMN_DEPS := $(CMN_OBJS:.o=.d)

#INC_DIRS := $(shell find $(SRC_DIRS) -type d)
#INC_FLAGS := $(addprefix -I,$(INC_DIRS))
INC_FLAGS := -I../..

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP
LDFLAGS ?= -lpthread

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

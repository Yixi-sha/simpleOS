.PHONY : all clean  test

TYPE_SRC := c
TYPE_OBJ := o
TYPE_DEP := dep
INC_DIR := inc
SRC_DIR := src
CC := gcc
CFLAGS := -mcmodel=large -fno-builtin -m64 -fno-stack-protector

OUT_DIR := $(realpath ../..)
APP_DIR := $(OUT_DIR)/build
OUT_DIR := $(OUT_DIR)/build/kernel

SRCS := $(wildcard $(SRC_DIR)/*.c)

OBJS := $(notdir $(SRCS))
OBJS := $(OBJS:.$(TYPE_SRC)=.$(TYPE_OBJ))
OBJS := $(addprefix $(OUT_DIR)/,$(OBJS))

DEPS := $(OBJS:.$(TYPE_OBJ)=.$(TYPE_DEP))

TARGET := $(APP_DIR)/kernel.bin
SYSTEM := $(OUT_DIR)/system
 
SRCS_ASM := $(wildcard *.S)
SRCS_ASM := $(SRCS_ASM:head.S=)
SRCS_ASM := head.S $(SRCS_ASM)
MIDS_ASM := $(SRCS_ASM:.S=.s)
MIDS_ASM := $(addprefix $(OUT_DIR)/,$(MIDS_ASM))

OBJS_ASM := $(MIDS_ASM:.s=.o)

include $(DEPS)

all: $(SYSTEM)
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $^ $(TARGET)

$(SYSTEM) : $(OBJS_ASM) $(OBJS)
	ld -b  elf64-x86-64 -z muldefs -o $@ $^ -T kernel.lds 



$(OUT_DIR)/%.o : $(OUT_DIR)/%.s
	as --64 -o $@ $^

$(OUT_DIR)/%.s : %.S
	gcc -E $^ > $@

test:
	@echo "SRCS_ASM ===> $(SRCS_ASM)"

$(OUT_DIR)/%.$(TYPE_OBJ) : $(SRC_DIR)/%.$(TYPE_SRC)
	$(CC) $(CFLAGS) -o $@ -c $(filter %.$(TYPE_SRC), $^)

$(OUT_DIR)/%.$(TYPE_DEP) : $(SRC_DIR)/%.$(TYPE_SRC)
	@echo "creat $@"
	@set -e ; \
	$(CC)  -MM -E $(filter %$(TYPE_SRC), $^) | sed 's,\(.*\)\.o[ :]*,$(OUT_DIR)/\1.$(TYPE_OBJ) $@ \: ,g' > $@
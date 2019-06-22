.PHONY : all clean  test move

OUT_DIR := build
MNT_DIR := mnt
OUT_KERNEL_DIR := $(OUT_DIR)/kernel

IMG := boot.img
LOADER_SRC := code/loader.asm
LOADER_INC := code/fat12.inc
BOOT_SRC := code/boot.asm

BOOT_OUT := $(notdir $(BOOT_SRC))
BOOT_OUT := $(BOOT_OUT:.asm=.bin)
BOOT_OUT := $(addprefix $(OUT_DIR)/,$(BOOT_OUT))


LOADER_OUT := $(notdir $(LOADER_SRC))
LOADER_OUT := $(LOADER_OUT:.asm=.bin)
LOADER_OUT_PATH := $(addprefix $(MNT_DIR)/,$(LOADER_OUT))
LOADER_OUT := $(addprefix $(OUT_DIR)/,$(LOADER_OUT))


KERNEL_OUT := $(OUT_DIR)/kernel.bin
KERNEL_OUT_PATH := $(MNT_DIR)/kernel.bin


RM := rm -rf
MKDIR := mkdir
DIRS := $(OUT_DIR)  $(MNT_DIR) $(OUT_KERNEL_DIR)

all : $(IMG)  $(BOOT_OUT) move
	@echo "Build Success ==> YIXI!"

$(IMG) :
	bximage $@ -q 

move : $(LOADER_OUT) $(KERNEL_OUT)
	sudo mount -o loop boot.img $(MNT_DIR) ;
	sudo cp $(LOADER_OUT) $(LOADER_OUT_PATH) ;
	sudo cp $(KERNEL_OUT) $(KERNEL_OUT_PATH) ;
	cd $(MNT_DIR) ;
	cd .. ;
	sudo umount $(MNT_DIR) ;


ifeq ("$(wildcard $(OUT_DIR))", "")
$(BOOT_OUT) : $(OUT_DIR) $(BOOT_SRC)  
else
$(BOOT_OUT) : $(BOOT_SRC)
endif
	nasm $(filter %.asm,$^) -o $@
	dd if=$@ of=$(IMG) bs=512 count=1 conv=notrunc

$(LOADER_OUT) : $(LOADER_SRC)  $(LOADER_INC)
	set -e ;
	nasm $(filter %.asm,$^) -o $@ ;
ifeq ("$(wildcard $(OUT_KERNEL_DIR))", "")
$(KERNEL_OUT) : $(OUT_KERNEL_DIR)
else 
$(KERNEL_OUT) :
endif
	set -e ; \
	cd code/kernel ; \
	$(MAKE) all 

test :
	@echo " KERNEL_OUT_PATH ==> $(KERNEL_OUT_PATH)"

clean :
	$(RM)  $(OUT_DIR) 

$(DIRS) : % :
	$(MKDIR) $@
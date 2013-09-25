TOOL = arm-none-eabi
CFLAGS = -O3 -nostdlib -nostartfiles -ffreestanding --no-common -Wno-write-strings  
LINKER_FLAGS = --no-wchar-size-warning --no-undefined

# Make sure gcc searches the include folder
C_INCLUDE_PATH=include;csud\include
export C_INCLUDE_PATH

LIBRARIES = csud
BUILD_DIR = bin
SOURCE_DIR = source

CSOURCE := $(wildcard $(SOURCE_DIR)/*.c)
ASOURCE := $(wildcard $(SOURCE_DIR)/*.s)

_COBJECT := $(patsubst %.c,%.o, $(CSOURCE))
_AOBJECT := $(patsubst %.s,%.o, $(ASOURCE))
AOBJECT = $(addprefix $(BUILD_DIR)/, $(notdir $(_AOBJECT)))
COBJECT = $(addprefix $(BUILD_DIR)/, $(notdir $(_COBJECT)))

all: kernel

# Create the final binary
kernel: theelf
	$(TOOL)-objcopy $(BUILD_DIR)/kernel.elf -O binary $(BUILD_DIR)/kernel.img

# Link all of the objects
theelf: $(AOBJECT) $(COBJECT) libcsud.a
	$(TOOL)-ld $(LINKER_FLAGS) $(AOBJECT) $(COBJECT) -L. -l $(LIBRARIES) -Map $(BUILD_DIR)/kernel.map -T memorymap -o $(BUILD_DIR)/kernel.elf

#build c files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(TOOL)-gcc -c $< -o $@ $(CFLAGS)

#build s files (Assembly)
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.s
	$(TOOL)-as $< -o $@

clean:
	rm $(BUILD_DIR)/*.o
	rm $(BUILD_DIR)/*.map
	rm $(BUILD_DIR)/*.elf
	rm $(BUILD_DIR)/*.img

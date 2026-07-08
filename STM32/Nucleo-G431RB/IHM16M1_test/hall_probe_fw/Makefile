TARGET := hall_probe_fw
BUILD_DIR := build

CUBE_G4_DIR ?= $(USERPROFILE)/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.2
DEVICE_INC := $(CUBE_G4_DIR)/Drivers/CMSIS/Device/ST/STM32G4xx/Include
CMSIS_INC := $(CUBE_G4_DIR)/Drivers/CMSIS/Include
STARTUP := $(CUBE_G4_DIR)/Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/gcc/startup_stm32g431xx.s

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
DEFS := -DSTM32G431xx
INCS := -I$(DEVICE_INC) -I$(CMSIS_INC)
WARN := -Wall -Wextra -Werror
OPT := -Os -ffunction-sections -fdata-sections

CFLAGS := $(CPU) $(DEFS) $(INCS) $(WARN) $(OPT) -std=c11
ASFLAGS := $(CPU) $(DEFS) $(INCS) -x assembler-with-cpp
LDFLAGS := $(CPU) -nostdlib -TSTM32G431RB_FLASH.ld -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -lgcc

OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/startup_stm32g431xx.o

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR):
	powershell -NoProfile -ExecutionPolicy Bypass -Command "New-Item -ItemType Directory -Force -Path '$(BUILD_DIR)' | Out-Null"

$(BUILD_DIR)/main.o: src/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup_stm32g431xx.o: $(STARTUP) | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) STM32G431RB_FLASH.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Test-Path '$(BUILD_DIR)') { Remove-Item -LiteralPath '$(BUILD_DIR)' -Recurse -Force }"

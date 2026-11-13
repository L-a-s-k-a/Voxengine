# ------------------------------------------------
# Generic Makefile (based on gcc)
# ------------------------------------------------
# List of supported controllers:
#   STM32F756ZG -- Target series -> STM32F756 | Target version -> ZG


######################################
# target
######################################
TARGET_SERIES = STM32F756
TARGET_VERSION = ZG

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og

#######################################
# paths
#######################################
# Build path
ifeq ($(TARGET_SERIES)$(TARGET_VERSION), STM32F756ZG)
	BUILD_DIR = build_F756ZG
endif

ifeq ($(OS),Windows_NT)
    # Для Windows заставляем make использовать cmd.exe
    MKDIR_CMD = cmd /c if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM_CMD    = cmd /c if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
else
    # Для Linux используем стандартные утилиты
    MKDIR_CMD = mkdir -p $(BUILD_DIR)
    RM_CMD    = rm -rf $(BUILD_DIR)
endif

######################################
# selecting source
######################################
# C sources
ifeq ($(TARGET_SERIES), STM32F756) 
	SYS = CMSIS/Devices/STM32F7xx/Src/system_stm32f7xx.c CMSIS/Devices/Src/syscalls.c CMSIS/Devices/Src/sysmem.c
	ASM = STM32F756ZGT6/startup_stm32f756xx.s
	
	CMSIS_INC_DEV = CMSIS/Devices/STM32F7xx/Inc
	CMSIS_INC_UNIT = CMSIS/Devices/STM32F7xx/Inc/STM32F756xx
	CMSIS_INC = CMSIS/Include
	
	ifeq ($(TARGET_SERIES)$(TARGET_VERSION), STM32F756ZG)
		LD = STM32F756ZGT6/STM32F756ZGTX_FLASH.ld
	endif
	
	DEF = STM32F756xx
	MCPU = cortex-m7
	MFPU = fpv5-sp-d16
	TRGT_CFG = stm32f7x
endif

######################################
# source
######################################
# C sources
C_SOURCES =  \
$(SYS) \
Core/src/main.c \
Core/src/init.c \
Core/src/it_handlers.c \
Core/src/app_data.c \

# ASM sources
ASM_SOURCES =  \
$(ASM)

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
	CC = $(GCC_PATH)/$(PREFIX)gcc
	AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
	CP = $(GCC_PATH)/$(PREFIX)objcopy
	SZ = $(GCC_PATH)/$(PREFIX)size
else
	CC = $(PREFIX)gcc
	AS = $(PREFIX)gcc -x assembler-with-cpp
	CP = $(PREFIX)objcopy
	SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=$(MCPU)

ifneq ($(MFPU), NONE)
	# fpu
	FPU = -mfpu=$(MFPU)
	# float-abi
	FLOAT-ABI = -mfloat-abi=hard
endif

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-D$(DEF) \

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-I$(CMSIS_INC_DEV) \
-I$(CMSIS_INC_UNIT) \
-I$(CMSIS_INC) \
-ICore/inc \

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
	CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = $(LD) 				 # "-Wl,--no-warn-rwx-segment" - отключает вывод предупреждения о том,
#								 # что .elf содержит сегмент LOAD с правами RWX (Read, Write, X)
#								 # Нужно разбораться с этой проблемой  
#								 #
# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -u _printf_float -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).elf $(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).hex $(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).bin 
	@echo "==========================================="
	@echo "|| The build was completed successfully! ||"
	@echo "==========================================="

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	

$(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).elf: $(OBJECTS) Makefile
	@echo "---------------------   SIZE   ----------------------"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@  -Wl,--print-memory-usage
	@echo "" 
	@$(SZ) $@
	@echo "-----------------------------------------------------"	

$(BUILD_DIR):
	@$(MKDIR_CMD)

#######################################
# clean up
#######################################
clean:
	@$(RM_CMD)

#######################################
# openocd
#######################################
flash: all
	openocd -f interface/stlink.cfg -f target/$(TRGT_CFG).cfg \
			-c "reset_config srst_only srst_nogate connect_assert_srst" \
			-c "program $(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).elf verify reset exit"
# 	openocd -f interface/stlink.cfg -f target/$(TRGT_CFG).cfg -c "program $(BUILD_DIR)/$(TARGET_SERIES)$(TARGET_VERSION).elf verify reset exit"
	@echo "=================================="
	@echo "|| The download was successful! ||"
	@echo "=================================="

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***

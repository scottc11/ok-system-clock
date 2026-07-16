
OS := $(shell uname)
$(info Operating System: $(OS))

ifeq ($(OS), Linux)
# Linux-specific rules or variable definitions
PGM_DEVICE = interface/stlink-v2.cfg
OCD_DIR = /usr/local/share/openocd/scripts # this might need to be adjusted for Linux
else ifeq ($(OS), Darwin)
    # macOS-specific rules or variable definitions
endif

######################################
# target
######################################
TARGET = firmware

FLASH_SIZE = $$((256 * 1024)) # 256 kB (Sector 6 and 7 used for config data)
RAM_SIZE = $$((128 * 1024)) # 128 kB

######################################
# building variables
######################################
# debug build?
DEBUG = 1

USB_DEBUG ?= 1

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build
LIB_PATH = ok-STM32F4


CPP_SOURCES += $(shell find firmware -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/DAC8554 -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/CD4051 -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/SNx4HC595 -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/IS31FL3246 -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/IS31FL3730 -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/DACx311 -name '*.cpp')
CPP_SOURCES += $(shell find $(LIB_PATH)/drivers/M24256 -name '*.cpp')

# C includes
C_INCLUDES += \
-Ifirmware/inc \
-I$(LIB_PATH)/drivers/DAC8554 \
-I$(LIB_PATH)/drivers/CD4051 \
-I$(LIB_PATH)/drivers/SNx4HC595 \
-I$(LIB_PATH)/drivers/IS31FL3246 \
-I$(LIB_PATH)/drivers/IS31FL3730 \
-I$(LIB_PATH)/drivers/DACx311 \
-I$(LIB_PATH)/drivers/M24256

include $(LIB_PATH)/Makefile

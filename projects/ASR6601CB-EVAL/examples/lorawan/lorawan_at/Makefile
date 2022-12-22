
PROJECT := $(notdir $(CURDIR))

$(PROJECT)_SOURCE := $(wildcard src/*.c)  \
    $(TREMO_SDK_PATH)/platform/system/printf-stdarg.c  \
    $(TREMO_SDK_PATH)/platform/system/system_cm4.c  \
    $(TREMO_SDK_PATH)/platform/system/startup_cm4.S \
    $(wildcard $(TREMO_SDK_PATH)/drivers/peripheral/src/*.c)  \
    $(wildcard $(TREMO_SDK_PATH)/lora/system/*.c)  \
    $(wildcard $(TREMO_SDK_PATH)/lora/system/crypto/*.c)  \
    $(wildcard $(TREMO_SDK_PATH)/lora/radio/sx126x/*.c)  \
    $(wildcard $(TREMO_SDK_PATH)/lora/driver/*.c) \
    $(wildcard $(TREMO_SDK_PATH)/lora/mac/*.c) \
    $(TREMO_SDK_PATH)/lora/mac/region/Region.c \
    $(TREMO_SDK_PATH)/lora/mac/region/RegionCommon.c \
    $(TREMO_SDK_PATH)/lora/linkwan/linkwan.c \
    $(TREMO_SDK_PATH)/lora/linkwan/linkwan_ica_at.c \
    $(TREMO_SDK_PATH)/lora/linkwan/lwan_config.c

$(PROJECT)_INC_PATH := inc \
    $(TREMO_SDK_PATH)/platform/CMSIS \
    $(TREMO_SDK_PATH)/platform/common \
    $(TREMO_SDK_PATH)/platform/system \
    $(TREMO_SDK_PATH)/drivers/crypto/inc \
    $(TREMO_SDK_PATH)/drivers/peripheral/inc \
    $(TREMO_SDK_PATH)/lora/driver \
    $(TREMO_SDK_PATH)/lora/system \
    $(TREMO_SDK_PATH)/lora/system/crypto \
    $(TREMO_SDK_PATH)/lora/radio \
    $(TREMO_SDK_PATH)/lora/radio/sx126x \
    $(TREMO_SDK_PATH)/lora/mac \
    $(TREMO_SDK_PATH)/lora/mac/region \
    $(TREMO_SDK_PATH)/lora/linkwan/inc \
    $(TREMO_SDK_PATH)/lora/linkwan/region

$(PROJECT)_CFLAGS  := -Wall -Os -ffunction-sections -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -fsingle-precision-constant -std=gnu99 -fno-builtin-printf -fno-builtin-sprintf -fno-builtin-snprintf
$(PROJECT)_ADDRESS := 0x08008000

#自己添加的根据 LoraWanRegionDefine 编译不同频段的LoRaWAN
ifeq ($(LoraWanRegionDefine),REGION_EU868)
	$(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionEU868.c
	$(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_EU868 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_EU433)
	$(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionEU433.c
	$(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_EU433 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_AU915)
	$(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionAU915.c
	$(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_AU915 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_US915)
	$(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionUS915.c
	$(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_US915 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_AS923)
        $(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionAS923.c
        $(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_AS923 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_CN779)
        $(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionCN779.c
        $(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_CN779 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_IN865)
        $(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionIN865.c
        $(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_IN865 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_KR920)
        $(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionKR920.c
        $(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_KR920 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else ifeq ($(LoraWanRegionDefine),REGION_US915-Hybrid)
        $(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionUS915-Hybrid.c
        $(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_US915-Hybrid -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
else
	$(PROJECT)_SOURCE += $(TREMO_SDK_PATH)/lora/mac/region/RegionCN470.c
	$(PROJECT)_DEFINES := -DCONFIG_DEBUG_UART=UART0 -DREGION_CN470 -DCONFIG_LWAN -DCONFIG_LWAN_AT -DCONFIG_LOG -DPRINT_BY_DMA
endif

$(PROJECT)_LDFLAGS := -Wl,--gc-sections -Wl,--wrap=printf -Wl,--wrap=sprintf -Wl,--wrap=snprintf

$(PROJECT)_LIBS := $(TREMO_SDK_PATH)/drivers/crypto/lib/libcrypto.a

$(PROJECT)_LINK_LD := cfg/gcc.ld

# please change the settings to download the app
#SERIAL_PORT        :=
#SERIAL_BAUDRATE    :=
#$(PROJECT)_ADDRESS :=

##################################################################################################
include $(TREMO_SDK_PATH)/build/make/common.mk




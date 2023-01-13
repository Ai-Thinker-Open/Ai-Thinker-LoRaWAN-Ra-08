ARM_GNU_ARCH_LIST := ARM968E-S

THUMB_GNU_ARCH_LIST := Cortex-M0 \
                       Cortex-M3 \
                       Cortex-M4 \
                       Cortex-M4F\
                       Cortex-M7\
                       Cortex-R3


ifneq ($(filter $(HOST_ARCH), $(THUMB_GNU_ARCH_LIST) $(ARM_GNU_ARCH_LIST)),)

ifneq ($(filter $(HOST_ARCH), $(ARM_GNU_ARCH_LIST)),)
HOST_INSTRUCTION_SET := ARM
else
HOST_INSTRUCTION_SET := THUMB
endif

TOOLCHAIN_PATH    ?=
TOOLCHAIN_PREFIX  := arm-none-eabi-
TOOLCHAIN_DEFAULT_FOLDER := gcc-arm-none-eabi

BINS ?=

CC      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)gcc$(EXECUTABLE_SUFFIX)"
CXX     := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)g++$(EXECUTABLE_SUFFIX)"
AS      := $(CC)
AR      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)ar$(EXECUTABLE_SUFFIX)"
LD      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)ld$(EXECUTABLE_SUFFIX)"
CPP     := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)cpp$(EXECUTABLE_SUFFIX)"
OBJDUMP := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objdump$(EXECUTABLE_SUFFIX)"
OBJCOPY := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)objcopy$(EXECUTABLE_SUFFIX)"
STRIP   := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)strip$(EXECUTABLE_SUFFIX)"
NM      := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)nm$(EXECUTABLE_SUFFIX)"
READELF := "$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)readelf$(EXECUTABLE_SUFFIX)"

ADD_COMPILER_SPECIFIC_STANDARD_CFLAGS   = $(1) -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -std=gnu99 $(if $(filter yes,$(MXCHIP_INTERNAL) $(TESTER)),-Werror)
ADD_COMPILER_SPECIFIC_STANDARD_CXXFLAGS = $(1) -Wall -Wfatal-errors -fsigned-char -ffunction-sections -fdata-sections -fno-common -fno-rtti -fno-exceptions  $(if $(filter yes,$(MXCHIP_INTERNAL) $(TESTER)),-Werror)
ADD_COMPILER_SPECIFIC_STANDARD_ADMFLAGS = $(1)
COMPILER_SPECIFIC_OPTIMIZED_CFLAGS    := -Os
COMPILER_SPECIFIC_UNOPTIMIZED_CFLAGS  := -O0
COMPILER_SPECIFIC_PEDANTIC_CFLAGS  := $(COMPILER_SPECIFIC_STANDARD_CFLAGS) -Werror -Wstrict-prototypes  -W -Wshadow  -Wwrite-strings -pedantic -std=c99 -U__STRICT_ANSI__ -Wconversion -Wextra -Wdeclaration-after-statement -Wconversion -Waddress -Wlogical-op -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wmissing-field-initializers -Wdouble-promotion -Wswitch-enum -Wswitch-default -Wuninitialized -Wunknown-pragmas -Wfloat-equal  -Wundef  -Wshadow # -Wcast-qual -Wtraditional -Wtraditional-conversion
COMPILER_SPECIFIC_ARFLAGS_CREATE   := -rc
COMPILER_SPECIFIC_ARFLAGS_ADD      := -rcs
COMPILER_SPECIFIC_ARFLAGS_VERBOSE  := -v

#debug: no optimize and log enable
COMPILER_SPECIFIC_DEBUG_CFLAGS     := -DDEBUG -ggdb $(COMPILER_SPECIFIC_UNOPTIMIZED_CFLAGS)
COMPILER_SPECIFIC_DEBUG_CXXFLAGS   := -DDEBUG -ggdb $(COMPILER_SPECIFIC_UNOPTIMIZED_CFLAGS)
COMPILER_SPECIFIC_DEBUG_ASFLAGS    := -DDEBUG=1 -ggdb
COMPILER_SPECIFIC_DEBUG_LDFLAGS    := -Wl,--gc-sections -Wl,--cref

#release_log: optimize but log enable
COMPILER_SPECIFIC_RELEASE_LOG_CFLAGS   := -gdwarf-2 -MD -Wall $(COMPILER_SPECIFIC_OPTIMIZED_CFLAGS) -mthumb-interwork
COMPILER_SPECIFIC_RELEASE_LOG_CXXFLAGS := -ggdb $(COMPILER_SPECIFIC_OPTIMIZED_CFLAGS)
COMPILER_SPECIFIC_RELEASE_LOG_ASFLAGS  := -ggdb
COMPILER_SPECIFIC_RELEASE_LOG_LDFLAGS  := -Wl,--gc-sections -Wl,$(COMPILER_SPECIFIC_OPTIMIZED_CFLAGS) -Wl,--cref

#release: optimize and log disable
COMPILER_SPECIFIC_RELEASE_CFLAGS   := -DNDEBUG -ggdb $(COMPILER_SPECIFIC_OPTIMIZED_CFLAGS)
COMPILER_SPECIFIC_RELEASE_CXXFLAGS := -DNDEBUG -ggdb $(COMPILER_SPECIFIC_OPTIMIZED_CFLAGS)
COMPILER_SPECIFIC_RELEASE_ASFLAGS  := -ggdb
COMPILER_SPECIFIC_RELEASE_LDFLAGS  := -Wl,--gc-sections -Wl,$(COMPILER_SPECIFIC_OPTIMIZED_CFLAGS) -Wl,--cref

COMPILER_SPECIFIC_DEPS_FLAG        := -MD
COMPILER_SPECIFIC_COMP_ONLY_FLAG   := -c
COMPILER_SPECIFIC_COMP_C99_FLGA    := -std=gnu99
COMPILER_SPECIFIC_LINK_MAP          =  -Wl,-Map=
COMPILER_SPECIFIC_LINK_FILES        =  -Wl,--whole-archive -Wl,--start-group $(1) -Wl,--end-group -Wl,-no-whole-archive
COMPILER_SPECIFIC_LINK_SCRIPT_DEFINE_OPTION = -Wl$(COMMA)-T
COMPILER_SPECIFIC_LINK_SCRIPT       =  $(addprefix -Wl$(COMMA)-T ,$(1))
LINKER                             := $(CC) -mcpu=cortex-m4 -mthumb -mthumb-interwork
LINKER_PLUS                        := -Wl,--wrap=printf -Wl,--wrap=sprintf -Wl,--wrap=snprintf
LINK_SCRIPT_SUFFIX                 := .ld
TOOLCHAIN_NAME := GCC
OPTIONS_IN_FILE_OPTION    := @

ENDIAN_CFLAGS_LITTLE   := -mlittle-endian
ENDIAN_CXXFLAGS_LITTLE := -mlittle-endian
ENDIAN_ASMFLAGS_LITTLE :=
ENDIAN_LDFLAGS_LITTLE  := -mlittle-endian
CLIB_LDFLAGS_NANO      := --specs=nano.specs

CLIB_LDFLAGS_NANO_FLOAT:= --specs=nano.specs

# Chip specific flags for GCC

ifeq ($(HOST_ARCH),Cortex-M4F)
CPU_CFLAGS     := -mcpu=cortex-m4 -mthumb
CPU_CXXFLAGS   := -mcpu=cortex-m4 -mthumb
CPU_ASMFLAGS   := $(CPU_CFLAGS)
CPU_LDFLAGS    := -mthumb -mcpu=cortex-m4 -Wl,-A,thumb
CLIB_LDFLAGS_NANO       += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
CLIB_LDFLAGS_NANO_FLOAT += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
endif

ifeq ($(HOST_ARCH),Cortex-M4)
CPU_CFLAGS     := -mcpu=cortex-m4 -mthumb
CPU_CXXFLAGS   := -mcpu=cortex-m4 -mthumb
CPU_ASMFLAGS   := $(CPU_CFLAGS)
CPU_LDFLAGS    := -mthumb -mcpu=cortex-m4 -Wl,-A,thumb
endif

ifeq ($(HOST_ARCH),Cortex-M3)
CPU_CFLAGS   := -mthumb -mcpu=cortex-m3
CPU_CXXFLAGS := -mthumb -mcpu=cortex-m3
CPU_ASMFLAGS   := $(CPU_CFLAGS)
CPU_LDFLAGS  := -mthumb -mcpu=cortex-m3 -Wl,-A,thumb
endif

ifeq ($(HOST_ARCH),Cortex-M0)
CPU_CFLAGS   := -mthumb -mcpu=cortex-m0
CPU_CXXFLAGS := -mthumb -mcpu=cortex-m0
CPU_ASMFLAGS   := $(CPU_CFLAGS)
CPU_LDFLAGS  := -mthumb -mcpu=cortex-m0 -Wl,-A,thumb
endif

ifeq ($(HOST_ARCH),Cortex-R4)
CPU_BASE_FLAGS     := -mcpu=cortex-r4 -mthumb-interwork
CPU_COMPILER_FLAGS := $(CPU_BASE_FLAGS) -mthumb -fno-builtin-memcpy
CPU_CFLAGS         := $(CPU_COMPILER_FLAGS)
CPU_CXXFLAGS       := $(CPU_COMPILER_FLAGS)
CPU_ASMFLAGS       := $(CPU_BASE_FLAGS)
CPU_LDFLAGS        := -mthumb $(CPU_BASE_FLAGS) -Wl,-A,thumb -Wl,-z,max-page-size=0x10 -Wl,-z,common-page-size=0x10
endif

ifeq ($(HOST_ARCH),ARM968E-S)
CPU_BASE_FLAGS     := -mcpu=arm968e-s -march=armv5te -marm -mlittle-endian -mthumb-interwork
CPU_CFLAGS         := $(CPU_BASE_FLAGS)
CPU_CXXFLAGS       := $(CPU_BASE_FLAGS)
CPU_ASMFLAGS       := $(CPU_BASE_FLAGS)
CPU_LDFLAGS        := $(CPU_BASE_FLAGS)
endif

ifeq ($(HOST_ARCH),Cortex-M7)
CPU_CFLAGS     := -mthumb -mcpu=cortex-m7
CPU_CXXFLAGS   := -mthumb -mcpu=cortex-m7
CPU_ASMFLAGS   := $(CPU_CFLAGS)
CPU_LDFLAGS    := -mthumb -mcpu=cortex-m7 -Wl,-A,thumb
endif


STRIP_OUTPUT_PREFIX := -o
OBJCOPY_BIN_FLAGS   := -O binary -R .eh_frame -R .init -R .fini -R .comment -R .ARM.attributes
OBJCOPY_HEX_FLAGS   := -O ihex -R .eh_frame -R .init -R .fini -R .comment -R .ARM.attributes

MAP_OUTPUT_SUFFIX :=.map
LINK_OUTPUT_SUFFIX :=.elf
BIN_OUTPUT_SUFFIX  :=.bin
HEX_OUTPUT_SUFFIX  :=.hex

endif #ifneq ($(filter $(HOST_ARCH), Cortex-M3 Cortex-M4),)


default: all

export MAKEFILES_PATH ?= $(TREMO_SDK_PATH)/build/make
export SCRIPTS_PATH ?= $(TREMO_SDK_PATH)/build/scripts
export OUT_DIR ?= out
export VIEW
export PYTHON = python
export HOST_ARCH := Cortex-M4F

ifeq ($(shell uname), Linux)
export EXECUTABLE_SUFFIX :=
else
export EXECUTABLE_SUFFIX := .exe
endif

ifneq ($(OUT_DIR),$(wildcard $(OUT_DIR)))
$(shell mkdir -p $(OUT_DIR))
endif

ifneq ($(VERBOSE),1)
VIEW:=@
SILENT:=-s
else
VIEW:=
SILENT:=
endif

include $(MAKEFILES_PATH)/toolchain_arm-none-eabi.mk
include $(MAKEFILES_PATH)/gen_3rd_project.mk

SIZE:="$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)size$(EXECUTABLE_SUFFIX)"

##################################################################################################
$(PROJECT)_INCLUDES := $(addprefix -I ,$($(PROJECT)_INC_PATH))

COMMON_CFLAGS := $(COMPILER_SPECIFIC_COMP_ONLY_FLAG) $(CPU_CFLAGS)

C_SOURCES := $(filter %.c,$($(PROJECT)_SOURCE))
CXX_SOURCES := $(filter %.cpp,$($(PROJECT)_SOURCE))
S_SOURCES := $(filter %.s %.S,$($(PROJECT)_SOURCE))

$(PROJECT)_C_OBJ := $(patsubst %.c,%.o,$(C_SOURCES))
$(PROJECT)_CXX_OBJ := $(patsubst %.cpp,%.o,$(CXX_SOURCES))
$(PROJECT)_S_OBJ := $(patsubst %.S,%.o,$(S_SOURCES:.s=.o))
$(PROJECT)_PACK_OBJ := $($(PROJECT)_C_OBJ) $($(PROJECT)_CXX_OBJ) $($(PROJECT)_S_OBJ)

define BUILD_C_PROCESS
$(OUT_DIR)/$(notdir $(2:.c=.o)): $(2)
	$(VIEW)echo Compiling $(notdir $(2))...
	$(VIEW)$(CC) $$(COMMON_CFLAGS) $$($(1)_INCLUDES) $$($(1)_DEFINES) $$($(1)_CFLAGS) $$< -o $$@
endef

define BUILD_CXX_PROCESS
$(OUT_DIR)/$(notdir $(2:.cpp=.o)): $(2)
	$(VIEW)echo Compiling $(notdir $(2))...
	$(VIEW)$(CXX) $$(COMMON_CFLAGS) $$($(1)_INCLUDES) $$($(1)_DEFINES) $$($(1)_CXXFLAGS) $$< -o $$@
endef

define BUILD_ASM_PROCESS
$(OUT_DIR)/$(notdir $(patsubst %.S,%.o,$(1:.s=.o))): $(1)
	$(VIEW)$(CC) $$(COMMON_CFLAGS) $$< -o $$@
endef

$(foreach src,$(S_SOURCES),$(eval $(call BUILD_ASM_PROCESS,$(src))))
$(foreach src,$(C_SOURCES),$(eval $(call BUILD_C_PROCESS,$(PROJECT),$(src))))
$(foreach src,$(CXX_SOURCES),$(eval $(call BUILD_CXX_PROCESS,$(PROJECT),$(src))))

# flash settings
TREMO_LOADER := $(SCRIPTS_PATH)/tremo_loader.py
SERIAL_PORT        ?= /dev/ttyUSB0
SERIAL_BAUDRATE    ?= 921600
$(PROJECT)_ADDRESS ?= 0x08000000

##################################################################################################
ifeq ($(IDE),keil)
all: keil_project
	$(VIEW)echo generate keil project done
else
all: $(OUT_DIR)/$(PROJECT)$(BIN_OUTPUT_SUFFIX)
	$(VIEW)echo Build completed.
	$(SIZE) $(OUT_DIR)/$(PROJECT)$(LINK_OUTPUT_SUFFIX)
	$(VIEW)echo "Please run 'make flash' or the following command to download the app"
	@echo $(PYTHON) $(TREMO_LOADER) -p $(SERIAL_PORT) -b $(SERIAL_BAUDRATE) flash $($(PROJECT)_ADDRESS) $(OUT_DIR)/$(PROJECT)$(BIN_OUTPUT_SUFFIX)
endif    

$(OUT_DIR)/$(PROJECT)$(BIN_OUTPUT_SUFFIX): $(OUT_DIR)/$(PROJECT)$(LINK_OUTPUT_SUFFIX)
	$(VIEW)echo $@
	$(VIEW)$(OBJCOPY) $(OBJCOPY_BIN_FLAGS) $< $@
	$(VIEW)$(OBJCOPY) $(OBJCOPY_HEX_FLAGS) $< $(OUT_DIR)/$(PROJECT)$(HEX_OUTPUT_SUFFIX)

$(OUT_DIR)/$(PROJECT)$(LINK_OUTPUT_SUFFIX): $(addprefix $(OUT_DIR)/,$(notdir $($(PROJECT)_PACK_OBJ)))
	$(VIEW)echo $@
	$(VIEW)$(LINKER) $($(PROJECT)_LDFLAGS) $(COMPILER_SPECIFIC_LINK_MAP)$(OUT_DIR)/$(PROJECT)$(MAP_OUTPUT_SUFFIX) -T$($(PROJECT)_LINK_LD) -o $@ $(addprefix $(OUT_DIR)/,$(notdir $($(PROJECT)_PACK_OBJ))) $($(PROJECT)_LIBS)

flash: $(OUT_DIR)/$(PROJECT)$(BIN_OUTPUT_SUFFIX) $(TREMO_LOADER)
	$(VIEW)echo Start flashing...
	$(VIEW)$(PYTHON) $(TREMO_LOADER) -p $(SERIAL_PORT) -b $(SERIAL_BAUDRATE) flash $($(PROJECT)_ADDRESS) $(OUT_DIR)/$(PROJECT)$(BIN_OUTPUT_SUFFIX)
	
clean:
	$(VIEW)echo Cleaning...
	$(VIEW)rm -rf $(OUT_DIR)
	$(VIEW)echo Done


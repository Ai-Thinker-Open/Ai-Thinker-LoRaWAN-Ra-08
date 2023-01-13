comma:=,
semi:=;
keil_config := $(OUT_DIR)/keil_config.ini
keil_gen_script := $(SCRIPTS_PATH)/keil.py
FILE_CREATE = $(file >$(1),$(2))
FILE_APPEND = $(file >>$(1),$(2))

PATH_PREFIX := $(shell realpath $(TREMO_SDK_PATH) --relative-to=.)/
SRC_PATH := $(foreach src, $($(PROJECT)_SOURCE),'$(addprefix $(PATH_PREFIX),$(shell realpath $(src) --relative-to=$(TREMO_SDK_PATH)))'$(comma))
LIB_PATH := $(foreach lib, $($(PROJECT)_LIBS),'$(addprefix $(PATH_PREFIX),$(shell realpath $(lib) --relative-to=$(TREMO_SDK_PATH)))'$(comma))
INC_PATH := $(foreach inc, $($(PROJECT)_INC_PATH),$(addprefix $(PATH_PREFIX),$(shell realpath $(inc) --relative-to=$(TREMO_SDK_PATH)))$(semi))
DEFINES := $(foreach dflags, $($(PROJECT)_DEFINES), $(dflags)$(comma))
ASMDEFINES := $(foreach adflags,$($(PROJECT)_AFLAGS),$(adflags)$(comma))
C_MiscControls := $($(PROJECT)_CFLAGS)
A_MissControls := $(AMISC)

ifeq ($($(PROJECT)_PRO_OUT),LIB)
OUTPUT_TARGET := $(subst lib,,$($(PROJECT)_LIB))
else
OUTPUT_TARGET := project
endif

ifeq ($(IDE),keil)
CPU_TYPE := Cortex-M4

ifneq ($($(PROJECT)_LINK_LD),)
LD_FILES := ./cfg/gcc.ld
endif

INI_FILES := ./cfg/ram.ini

LD_MISC := $($(PROJECT)_LDFLAGS)

ifneq ($(LINKER_MISC_ROM_ELF_PATH),)
LD_MISC += $(PATH_PREFIX)$(LINKER_MISC_ROM_ELF_PATH)
endif

ifeq ($($(PROJECT)_LIB),)
RUNUSER1 :=1
else
RUNUSER1 :=0
endif
RUNUSER2 :=0
RUNUSER1_PRO :=./utils/genbinary.bat
RUNUSER2_PRO :=

ifeq ($(findstring LIB,$($(PROJECT)_PRO_OUT)),LIB)
CREATELIB := 1
CREATEEXE := 0
else
CREATEEXE := 1
CREATELIB := 0
endif
endif


keil_project:
ifeq ($(IDE),keil)
	$(call FILE_CREATE,$(keil_config),[settings])
	$(call FILE_APPEND,$(keil_config),src=$(SRC_PATH))
	$(call FILE_APPEND,$(keil_config),lib=$(LIB_PATH))
	$(call FILE_APPEND,$(keil_config),include_path='$(INC_PATH)')
	$(call FILE_APPEND,$(keil_config),defines='$(DEFINES)')
	$(call FILE_APPEND,$(keil_config),adefines='$(ASMDEFINES)')
	$(call FILE_APPEND,$(keil_config),cMisc='$(C_MiscControls)')
	$(call FILE_APPEND,$(keil_config),aMisc='$(A_MissControls)')
	$(call FILE_APPEND,$(keil_config),host_arch='$(CPU_TYPE)')
	$(call FILE_APPEND,$(keil_config),ld_files='$(LD_FILES)')
	$(call FILE_APPEND,$(keil_config),ini_files='$(INI_FILES)')
	$(call FILE_APPEND,$(keil_config),ld_misc='$(LD_MISC)')
	$(call FILE_APPEND,$(keil_config),runuser1='$(RUNUSER1)')
	$(call FILE_APPEND,$(keil_config),runuser2='$(RUNUSER2)')
	$(call FILE_APPEND,$(keil_config),runuser1_pro='$(RUNUSER1_PRO)')
	$(call FILE_APPEND,$(keil_config),runuser2_pro='$(RUNUSER2_PRO)')
	$(call FILE_APPEND,$(keil_config),createLib='$(CREATELIB)')
	$(call FILE_APPEND,$(keil_config),createexe='$(CREATEEXE)')
	$(VIEW)$(PYTHON) $(keil_gen_script) $(TREMO_SDK_PATH) $(keil_config) project $(OUTPUT_TARGET)
else
	$(VIEW)echo not support IDE currently
endif

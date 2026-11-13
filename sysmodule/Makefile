#---------------------------------------------------------------------------------
# Sysmodule Makefile for boxart-nx (Atmosphere MitM)
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))

ATMOSPHERE_DIR    ?= /home/nlacarre/thirdparty/atmosphere
ifneq ($(wildcard $(CURRENT_DIRECTORY)/lib/ams/libraries),)
    LIBAMS        ?= $(CURRENT_DIRECTORY)/lib/ams/libraries
else
    LIBAMS        ?= $(ATMOSPHERE_DIR)/libraries
endif

PORTLIBS          ?= /home/nlacarre/thirdparty/portlibs/switch/opt/devkitpro/portlibs/switch
export ATMOSPHERE_LIBDIRS := $(PORTLIBS)

include $(LIBAMS)/config/templates/stratosphere.mk

export SETTINGS += -Wno-error=maybe-uninitialized -Wno-maybe-uninitialized
export LDFLAGS  += -Wno-error=maybe-uninitialized -Wno-maybe-uninitialized

VERSION           := 1.0.0
TARGET            := boxart-nx
BUILD             := build
OUTDIR            := out
RESOURCES         := res
SOURCES           := src
INCLUDES          := src $(LIBAMS)/libstratosphere/include $(LIBAMS)/libvapours/include
DEFINES           += -DTARGET="\"$(TARGET)\"" -DVERSION="\"$(VERSION)\""

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT     := $(CURDIR)/$(OUTDIR)/$(TARGET)
export TOPDIR     := $(CURDIR)
export VPATH      := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR    := $(CURDIR)/$(BUILD)
export APP_JSON   := $(TOPDIR)/$(RESOURCES)/sysmodule.json

CFILES            := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES            := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD         := $(CXX)
export OFILES     := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE    := $(foreach dir,$(INCLUDES),$(if $(filter /%,$(dir)),-I$(dir),-I$(TOPDIR)/$(dir))) \
                     $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                     -I$(CURDIR)

export LIBPATHS   := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LIBPATHS   += $(foreach dir,$(AMS_LIBDIRS),-L$(dir)/lib/nintendo_nx_arm64_armv8a/release)

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@[ -d $(OUTDIR) ] || mkdir -p $(OUTDIR)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@mkdir -p ../out/atmosphere/contents/010000000000BD00/flags/
	@touch ../out/atmosphere/contents/010000000000BD00/flags/boot2.flag
	@cp $(OUTDIR)/$(TARGET).nsp ../out/atmosphere/contents/010000000000BD00/exefs.nsp
	@echo "{" > ../out/atmosphere/contents/010000000000BD00/toolbox.json
	@echo "    \"name\": \"$(TARGET)\"," >> ../out/atmosphere/contents/010000000000BD00/toolbox.json
	@echo "    \"tid\": \"010000000000BD00\"," >> ../out/atmosphere/contents/010000000000BD00/toolbox.json
	@echo "    \"requires_reboot\": true," >> ../out/atmosphere/contents/010000000000BD00/toolbox.json
	@echo "    \"sysmodule\": true," >> ../out/atmosphere/contents/010000000000BD00/toolbox.json
	@echo "    \"version\": \"$(VERSION)\"" >> ../out/atmosphere/contents/010000000000BD00/toolbox.json
	@echo "}" >> ../out/atmosphere/contents/010000000000BD00/toolbox.json

clean:
	@echo "Cleaning sysmodule..."
	@rm -rf $(BUILD) $(OUTDIR)

else

.PHONY: all
DEPENDS := $(OFILES:.o=.d)

all: $(OUTPUT).nsp

$(OUTPUT).elf: $(OFILES)
$(OUTPUT).nsp: $(OUTPUT).nso $(OUTPUT).npdm

-include $(DEPENDS)

endif

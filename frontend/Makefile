#---------------------------------------------------------------------------------
# Frontend Makefile for boxart-nx (CMake / Borealis)
#---------------------------------------------------------------------------------
export DEVKITPRO ?= /opt/devkitpro
export DEVKITA64 ?= $(DEVKITPRO)/devkitA64

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

BUILD_DIR ?= build
OUT_DIR   ?= ../out/switch/boxart-nx

.PHONY: all clean

all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_TOOLCHAIN_FILE=$(DEVKITPRO)/cmake/Switch.cmake \
		-DPLATFORM_SWITCH=ON -DUSE_DEKO3D=OFF -DUSE_SDL2=ON ..
	@cmake --build $(BUILD_DIR) --target boxart-nx.nro -j$(shell nproc)
	@mkdir -p $(OUT_DIR)
	@cp $(BUILD_DIR)/boxart-nx.nro $(OUT_DIR)/boxart-nx.nro
	@echo "Frontend built successfully: $(OUT_DIR)/boxart-nx.nro"

clean:
	@echo "Cleaning frontend..."
	@rm -rf $(BUILD_DIR)

#---------------------------------------------------------------------------------
# Root Makefile for boxart-nx
# Builds both sysmodule and frontend, and prepares out/ directory structure.
#---------------------------------------------------------------------------------

export DEVKITPRO ?= /opt/devkitpro
export DEVKITA64 ?= $(DEVKITPRO)/devkitA64

.PHONY: all sysmodule frontend dist clean distclean

all: sysmodule frontend

dist: all
	@echo "=========================================="
	@echo "Packaging release zip (boxart-nx.zip)..."
	@echo "=========================================="
	@cd out && zip -FSr ../boxart-nx.zip atmosphere switch
	@echo "Done: boxart-nx.zip"

sysmodule:
	@echo "=========================================="
	@echo "Building Sysmodule (libstratosphere MitM)..."
	@echo "=========================================="
	@$(MAKE) -C sysmodule

frontend:
	@echo "=========================================="
	@echo "Building Frontend (.nro)..."
	@echo "=========================================="
	@$(MAKE) -C frontend

clean:
	@echo "Cleaning all components..."
	@$(MAKE) -C sysmodule clean
	@$(MAKE) -C frontend clean
	@rm -rf out/atmosphere out/switch

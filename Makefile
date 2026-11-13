#---------------------------------------------------------------------------------
# Root Makefile for boxart-nx
# Builds both sysmodule and frontend, and prepares out/ directory structure.
#---------------------------------------------------------------------------------

.PHONY: all sysmodule frontend clean distclean

all: sysmodule frontend

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

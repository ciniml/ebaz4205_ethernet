.PHONY: all clean restore device-tree open

PROJECT_NAME := ethernet_loopback
BD_NAME := design_1
BITSTREAM := $(PROJECT_NAME).runs/impl_1/$(BD_NAME)_wrapper.bit
BITSTREAM_BIN := $(BD_NAME)_wrapper.bit.bin
SDK_DIR := $(PROJECT_NAME).sdk
HARDWARE_DEF := $(SDK_DIR)/$(BD_NAME)_wrapper.xsa

BASH := /bin/bash
SCRIPTS_DIR := ../../scripts
VIVADO ?= vivado
HSI ?= hsi
BOOTGEN ?= bootgen

all: $(HARDWARE_DEF)

restore: $(PROJECT_NAME).xpr

clean:
	-@rm -rf $(PROJECT_NAME).* .Xil NA
	-@rm *.jou *.str *.btree *.log

open: $(PROJECT_NAME).xpr
	$(VIVADO) $<&

$(PROJECT_NAME).xpr:
	$(VIVADO) -mode batch -source restore_project.tcl -tclargs $(PROJECT_NAME)

$(BITSTREAM) $(HARDWARE_DEF): $(PROJECT_NAME).xpr $(SRCS) $(PROJECT_NAME).srcs/sources_1/bd/$(BD_NAME)/$(BD_NAME).bd
	$(VIVADO) -mode batch -source implement.tcl -tclargs $(PROJECT_NAME) $(BD_NAME)

NAME = nono
BINARY = bin/$(NAME).bin
DESCRIPTION = "Nonogram game by Robogeek"
FAE_HOME = ~/agon/fab
FAE_ARGS = --scale integer --renderer hw

SDCARD=~/agon/sdcard_sync
SD_INSTALL_DIR=$(SDCARD)/nono
EMU_INSTALL_DIR=$(FAE_HOME)/sdcard/nono

# ----------------------------
include $(shell agondev-config --makefile)

.PHONY: emu

install: $(BINARY)
	@echo Install to $(SD_INSTALL_DIR)
	@mkdir -p $(SD_INSTALL_DIR)
	@cp $(BINARY) $(SD_INSTALL_DIR)
	@mkdir -p $(SD_INSTALL_DIR)/data
	@rm -f $(SD_INSTALL_DIR)/data/*
	@cp -rf data/* $(SD_INSTALL_DIR)/data/
	@mkdir -p $(SD_INSTALL_DIR)/user

emu: $(BINARY)
	@echo Install to $(EMU_INSTALL_DIR)
	@mkdir -p $(EMU_INSTALL_DIR)
	@cp $(BINARY) $(EMU_INSTALL_DIR)
	@cp -rf data $(EMU_INSTALL_DIR)
	@cp -rf user $(EMU_INSTALL_DIR)

send: nono.bin
	acmd cd /nono
	asend nono.bin

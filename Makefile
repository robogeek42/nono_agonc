APPS = nono nonoed
BINARIES = $(patsubst %,%.bin,$(APPS))
MYLIBS = common
FAE_HOME = ~/agon/fab
FAE_ARGS = --scale integer --renderer hw

SDCARD=~/agon/sdcard_sync
SD_INSTALL_DIR=$(SDCARD)/nono

all: $(APPS)
$(APPS): $(BINARIES)

%.bin: $(MYLIBS) %/src/main.c
	@echo '|'===========================
	@echo '|' $*
	@echo '|'---------------------------
	@echo "|Copy librares to $*"
	@mkdir -p $*/lib
# I don't know how to loop over a variable in a makefile :L
# But I am just assuming it is one LIB
	@cp $(MYLIBS)/bin/lib$(MYLIBS).a $*/lib
	@echo "Copy includes"
	@mkdir -p $*/include
	@cp -f include/* $*/include/
#
	@echo '|'---------------------------
	@echo "|Make App $*"
	$(MAKE) -C $* clean
	$(MAKE) -C $* install
	@echo '|'===========================
	@echo

$(MYLIBS): 
	@echo '|'===========================
	@echo "|Copy includes to $@"
	@mkdir -p $@/include
	@cp -f include/* $@/include/
	@echo '|'---------------------------
	@echo "|Make Library $@"
	cd $@ ; make clean
	cd $@ ; make lib
	@echo '|'===========================
	@echo
	

install: $(BINARIES)
	@echo Install to $(SD_INSTALL_DIR)
	@mkdir -p $(SD_INSTALL_DIR)
	@cp $(BINARIES) $(SD_INSTALL_DIR)
	@mkdir -p $(SD_INSTALL_DIR)/data
	@rm -f $(SD_INSTALL_DIR)/data/*
	@cp -rf data/* $(SD_INSTALL_DIR)/data/
	
clean:
	@rm -f $(BINARIES)
	@cd nono; make clean
	@cd nono; rm -rf lib include
	@cd nonoed; make clean
	@cd nonoed; rm -rf lib include
	@cd common; rm -rf include

.PHONY: all clean $(MYLIBS)


emu: $(BINARIES)
	cp $(BINARIES) $(FAE_HOME)/sdcard/
	cp -rf data $(FAE_HOME)/sdcard/
	cd $(FAE_HOME) ; $(FAE_HOME)/fab-agon-emulator $(FAE_ARGS)

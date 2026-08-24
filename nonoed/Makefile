# ----------------------------
# Makefile Options
# ----------------------------

NAME = nonoed
DESCRIPTION = "Nonogram Editor by Robogeek"
LIBS = "-lcommon"

# Select option for Argument Processing at 'int main( int argc, char* argv[] )'
# 0: Simple Command Line Processing
# 1: Complex Command Line Processing - for Redirection & Quoting
LDHAS_ARG_PROCESSING = 0
LDHAS_EXIT_HANDLER = 0

FAE_HOME = ~/agon/fab
FAE_ARGS = --scale integer --renderer hw

# ----------------------------
#
include $(shell agondev-config --makefile)

install: bin/$(NAME).bin
	cp $< ..


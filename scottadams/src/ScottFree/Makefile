#
#	Makefile for the thing
#

# ARCH: Which Cosmopolitan build do you want - x86_64 or ARM64?
#       `cosmocc` can build an APE, but for ncurses it is more reliable to do two builds and `apelink` them later
# CC: CosmoScott auto-switches to an arch-appropriate compiler; `CC` is legacy for the original makefile recipes
# COSMOS:  root path where external libs and includes are stored (specifically `ncurses-related` in this case)
# COSMOCC: root path where the `cosmocc` folder lives
# BUILD: where should the final build be saved?

CC	=	gcc

export BASELOC:=$(shell pwd)
export ARCH:=x86_64
export COSMOS:=/home/christopherdrum/_Repositories/superconfigure/cosmos
export COSMOCC:=/home/christopherdrum/_Repositories/superconfigure/cosmopolitan/cosmocc
export BUILD:=$(BASELOC)/build
export GAMES:=$(BASELOC)/../../games
export CFLAGS:=-std=c99 -o

all	:	ScottCurses

ScottCurses.o:	ScottCurses.c Scott.h

ScottCurses:	ScottCurses.o
	$(CC) ScottCurses.o -o ScottCurses -lcurses -ltermcap

CosmoScott:
	mkdir -p $(BUILD)

#   Build the x86_64 version
	$(COSMOCC)/bin/x86_64-unknown-cosmo-cc $(CFLAGS) $(BUILD)/CosmoScott.x86_64 ScottCurses.c \
		-I$(COSMOS)/x86_64/include/ncurses \
		-I$(COSMOS)/x86_64/include \
		-L$(COSMOS)/x86_64/lib \
		-lncurses -ltinfo \
		-DNCURSES_STATIC -D_DEFAULT_SOURCE

#   Build the ARM64 version
	$(COSMOCC)/bin/aarch64-unknown-cosmo-cc $(CFLAGS) $(BUILD)/CosmoScott.aarch64 ScottCurses.c \
		-I$(COSMOS)/aarch64/include/ncurses \
		-I$(COSMOS)/aarch64/include \
		-L$(COSMOS)/aarch64/lib \
		-lncurses -ltinfo \
		-DNCURSES_STATIC -D_DEFAULT_SOURCE

#   Put them together into a single APE file
	$(COSMOCC)/bin/apelink -o $(BUILD)/CosmoScott.zip \
		-l$(COSMOCC)/bin/ape-aarch64.elf \
		-l$(COSMOCC)/bin/ape-x86_64.elf \
		-M$(COSMOCC)/bin/ape-m1.c \
		$(BUILD)/CosmoScott.aarch64 $(BUILD)/CosmoScott.x86_64

#   Embed the game library into the APE; source has been modified to display a game chooser menu at launch
	cd $(dir $(GAMES)) && zip -r $(BUILD)/CosmoScott.zip $(notdir $(GAMES))
	mv $(BUILD)/CosmoScott.zip $(BUILD)/CosmoScott

CosmoClean:
	rm -Rf $(BUILD)
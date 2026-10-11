# Firmware build and flash via arduino-cli + ATTinyCore (no Arduino IDE).
#
#   make deps    -> install/update the ATTinyCore board package
#   make         -> compile, binaries land in build/
#   make fuses   -> burn fuses for 8 MHz internal RC, BOD 4.3 V (once per chip)
#   make flash   -> upload build/tinyfan.ino.hex with an ISP programmer
#
# Programmer defaults to USBasp; override with e.g.
#   make flash PROGRAMMER=arduinoasisp PORT=/dev/ttyUSB0
# `arduino-cli board details -b ATTinyCore:avr:attinyx5 --list-programmers` lists the ids.

SKETCH     := tinyfan
BUILD_DIR  := build
ARDUINO    ?= arduino-cli
PROGRAMMER ?= usbasp
PORT       ?=

# ATtiny85, 8 MHz internal RC, brown-out reset at 4.3 V (we run from 5 V),
# EEPROM retained, millis() enabled (delay() needs it).
FQBN := ATTinyCore:avr:attinyx5:chip=85,clock=8internal,bod=4v3,eesave=aenable,millis=enabled

ATTINYCORE_URL := http://drazzy.com/package_drazzy.com_index.json

PORT_ARG := $(if $(PORT),-p $(PORT),)

.PHONY: all build size fuses flash deps clean

all: build

build:
	$(ARDUINO) compile --fqbn "$(FQBN)" --warnings all \
		--output-dir $(BUILD_DIR) $(SKETCH)

size: build
	@echo "hex: $(BUILD_DIR)/$(SKETCH).ino.hex"

fuses:
	$(ARDUINO) burn-bootloader --fqbn "$(FQBN)" -P $(PROGRAMMER) $(PORT_ARG) --verbose

flash: build
	$(ARDUINO) upload --fqbn "$(FQBN)" -P $(PROGRAMMER) $(PORT_ARG) \
		--input-dir $(BUILD_DIR) $(SKETCH)

deps:
	$(ARDUINO) core update-index --additional-urls $(ATTINYCORE_URL)
	$(ARDUINO) core install ATTinyCore:avr --additional-urls $(ATTINYCORE_URL)

clean:
	rm -rf $(BUILD_DIR)

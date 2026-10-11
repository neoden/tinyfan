# Fabrication outputs for psu_controller, driven by kicad-cli (KiCad 10).
#
#   make            -> erc + drc + gerbers + bom + pos
#   make gerbers    -> gerbers/ (Gerber X2, Protel extensions) + Excellon drill
#   make bom        -> production/bom.csv (JLCPCB columns, grouped by value)
#   make pos        -> production/psu_controller-all-pos.csv (JLCPCB CPL header)
#   make zip        -> build/psu_controller-gerbers.zip for upload to the fab
#   make pdf        -> build/psu_controller-schematic.pdf
#   make firmware   -> build the ATtiny85 firmware (see firmware/Makefile)
#   make clean      -> remove build/ (hardware and firmware)

PROJECT   := psu_controller
SCH       := $(PROJECT).kicad_sch
PCB       := $(PROJECT).kicad_pcb
KICAD_CLI ?= kicad-cli

GERBER_DIR := gerbers
PROD_DIR   := production
BUILD_DIR  := build

# Same layer set as used in pcbnew's Plot dialog for this board.
LAYERS := F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts

BOM_FIELDS := Reference,Value,Footprint,LCSC Part \#
BOM_LABELS := Designator,Comment,Footprint,LCSC Part \#

.PHONY: all check erc drc gerbers drill bom pos zip pdf firmware clean

all: check gerbers bom pos

check: erc drc

$(BUILD_DIR):
	mkdir -p $@

erc: $(BUILD_DIR)
	$(KICAD_CLI) sch erc --output $(BUILD_DIR)/erc.rpt --severity-all --exit-code-violations $(SCH)

# Reports everything; only errors fail the build (silkscreen warnings are tolerated).
drc: $(BUILD_DIR)
	$(KICAD_CLI) pcb drc --output $(BUILD_DIR)/drc.rpt --severity-all $(PCB)
	@grep -E '\*\* Found' $(BUILD_DIR)/drc.rpt
	$(KICAD_CLI) pcb drc --output $(BUILD_DIR)/drc-errors.rpt --severity-error --exit-code-violations $(PCB)

# --no-netlist matches "advanced attributes off" in the Plot dialog. Output is
# identical to the GUI export apart from aperture numbering order.
gerbers: drill
	$(KICAD_CLI) pcb export gerbers --output $(GERBER_DIR)/ --layers $(LAYERS) \
		--subtract-soldermask --no-netlist $(PCB)

drill:
	mkdir -p $(GERBER_DIR)
	$(KICAD_CLI) pcb export drill --output $(GERBER_DIR)/ --format excellon \
		--excellon-units mm --excellon-zeros-format decimal --drill-origin absolute $(PCB)

# Pin headers J1-J3 are hand-soldered and are dropped from the assembly BOM.
bom: $(BUILD_DIR)
	mkdir -p $(PROD_DIR)
	$(KICAD_CLI) sch export bom --output $(BUILD_DIR)/bom-full.csv \
		--fields "$(BOM_FIELDS)" --labels "$(BOM_LABELS)" \
		--group-by Value --ref-range-delimiter "" --exclude-dnp $(SCH)
	grep -vE '^"J[0-9]' $(BUILD_DIR)/bom-full.csv > $(PROD_DIR)/bom.csv

pos:
	mkdir -p $(PROD_DIR)
	$(KICAD_CLI) pcb export pos --output $(PROD_DIR)/$(PROJECT)-all-pos.csv \
		--format csv --units mm --side both --smd-only $(PCB)
	sed -i '1s/.*/Designator,Val,Package,Mid X,Mid Y,Rotation,Layer/' $(PROD_DIR)/$(PROJECT)-all-pos.csv
	sed -i 's/,top$$/,Top/; s/,bottom$$/,Bottom/' $(PROD_DIR)/$(PROJECT)-all-pos.csv

zip: gerbers $(BUILD_DIR)
	rm -f $(BUILD_DIR)/$(PROJECT)-gerbers.zip
	zip -j $(BUILD_DIR)/$(PROJECT)-gerbers.zip $(GERBER_DIR)/*

pdf: $(BUILD_DIR)
	$(KICAD_CLI) sch export pdf --output $(BUILD_DIR)/$(PROJECT)-schematic.pdf $(SCH)

firmware:
	$(MAKE) -C firmware

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C firmware clean

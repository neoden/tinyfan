# psu_controller

Temperature-controlled fan driver for a PC power supply. An ATtiny85 reads an
NTC thermistor and drives a standard 4-pin PWM fan from the PSU's own +5 V and
+12 V rails. Two-layer 26.55 × 18.05 mm board, all SMD parts on the top side,
designed in KiCad 10.

## How it works

| Block | Parts | Notes |
|---|---|---|
| MCU | U1 ATtiny85-20S (SOIC-8), C1 100 nF, R1 10 kΩ pull-up on RESET | Runs on +5 V |
| Temperature input | J3 (1×2), R2 10 kΩ, C2 100 nF | NTC between J3.1 and GND forms a divider with R2 to +5 V, filtered by C2, read on PB3/ADC3 |
| Fan PWM output | R3 1 kΩ, Q1 MMBT3904 | PB1 drives Q1 as an open-collector stage on the fan's PWM pin (J2.4) |
| Fan tach input | — | Fan tach (J2.3) goes to PB0 |
| Power | J1 (1×3) | +5 V, GND, +12 V from the PSU |

Connectors:

| Ref | Pins | Pinout |
|---|---|---|
| J1 | 1×3 | 1 = +5 V, 2 = GND, 3 = +12 V |
| J2 | 1×4 | 1 = GND, 2 = +12 V, 3 = TACH → PB0, 4 = PWM ← Q1 collector (standard 4-pin fan order) |
| J3 | 1×2 | 1 = NTC / ADC3, 2 = GND |

PB2 and PB4 are unused. The firmware is not part of this repository.

## Layout

```
psu_controller.kicad_pro / .kicad_sch / .kicad_pcb   KiCad 10 project
gerbers/      Gerber X2 (Protel extensions) + Excellon drill, as sent to the fab
production/   bom.csv (JLCPCB columns), bom_pcbway.csv (hand-edited, with MPNs),
              psu_controller-all-pos.csv (JLCPCB CPL format, SMD parts only)
lib/          digispark_attiny85.kicad_sym — early Digispark symbol, not used by
              the current schematic (it uses the stock MCU_Microchip_ATtiny lib)
Makefile      regenerates everything above with kicad-cli
```

## Regenerating outputs

Requires `kicad-cli` from KiCad 10 on `PATH` (override with `KICAD_CLI=...`).

```sh
make            # ERC + DRC, gerbers, drill, BOM, pos file
make zip        # build/psu_controller-gerbers.zip for the fab
make pdf        # build/psu_controller-schematic.pdf
```

ERC is clean. DRC reports 15 `silk_edge_clearance` warnings (J1 and J2
silkscreen outlines cross the board edge); `make drc` prints them but only
errors fail the build.

## BOM

| Ref | Value | Package | LCSC |
|---|---|---|---|
| C1, C2 | 100 nF | 0805 | C49678 |
| Q1 | MMBT3904 | SOT-23 | C20526 |
| R1, R2 | 10 kΩ | 0805 | C17414 |
| R3 | 1 kΩ | 0805 | C17513 |
| U1 | ATtiny85-20S | SOIC-8 | C89852 |
| J1–J3 | 2.54 mm right-angle pin headers | 1×3, 1×4, 1×2 | — |

## Working on it with Claude Code

The project is set up for the [`kicad` plugin](https://github.com/ProductOfAmerica/mcp-server-kicad)
(file-based KiCad MCP server, no running KiCad needed):

```sh
claude plugin marketplace add ProductOfAmerica/mcp-server-kicad
claude plugin install kicad
```

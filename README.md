# tinyfan

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

PB2 and PB4 are unused.

## Firmware

`firmware/tinyfan/tinyfan.ino` is an Arduino sketch for
[ATTinyCore](https://github.com/SpenceKonde/ATTinyCore), built with
`arduino-cli` (no IDE needed):

```sh
cd firmware
make deps                 # once: install the ATTinyCore board package
make                      # compile -> build/tinyfan.ino.hex
make fuses PROGRAMMER=usbasp   # once per chip: 8 MHz internal RC, BOD 4.3 V
make flash PROGRAMMER=usbasp   # upload over ISP
```

What it does, once a second:

1. Reads the NTC on ADC3 (16-sample average), converts with the Beta equation
   (10 kΩ @ 25 °C, B = 3435). Open or shorted sensor forces 100 %.
2. PI controller around a 55 °C setpoint: duty = 20 % + 3·err + 0.05·∫err,
   clamped to 20–100 %, integral clamped for anti-windup. Above 70 °C the fan
   goes to 100 % immediately.
3. Writes the duty to Timer1 PWM on PB1 at ~24 kHz. Q1 inverts it, so the
   register value is `TOP − duty`; at 100 % the output is detached and held
   low so Q1 is off and the fan's PWM line is released.

A 2 s watchdog restarts the chip if the loop stalls; the fan runs at 100 %
whenever PB1 is not actively driven (reset, boot, dead MCU).

The board has no ISP header. Program the ATtiny85 before soldering (SOIC-8
adapter) or with a SOIC-8 test clip. In-circuit, MISO (PB1) is loaded by
R3 and Q1's base, which may make clip programming unreliable.

## Layout

```
tinyfan.kicad_pro / .kicad_sch / .kicad_pcb   KiCad 10 project
gerbers/      Gerber X2 (Protel extensions) + Excellon drill, as sent to the fab
production/   bom.csv (JLCPCB columns), bom_pcbway.csv (hand-edited, with MPNs),
              tinyfan-all-pos.csv (JLCPCB CPL format, SMD parts only)
lib/          digispark_attiny85.kicad_sym — early Digispark symbol, not used by
              the current schematic (it uses the stock MCU_Microchip_ATtiny lib)
firmware/     ATtiny85 sketch + Makefile (arduino-cli, ATTinyCore)
Makefile      regenerates everything above with kicad-cli
```

## Regenerating outputs

Requires `kicad-cli` from KiCad 10 on `PATH` (override with `KICAD_CLI=...`).

```sh
make            # ERC + DRC, gerbers, drill, BOM, pos file
make zip        # build/tinyfan-gerbers.zip for the fab
make pdf        # build/tinyfan-schematic.pdf
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

# psu_controller

KiCad 10 hardware project: ATtiny85 PWM fan controller with an NTC thermistor
input, powered from a PC PSU. Single schematic sheet, two-layer board. No
firmware here. See README.md for the pinout and block description.

## Tooling

- `kicad-cli` 10.0.6 is on PATH. `make` runs ERC/DRC and regenerates
  `gerbers/` and `production/` (see Makefile for the exact export flags; they
  match the plot settings stored in the board file).
- The `kicad` Claude Code plugin (ProductOfAmerica/mcp-server-kicad, via uvx)
  provides MCP tools for reading and editing the schematic and board. Its hook
  blocks raw Read/Write/Edit on `.kicad_*` files; use the MCP tools instead.
- `.history/` is KiCad's own local-history git repo, ignored by this repo.

## Conventions

- Commit `gerbers/` and `production/` only when they were regenerated from the
  current board (`make all`), so the repo always holds fab files matching the
  design.
- `production/bom_pcbway.csv` is hand-maintained (manufacturer part numbers);
  `bom.csv` and the pos file are generated.
- Known DRC state: 15 silkscreen-over-edge warnings on J1/J2, no errors.

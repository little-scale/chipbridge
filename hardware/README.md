# Chipbridge hardware

## Projects

- `main/` — shared RP2040-Zero Chipbridge board.
- `adapters/game-gear/` — Game Gear EXT-port adapter.
- `adapters/sega-9pin/` — Sega 9-pin controller-port adapter.
- `adapters/atari-9pin/` — Atari 9-pin adapter.

Each directory contains the editable KiCad project, schematic, and PCB layout.
KiCad automatic backups, lock files, history, and per-user project state are
intentionally excluded.

## Bill of materials

See the [Chipbridge bill of materials](BOM.md) for the main board and adapter
parts lists. Price-free CSV exports are also provided alongside each KiCad
project. The component properties in the schematics are the source of truth.

## Project-local libraries

The main board uses a custom Waveshare RP2040-Zero symbol and footprint under
`libraries/`. Its `sym-lib-table` and `fp-lib-table` use `${KIPRJMOD}` relative
paths while retaining the original library nicknames used by the schematic and
PCB. No global or user-specific KiCad library entry is required.

The adapter projects currently use standard KiCad libraries only.

## Licensing

The KiCad schematics, PCB layouts, and project-local hardware libraries in
this directory are licensed under the permissive
[CERN Open Hardware Licence Version 2 – Permissive](LICENSE-CERN-OHL-P-2.0.txt),
SPDX identifier `CERN-OHL-P-2.0`.

# Chipbridge bill of materials

This bill of materials covers one populated Chipbridge main board and the
optional console adapter boards. Prices are intentionally omitted because they
vary by supplier, quantity, region, and date.

The KiCad schematic properties are the source of truth. The CSV files are
exported views for builders and sourcing:

- [Main board BOM](main/BOM.csv)
- [Game Gear adapter BOM](adapters/game-gear/BOM.csv)
- [Sega 9-pin adapter BOM](adapters/sega-9pin/BOM.csv)
- [Atari 9-pin adapter BOM](adapters/atari-9pin/BOM.csv)

Quantities below are per PCB. A dash in the MPN column means the part is
generic and may be substituted if the stated electrical and mechanical
requirements are met.

## Main board

| References | Qty | Part | Manufacturer | MPN | Requirements / notes |
|---|---:|---|---|---|---|
| C1 | 1 | 100 nF ceramic capacitor | — | — | 16 V minimum; 2.50 mm lead pitch; body must fit the assigned footprint |
| D1, D2 | 2 | BAT46 Schottky diode | Vishay | BAT46-TAP | DO-35 / SOD27 axial package |
| D3 | 1 | 1N4148 switching diode | onsemi | 1N4148 | DO-35 axial package |
| D4 | 1 | 3 mm indicator LED | — | — | Through-hole; observe polarity |
| J1 | 1 | Console-data 3.5 mm TRS jack | Same Sky | SJ1-3525N | Right-angle through-hole, with switching contacts |
| J2 | 1 | MIDI-input 3.5 mm TRS jack | Same Sky | SJ1-3525N | Right-angle through-hole, with switching contacts |
| R1, R2, R4, R5 | 4 | 470 ohm resistor | — | — | Axial DIN0204 body; 0.25 W or greater |
| R3 | 1 | 220 ohm resistor | — | — | Axial DIN0204 body; 0.25 W or greater |
| U1 | 1 | RP2040-Zero module | Waveshare Electronics | RP2040-ZERO | Use the Waveshare RP2040-Zero pinout and module footprint |
| U2 | 1 | High-gain optocoupler | onsemi | 6N138M | 8-pin DIP; a socket is optional and is not included |

## Game Gear adapter

| References | Qty | Part | Manufacturer | MPN | Requirements / notes |
|---|---:|---|---|---|---|
| J1 | 1 | 1x3 pin header | — | — | Vertical through-hole, 2.54 mm pitch |
| J2 | 1 | 3.5 mm TRS jack | Same Sky | SJ1-3525N | Right-angle through-hole, with switching contacts |

## Sega 9-pin adapter

| References | Qty | Part | Manufacturer | MPN | Requirements / notes |
|---|---:|---|---|---|---|
| J1 | 1 | 3.5 mm TRS jack | Same Sky | SJ1-3525N | Right-angle through-hole, with switching contacts |

## Atari 9-pin adapter

| References | Qty | Part | Manufacturer | MPN | Requirements / notes |
|---|---:|---|---|---|---|
| J1 | 1 | 3.5 mm TRS jack | Same Sky | SJ1-3525N | Right-angle through-hole, with switching contacts |

## Items not included

The BOM does not include bare PCBs, a USB-C data cable, console-specific
cables, solder, mounting hardware, or an optional DIP-8 socket for U2. The
Atari Lynx 2.5 mm cable and Super Nintendo cable are system-level items rather
than components on the published PCBs.

Before ordering, check manufacturer lifecycle status, package compatibility,
and supplier stock. Distributor order codes and dated pricing can be added to
a generated sourcing report without changing this stable BOM.

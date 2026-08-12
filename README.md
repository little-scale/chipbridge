# Chipbridge

Chipbridge is a shared RP2040-based MIDI and synchronisation interface for
retro-console music systems. A common RP2040-Zero board provides USB MIDI,
opto-isolated serial MIDI, status indication, and a protected two-wire console
interface. Console-specific cables and passive adapters connect that interface
to supported systems.

This repository owns the shared Chipbridge hardware and RP2040 firmware. Atari
Lynx and Atari 2600 firmware remain in their respective software projects; this
repository contains only their applicable shared hardware and adapters.

## Compatibility

In this table, **Sync** means synchronisation from MIDI Clock. **Notes** means
MIDI note events are delivered to the relevant tracker channel or instrument
inputs.

| Console | Software | Sync | Notes | PCB | Firmware | Control input | Test |
|---|---|---:|---:|---|---|---|---|
| Sega Master System | [SMSGGDJ tracker](https://github.com/little-scale/smsggdj) | Yes | Yes | Shared | Shared | Sega 9-pin | OK |
| Mega Drive / Genesis | [genmddj tracker](https://github.com/little-scale/genmddj) | Yes | Yes | Shared | Shared | Sega 9-pin | OK |
| Game Gear | [SMSGGDJ tracker](https://github.com/little-scale/smsggdj) | Yes | Yes | Shared | Shared | Game Gear sync PCB | OK |
| Atari Lynx | [alynxdj tracker](https://github.com/little-scale/alynxdj) | Yes | Yes | Shared | Target-specific | 2.5 mm cable | OK |
| Super Nintendo | [SNDJ tracker](https://github.com/little-scale/sndj) | Yes | No | Shared | Shared | SNES cable | OK |
| Atari 2600 | [a26f-neo interface](https://github.com/little-scale/a26f-neo) | No | Yes | Shared | Target-specific | Atari 9-pin PCB | OK |

“Shared” means the component is maintained in Chipbridge. Target-specific
firmware remains in its console software project. See the
[`compatibility notes`](docs/COMPATIBILITY.md) for the ownership boundary.

For Wi-Fi Ableton Link rather than wired MIDI, see the complementary
[SMSGGDJ Link ESP32 bridge](https://github.com/little-scale/smsggdj-link-esp32).

## Repository layout

- `firmware/rp2040/` — shared Pico-family firmware and build instructions.
- `hardware/main/` — the RP2040-Zero Chipbridge board; see the
  [bill of materials](hardware/BOM.md).
- `hardware/adapters/` — console connector and cable adapter PCBs.
- `docs/` — MIDI protocol, serial-MIDI circuit, and wiring documentation.
- `releases/rp2040/` — current prebuilt UF2 images.
- `tests/` — host-side protocol/parser tests.
- `tools/` — reproducible release builders.

## Licensing

- Hardware designs: CERN-OHL-P-2.0.
- Firmware, tools, tests, and corresponding UF2 binaries: GPL-2.0-only.
- Documentation: CC BY 4.0.

See [`LICENSE.md`](LICENSE.md) for the precise scope, attribution, and licence
texts.

# Compatibility

This matrix records the currently tested Chipbridge hardware capabilities.
"Shared firmware" means the common RP2040 firmware in this repository. Target-
specific firmware remains with the target software project.

| Console | Software | Sync | Notes | PCB | Firmware | Control input | Test |
|---|---|---:|---:|---|---|---|---|
| Sega Master System | SMSGGDJ tracker | Yes | Yes | Shared | Shared | Sega 9-pin | OK |
| Mega Drive / Genesis | genmddj tracker | Yes | Yes | Shared | Shared | Sega 9-pin | OK |
| Game Gear | SMSGGDJ tracker | Yes | Yes | Shared | Shared | Game Gear sync PCB | OK |
| Atari Lynx | alynxdj tracker | Yes | Yes | Shared | Target-specific | 2.5 mm cable | OK |
| Super Nintendo | SNDJ tracker | Yes | No | Shared | Shared | SNES cable | OK |
| Atari 2600 | a26f-neo interface | No | Yes | Shared | Target-specific | Atari 9-pin PCB | OK |

## Ownership boundary

- Chipbridge owns the shared RP2040-Zero hardware, electrical interface,
  adapter PCBs, shared firmware, and shared protocol documentation.
- `alynxdj` owns its Lynx-specific Chipbridge firmware and console protocol.
- `a26f-neo` owns its Atari 2600-specific Chipbridge firmware and console
  protocol.
- Console tracker projects own their console-side receivers and behaviour.

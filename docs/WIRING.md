# Bridge wiring — SMS / Mega Drive DE-9 and Game Gear EXT

Every target uses the same **non-inverted logical mapping** documented by
`sms_tracker`: connector pin 9 is counter bit 0 / takeover CLK, connector pin 7
is counter bit 1 / takeover DAT, and pin 8 is ground. The bridge presents a
rolling 2-bit counter `presented & 3`; a high line is logical 1 and a low line
is logical 0.

## Board pinout and capability matrix

| Board | bit 0 / CLK → connector pin 9 | bit 1 / DAT → connector pin 7 | Ground → pin 8 | Serial MIDI RX | Firmware capability |
|---|---|---|---|---|---|
| **Raspberry Pi Pico** | GP0, physical pin 1 | GP1, physical pin 2 | physical pin 3 or any GND | GP13, physical pin 17 | USB + serial MIDI |
| **Raspberry Pi Pico 2** | GP0, physical pin 1 | GP1, physical pin 2 | physical pin 3 or any GND | GP13, physical pin 17 | USB + serial MIDI |
| **Raspberry Pi Pico 2W** | GP0, physical pin 1 | GP1, physical pin 2 | physical pin 3 or any GND | GP13, physical pin 17 | USB + serial MIDI; onboard Wi-Fi unused |
| **Waveshare RP2040-Zero** | GP0 | GP1 | GND | GP13 / header pad 14 | USB + serial MIDI |
| **Seeed XIAO RP2040** | D6 / GP0 | D7 / GP1 | GND | D3 / GP29 | USB + serial MIDI |

### Optional RP2040 firmware-alive LED

All Pico-family builds use GP7 for an optional active-high status LED. Connect
GP7 through a 1 kΩ resistor to the LED anode and connect its cathode to GND.
The firmware turns it on only after setup completes:

| Board | Status LED GPIO |
|---|---|
| Pico / Pico 2 / Pico 2W | GP7, physical pin 10 |
| RP2040-Zero | GP7, header pad 8 |
| XIAO RP2040 | D5 / GP7 |

## Universal TRS console-data connection

The bridge exposes one non-inverted, bidirectional two-wire contract. Use this
same mapping on every bridge board:

| TRS contact | Logical signal | RP2040-family example |
|---|---|---|
| Tip | `DATA0` / counter bit 0 / takeover `CLK` | GP0 |
| Ring | `DATA1` / counter bit 1 / takeover `DAT` | GP1 |
| Sleeve | shared ground | GND |

The connector is data **I/O**, not an audio socket and not the TRS MIDI input.
In counter mode the bridge owns both signals. In MIDI takeover, the console
owns DATA0/CLK and the bridge owns DATA1/DAT.

All bridge outputs use open-drain semantics: logic 0 pulls low and logic 1
releases the wire. Each line uses the current-limited BAT46 clamp shown below
instead of a separate level-shifter IC:

```text
TRS tip  / DATA0 ---- 470R ----+---- MCU DATA0 GPIO
                               |
                               +---- BAT46 anode
                                     BAT46 band/cathode ---- MCU 3V3

TRS ring / DATA1 ---- 470R ----+---- MCU DATA1 GPIO
                               |
                               +---- BAT46 anode
                                     BAT46 band/cathode ---- MCU 3V3

TRS sleeve ------------------------- MCU GND
```

The series resistor limits current when a console presents a 5 V high; the
Schottky diode clamps the GPIO-side node near the 3.3 V rail. Power the bridge
from USB whenever the console is powered so a console cannot back-power an
unpowered MCU through a clamp diode. Console +5 V is not carried through the
TRS cable.

Do not hot-plug the TRS console-data cable. TRS contacts can momentarily short
tip or ring to ground during insertion.

Console-specific cables are passive pin converters:

| Adapter | TRS tip / DATA0 | TRS ring / DATA1 | TRS sleeve |
|---|---|---|---|
| Master System port 2 | DE-9 pin 9 / TR | DE-9 pin 7 / TH | DE-9 pin 8 |
| Mega Drive port 2 | DE-9 pin 9 / TR | DE-9 pin 7 / TH | DE-9 pin 8 |
| Game Gear EXT | EXT pin 9 / PC5 | EXT pin 7 / PC6 | EXT pin 8 |
| Atari 2600 port 2 | DE-9 pin 1 / Up / PA0 | DE-9 pin 2 / Down / PA1 | DE-9 pin 8 |

The Atari 2600 wire behaviour is implemented by target-specific firmware in
the `a26f-neo` project. Clearly distinguish Atari and Sega adapters: both use
DE-9 shells, but Atari pin 7 is +5 V while Sega pin 7 is TH/DATA1.

Target-specific build and usage details:

- [Pico, Pico 2, Pico 2W, RP2040-Zero, and XIAO RP2040](../firmware/rp2040/README.md)
- [Five-pin DIN and TRS Type A MIDI input](SERIAL_MIDI.md)

## SMS controller port 2 — DE-9 (console side)

A standard DE-9. Only **TR and TH are bidirectional**, which is why the protocol
is a 2-bit counter. (From `~/Documents/sms_tracker/HARDWARE.md`.)

| pin | signal | sync use                         |
|-----|--------|----------------------------------|
| 1   | Up     | — (input only)                   |
| 2   | Down   | — (input only)                   |
| 3   | Left   | — (input only)                   |
| 4   | Right  | — (input only)                   |
| 5   | +5V    | shifter HV reference only; otherwise do not connect |
| 6   | TL     | leave unconnected (floats high)  |
| **7** | **TH** | **counter bit 1**              |
| 8   | GND    | shared ground                    |
| **9** | **TR** | **counter bit 0**              |

The tracker reads bit 0 as `TR AND TL`; with TL left floating-high, TR alone
carries bit 0. All lines are pulled up, so an idle/unplugged port reads high
(counter = 3) — `SYNC IN` latches the line state when armed and counts only
*changes*, so a clean start requires the bridge to be presenting a stable count
before the tracker arms.

## Mega Drive / Genesis — genmddj

> ✅ **Confirmed working on Mega Drive hardware (2026-06-25)** — the bridge clocks
> genmddj over controller port 2 with no MD-side changes.

The **same bridge, same wiring, no firmware change** also clocks **genmddj** (the
Mega Drive port of SMSGGDJ). The MD shares the DE-9 controller-port family, and
genmddj's `SYNC: IN` reads the identical 2-bit counter — `$A10005` bits 5/6, i.e.
**TR (pin 9) = bit 0, TH (pin 7) = bit 1**, at 24 PPQN. Wire it exactly as the
tables above, into MD **controller port 2** (a normal pad in port 1 for editing).
In genmddj: **OPTIONS → SYNC: IN**, press Start (shows `WAIT`), then start the
Link/MIDI transport.

Two MD-specific notes:

- **TL (pin 6) is unused here.** The SMS reads bit 0 as `TR AND TL`; genmddj reads
  **TR alone** (`$A10005` bit 5), so pin 6 just stays floating (as above).
- **Check the TH (pin 7) pull-up.** ⚠ The open-drain bridge leans on the console's
  pull-ups for the high level. An idle MD port reads `$7F` (pull-ups present), so
  **TR (pin 9)** is fine — but **TH (pin 7)** is normally the MD's *select output*,
  and genmddj reconfigures it as an input (`$A1000B = 0`). If its input-mode pull-up
  is weak, counter bit 1 can read flaky — symptom: genmddj won't advance, or runs
  erratically / double-time. Fix: a **10 kΩ pull-up on pin 7** to a logic-high rail —
  the XIAO **3V3** pad (3.3 V is a valid MD logic high) or the console **+5 V (pin
  5)**. A resistor pull-up draws microamps, so this does *not* break the "don't power
  from +5 V" rule above (that rule is about the board's WiFi-peak supply current).
  One on pin 9 won't hurt either.

Counted-not-timed, so PAL/NTSC region doesn't matter — genmddj's slave locks to flat
groove-6 (24 PPQN) to match the wire. The same 3-wire cable also cross-syncs a Mega
Drive directly to an SMS / Game Gear — genmddj ↔ SMSGGDJ.

## MIDI takeover mode — same wires, different directions

> ✅ **Hardware-verified 2026-07-07** — on a **Mega Drive 2 in Master System mode**
> (+ Everdrive) driving smsggdj: USB-MIDI on ch 1–4 plays the four PSG voices live,
> bare 3-wire (no divider), with `k on` on the bridge. The *counter/clock-sync*
> direction (`SYNC: IN24`) was verified earlier (2026-07-06, incl. from a DAW's
> USB-MIDI clock). That record describes the historical test setup, not the
> universal Instrument Edition wiring below.
>
> Bring-up fixed one real bug: the bridge's idle-gap resync (`takeover_idle_check`)
> discarded a frame that had been popped from the queue but not yet clocked out,
> silently losing **isolated** note-ons/offs (bursts masked it — "you had to play
> overlapping notes to hear anything"). Fixed: only re-present when nothing is loaded.

> ⚠ **Send clock OR notes to the bridge, never both at once** — takeover (notes) and the
> counter (clock) are mutually exclusive on the two wires, auto-arbitrated by traffic.
> Notes arriving during clock-sync hijack the wire and garble `IN24`. See README →
> *Clock source* for the full rule and the `k off`/`k on`/`k auto` overrides.

MIDI takeover (`SYNC=MIDI` on the console; fixed on in an Instrument image) reuses the **same
three wires — no re-cabling**. Only the pin *directions* change, and the firmware
(`wire_set_mode`) and console flip them automatically when the mode engages:

| Line | DE-9 | Counter mode | **MIDI mode** |
|------|------|--------------|---------------|
| **CLK** | TR (pin 9) | bridge → console (counter bit 0) | **console → bridge** (clock master) |
| **DAT** | TH (pin 7) | bridge → console (counter bit 1) | **bridge → console** (data, MSB-first) |
| GND | pin 8 | shared | shared |

With the current **genmddj** build, CLK is push-pull and DAT is bridge-driven
open-drain. The series resistor and BAT46 clamp protect the bridge input from
the console's 5 V CLK high.

- **DAT (TH, pin 7)** — the bridge drives it **open-drain**, the console reads it. Its high comes
  from the console-side pull-up.
- **CLK (TR, pin 9)** — the *console* now drives it and the bridge reads it as an input.
  Its push-pull high is current-limited and clamped before the 3.3 V GPIO.

## Bench-testing the bridge (no console needed)

1. Flash the general RP2040 image and send a channel-voice message to engage
   automatic takeover, or compile a fixed MIDI-only diagnostic image.
2. Send MIDI notes/CCs from a DAW over USB.
3. **Drive CLK** (TR / GP0, through an appropriate level interface) from a bench clock or a second MCU
   emulating the console's `midi_clock_bit`: raise CLK, wait, read, lower CLK, wait
   `~MIDI_SETTLE`. **Capture DAT** (TH / GP1) on a logic analyser.
4. **Decode:** after an idle gap the bridge presents a **flag bit**; `1` → the next
   **24 bits** are `status·d1·d2` (`status = type<<4|chan`); `0` → queue empty. Confirm the
   decoded events match the MIDI you sent (NoteOn = type 2, etc.).
5. Measure falling-CLK to DAT transition time and verify DAT is stable before
   every following rising edge. Inspect the queue diagnostics in a dedicated
   diagnostic build before changing timing constants.

A full loop test with a real console validates both the responder and the
console-side poller.

## Power & ground

- Power the MCU board from its own USB supply.
- Share GND (board GND ↔ console pin 8).
- Leave console +5 V disconnected. It is not part of the three-wire TRS
  console-data contract and must never power the MCU.

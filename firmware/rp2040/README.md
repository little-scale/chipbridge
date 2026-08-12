# Chipbridge RP2040 firmware

The firmware source is licensed under GPL-2.0-only; see
[`../LICENSE-GPL-2.0.txt`](../LICENSE-GPL-2.0.txt).

This Arduino sketch supports:

- Raspberry Pi Pico (RP2040)
- Raspberry Pi Pico 2 (RP2350)
- Raspberry Pi Pico 2W (RP2350; Wi-Fi is deliberately unused)
- Waveshare RP2040-Zero
- Seeed XIAO RP2040

It is a wired MIDI device, not an Ableton Link client. MIDI can arrive as
class-compliant USB MIDI from a computer/DAW or as opto-isolated five-pin
DIN/TRS MIDI on an RX-only serial input. The general firmware in
`releases/rp2040/` contains both wire roles:

- MIDI Clock plus Start/Continue/Stop drives the rolling 2-bit `SYNC: IN24`
  counter.
- A MIDI channel message switches the same pins to the flag-framed,
  console-clocked stream used by `SYNC: MIDI`. Note on/off, CC, program change,
  pitch bend, and panic are forwarded.

While in takeover, continued console CLK polling keeps that role latched even
when a note is held and no new MIDI bytes arrive. If both channel traffic and
console polling stop for 500 ms, the firmware safely returns to the counter.

## Pinout

The pinout intentionally matches the **non-inverted logic output** documented
in `sms_tracker/adapter`:

| Board | counter bit 0 / takeover CLK | counter bit 1 / takeover DAT | Ground | Game Gear EXT / Sega DE-9 |
|---|---|---|---|---|
| Raspberry Pi Pico | GP0, physical pin 1 | GP1, physical pin 2 | physical pin 3 or any GND | pins 9 / 7 / 8 |
| Raspberry Pi Pico 2 | GP0, physical pin 1 | GP1, physical pin 2 | physical pin 3 or any GND | pins 9 / 7 / 8 |
| Raspberry Pi Pico 2W | GP0, physical pin 1 | GP1, physical pin 2 | physical pin 3 or any GND | pins 9 / 7 / 8 |
| Waveshare RP2040-Zero | GP0 | GP1 | any GND | pins 9 / 7 / 8 |
| Seeed XIAO RP2040 | D6 / GP0 | D7 / GP1 | GND | pins 9 / 7 / 8 |

In the final column, pin 9 is PC5/TR (bit 0/CLK), pin 7 is PC6/TH
(bit 1/DAT), and pin 8 is ground.

Serial MIDI receiver output pins:

| Board | Opto-isolated MIDI receiver OUT |
|---|---|
| Pico / Pico 2 / Pico 2W | GP13, physical pin 17 |
| Waveshare RP2040-Zero | GP13 / header pad 14 |
| XIAO RP2040 | D3 / GP29 |

Use a 3.3 V-compatible, non-inverted MIDI-IN receiver. Do not connect a DIN or
TRS socket directly to the GPIO. Full DIN and TRS Type A wiring is in
[`../../docs/SERIAL_MIDI.md`](../../docs/SERIAL_MIDI.md).

GP13 is not a hardware-UART RX mux position, so Pico, Pico 2, Pico 2W, and
RP2040-Zero builds receive MIDI there with the Arduino-Pico core's PIO UART.
XIAO RP2040 uses hardware UART0 on D3/GP29.

### Firmware-alive LED

GP7 drives an optional external status LED on every Pico-family build. It turns
on after firmware setup completes. Received USB or serial channel MIDI makes it
pulse off briefly; MIDI Clock produces one inverse pulse per quarter note:

```text
GP7 ---- 1K ---- LED anode
                  LED cathode ---- GND
```

On a standard Pico/Pico 2/Pico 2W, GP7 is physical pin 10. On RP2040-Zero it is
header pad 8, and on XIAO RP2040 it is D5/GP7. A high-efficiency LED and 1 kΩ
resistor keep the GPIO load low. This is a firmware indicator rather than a
direct power LED: it remains off if setup does not finish.

## Universal TRS console-data interface

Do not connect a console's 5 V signal directly to GP0 or GP1. Each TRS data
line passes through a 470 Ω series resistor and then a BAT46 clamp at the
GPIO-side node:

```text
TRS tip  / DATA0 ---- 470R ----+---- GP0
                               +---- BAT46 anode
                                     BAT46 band/cathode ---- 3V3

TRS ring / DATA1 ---- 470R ----+---- GP1
                               +---- BAT46 anode
                                     BAT46 band/cathode ---- 3V3

TRS sleeve ------------------------- GND
```

The firmware never actively drives a high. In both counter and takeover roles,
logic 0 drives the GPIO low and logic 1 switches it to high impedance so the
console-side pull-up supplies high. In takeover, GP0 is fully released for the
console-owned CLK while GP1 continues to drive-low/release-high for DAT.

Power the Pico from USB before powering the console. Do not carry console +5 V
through the TRS cable, and do not hot-plug the console-data jack. The passive
console cable maps tip/ring/sleeve to the connector pins listed in
[`../../docs/WIRING.md`](../../docs/WIRING.md#universal-trs-console-data-connection).

For the Game Gear, pin 6 may remain disconnected. SMSGGDJ reads counter bit 0
as PC4 AND PC5, and the unconnected PC4 input is pulled high.

## Ready-to-use UF2 builds

The general images below contain both `SYNC: IN24` clock and `SYNC: MIDI`
takeover support.

Choose the file that exactly matches the board. Pico and Pico 2 firmware is not
interchangeable:

| Hardware | Ready-to-flash firmware |
|---|---|
| Raspberry Pi Pico | [`chipbridge-pico.uf2`](../../releases/rp2040/chipbridge-pico.uf2) |
| Raspberry Pi Pico 2 | [`chipbridge-pico2.uf2`](../../releases/rp2040/chipbridge-pico2.uf2) |
| Raspberry Pi Pico 2W | [`chipbridge-pico2w.uf2`](../../releases/rp2040/chipbridge-pico2w.uf2) |
| Waveshare RP2040-Zero | [`chipbridge-rp2040-zero.uf2`](../../releases/rp2040/chipbridge-rp2040-zero.uf2) |
| Seeed XIAO RP2040 | [`chipbridge-xiao-rp2040.uf2`](../../releases/rp2040/chipbridge-xiao-rp2040.uf2) |

The SHA-256 hashes are in
[`SHA256SUMS.txt`](../../releases/rp2040/SHA256SUMS.txt). These files were built
with Arduino-Pico core 5.5.1 using the default Pico SDK USB stack.

Regenerate all five unified images with `tools/make-pico-release.sh`. The script
passes the automatic runtime role explicitly and refreshes `SHA256SUMS.txt`.

To flash a Pico/Pico 2/Pico 2W:

1. Disconnect USB.
2. Hold **BOOTSEL** while reconnecting USB.
3. Release BOOTSEL when the `RPI-RP2`/RP2350 boot drive appears.
4. Copy the matching UF2 onto that drive. The board reboots automatically.

For XIAO RP2040, hold **B** while connecting USB, then copy its matching UF2 to
the mounted boot drive.

## Build

Install Arduino IDE 2 or `arduino-cli`, then install Earle Philhower's
**Raspberry Pi Pico/RP2040/RP2350** core. Open
`chipbridge/chipbridge.ino` and select the relevant board:

| Hardware | Arduino board |
|---|---|
| Pico | Raspberry Pi Pico |
| Pico 2 | Raspberry Pi Pico 2 |
| Pico 2W | Raspberry Pi Pico 2W |
| RP2040-Zero | Waveshare RP2040 Zero |
| XIAO RP2040 | Seeed XIAO RP2040 |

The default **Pico SDK USB stack** is supported. Build/upload normally. For a
command-line build:

```sh
arduino-cli compile --fqbn rp2040:rp2040:rpipico firmware/rp2040/chipbridge
arduino-cli compile --fqbn rp2040:rp2040:rpipico2 firmware/rp2040/chipbridge
arduino-cli compile --fqbn rp2040:rp2040:rpipico2w firmware/rp2040/chipbridge
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero firmware/rp2040/chipbridge
arduino-cli compile --fqbn rp2040:rp2040:seeed_xiao_rp2040 firmware/rp2040/chipbridge
```

On a Pico-style board, hold BOOTSEL while plugging it in. On XIAO RP2040, hold
the **B** button while connecting USB (or use its reset/boot procedure). Copy
the generated UF2 to the mounted drive, or upload from Arduino normally. The
firmware enumerates as **Chipbridge** with a MIDI interface named
**Chipbridge MIDI**.

## Use

For clock sync with the general image:

1. Select `SYNC: IN24` in SMSGGDJ/genmddj.
2. Route MIDI Clock and transport to `Chipbridge MIDI`.
3. Press play on the tracker so it waits, then start the DAW.

For MIDI takeover with that same general image:

1. Select `SYNC: MIDI` in the tracker.
2. Route channel-voice MIDI to the bridge over USB or DIN/TRS. SMSGGDJ maps channels 1–4 to its
   four PSG voices.
3. The first channel message switches the wire into takeover. The tracker CLK
   polling keeps it there until `SYNC: MIDI` is no longer active.

Do not send MIDI Clock and channel-voice data at the same time; the two console
wire roles are mutually exclusive. Use USB MIDI **or** serial MIDI, not both at
once. To make a fixed Instrument build manually, compile with
`BRIDGE_WIRE_ROLE=BRIDGE_ROLE_MIDI_ONLY`. A diagnostic clock-only build remains available with
`BRIDGE_WIRE_ROLE=BRIDGE_ROLE_CLOCK_ONLY`.

## Verification status

The RP2040-Zero hardware has been exercised with SMS, Mega Drive / Genesis,
Game Gear, Atari Lynx, SNES, and Atari 2600 targets. See the current
[`compatibility matrix`](../../docs/COMPATIBILITY.md) for the capability and
firmware ownership boundary.

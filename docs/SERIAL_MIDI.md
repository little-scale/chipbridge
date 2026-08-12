# Five-pin DIN and TRS MIDI input

Every firmware target accepts conventional MIDI 1.0 through a
**31,250-baud, 8-N-1 UART input**. Five-pin DIN and 3.5 mm TRS are only
different connector choices in front of the same MIDI receiver; the firmware
does not need a connector setting.

The serial input uses the same handlers as USB MIDI:

- Timing Clock, Start, Continue, Stop, and System Reset drive the clock path.
- Note On/Off, Control Change, Program Change, and Pitch Bend drive
  `SYNC: MIDI` takeover.
- Note On with velocity zero becomes Note Off; CC 120/123 becomes panic.
- Poly aftertouch, channel pressure, System Common, and SysEx are ignored.
- Running status and Real-Time bytes interleaved inside another message are
  handled correctly.

Use **one MIDI source at a time**: USB MIDI or serial MIDI. There is no
arbitration or merging between simultaneous inputs.

## Board-side RX pin

| Board | Serial MIDI receiver OUT → |
|---|---|
| Raspberry Pi Pico | GP13, physical pin 17 |
| Raspberry Pi Pico 2 | GP13, physical pin 17 |
| Raspberry Pi Pico 2W | GP13, physical pin 17 |
| Waveshare RP2040-Zero | GP13 / header pad 14 |
| Seeed XIAO RP2040 | D3 / GP29 |

Pico, Pico 2, Pico 2W, and RP2040-Zero use one PIO
state machine as an RX-only UART because GP13 is not a hardware-UART RX mux
position. XIAO RP2040 uses UART0 on D3/GP29. UART TX is unused and must not be
connected to the MIDI input connector.

## Required MIDI receiver

Do not connect DIN or TRS pins directly to an MCU GPIO. MIDI DIN/TRS is a
current-loop interface and its input is required to be optically isolated. Use
a standards-compliant MIDI-IN receiver circuit or module whose logic output is:

- **3.3 V compatible**;
- **non-inverted**;
- idle high; and
- fast enough for MIDI's 31,250-baud data.

Connect its logic side according to the receiver part's supply requirements.
The receiver output presented to the board RX pin must be pulled up to 3.3 V:

```text
MIDI-IN receiver VCC  -> its specified supply (5V/VBUS for 6N138)
MIDI-IN receiver GND  -> board GND
MIDI-IN receiver OUT  -> board RX pin and pull-up to board 3V3
```

The optocoupler's connector side remains isolated. Do not join DIN pin 2 or a
TRS sleeve to board logic ground unless the particular compliant receiver
design explicitly specifies a shield/chassis connection.

### Exact TRS Type A receiver using a 6N138

This follows Paul Stoffregen's 6N138 MIDI-input circuit: the 6N138 runs from
5 V, pin 6 is pulled up to the MCU's 3.3 V rail through 470 ohms, and pin 7 is
left open. This is the concrete circuit for a Waveshare RP2040-Zero. It also
works with the other boards when `GP13` is replaced by that board's RX pin from
the table above.

Parts:

- one 3.5 mm TRS female socket;
- one 6N138 optocoupler;
- `R1`: 220 ohm, 1/4 W;
- `R2`: 470 ohm;
- `D1`: 1N4148 or 1N914 reverse-protection diode;
- `C1`: 100 nF ceramic decoupling capacitor.

A BAT46, BAT48, or 1N5819 can be used for `D1` if that is what is available.
The diode's **banded cathode end must face 6N138 pin 2**.

Start at the TRS socket and wire it in this order:

```text
TRS Type A RING ---- R1 220R ----+---- 6N138 pin 2 (LED anode)
                                 |
                                 +----|<|----+
                                      D1     |
TRS Type A TIP ------------------------------+---- 6N138 pin 3 (LED cathode)

TRS SLEEVE: leave electrically unconnected
```

`D1` is directly across 6N138 pins 2 and 3, opposite to the internal LED:

```text
D1 cathode / band -> pin 2
D1 anode          -> pin 3
```

Then wire the isolated logic side:

```text
RP2040-Zero 5V/VBUS -------- 6N138 pin 8 (VCC)
RP2040-Zero GND ------------ 6N138 pin 5 (GND)

RP2040-Zero 3V3 --- R2 470R ----+--- 6N138 pin 6 (OUT)
                               |
                               +--- RP2040-Zero GP13 (serial MIDI RX)

6N138 pin 7 (BASE): no connection

C1 100nF directly between 6N138 pin 8 and pin 5
6N138 pins 1 and 4: no connection
```

The 6N138 DIP pin numbers, viewed from above with its notch at the top, are:

```text
             notch
          +--- U ---+
      NC 1|         |8 VCC -> 5V/VBUS
   anode 2|  6N138  |7 base -> no connection
 cathode 3|         |6 OUT  -> GP13 and 470R -> 3V3
      NC 4|         |5 GND  -> board GND
          +---------+
```

Do not connect TRS sleeve to RP2040-Zero GND. The 6N138 input side—pins 2 and
3—must have no DC connection to the RP2040 side. Power the RP2040-Zero from
USB; its `5V/VBUS` pin supplies the 6N138, while the separate `3V3` pull-up
guarantees that GP13 never receives a 5 V logic high.

With no MIDI cable activity, GP13 should measure approximately 3.3 V. During
MIDI data it switches between approximately 3.3 V and 0 V. A multimeter will
usually show only an average; use a logic analyser or oscilloscope to see the
31,250-baud pulses.

### Five-pin DIN input

Use a 180-degree five-pin DIN **female** MIDI-IN connector wired to the receiver
according to its schematic:

| DIN contact | MIDI role |
|---|---|
| pin 4 | current source |
| pin 5 | current sink |
| pin 2 | cable shield |
| pins 1 and 3 | no connection |

Be careful with connector drawings: pin order reverses between the mating face
and solder side. Follow the pin numbers moulded into the connector.

### TRS MIDI input

Use the MIDI Association's standardized **TRS Type A** mapping:

| TRS contact | Equivalent DIN contact |
|---|---|
| Tip | pin 5, current sink |
| Ring | pin 4, current source |
| Sleeve | pin 2, shield |

Type B swaps tip and ring and is not the documented wiring here. A Type-B
source can still be used with a Type-B-to-DIN adapter feeding the five-pin
input.

DIN and TRS sockets may both be fitted ahead of one receiver, but connect only
one MIDI source at a time. Two active outputs must never be tied together.

## Console connection is unchanged

The serial MIDI connector is the **input side of the bridge**. The Game Gear
EXT / Sega DE-9 connection remains the non-inverted two-wire interface in
[`WIRING.md`](WIRING.md):

- connector pin 9 = counter bit 0 / takeover CLK;
- connector pin 7 = counter bit 1 / takeover DAT;
- connector pin 8 = shared console ground.

The MIDI receiver and the console BAT46 protection network are separate
circuits.

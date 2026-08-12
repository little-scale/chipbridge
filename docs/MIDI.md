# Chipbridge MIDI and sync wire protocol

Chipbridge presents two mutually exclusive behaviours on the same protected
two-wire console connection:

- MIDI Clock and transport drive a rolling 2-bit sync counter.
- MIDI channel messages engage console-clocked note takeover.

The common RP2040 firmware selects the role automatically. A fixed role can be
chosen at compile time for diagnostics. Console-side receivers remain in their
respective tracker projects.

## Electrical contract

The logical connection is `DATA0`, `DATA1`, and ground. Outputs are open-drain:
logic 0 pulls the wire low and logic 1 releases it to the console-side pull-up.
The series-resistor and BAT46 clamp interface is documented in `WIRING.md`.

| Role | DATA0 | DATA1 |
|---|---|---|
| Counter | bridge output, counter bit 0 | bridge output, counter bit 1 |
| Takeover | console output, `CLK` | bridge output, `DAT` |

## Clock and transport counter

MIDI Timing Clock (`0xF8`) is 24 PPQN. While transport is running, each clock
increments a rolling counter modulo four and presents its two bits on DATA0 and
DATA1. Start resets the counter and begins counting; Continue resumes; Stop
freezes it; System Reset clears and stops it.

The receiving console samples the counter once per video frame and advances by
the modulo-four delta. It therefore counts elapsed ticks instead of relying on
the exact timing of a GPIO edge.

## MIDI takeover transfer

The console is the clock master:

- `CLK` idles low. The console raises it and samples `DAT` on the rising edge.
- After sampling, the console lowers `CLK`.
- The bridge presents the next `DAT` bit from the falling-edge handler.
- Data is sent most-significant bit first.

After an idle gap, the bridge presents a leading flag bit:

- `0` means the queue is empty and the console stops polling.
- `1` means a fixed three-byte event follows.

```text
[flag=1] [status] [data1] [data2]    25 bits total
[flag=1] [status] [data1] [data2]    next queued event
[flag=0]                              queue empty
```

`status = type << 4 | MIDI channel`, where the channel is 0–15:

| Type | Value | Payload |
|---|---:|---|
| Note Off | 1 | note, release velocity |
| Note On | 2 | note, velocity |
| Control Change | 3 | controller, value |
| Program Change | 4 | program, zero |
| Pitch Bend | 5 | LSB, MSB |
| Panic | 7 | zero, zero |

The bridge converts Note On with velocity zero to Note Off and CC 120/123 to
Panic. Repeated unread changes for the same controller are coalesced to their
latest value. Polyphonic aftertouch and channel pressure are not forwarded.

Each console decides which channels and event types it implements. SMSGGDJ
maps MIDI channels 1–4 to its four PSG tracks; genmddj provides its own wider
voice mapping. Target-specific firmware and protocol interpretation for Atari
Lynx and Atari 2600 remain in their respective projects.

## Idle-gap and queue invariant

Loading a frame removes it from the event queue. If an idle gap occurs while a
loaded frame still has bits remaining, the bridge must preserve that frame.
Resetting the shift register during the gap would discard isolated events that
have already been removed from the queue.

The current RP2040 implementation uses a 32-event queue and a 1.5 ms idle-gap
threshold. Queue overflow is recoverable for release events by scheduling a
channel Panic, but the protocol has no acknowledgement, checksum, or retry.
Those would require coordinated changes to both the bridge and console-side
receivers.

## Automatic role arbitration

A normalised channel message is queued before takeover is engaged, allowing the
leading data-ready flag to be presented immediately. Automatic mode returns to
the counter only after both MIDI channel traffic and console polling have been
quiet for 500 ms. Active console polling therefore keeps takeover engaged even
while a note is held and no new MIDI bytes arrive.

MIDI Clock and channel messages should not be sent to the same bridge port at
the same time: counter and takeover roles are intentionally exclusive on the
two-wire connection.

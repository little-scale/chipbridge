// SPDX-License-Identifier: GPL-2.0-only
// config.h -- Chipbridge Pico-family USB/serial-MIDI firmware.
#pragma once

// Universal two-wire TRS data interface:
//
//   GP0 = DATA0 / counter bit 0 / takeover CLK -> TRS tip
//   GP1 = DATA1 / counter bit 1 / takeover DAT -> TRS ring
//   GND                                      -> TRS sleeve
//
// Each data line passes through the documented series resistor and BAT46 clamp
// before the GPIO. All outputs use low-or-release open-drain semantics; the
// console-side pull-up supplies logic high. Console adapters map the common TRS
// signals to each target connector without changing firmware polarity.
#define PIN_DATA0 0
#define PIN_DATA1 1
#define PIN_COUNTER_BIT0 PIN_DATA0
#define PIN_COUNTER_BIT1 PIN_DATA1

// External active-high firmware-alive LED. Connect GP7 through a 1 kΩ series
// resistor to the LED anode, and connect the LED cathode to GND. It turns on
// only after setup completes successfully.
#define PIN_STATUS_LED 7

// The same lines reverse roles for tracker SYNC: MIDI takeover.
#define PIN_MIDI_CLK PIN_COUNTER_BIT0
#define PIN_MIDI_DAT PIN_COUNTER_BIT1

// --- DIN / TRS MIDI input ---------------------------------------------------
// Both connector styles feed the same opto-isolated, non-inverted 31,250-baud
// UART input. The TX pin is configured by the Arduino core but is not connected.
// XIAO RP2040 exposes UART0 on D2/D3. The RP2040-Zero hardware model connects
// its 6N138 receiver output to GP13; use the same GP13 RX mapping on Pico-style
// boards so one firmware source retains a single non-XIAO pin convention.
#define SERIAL_MIDI_BAUD 31250
#if defined(ARDUINO_SEEED_XIAO_RP2040)
#define SERIAL_MIDI_USE_PIO 0
#define PIN_SERIAL_MIDI_TX 28  // D2 / GP28 -- leave unconnected
#define PIN_SERIAL_MIDI_RX 29  // D3 / GP29 <- MIDI receiver logic output
#else
#define SERIAL_MIDI_USE_PIO 1
#define PIN_SERIAL_MIDI_RX 13  // Pico physical pin 17 <- MIDI receiver output
#endif

// General releases select the wire role at runtime: MIDI clock/transport uses
// the counter, while channel-voice messages enter console-clocked takeover.
// Instrument releases override this at build time and keep takeover forced.
#define BRIDGE_ROLE_AUTO       0
#define BRIDGE_ROLE_MIDI_ONLY  1
#define BRIDGE_ROLE_CLOCK_ONLY 2
#ifndef BRIDGE_WIRE_ROLE
#define BRIDGE_WIRE_ROLE BRIDGE_ROLE_AUTO
#endif

#if BRIDGE_WIRE_ROLE != BRIDGE_ROLE_AUTO && \
    BRIDGE_WIRE_ROLE != BRIDGE_ROLE_MIDI_ONLY && \
    BRIDGE_WIRE_ROLE != BRIDGE_ROLE_CLOCK_ONLY
#error BRIDGE_WIRE_ROLE must be BRIDGE_ROLE_AUTO, BRIDGE_ROLE_MIDI_ONLY, or BRIDGE_ROLE_CLOCK_ONLY
#endif

#define MIDI_TAKEOVER_ENABLED (BRIDGE_WIRE_ROLE != BRIDGE_ROLE_CLOCK_ONLY)
#define TAKEOVER_AUTO_RELEASE_US 500000UL
#define TAKEOVER_IDLE_US      1500UL
#define MIDI_LED_OFF_US       25000UL   // inverse activity pulse: LED off for 25 ms
#define MIDI_LED_PERIOD_US    100000UL  // at most ten visible pulses per second
#define MIDI_EVTQ_SIZE        32

#if (MIDI_EVTQ_SIZE & (MIDI_EVTQ_SIZE - 1)) != 0
#error MIDI_EVTQ_SIZE must be a power of two
#endif

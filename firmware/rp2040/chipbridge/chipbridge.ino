// SPDX-License-Identifier: GPL-2.0-only
// Chipbridge wired USB/serial-MIDI firmware for Pico-family boards.
//
// The general build switches one protected two-wire connection between the
// 24-PPQN counter and the console-clocked MIDI takeover protocol. Instrument
// builds can still force takeover at compile time.
//
// Build with Earle Philhower's Arduino-Pico core. See ../README.md.

#include <Arduino.h>
#include <MIDIUSB.h>
#include <USB.h>

#include "config.h"
#include "midi_proto.h"
#include "serial_midi_parser.h"

enum WireMode {
  WIRE_COUNTER = 0,
  WIRE_TAKEOVER = 1,
};

volatile WireMode g_wireMode = WIRE_COUNTER;
volatile uint8_t g_counter = 0;
volatile bool g_playing = false;

volatile MidiEvt g_evtq[MIDI_EVTQ_SIZE];
volatile uint16_t g_eHead = 0;
volatile uint16_t g_eTail = 0;
volatile uint32_t g_shift = 0;
volatile int g_shiftBits = 0;
volatile uint32_t g_lastClkEdgeUs = 0;
volatile uint32_t g_lastChannelVoiceUs = 0;
volatile bool g_idleArmed = false;
volatile uint32_t g_dropped = 0;
// A queue overflow may evict an earlier release. Record its channel outside the
// ring so the consumer emits a recovery Panic before normal queued traffic.
volatile uint16_t g_recoveryPanicMask = 0;

uint32_t g_ledPulseStartedUs = 0;
bool g_ledPulseActive = false;
uint8_t g_midiClockLedTick = 0;

// GP7 is normally high as the firmware-alive indication. Incoming MIDI starts
// a short, non-blocking low pulse. Rate limiting keeps a clock stream visibly
// flickering instead of retriggering so quickly that the LED stays dark.
static void indicate_midi_activity() {
  const uint32_t now = micros();
  if (!g_ledPulseActive &&
      (g_ledPulseStartedUs == 0 ||
       (uint32_t)(now - g_ledPulseStartedUs) >= MIDI_LED_PERIOD_US)) {
    digitalWriteFast(PIN_STATUS_LED, LOW);
    g_ledPulseStartedUs = now;
    g_ledPulseActive = true;
  }
}

static void service_activity_led() {
  if (g_ledPulseActive &&
      (uint32_t)(micros() - g_ledPulseStartedUs) >= MIDI_LED_OFF_US) {
    digitalWriteFast(PIN_STATUS_LED, HIGH);
    g_ledPulseActive = false;
  }
}

static inline uint16_t evtq_count() {
  return (uint16_t)(g_eHead - g_eTail);
}

// GP0/GP1 are the non-inverted TRS DATA0/DATA1 pair. Every driven state uses
// low-or-release semantics so the same current-limited BAT46 interface can
// safely reverse DATA0 for console-clocked MIDI takeover.
static inline void write_open_drain(uint8_t pin, bool high) {
  const uint32_t mask = 1u << pin;
  digitalWriteFast(pin, LOW);  // the output latch is always low
  if (high) {
    sio_hw->gpio_oe_clr = mask;  // release: external LV pull-up makes logic 1
  } else {
    sio_hw->gpio_oe_set = mask;  // assert logic 0
  }
}

static inline void present_counter(uint8_t counter) {
  write_open_drain(PIN_COUNTER_BIT0, (counter & 0x01) != 0);
  write_open_drain(PIN_COUNTER_BIT1, (counter & 0x02) != 0);
}

static void load_frame_locked() {
  if (g_recoveryPanicMask != 0) {
    uint8_t channel = 0;
    while ((g_recoveryPanicMask & (1u << channel)) == 0) ++channel;
    g_recoveryPanicMask &= (uint16_t)~(1u << channel);
    MidiEvt event = {(uint8_t)((EVT_PANIC << 4) | channel), 0, 0};
    g_shift = midi_frame_word(event);
    g_shiftBits = MIDI_FRAME_BITS;
  } else if (evtq_count() != 0) {
    const uint16_t index = g_eTail++ & (MIDI_EVTQ_SIZE - 1);
    MidiEvt event;
    event.status = g_evtq[index].status;
    event.d1 = g_evtq[index].d1;
    event.d2 = g_evtq[index].d2;
    g_shift = midi_frame_word(event);
    g_shiftBits = MIDI_FRAME_BITS;
  } else {
    g_shift = 0;
    g_shiftBits = 1;  // queue-empty flag
  }
}

static inline void present_bit_locked() {
  if (g_shiftBits == 0) load_frame_locked();
  const bool bit = (g_shift & 0x80000000u) != 0;
  g_shift <<= 1;
  --g_shiftBits;
  write_open_drain(PIN_MIDI_DAT, bit);
}

// The console samples DAT on CLK's rising edge. Once it lowers CLK again, this
// ISR presents the next bit so it is stable before the following rising edge.
static void clk_falling_isr() {
  g_lastClkEdgeUs = micros();
  g_idleArmed = false;
  present_bit_locked();
}

static void takeover_idle_check() {
  const uint32_t now = micros();
  if (!g_idleArmed &&
      (uint32_t)(now - g_lastClkEdgeUs) > TAKEOVER_IDLE_US) {
    noInterrupts();
    if (!g_idleArmed) {
      g_idleArmed = true;
      // A loaded frame has already been popped from the queue. Keep it until
      // the console clocks it out; resetting here loses isolated notes.
      if (g_shiftBits == 0) present_bit_locked();
    }
    interrupts();
  }
}

// AUTO enters takeover when channel-voice MIDI arrives. It may return to the
// counter only after both MIDI traffic and console CLK polling have stopped.
// The CLK condition is important: a held note can be quiet for seconds while a
// tracker in SYNC:MIDI continues polling, and must never see GP0 become an
// output underneath it.
static bool takeover_auto_active() {
  const uint32_t now = micros();
  return (uint32_t)(now - g_lastChannelVoiceUs) <= TAKEOVER_AUTO_RELEASE_US ||
         (g_wireMode == WIRE_TAKEOVER &&
          (uint32_t)(now - g_lastClkEdgeUs) <= TAKEOVER_AUTO_RELEASE_US);
}

static void evtq_push(const MidiEvt& event, bool critical) {
  noInterrupts();

  // Collapse an unread sweep of the same controller to its latest value.
  if ((event.status & 0xF0) == (EVT_CC << 4)) {
    for (uint16_t i = g_eTail; i != g_eHead; ++i) {
      volatile MidiEvt& queued = g_evtq[i & (MIDI_EVTQ_SIZE - 1)];
      if (queued.status == event.status && queued.d1 == event.d1) {
        queued.d2 = event.d2;
        interrupts();
        return;
      }
    }
  }

  if (evtq_count() >= MIDI_EVTQ_SIZE) {
    if (!critical) {
      ++g_dropped;
      interrupts();
      return;
    }
    const MidiEvt evicted = {
      g_evtq[g_eTail & (MIDI_EVTQ_SIZE - 1)].status,
      g_evtq[g_eTail & (MIDI_EVTQ_SIZE - 1)].d1,
      g_evtq[g_eTail & (MIDI_EVTQ_SIZE - 1)].d2,
    };
    ++g_eTail;  // release events take priority over the oldest queued event
    const uint8_t evictedType = evicted.status >> 4;
    if (evictedType == EVT_NOTE_OFF || evictedType == EVT_PANIC)
      g_recoveryPanicMask |= (uint16_t)(1u << (evicted.status & 0x0F));
    ++g_dropped;
  }

  volatile MidiEvt& slot = g_evtq[g_eHead & (MIDI_EVTQ_SIZE - 1)];
  slot.status = event.status;
  slot.d1 = event.d1;
  slot.d2 = event.d2;
  ++g_eHead;
  interrupts();
}

static void wire_set_mode(WireMode mode) {
  if (mode == g_wireMode) return;

  if (mode == WIRE_TAKEOVER) {
    // Avoid contention first: the console owns CLK in takeover mode.
    sio_hw->gpio_oe_clr = 1u << PIN_MIDI_CLK;

    noInterrupts();
    g_shiftBits = 0;
    g_idleArmed = false;
    g_lastClkEdgeUs = micros();
    present_bit_locked();  // make the leading flag stable before the next poll
    interrupts();

    attachInterrupt(digitalPinToInterrupt(PIN_MIDI_CLK),
                    clk_falling_isr, FALLING);
  } else {
    detachInterrupt(digitalPinToInterrupt(PIN_MIDI_CLK));

    noInterrupts();
    // Unconsumed events belong to the takeover session being left explicitly.
    g_eTail = g_eHead;
    g_shiftBits = 0;
    g_recoveryPanicMask = 0;
    interrupts();

    present_counter(g_counter);
  }

  g_wireMode = mode;
}

static void handle_realtime(uint8_t status) {
  switch (status) {
    case 0xF8:  // Timing Clock
      if (++g_midiClockLedTick >= 24) {
        g_midiClockLedTick = 0;
        indicate_midi_activity();  // one inverse pulse per quarter note
      }
      if (g_playing) {
        g_counter = (uint8_t)((g_counter + 1u) & 0x03u);
        if (g_wireMode == WIRE_COUNTER) present_counter(g_counter);
      }
      break;
    case 0xFA:  // Start
      g_midiClockLedTick = 0;
      indicate_midi_activity();  // transport start is the initial downbeat
      g_counter = 0;
      g_playing = true;
      if (g_wireMode == WIRE_COUNTER) present_counter(g_counter);
      break;
    case 0xFB:  // Continue
      indicate_midi_activity();
      g_playing = true;
      break;
    case 0xFC:  // Stop
      indicate_midi_activity();
      g_playing = false;
      break;
    case 0xFF:  // System Reset
      g_midiClockLedTick = 0;
      indicate_midi_activity();
      g_counter = 0;
      g_playing = false;
      if (g_wireMode == WIRE_COUNTER) present_counter(g_counter);
      break;
    default:
      break;
  }
}

static void handle_channel_message(uint8_t status, uint8_t d1, uint8_t d2) {
  if (status < 0x80 || status >= 0xF0) return;

#if MIDI_TAKEOVER_ENABLED
  MidiEvt event;
  bool critical = false;
  if (midi_normalise(status & 0xF0, status & 0x0F,
                     d1, d2, event, critical)) {
    indicate_midi_activity();
    evtq_push(event, critical);
#if BRIDGE_WIRE_ROLE == BRIDGE_ROLE_AUTO
    g_lastChannelVoiceUs = micros();
    // The event is already queued, so entering takeover presents a data-ready
    // flag immediately rather than an avoidable empty poll.
    wire_set_mode(WIRE_TAKEOVER);
#endif
  }
#endif
}

static void handle_midi_packet(const midiEventPacket_t& packet) {
  const uint8_t bytes[3] = {packet.byte1, packet.byte2, packet.byte3};

  // Real-time messages normally occupy byte1 of their own packet. Scan all
  // payload positions as a defensive measure against host packet variation.
  for (unsigned i = 0; i < 3; ++i) {
    if (bytes[i] >= 0xF8) handle_realtime(bytes[i]);
  }

  handle_channel_message(packet.byte1, packet.byte2, packet.byte3);
}

static void poll_usb_midi() {
  midiEventPacket_t packet;
  do {
    packet = MidiUSB.read();
    if (packet.header != 0) handle_midi_packet(packet);
  } while (packet.header != 0);
}

static SerialMidiParser g_serialMidiParser;

#if SERIAL_MIDI_USE_PIO
// GP13 is not one of UART0's hardware RX mux positions. SerialPIO uses one PIO
// state machine so the PCB's GP13 MIDI_DATA connection remains valid at the
// standard 31,250-baud MIDI rate.
static SerialPIO g_serialMidi(NOPIN, PIN_SERIAL_MIDI_RX, 64);
#else
#define g_serialMidi Serial1
#endif

static void poll_serial_midi() {
  SerialMidiMessage message;
  while (g_serialMidi.available()) {
    const SerialMidiResult result = serial_midi_parser_feed(
        g_serialMidiParser, (uint8_t)g_serialMidi.read(), message);
    if (result == SERIAL_MIDI_REALTIME) {
      handle_realtime(message.status);
    } else if (result == SERIAL_MIDI_CHANNEL) {
      handle_channel_message(message.status, message.d1, message.d2);
    }
  }
}

void setup() {
  // Remain dark during reset/initialisation, then light once setup is complete.
  digitalWriteFast(PIN_STATUS_LED, LOW);
  pinMode(PIN_STATUS_LED, OUTPUT);

  // Start high-impedance so an already-running console can never contend with
  // the bridge during boot. The selected fixed role configures directions next.
  pinMode(PIN_COUNTER_BIT0, INPUT);
  pinMode(PIN_COUNTER_BIT1, INPUT);
  digitalWriteFast(PIN_COUNTER_BIT0, LOW);
  digitalWriteFast(PIN_COUNTER_BIT1, LOW);

#if BRIDGE_WIRE_ROLE == BRIDGE_ROLE_MIDI_ONLY
  wire_set_mode(WIRE_TAKEOVER);
#else
  present_counter(0);
#endif

  USB.setManufacturer("little-scale");
  USB.setProduct("Chipbridge");
  MidiUSB.setName("Chipbridge MIDI");
  MidiUSB.begin();

  serial_midi_parser_reset(g_serialMidiParser);
#if SERIAL_MIDI_USE_PIO
  g_serialMidi.begin(SERIAL_MIDI_BAUD);
#else
  Serial1.setTX(PIN_SERIAL_MIDI_TX);
  Serial1.setRX(PIN_SERIAL_MIDI_RX);
  Serial1.begin(SERIAL_MIDI_BAUD);
#endif

  digitalWriteFast(PIN_STATUS_LED, HIGH);
}

void loop() {
  poll_usb_midi();
  poll_serial_midi();
  service_activity_led();
#if BRIDGE_WIRE_ROLE == BRIDGE_ROLE_MIDI_ONLY
  wire_set_mode(WIRE_TAKEOVER);
  if (g_wireMode == WIRE_TAKEOVER) takeover_idle_check();
#elif BRIDGE_WIRE_ROLE == BRIDGE_ROLE_AUTO
  if (g_wireMode == WIRE_TAKEOVER) {
    takeover_idle_check();
    if (!takeover_auto_active()) wire_set_mode(WIRE_COUNTER);
  }
#else
  wire_set_mode(WIRE_COUNTER);
#endif
}

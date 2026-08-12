// SPDX-License-Identifier: GPL-2.0-only
#include <assert.h>

#include "../firmware/rp2040/chipbridge/serial_midi_parser.h"

static SerialMidiResult feed(SerialMidiParser& parser, SerialMidiMessage& message,
                             uint8_t byte) {
  return serial_midi_parser_feed(parser, byte, message);
}

int main() {
  SerialMidiParser parser;
  SerialMidiMessage message;
  serial_midi_parser_reset(parser);

  assert(feed(parser, message, 0x90) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 60) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 0xF8) == SERIAL_MIDI_REALTIME);
  assert(message.status == 0xF8);
  assert(feed(parser, message, 100) == SERIAL_MIDI_CHANNEL);
  assert(message.status == 0x90 && message.d1 == 60 && message.d2 == 100);

  // Running status reconstructs the next Note On.
  assert(feed(parser, message, 61) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 110) == SERIAL_MIDI_CHANNEL);
  assert(message.status == 0x90 && message.d1 == 61 && message.d2 == 110);

  // One-data-byte messages and their running status.
  assert(feed(parser, message, 0xC2) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 7) == SERIAL_MIDI_CHANNEL);
  assert(message.status == 0xC2 && message.d1 == 7 && message.d2 == 0);
  assert(feed(parser, message, 8) == SERIAL_MIDI_CHANNEL);
  assert(message.status == 0xC2 && message.d1 == 8 && message.d2 == 0);

  // System Common cancels running status.
  assert(feed(parser, message, 0xF1) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 0x01) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 9) == SERIAL_MIDI_NONE);

  // SysEx payload is ignored, but Real-Time remains visible.
  assert(feed(parser, message, 0xF0) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 0x01) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 0xFA) == SERIAL_MIDI_REALTIME);
  assert(message.status == 0xFA);
  assert(feed(parser, message, 0xF7) == SERIAL_MIDI_NONE);

  // Reset is emitted and clears any partial message.
  assert(feed(parser, message, 0x90) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 64) == SERIAL_MIDI_NONE);
  assert(feed(parser, message, 0xFF) == SERIAL_MIDI_REALTIME);
  assert(feed(parser, message, 127) == SERIAL_MIDI_NONE);
}

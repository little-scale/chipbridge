// Minimal byte-stream MIDI 1.0 parser for a 31,250-baud UART input.
//
// Expands running status, lets System Real-Time bytes interrupt any message,
// and ignores SysEx/System Common payloads that this bridge does not consume.
// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include <stdint.h>

enum SerialMidiResult {
  SERIAL_MIDI_NONE = 0,
  SERIAL_MIDI_REALTIME,
  SERIAL_MIDI_CHANNEL,
};

struct SerialMidiMessage {
  uint8_t status;
  uint8_t d1;
  uint8_t d2;
};

struct SerialMidiParser {
  uint8_t runningStatus;
  uint8_t pendingStatus;
  uint8_t data[2];
  uint8_t dataCount;
  uint8_t dataNeeded;
  uint8_t systemRemaining;
  bool inSysEx;
};

static inline void serial_midi_parser_reset(SerialMidiParser& parser) {
  parser.runningStatus = 0;
  parser.pendingStatus = 0;
  parser.data[0] = 0;
  parser.data[1] = 0;
  parser.dataCount = 0;
  parser.dataNeeded = 0;
  parser.systemRemaining = 0;
  parser.inSysEx = false;
}

static inline uint8_t serial_midi_channel_data_length(uint8_t status) {
  const uint8_t high = status & 0xF0;
  return (high == 0xC0 || high == 0xD0) ? 1 : 2;
}

static inline SerialMidiResult serial_midi_parser_feed(
    SerialMidiParser& parser, uint8_t byte, SerialMidiMessage& message) {
  // Real-Time bytes may appear between any two bytes without disturbing the
  // message being assembled or its running status.
  if (byte >= 0xF8) {
    message.status = byte;
    message.d1 = 0;
    message.d2 = 0;
    if (byte == 0xFF) serial_midi_parser_reset(parser);
    return SERIAL_MIDI_REALTIME;
  }

  if (byte & 0x80) {
    if (byte < 0xF0) {
      parser.runningStatus = byte;
      parser.pendingStatus = byte;
      parser.dataCount = 0;
      parser.dataNeeded = serial_midi_channel_data_length(byte);
      parser.systemRemaining = 0;
      parser.inSysEx = false;
      return SERIAL_MIDI_NONE;
    }

    // System Common cancels running status.
    parser.runningStatus = 0;
    parser.pendingStatus = 0;
    parser.dataCount = 0;
    parser.dataNeeded = 0;
    parser.inSysEx = byte == 0xF0;
    switch (byte) {
      case 0xF1: parser.systemRemaining = 1; break;
      case 0xF2: parser.systemRemaining = 2; break;
      case 0xF3: parser.systemRemaining = 1; break;
      default:   parser.systemRemaining = 0; break;
    }
    return SERIAL_MIDI_NONE;
  }

  if (parser.inSysEx) return SERIAL_MIDI_NONE;
  if (parser.systemRemaining != 0) {
    --parser.systemRemaining;
    return SERIAL_MIDI_NONE;
  }

  if (parser.pendingStatus == 0) {
    if (parser.runningStatus == 0) return SERIAL_MIDI_NONE;
    parser.pendingStatus = parser.runningStatus;
    parser.dataNeeded = serial_midi_channel_data_length(parser.runningStatus);
    parser.dataCount = 0;
  }

  parser.data[parser.dataCount++] = byte;
  if (parser.dataCount < parser.dataNeeded) return SERIAL_MIDI_NONE;

  message.status = parser.pendingStatus;
  message.d1 = parser.data[0];
  message.d2 = parser.dataNeeded == 2 ? parser.data[1] : 0;
  parser.pendingStatus = parser.runningStatus;
  parser.dataCount = 0;
  parser.dataNeeded = serial_midi_channel_data_length(parser.runningStatus);
  return SERIAL_MIDI_CHANNEL;
}

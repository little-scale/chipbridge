// Portable USB-MIDI -> SMSGGDJ/genmddj takeover frame normalisation.
// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <stdint.h>

enum MidiEvtType {
  EVT_NOTE_OFF = 1,
  EVT_NOTE_ON  = 2,
  EVT_CC       = 3,
  EVT_PGM      = 4,
  EVT_BEND     = 5,
  EVT_PANIC    = 7,
};

struct MidiEvt {
  uint8_t status;
  uint8_t d1;
  uint8_t d2;
};

static const int MIDI_FRAME_BITS = 25;

// Leading flag=1 followed by status:d1:d2, left-justified for MSB-first shift.
static inline uint32_t midi_frame_word(const MidiEvt& e) {
  const uint32_t frame = (1u << 24) | ((uint32_t)e.status << 16) |
                         ((uint32_t)e.d1 << 8) | e.d2;
  return frame << (32 - MIDI_FRAME_BITS);
}

// Expand the USB-MIDI channel message into the bounded three-byte wire event.
// NoteOn velocity 0 becomes NoteOff; CC 120/123 becomes Panic. NoteOff and
// Panic are critical; if overflow evicts an earlier release, the queue layer
// emits a recovery Panic for that channel.
static inline bool midi_normalise(uint8_t statusHi, uint8_t channel,
                                  uint8_t d1, uint8_t d2,
                                  MidiEvt& out, bool& critical) {
  uint8_t type;
  critical = false;

  switch (statusHi) {
    case 0x80:
      type = EVT_NOTE_OFF;
      critical = true;
      break;
    case 0x90:
      if (d2 == 0) {
        type = EVT_NOTE_OFF;
        critical = true;
      } else {
        type = EVT_NOTE_ON;
      }
      break;
    case 0xB0:
      if (d1 == 120 || d1 == 123) {
        type = EVT_PANIC;
        d1 = 0;
        d2 = 0;
        critical = true;
      } else {
        type = EVT_CC;
      }
      break;
    case 0xC0:
      type = EVT_PGM;
      d2 = 0;
      break;
    case 0xE0:
      type = EVT_BEND;
      break;
    default:
      return false;  // poly aftertouch and channel pressure are not forwarded
  }

  out.status = (uint8_t)((type << 4) | (channel & 0x0F));
  out.d1 = d1;
  out.d2 = d2;
  return true;
}

#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Build the five general Pico-family runtime-switching releases.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/releases/rp2040"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/chipbridge-rp2040-release.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "error: arduino-cli not found" >&2
  exit 1
fi

mkdir -p "$OUT"
SKETCH="$ROOT/firmware/rp2040/chipbridge"

build_pico() {
  local name="$1"
  local fqbn="$2"
  local build="$WORK/$name"
  echo ">> $name (MIDI clock + channel-voice takeover)"
  arduino-cli compile --fqbn "$fqbn" --build-path "$build" \
    --build-property "compiler.cpp.extra_flags=-DBRIDGE_WIRE_ROLE=BRIDGE_ROLE_AUTO" \
    "$SKETCH"
  cp "$build/chipbridge.ino.uf2" \
    "$OUT/chipbridge-$name.uf2"
}

build_pico pico rp2040:rp2040:rpipico
build_pico pico2 rp2040:rp2040:rpipico2
build_pico pico2w rp2040:rp2040:rpipico2w
build_pico rp2040-zero rp2040:rp2040:waveshare_rp2040_zero
build_pico xiao-rp2040 rp2040:rp2040:seeed_xiao_rp2040

(
  cd "$OUT"
  shasum -a 256 \
    chipbridge-pico.uf2 \
    chipbridge-pico2.uf2 \
    chipbridge-pico2w.uf2 \
    chipbridge-rp2040-zero.uf2 \
    chipbridge-xiao-rp2040.uf2 > SHA256SUMS.txt
)

echo ">> wrote five runtime-switching images to releases/rp2040/"

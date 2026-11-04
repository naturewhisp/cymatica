#!/bin/bash
# Three repetitions per MMCSS usage, in rotated order. Run from the repository root.
for rep in 1 2 3; do
  for u in default games pro_audio; do
    build/x64-release/apps/cymatica_game/cymatica_game.exe --cacp-run device_probe_v1 --device-usage $u > artifacts/m25_device_baseline/${u}_r${rep}.log 2>&1
    cp artifacts/cacp/device_probe_v1_${u}_manifest.json artifacts/m25_device_baseline/${u}_r${rep}.json
  done
done

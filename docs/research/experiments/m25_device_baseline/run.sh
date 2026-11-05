#!/bin/bash
# Three repetitions per MMCSS usage, with the order rotated at each repetition. Run from the repository root.
# Fail-closed: stops at the first run that fails, is not MEASURED or did not write its manifest (DIF-M2.5.1a-01).
set -u
out=artifacts/m25_device_baseline
mkdir -p "$out"
rep=0
for order in "default games pro_audio" "games pro_audio default" "pro_audio default games"; do
  rep=$((rep + 1))
  for u in $order; do
    manifest="artifacts/cacp/device_probe_v1_${u}_manifest.json"
    rm -f "$manifest"
    build/x64-release/apps/cymatica_game/cymatica_game.exe --cacp-run device_probe_v1 --device-usage "$u" \
      > "$out/${u}_r${rep}.log" 2>&1 || { echo "run failed: $u r$rep"; exit 1; }
    python docs/research/experiments/m25_device_baseline/check_manifest.py "$manifest" \
      || { echo "manifest not valid: $u r$rep"; exit 1; }
    cp "$manifest" "$out/${u}_r${rep}.json"
  done
done

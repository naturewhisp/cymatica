#!/bin/bash
# Three repetitions per MMCSS usage, with the order rotated at each repetition. Run from the repository root.
# Each series goes to a new directory, whose name is printed at the end; pass it to summarize.py.
# Fail-closed: any failed run, invalid manifest or failed copy stops the series with a non-zero exit code
# (DIF-M2.5.1a-01, DIF-M2.5.1a-R2-01). GAME_EXE, CACP_DIR and SERIES_ROOT exist for test_series_tools.py.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
game="${GAME_EXE:-build/x64-release/apps/cymatica_game/cymatica_game.exe}"
cacp="${CACP_DIR:-artifacts/cacp}"
root="${SERIES_ROOT:-artifacts/m25_device_baseline}"
mkdir -p "$root"
series="$root/series_$(date +%Y%m%d_%H%M%S)"
mkdir "$series"
rep=0
for order in "default games pro_audio" "games pro_audio default" "pro_audio default games"; do
  rep=$((rep + 1))
  for u in $order; do
    manifest="$cacp/device_probe_v1_${u}_manifest.json"
    rm -f "$manifest"
    "$game" --cacp-run device_probe_v1 --device-usage "$u" > "$series/${u}_r${rep}.log" 2>&1
    python "$here/check_manifest.py" "$manifest"
    cp "$manifest" "$series/${u}_r${rep}.json"
  done
done
echo "$series"

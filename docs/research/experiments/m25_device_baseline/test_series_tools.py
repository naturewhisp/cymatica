"""Negative checks of run.sh and summarize.py with a fake game: no audio device needed (DIF-M2.5.1a-R2-01).

Run from the repository root: python docs/research/experiments/m25_device_baseline/test_series_tools.py
"""
import json
import os
import shutil
import stat
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
RUN = os.path.join(HERE, 'run.sh').replace('\\', '/')
SUMMARIZE = os.path.join(HERE, 'summarize.py')

FAKE_GAME = """#!/bin/bash
# $4 is the usage. FAIL_AT=<nth call> fails that call, STATUS sets the manifest status.
count=$(($(cat "$CACP_DIR/count" 2>/dev/null || echo 0) + 1))
echo "$count" > "$CACP_DIR/count"
if [ "$count" = "${FAIL_AT:-0}" ]; then exit 1; fi
status="${STATUS:-MEASURED}"
if [ "$count" != "${STATUS_AT:-0}" ] && [ -n "${STATUS_AT:-}" ]; then status=MEASURED; fi
if [ "$count" = "${NO_MANIFEST_AT:-0}" ]; then exit 0; fi
printf '{"scenario":"device_probe_v1","build_id":"test","status":"%s","manifest_write_success":true,"usage":"%s",' "$status" "$4" > "$CACP_DIR/device_probe_v1_$4_manifest.json"
printf '"device":{"mmcss_active":true},"run":{"elapsed_s":1.0,"dropped_events":0},' >> "$CACP_DIR/device_probe_v1_$4_manifest.json"
printf '"callback":{"p999_ms":0.1,"max_ms":0.2,"blocks_exceeding_block_period":0,"max_interval_ms":10.0,"suspected_underruns":0}}' >> "$CACP_DIR/device_probe_v1_$4_manifest.json"
"""

FAKE_CP = """#!/bin/bash
# Fails the fifth copy, then behaves like cp.
count=$(($(cat "$CACP_DIR/copies" 2>/dev/null || echo 0) + 1))
echo "$count" > "$CACP_DIR/copies"
if [ "$count" = "5" ]; then exit 1; fi
exec /usr/bin/cp "$@"
"""

results = []


def check(name, condition):
    results.append(condition)
    print(f"{'ok  ' if condition else 'FAIL'} {name}")


def write_script(path, text):
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)
    os.chmod(path, os.stat(path).st_mode | stat.S_IEXEC)


def run_series(base, extra_env=None, fake_cp=False):
    cacp = os.path.join(base, 'cacp')
    root = os.path.join(base, 'series')
    shutil.rmtree(cacp, ignore_errors=True)
    shutil.rmtree(root, ignore_errors=True)
    os.makedirs(cacp)
    env = dict(os.environ, GAME_EXE=os.path.join(base, 'fake_game.sh').replace('\\', '/'),
               CACP_DIR=cacp.replace('\\', '/'), SERIES_ROOT=root.replace('\\', '/'), **(extra_env or {}))
    if fake_cp:
        env['PATH'] = os.path.join(base, 'bin') + os.pathsep + env['PATH']
    done = subprocess.run([shutil.which('bash'), RUN], env=env, capture_output=True, text=True)
    return done.returncode, os.path.join(root, os.listdir(root)[0])


def summarize(directory):
    return subprocess.run(['python', SUMMARIZE, directory], capture_output=True, text=True).returncode


with tempfile.TemporaryDirectory() as base:
    write_script(os.path.join(base, 'fake_game.sh'), FAKE_GAME)
    os.makedirs(os.path.join(base, 'bin'))
    write_script(os.path.join(base, 'bin', 'cp'), FAKE_CP)

    code, series = run_series(base)
    check('complete series exits 0', code == 0)
    check('complete series is summarised', summarize(series) == 0)

    code, series = run_series(base, {'FAIL_AT': '4'})
    check('failed run stops the series', code != 0)
    check('partial series after a failed run is rejected', summarize(series) == 1)

    code, series = run_series(base, {'STATUS': 'INVALID', 'STATUS_AT': '3'})
    check('INVALID manifest stops the series', code != 0)

    code, series = run_series(base, {'NO_MANIFEST_AT': '2'})
    check('missing manifest stops the series', code != 0)

    code, series = run_series(base, fake_cp=True)
    check('failed copy stops the series', code != 0)
    check('partial series after a failed copy is rejected', summarize(series) == 1)

    code, series = run_series(base)
    path = os.path.join(series, 'games_r2.json')
    original = open(path, encoding='utf-8').read()

    def tampered(change):
        manifest = json.loads(original)
        change(manifest)
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(manifest, f)
        outcome = summarize(series)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(original)
        return outcome

    check('mixed builds are rejected', tampered(lambda m: m.update(build_id='other')) == 1)
    check('wrong usage is rejected', tampered(lambda m: m.update(usage='default')) == 1)
    check('status INVALID is rejected', tampered(lambda m: m.update(status='INVALID')) == 1)
    check('unwritten manifest is rejected', tampered(lambda m: m.update(manifest_write_success=False)) == 1)
    extra = os.path.join(series, 'stray.json')
    open(extra, 'w', encoding='utf-8').write('{}')
    check('unexpected manifest is rejected', summarize(series) == 1)
    os.remove(extra)
    check('restored series is summarised again', summarize(series) == 0)

print(f'{sum(results)}/{len(results)} checks passed')
raise SystemExit(0 if all(results) else 1)

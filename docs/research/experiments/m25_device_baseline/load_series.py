"""Controlled load on the device (M2.5.1d): python load_series.py <output_dir>

Runs device_probe_v1 at rest and with a fixed synthetic load, each with and without MMCSS, always with PCM capture
and loopback. The load is one busy process per logical CPU at normal priority, each walking 64 MB of memory.
It does not exercise the GPU. CPU use is sampled once a second from GetSystemTimes. Every run stops the series on failure.
"""
import ctypes
import json
import math
import multiprocessing
import os
import shutil
import subprocess
import sys
import threading
import time

GAME = os.path.join('build', 'x64-release', 'apps', 'cymatica_game', 'cymatica_game.exe')
CACP = os.path.join('artifacts', 'cacp')
HERE = os.path.dirname(os.path.abspath(__file__))
RUNS = (('rest', 'games'), ('load', 'games'), ('rest', 'default'), ('load', 'default'))
# Validity of the experiment, not a hardware criterion: that one is decided at CP-1
LOAD_VALIDITY_V1 = {'version': 'load_validity_v1', 'min_samples': 150,
                    'load_min_median_percent': 95.0, 'rest_max_median_percent': 20.0}


def burn(stop):
    block = bytearray(64 * 1024 * 1024)
    position = 0
    while not stop.is_set():
        for _ in range(100000):
            block[position] = (block[position] + 1) & 0xFF
            position = (position + 4099) % len(block)


def system_times():
    idle, kernel, user = (ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong())
    ctypes.windll.kernel32.GetSystemTimes(ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user))
    return idle.value, kernel.value + user.value  # kernel time includes idle time


def sample_cpu(path, stop):
    with open(path, 'w', encoding='utf-8') as out:
        last_idle, last_total = system_times()
        while not stop.wait(1.0):
            idle, total = system_times()
            busy = 100.0 * (1.0 - (idle - last_idle) / max(total - last_total, 1))
            out.write(f'{busy:.1f}\n')
            out.flush()
            last_idle, last_total = idle, total


def run(condition, usage, folder):
    tag = f'{condition}_{usage}'
    stop = multiprocessing.Event()
    workers = []
    if condition == 'load':
        workers = [multiprocessing.Process(target=burn, args=(stop,), daemon=True) for _ in range(os.cpu_count())]
        for worker in workers:
            worker.start()
        time.sleep(5)
    sampling = threading.Event()
    sampler = threading.Thread(target=sample_cpu, args=(os.path.join(folder, f'{tag}_cpu.txt'), sampling))
    sampler.start()
    try:
        manifest = os.path.join(CACP, f'device_probe_v1_{usage}_manifest.json')
        if os.path.exists(manifest):
            os.remove(manifest)
        with open(os.path.join(folder, f'{tag}.log'), 'w', encoding='utf-8') as log:
            code = subprocess.run([GAME, '--cacp-run', 'device_probe_v1', '--device-usage', usage,
                                   '--device-capture-pcm', '--device-loopback'], stdout=log, stderr=log).returncode
    finally:
        sampling.set()
        sampler.join()
        stop.set()
        for worker in workers:
            worker.join(5)
    if code != 0:
        raise SystemExit(f'{tag}: exit code {code}')
    shutil.copy(manifest, os.path.join(folder, f'{tag}.manifest.json'))
    capture = os.path.join(folder, f'{tag}_loopback.f32')
    shutil.move(os.path.join(CACP, f'device_probe_v1_{usage}_loopback.f32'), capture)
    analysis = os.path.join(folder, f'{tag}.analysis.json')
    done = subprocess.run([sys.executable, os.path.join(HERE, 'analyze_loopback.py'), capture,
                           os.path.join(CACP, 'device_probe_v1_reference.f32'), analysis])
    if done.returncode != 0:
        raise SystemExit(f'{tag}: analysis failed')


def cpu_samples_valid(cpu):
    return all(math.isfinite(value) and 0.0 <= value <= 100.0 for value in cpu)


def load_errors(condition, cpu):
    """Experimental validity of one run (load_validity_v1). It says the load was the declared one, nothing about audio."""
    if not cpu_samples_valid(cpu):
        return ['CPU samples not finite or outside 0..100 %']
    if len(cpu) < LOAD_VALIDITY_V1['min_samples']:
        return [f'{len(cpu)} CPU samples, at least {LOAD_VALIDITY_V1["min_samples"]} required']
    median = sorted(cpu)[len(cpu) // 2]
    if condition == 'load' and median < LOAD_VALIDITY_V1['load_min_median_percent']:
        return [f'load not reached: median CPU {median:.1f} %']
    if condition == 'rest' and median > LOAD_VALIDITY_V1['rest_max_median_percent']:
        return [f'machine not at rest: median CPU {median:.1f} %']
    return []


# Fields every run must carry, with their type (DIF-M2.5.1c-01): a missing or mistyped field makes the run INVALID,
# so an absent metric is never read as zero anomalies. 'number' excludes booleans; 'count' is a non-negative
# integer, for counters of events and anomalies (OSS-M2.5.1c-R2-01).
REQUIRED_MANIFEST_FIELDS = {
    ('device', 'mmcss_active'): 'bool',
    ('run', 'dropped_events'): 'count',
    ('run', 'max_coordinator_gap_ms'): 'number',
    ('pcm', 'device_matches_golden'): 'bool',
    ('callback', 'mean_ms'): 'number',
    ('callback', 'p50_ms'): 'number',
    ('callback', 'p999_ms'): 'number',
    ('callback', 'max_ms'): 'number',
    ('callback', 'blocks_exceeding_block_period'): 'count',
    ('callback', 'max_interval_ms'): 'number',
    ('callback', 'suspected_underruns'): 'count',
}
REQUIRED_ANALYSIS_FIELDS = {
    ('gain_db',): 'number',
    ('residual_db',): 'number',
    ('alignment_slips',): 'list',
    ('residual_bursts',): 'count',
    ('unexpected_silences',): 'count',
}


def has_type(value, kind):
    if kind == 'bool':
        return isinstance(value, bool)
    if kind == 'list':
        return isinstance(value, list)
    if isinstance(value, bool):
        return False
    if kind == 'int':
        return isinstance(value, int)
    if kind == 'count':
        return isinstance(value, int) and value >= 0
    return isinstance(value, (int, float)) and math.isfinite(value)


def field_errors(data, required, label):
    """Names of the required fields that are missing or of the wrong type."""
    if not isinstance(data, dict):
        return [f'{label} is not a JSON object']
    errors = []
    for path, kind in required.items():
        value = data
        for key in path:
            value = value.get(key) if isinstance(value, dict) else None
        if value is None or not has_type(value, kind):
            errors.append(f'{label} field {".".join(path)} missing or not {kind}')
    return errors


def summarize_series(target, manifest_suffix='.manifest.json'):
    """Writes summary.json. The status is INVALID as soon as one run is missing or did not have the declared load."""
    summary = {'load_validity': LOAD_VALIDITY_V1, 'status': 'MEASURED', 'errors': [], 'runs': {}}
    for condition_name, usage_name in RUNS:
        tag = f'{condition_name}_{usage_name}'
        try:
            with open(os.path.join(target, tag + manifest_suffix), encoding='utf-8') as f:
                manifest_data = json.load(f)
            with open(os.path.join(target, f'{tag}.analysis.json'), encoding='utf-8') as f:
                analysis_data = json.load(f)
            with open(os.path.join(target, f'{tag}_cpu.txt'), encoding='utf-8') as f:
                cpu = [float(line) for line in f if line.strip()]
        except (OSError, ValueError) as error:
            summary['errors'].append(f'{tag}: {error}')
            continue
        summary['errors'] += [f'{tag}: {error}' for error in load_errors(condition_name, cpu)]
        cpu = sorted(cpu) if cpu_samples_valid(cpu) else []
        malformed = (field_errors(manifest_data, REQUIRED_MANIFEST_FIELDS, 'manifest') +
                     field_errors(analysis_data, REQUIRED_ANALYSIS_FIELDS, 'analysis'))
        if malformed:  # INVALID with a reason, never an exception or a default (OSS-M2.5.1d-R4-01, DIF-M2.5.1c-01)
            summary['errors'] += [f'{tag}: malformed {error}' for error in malformed]
            continue
        summary['runs'][tag] = run_summary(manifest_data, analysis_data, cpu)
        expected_mmcss = usage_name != 'default'
        if manifest_data['device']['mmcss_active'] != expected_mmcss:
            summary['errors'].append(f'{tag}: MMCSS is not the requested one')
        if manifest_data.get('status') != 'MEASURED' or analysis_data.get('status') != 'MEASURED':
            summary['errors'].append(f'{tag}: run or analysis not MEASURED')

    if summary['errors']:
        summary['status'] = 'INVALID'
    with open(os.path.join(target, 'summary.json'), 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2, allow_nan=False)
    return summary


def run_summary(manifest_data, analysis_data, cpu):
    callback = manifest_data['callback']
    return {
        'cpu_samples': len(cpu),
        'cpu_median_percent': cpu[len(cpu) // 2] if cpu else None,
        'cpu_min_percent': cpu[0] if cpu else None,
        'mmcss': manifest_data['device']['mmcss_active'],
        'dropped_events': manifest_data['run']['dropped_events'],
        'pcm_matches_golden': manifest_data['pcm']['device_matches_golden'],
        'mean_ms': callback['mean_ms'],
        'p50_ms': callback['p50_ms'],
        'p999_ms': callback['p999_ms'],
        'max_ms': callback['max_ms'],
        'over_period': callback['blocks_exceeding_block_period'],
        'max_interval_ms': callback['max_interval_ms'],
        'suspected_underruns': callback['suspected_underruns'],
        'max_coordinator_gap_ms': manifest_data['run']['max_coordinator_gap_ms'],
        'gain_db': analysis_data['gain_db'],
        'residual_db': analysis_data['residual_db'],
        'slips': len(analysis_data['alignment_slips']),
        'bursts': analysis_data['residual_bursts'],
        'silences': analysis_data['unexpected_silences'],
    }


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    target = sys.argv[1]
    os.makedirs(target)
    for condition_name, usage_name in RUNS:
        print(condition_name, usage_name, flush=True)
        run(condition_name, usage_name, target)
    summary = summarize_series(target)
    print(json.dumps(summary, indent=2))
    sys.exit(0 if summary['status'] == 'MEASURED' else 1)

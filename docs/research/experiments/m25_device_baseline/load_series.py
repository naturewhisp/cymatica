"""Controlled load on the device (M2.5.1d): python load_series.py <output_dir>

Runs device_probe_v1 at rest and with a fixed synthetic load, each with and without MMCSS, always with PCM capture
and loopback. The load is one busy process per logical CPU at normal priority, each walking 64 MB of memory.
It does not exercise the GPU. CPU use is sampled once a second from GetSystemTimes. Every run stops the series on failure.
"""
import ctypes
import json
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


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    target = sys.argv[1]
    os.makedirs(target)
    for condition_name, usage_name in RUNS:
        print(condition_name, usage_name, flush=True)
        run(condition_name, usage_name, target)
    summary = {}
    for condition_name, usage_name in RUNS:
        tag = f'{condition_name}_{usage_name}'
        with open(os.path.join(target, f'{tag}.manifest.json'), encoding='utf-8') as f:
            manifest_data = json.load(f)
        with open(os.path.join(target, f'{tag}.analysis.json'), encoding='utf-8') as f:
            analysis_data = json.load(f)
        with open(os.path.join(target, f'{tag}_cpu.txt'), encoding='utf-8') as f:
            cpu = sorted(float(line) for line in f if line.strip())
        callback = manifest_data['callback']
        summary[tag] = {
            'cpu_median_percent': round(cpu[len(cpu) // 2], 1) if cpu else None,
            'mmcss': manifest_data['device']['mmcss_active'],
            'dropped_events': manifest_data['run']['dropped_events'],
            'pcm_matches_golden': manifest_data['pcm']['device_matches_golden'],
            'p999_ms': callback['p999_ms'],
            'max_ms': callback['max_ms'],
            'over_period': callback['blocks_exceeding_block_period'],
            'max_interval_ms': callback['max_interval_ms'],
            'suspected_underruns': callback['suspected_underruns'],
            'gain_db': analysis_data.get('gain_db'),
            'residual_db': analysis_data.get('residual_db'),
            'slips': len(analysis_data.get('alignment_slips', [])),
            'bursts': analysis_data.get('residual_bursts'),
            'silences': analysis_data.get('unexpected_silences'),
        }
    with open(os.path.join(target, 'summary.json'), 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2)
    print(json.dumps(summary, indent=2))

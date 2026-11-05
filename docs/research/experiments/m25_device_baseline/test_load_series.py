"""Negative checks of load_series.py on files only: no device and no load are needed (DIF-M2.5.1d-LOAD-02).

Run from the repository root: python docs/research/experiments/m25_device_baseline/test_load_series.py
"""
import json
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from load_series import RUNS, load_errors, summarize_series  # noqa: E402

ARCHIVE = os.path.join(HERE, 'load_series_m251d')
results = []


def check(name, condition):
    results.append(bool(condition))
    print(f"{'ok  ' if condition else 'FAIL'} {name}")


def rewrite_cpu(folder, tag, values):
    with open(os.path.join(folder, f'{tag}_cpu.txt'), 'w', encoding='utf-8') as f:
        f.write(''.join(f'{v}\n' for v in values))


check('load run at 100 % is valid', load_errors('load', [100.0] * 170) == [])
check('load run below the threshold is rejected', load_errors('load', [60.0] * 170) != [])
check('rest run at 2 % is valid', load_errors('rest', [2.0] * 170) == [])
check('busy machine at rest is rejected', load_errors('rest', [50.0] * 170) != [])
check('no CPU samples is rejected', load_errors('load', []) != [])
check('too few CPU samples is rejected', load_errors('rest', [2.0] * 10) != [])

with tempfile.TemporaryDirectory() as base:
    folder = os.path.join(base, 'series')
    shutil.copytree(ARCHIVE, folder)
    check('archived series is MEASURED', summarize_series(folder, '.manifest.json.txt')['status'] == 'MEASURED')

    rewrite_cpu(folder, 'load_games', [40.0] * 170)
    check('insufficient load makes the series INVALID', summarize_series(folder, '.manifest.json.txt')['status'] == 'INVALID')
    shutil.copy(os.path.join(ARCHIVE, 'load_games_cpu.txt'), folder)

    rewrite_cpu(folder, 'rest_default', [])
    check('missing CPU telemetry makes the series INVALID', summarize_series(folder, '.manifest.json.txt')['status'] == 'INVALID')
    shutil.copy(os.path.join(ARCHIVE, 'rest_default_cpu.txt'), folder)

    os.remove(os.path.join(folder, 'load_default_cpu.txt'))
    check('missing CPU file makes the series INVALID', summarize_series(folder, '.manifest.json.txt')['status'] == 'INVALID')
    shutil.copy(os.path.join(ARCHIVE, 'load_default_cpu.txt'), folder)

    path = os.path.join(folder, 'rest_games.analysis.json')
    with open(path, encoding='utf-8') as f:
        analysis = json.load(f)
    analysis['status'] = 'INVALID'
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(analysis, f)
    check('INVALID analysis makes the series INVALID', summarize_series(folder, '.manifest.json.txt')['status'] == 'INVALID')

check('all conditions are covered', len(RUNS) == 4)
print(f'{sum(results)}/{len(results)} checks passed')
sys.exit(0 if all(results) else 1)

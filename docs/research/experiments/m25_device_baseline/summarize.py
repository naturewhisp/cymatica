"""Summary table of one complete device_probe_v1 series: python summarize.py <series_dir>.

Exits 1, printing no table, unless the directory holds exactly the nine expected manifests, all MEASURED,
all written completely, each with the usage of its file name and all from the same build (DIF-M2.5.1a-R2-01).
"""
import glob
import json
import os
import sys

USAGES = ('default', 'games', 'pro_audio')
EXPECTED = {f'{usage}_r{rep}.json' for usage in USAGES for rep in (1, 2, 3)}


def load_series(directory):
    """Returns (manifests by file name, errors)."""
    found = {os.path.basename(path) for path in glob.glob(os.path.join(directory, '*.json'))}
    errors = [f'missing {name}' for name in sorted(EXPECTED - found)]
    errors += [f'unexpected {name}' for name in sorted(found - EXPECTED)]
    manifests = {}
    for name in sorted(found & EXPECTED):
        try:
            with open(os.path.join(directory, name), encoding='utf-8') as f:
                manifest = json.load(f)
        except (OSError, ValueError) as error:
            errors.append(f'{name}: unreadable ({error})')
            continue
        manifests[name] = manifest
        if manifest.get('scenario') != 'device_probe_v1':
            errors.append(f'{name}: wrong scenario')
        if manifest.get('status') != 'MEASURED':
            errors.append(f'{name}: status {manifest.get("status")}')
        if manifest.get('manifest_write_success') is not True:
            errors.append(f'{name}: manifest not written')
        if manifest.get('usage') != name.rsplit('_r', 1)[0]:
            errors.append(f'{name}: usage {manifest.get("usage")}')
    builds = {m.get('build_id') for m in manifests.values()}
    if len(builds) > 1:
        errors.append(f'mixed builds: {sorted(str(b) for b in builds)}')
    return manifests, errors


def main():
    if len(sys.argv) != 2:
        print('usage: summarize.py <series_dir>')
        return 2
    manifests, errors = load_series(sys.argv[1])
    if errors:
        print('series rejected:')
        for error in errors:
            print(f'  {error}')
        return 1
    print('run           mmcss elapsed_s  dropped p999_ms max_ms  over_period max_interval_ms suspected')
    for name, m in manifests.items():
        r, c = m['run'], m['callback']
        print(f"{name[:-5]:13} {str(m['device']['mmcss_active']):5} {r['elapsed_s']:9.3f} {r['dropped_events']:7} "
              f"{c['p999_ms']:7.2f} {c['max_ms']:6.2f} {c['blocks_exceeding_block_period']:11} {c['max_interval_ms']:15.2f} {c['suspected_underruns']:9}")
    return 0


if __name__ == '__main__':
    sys.exit(main())

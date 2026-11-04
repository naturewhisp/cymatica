"""Summary table of the device_probe_v1 manifests in this folder."""
import glob, json, os

here = os.path.dirname(os.path.abspath(__file__))
print('run           mmcss elapsed_s  dropped p999_ms max_ms  over_period max_interval_ms suspected')
for path in sorted(glob.glob(os.path.join(here, '*.json'))):
    m = json.load(open(path, encoding='utf-8'))
    r, c = m['run'], m['callback']
    print(f"{os.path.basename(path)[:-5]:13} {str(m['device']['mmcss_active']):5} {r['elapsed_s']:9.3f} {r['dropped_events']:7} "
          f"{c['p999_ms']:7.2f} {c['max_ms']:6.2f} {c['blocks_exceeding_block_period']:11} {c['max_interval_ms']:15.2f} {c['suspected_underruns']:9}")

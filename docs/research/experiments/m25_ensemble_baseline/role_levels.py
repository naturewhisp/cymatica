import json, os, statistics, sys

root = sys.argv[1]
m = json.load(open(os.path.join(root, 'artifacts', 'cacp', 'procedural_v1_manifest.json'), encoding='utf-8'))
print('build_id', m['build_id'], 'pcm', m['pcm_content_hash'], 'status', m['presence_per_phrase']['status'], 'validated', m['presence_per_phrase']['validated'])
groups = {'A1 (0-3)': range(0, 4), 'A2 (4-7)': range(4, 8), 'B (8-15)': range(8, 16), "A' (16-23)": range(16, 24)}
ph = {p['phrase']: p for p in m['presence_per_phrase']['phrases']}
for label, idx in groups.items():
    out = []
    for role in ('pulso', 'corpo', 'trama', 'vettore'):
        vals = [ph[i]['roles'][role]['stem_to_mix_db'] for i in idx if ph[i]['roles'][role]['stem_to_mix_db'] is not None]
        rms = [ph[i]['roles'][role]['rms_dbfs'] for i in idx if ph[i]['roles'][role]['rms_dbfs'] is not None]
        if vals:
            out.append(f'{role} {min(vals):.1f}..{max(vals):.1f} dB (rms {statistics.mean(rms):.1f})')
        else:
            out.append(f'{role} silence')
    print(label, '|', ' | '.join(out))

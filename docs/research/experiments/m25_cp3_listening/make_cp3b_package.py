"""Second CP-3 expert listening package (docs/research/M2_5_CP3_Expert_Listening_2_Hypotheses.md).

Usage: python -I make_cp3b_package.py --date 2026-11-24 --commit <sha> --checks cp3b_package_checks.json
Needs artifacts/cacp/tempo_ramp_v1 (I9 bis: k3_B2, bass_line, piece_B2) and tempo_ramp_v1_i9 (k3_p). Output as make_cp3_package.py.
"""
import argparse
import hashlib
import json
import random
import re
import secrets
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'm25_cp3_motif'))
import make_cp3_package as MP  # noqa: E402
import new_motif as NM  # noqa: E402

C, T, S, BP, CM3, FS = MP.C, MP.T, MP.S, MP.BP, MP.CM3, 48000
ORDER = 'MLBID'


def gains(db):
    edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]

    def g(n):
        v = np.full(n, 10 ** (db[2] / 20))
        for s in range(3):
            v[edges[s]:edges[s + 1]] = 10 ** (db[s] / 20)
        return v[:, None]
    return g


def material(c):
    new, i9 = c / 'tempo_ramp_v1', c / 'tempo_ramp_v1_i9'
    today, n2b = T.wav(i9 / 'k3_p.wav'), T.wav(new / 'k3_B2.wav')
    n = min(len(today), len(n2b))
    bass = np.column_stack([CM3.mono(T.wav(new / 'bass_line.wav'), n)] * 2)
    hi = bass * gains(NM.GAIN_DB)(n)
    v2m = json.loads((HERE.parent / 'm25_cp3_bass/kick_bass_joint.json').read_text())['c2']['V2m_gain_db']
    lo = bass * gains(v2m)(n)
    t_today, t_new = today[:n] + hi, n2b[:n] + hi
    ap, b = BP.START[2], BP.START[1]
    bar = BP.BAR[1]
    cut = MP.cut
    return {'M': ((cut(t_today, 32, 32), 'oggi'), (cut(t_new, 32, 32), 'N2')),
            'L': ((cut(n2b[:n] + lo, ap + 16, 16), 'V2m'), (cut(t_new, ap + 16, 16), '-3 LU')),
            'B': ((cut(t_today, b + 4 * bar, 12 * bar), 'oggi'), (cut(t_new, b + 4 * bar, 12 * bar), 'B2')),
            'I': ((cut(t_new, ap + 32, 16), 'Y'), (cut(t_new, ap + 32, 16), 'Y'))}


def page(pkg, pairs):
    h = MP.page(pkg, pairs)
    h = h.replace('Circa 20 minuti.', 'Circa 18 minuti.').replace('2 · Sette coppie<span class="t">9 min', '2 · Cinque coppie<span class="t">6 min')
    h = h.replace('Ascolto <span>di conferma</span>', 'Ascolto <span>due</span>')
    q, r, sc = C.q, C.radios, C.scale
    extra = (q('La melodia di questo brano, rispetto a quelle che hai già sentito nelle sessioni scorse, ti sembra…',
               r('1_melodia_nuova', ['nuova', 'simile ma diversa', 'la stessa']))
             + q('Ti stanca sentirla?', sc('1_stanca', '1 = per niente', '7 = molto')))
    marker = '<div class="q"><span class="qid">B1.6</span>'
    assert marker in h
    h = h.replace(marker, extra + marker)
    old = 'Osservazioni libere: musicalità, direzione artistica, quello che vuoi.'
    assert old in h
    h = h.replace(old, 'Osservazioni libere. Per ciascuna scrivi se è <b>un problema</b> (a tuo parere va corretto) o <b>un tuo gusto</b>.')
    js_old = "document.getElementById('show').addEventListener"
    assert js_old in h
    js_new = ("function missing(){const s=new Set(),m=[];for(const el of fields()){if(s.has(el.name))continue;s.add(el.name);"
              "const v=el.type==='radio'?form.querySelector('input[name=\"'+el.name+'\"]:checked'):el.value.trim();if(!v&&el.name.indexOf('perche')<0&&el.name.indexOf('_dove')<0"
              "&&el.name.indexOf('calo')<0&&el.name.indexOf('scatti')<0&&el.name.indexOf('osservazioni')<0&&el.name.indexOf('affaticamento')<0)m.push(el.name);}"
              "return m;}"
              "function warn(){const m=missing();document.getElementById('miss').textContent=m.length?('Domande senza risposta ('+m.length+'): '+m.join(', ')+'. Puoi inviare lo stesso.'):'Tutte le domande hanno una risposta.';}"
              "for(const k of ['show','copy','dl'])document.getElementById(k).addEventListener('click',warn);" + js_old)
    h = h.replace(js_old, js_new, 1)
    h = re.sub(r'>B2\.(\d)<', r'>P.\1<', h)  # question ids of the pairs, not a version label
    return h.replace('<pre id="out"></pre>', '<p class="muted" id="miss"></p><pre id="out"></pre>')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cacp', default='artifacts/cacp')
    ap.add_argument('--out', default='artifacts/expert_listening')
    for o in ('checks', 'date', 'commit'):
        ap.add_argument('--' + o, required=True)
    a = ap.parse_args()
    c, out = Path(a.cacp), Path(a.out)
    seed = secrets.randbits(32)
    rng = random.Random(seed)
    pkg = f'cymatica_ascolto_due_{a.date}'
    folder = out / pkg
    if folder.exists():
        sys.exit(f'{folder} exists')
    (folder / 'audio').mkdir(parents=True)
    mat = material(c)
    first = {k: rng.random() < 0.5 for k in 'MLB'}
    html, key, chk = [], [], {}
    for n, k in enumerate(ORDER, 1):
        x, y = mat['M' if k == 'D' else k]
        if k != 'I' and first['M' if k == 'D' else k]:
            x, y = y, x
        if k == 'D':
            x, y = y, x
        lu = [S.integrated_lufs(v[0], FS) for v in (x, y)]
        arrs, files = [], []
        for side, (arr, name), l in (('prima', x, lu[0]), ('seconda', y, lu[1])):
            z = MP.finish(arr.copy(), 10 ** ((min(lu) - l) / 20))
            wavfile.write(folder / 'audio' / f'coppia_{n}_{side}.wav', FS, z)
            arrs.append(z)
            files.append(f'audio/coppia_{n}_{side}.wav')
        chk[k] = {'gap': abs(S.integrated_lufs(arrs[0] / 32768.0, FS) - S.integrated_lufs(arrs[1] / 32768.0, FS)),
                  'peak': int(max(np.abs(z.astype(int)).max() for z in arrs)), 'identical': bool(len(arrs[0]) == len(arrs[1]) and (arrs[0] == arrs[1]).all())}
        html.append(MP.pair_html(n, files))
        key.append(f'coppia {n} ({k}): prima = {x[1]}, seconda = {y[1]}' + (' [duplicato nascosto di M]' if k == 'D' else '') + (' [coppia identica]' if k == 'I' else ''))
    src = (c / 'tempo_ramp_v1/piece_B2.wav').read_bytes()
    (folder / 'audio/brano.wav').write_bytes(src)
    (folder / 'ascolto.html').write_text(page(pkg, ''.join(html)), encoding='utf-8')
    (folder / 'LEGGIMI.txt').write_text(C.LEGGIMI.replace('ascolto cumulativo\n============================', 'ascolto due\n====================='), encoding='utf-8', newline='\r\n')
    text = (folder / 'ascolto.html').read_text(encoding='utf-8')
    refs = sorted(set(re.findall(r'src="(audio/[^"]+)"', text)))
    vis = re.sub(r'<(script|style).*?</\1>', '', text, flags=re.S)
    labels = [w for w in ('V2m', 'Do2', 'N2', 'B2', 'LU', 'duplicato', 'identica', 'candidat', 'oggi') if re.search(r'\b' + re.escape(w) + r'\b', vis)]
    pk = float(20 * np.log10(np.abs(T.wav(folder / 'audio/brano.wav')).max()))
    res = {'V1': (folder / 'audio/brano.wav').read_bytes() == src and pk <= -1, 'piece_peak_dbfs': pk,
           'V2': all(v['gap'] <= 0.1 and v['peak'] < 32767 for v in chk.values()) and chk['I']['identical'] and not any(chk[k]['identical'] for k in 'MLBD'),
           'V3': all((folder / f).exists() for f in refs) and len(refs) == 11 and not labels, 'labels_found': labels,
           'V4': 'id="miss"' in text and 'function missing' in text, 'files': len(refs)}
    Path(a.checks).write_text(json.dumps(res, indent=1) + '\n', encoding='utf-8', newline='\n')
    print(res, chk['M']['gap'])
    z = C.make_zip(folder)
    hyp = Path('docs/research/M2_5_CP3_Expert_Listening_2_Hypotheses.md')
    hh = hashlib.sha256(hyp.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    lines = [f'CHIAVE - secondo ascolto CP-3 {a.date} - NON INVIARE', '=' * 48, '', f'Pacchetto: {pkg}', f'Commit: {a.commit}', f'Ipotesi SHA-256 (LF): {hh}', f'Seed: {seed}', '', 'brano = Y', *key, '', 'SHA-256:']
    lines += [f'  {C.sha(p)}  {p.relative_to(folder).as_posix()}' for p in sorted(folder.rglob('*')) if p.is_file()] + ['', f'zip {C.sha(z)}']
    (out / f'CHIAVE_due_{a.date}_NON_INVIARE.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\r\n')
    print(z, round(z.stat().st_size / 1e6, 1), 'MB', C.sha(z))


if __name__ == '__main__':
    main()

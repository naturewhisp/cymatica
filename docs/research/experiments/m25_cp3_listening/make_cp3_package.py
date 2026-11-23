"""Build the CP-3 expert listening package and the conductor's key (docs/research/M2_5_CP3_Expert_Listening_Hypotheses.md).

Outputs (not versioned) under artifacts/expert_listening: cymatica_ascolto_cp3_<date>/ and .zip for the listener, and
CHIAVE_cp3_<date>_NON_INVIARE.txt for the conductor. The shuffle seed is drawn from the OS and written only in the key.
Usage: python -I make_cp3_package.py --date 2026-11-23 --commit <sha> [--out <json of the T5-T6 checks>]
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
for q in (HERE, HERE.parent / 'm25_cumulative_listening', HERE.parent / 'm25_cp3_bass', HERE.parent / 'm25_cp3_transitions', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import bass_prototype as BP  # noqa: E402
import cp3_material as CM3  # noqa: E402
import make_cumulative_package as C  # noqa: E402
import ramp_over_c as RC  # noqa: E402
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

FS = 48000
AP = BP.START[2]


def cut(x, t0, dur):
    i = int(round(t0 * FS))
    return x[i:i + int(round(dur * FS))].copy()


def material(c):
    p = c / 'bass_prototype_v1'
    v0, v1 = T.wav(p / 'c2_v0_mix.wav'), T.wav(p / 'c2_v1_mix.wav')
    n = min(len(v0), len(v1))
    gains = json.loads((HERE.parent / 'm25_cp3_bass/kick_bass_joint.json').read_text())['c2']['V2m_gain_db']
    edges = [int(x * FS) for x in (*BP.START, AP + 64.0)]
    g = np.full(n, 10 ** (gains[2] / 20))
    for s in range(3):
        g[edges[s]:edges[s + 1]] = 10 ** (gains[s] / 20)
    v2m = v0[:n] + np.column_stack([CM3.mono(T.wav(p / 'bass_c2_s0.wav'), n)] * 2) * g[:, None]
    r8b = float(RC.ramp_time('r8')[1][128])
    i5e, i46, i46b, i5d = (c / f'tempo_ramp_v1_{k}' for k in ('i5e', 'i46', 'i46b', 'i5d'))
    return {
        'K': ((cut(T.wav(p / 'now_v0_mix.wav'), 56, 16), 'kick di oggi'), (cut(v0, 56, 16), 'Do2 e click +3 dB')),
        'V': ((cut(v1, AP + 16, 16), 'V1'), (cut(v2m, AP + 16, 16), 'V2m')),
        'M': ((cut(T.wav(i46 / 'p.wav'), 32, 32), 'oggi'), (cut(T.wav(i46b / 'c.wav'), 32, 32), 'c')),
        'T': ((cut(T.wav(i5e / 'n.wav'), 54, 16), 'c senza rampa'), (cut(T.wav(i5e / 'r8.wav'), r8b - 10, 16), 'c con r8')),
        'R': ((cut(T.wav(i5d / 'p.wav'), AP - 8, 20), 'oggi'), (cut(T.wav(i5d / 'x.wav'), AP - 8, 20), 'x')),
        'I': ((cut(v1, AP + 32, 16), 'V1'), (cut(v1, AP + 32, 16), 'V1'))}


def finish(x, gain):
    nf = FS // 100
    x = x * gain
    x[:nf] *= np.linspace(0, 1, nf)[:, None]
    x[-nf:] *= np.linspace(1, 0, nf)[:, None]
    return (x * 32767).astype(np.int16)


def pair_html(n, files):
    q, r = C.q, C.radios
    a, b = files
    return (f'<section class="card b"><div class="label">coppia {n}</div><div class="pair">'
            f'<div><span class="label">prima</span><audio controls preload="none" src="{a}"></audio></div>'
            f'<div><span class="label">seconda</span><audio controls preload="none" src="{b}"></audio></div></div>'
            + q('Senti una differenza?', r(f'2_c{n}_differenza', ['sì', 'no', 'non so']), f'B2.{n}')
            + q('Quale preferisci?', r(f'2_c{n}_preferita', ['la prima', 'la seconda', 'nessuna']))
            + q('Quale funziona meglio musicalmente, e perché?', C.text(f'2_c{n}_perche', 2)) + '</section>')


def page(pkg, pairs):
    q, r, sc = C.q, C.radios, C.scale
    P = [f'<!doctype html><html lang="it"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">'
         f'<title>CYMATICA ascolto</title><style>{C.PAGE_CSS}</style></head><body data-pkg="{pkg}"><main>',
         '<div class="label">cymatica · ascolto · M2.5</div>',
         f'<div class="hero">{C.S.chladni_svg(3, 5, 120, C.S.CYAN)}<div><h1>Ascolto <span>di conferma</span></h1>'
         '<p class="intro">Circa 20 minuti. Non ci sono risposte giuste o sbagliate: anche un giudizio negativo o incerto è utile.</p></div></div>',
         '<section class="card c"><p><b>Prima di tutto.</b> Estrai lo zip in una cartella (tasto destro, «Estrai tutto») e apri questa pagina dalla cartella '
         'estratta. Non serve essere online. Le risposte restano su questo computer: si salvano da sole, e alla fine le copi o le scarichi.</p>'
         '<p>Rispetto all\'ultima volta sono cambiati alcuni strumenti, la melodia, il ritmo e dei passaggi. Non diciamo dove né quale versione sia più recente.</p></section>',
         '<section class="card"><ol><li>Usa le stesse cuffie o gli stessi diffusori per tutta la sessione.</li>'
         '<li>Nel mixer volume di Windows porta al 100 % il cursore del browser; annota il volume di sistema.</li>'
         '<li>Ascolta i primi 20 secondi del brano e scegli un volume a cui la cassa sia giudicabile. Poi non cambiarlo più.</li>'
         '<li>Segui i blocchi in ordine. Se ti stanchi o ti annoi, scrivilo alla fine: è un dato utile. Dopo il brano puoi fare una pausa.</li>'
         '<li>Per favore non confrontare i file con strumenti (dimensioni, forme d\'onda): solo ascolto.</li></ol></section>',
         '<form id="f" autocomplete="off"><section class="card"><div class="grid2">',
         q('Data e ora', C.line('data_ora')), q('Cuffie / diffusori (modello)', C.line('apparecchiatura')),
         q('Volume di sistema; cursore del browser al 100 %?', C.line('volume')), '</div></section>',
         '<h2>1 · Il brano<span class="t">8 min</span></h2>',
         '<section class="card a"><p>Circa 3 minuti, in tre parti. Ascoltalo <b>una volta</b> dall\'inizio alla fine senza guardare le domande. '
         'Mentre ascolti, premi <b>Segna adesso</b> quando l\'interesse cala, se cala.</p>', C.player('b1', 'audio/brano.wav', sections=True),
         '<div class="jumps"><button type="button" class="mark" data-mark="b1" data-into="1_calo">Segna adesso</button></div></section>',
         q('Che cosa hai sentito? Descrivilo con parole tue.', C.text('1_descrizione', 5), 'B1.1'),
         q('Istanti in cui l\'interesse cala (minuti:secondi)', C.line('1_calo', 'es. 1:42')),
         q('Coerenza: le parti stanno insieme?', sc('1_coerenza', '1 = non stanno insieme', '7 = un brano unico'), 'B1.2'),
         q('Interesse: tiene l\'attenzione?', sc('1_interesse', '1 = annoia subito', '7 = fino alla fine'), 'B1.3'),
         q('Ripetitività', sc('1_ripetitivita', '1 = mai ripetitivo', '7 = ripetitivo in modo fastidioso'), 'B1.4'),
         q('La melodia ti sembra…', r('1_melodia', ['una frase con un inizio e una fine', 'una figura che si ripete']), 'B1.5'),
         q('Riesci a battere il piede a tempo? Parte 1', r('1_tempo_p1', ['sempre', 'a tratti', 'mai'])),
         q('Parte 2', r('1_tempo_p2', ['sempre', 'a tratti', 'mai'])), q('Parte 3', r('1_tempo_p3', ['sempre', 'a tratti', 'mai'])),
         q('Il kick (la cassa)…', r('1_kick', ['è debole', 'è giusto', 'è troppo presente', 'rimbomba o è troppo lungo']), 'B1.6'),
         q('Il basso…', r('1_basso', ['non lo sento', 'lo sento ed è distinto dalla cassa', 'lo sento ma si confonde con la cassa']), 'B1.7'),
         q('Il livello del basso…', r('1_basso_livello', ['troppo basso', 'giusto', 'troppo alto', 'non so'])),
         q('Il passaggio dalla parte 1 alla parte 2 è…', r('1_passaggio_12', ['brusco', 'naturale', 'poco marcato']), 'B1.8'),
         q('Il passaggio dalla parte 2 alla parte 3 è…', r('1_passaggio_23', ['brusco', 'naturale', 'poco marcato']), 'B1.9'),
         q('C\'è un accordo o una nota che suona fuori posto? Se sì, dove (mm:ss)?', r('1_fuori_posto', ['no', 'sì']) + C.line('1_fuori_posto_dove', 'mm:ss')),
         '<h2>2 · Sette coppie<span class="t">9 min</span></h2>',
         '<section class="card"><p>Ogni coppia ha due estratti dello stesso passaggio. Ascoltali quante volte vuoi, in qualsiasi ordine. '
         'Alcune differenze sono piccole e «no» è una risposta valida.</p></section>', pairs,
         '<h2>3 · Domande finali<span class="t">3 min</span></h2>',
         q('Hai sentito scatti, click o fruscii? Dove (brano o coppia, mm:ss)?', C.line('3_scatti')),
         q('Osservazioni libere: musicalità, direzione artistica, quello che vuoi.', C.text('3_osservazioni', 5)),
         q('Ti sei stancato o annoiato? Se sì, da quando?', C.line('3_affaticamento')),
         q('Durata effettiva della sessione', C.line('3_durata', 'es. 20 minuti')), '</form>',
         '<h2>Fine</h2><section class="card a"><p>Grazie. Copia le risposte e incollale in una mail, oppure scarica il file di testo e allegalo.</p>'
         '<p><button type="button" id="copy">Copia le risposte</button> <button type="button" id="dl" class="ghost">Scarica .txt</button> '
         '<button type="button" id="show" class="ghost">Mostra</button><span class="saved" id="copied"></span><span class="saved" id="saved"></span></p>'
         '<pre id="out"></pre></section>', f'<footer><span>CYMATICA · M2.5 · ascolto</span><span>{pkg}</span></footer>',
         '</main><script>' + C.PAGE_JS.replace('CYMATICA - ascolto cumulativo - ', 'CYMATICA - ascolto di conferma - ') + '</script></body></html>']
    return ''.join(P)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cacp', default='artifacts/cacp')
    ap.add_argument('--out', default='artifacts/expert_listening')
    ap.add_argument('--checks')
    ap.add_argument('--date', required=True)
    ap.add_argument('--commit', required=True)
    a = ap.parse_args()
    c, out = Path(a.cacp), Path(a.out)
    seed = secrets.randbits(32)
    rng = random.Random(seed)
    pkg = f'cymatica_ascolto_cp3_{a.date}'
    folder = out / pkg
    if folder.exists():
        sys.exit(f'{folder} exists: choose another date or move it')
    (folder / 'audio').mkdir(parents=True)
    mat = material(c)
    order = ['K', 'M', 'V', 'T', 'I', 'R', 'D']
    first = {k: rng.random() < 0.5 for k in 'KMVTR'}
    html, key, checks = [], [], {}
    for n, k in enumerate(order, 1):
        x, y = mat['V' if k == 'D' else k]
        if k != 'I' and (first['V'] if k == 'D' else first[k]):
            x, y = y, x
        if k == 'D':
            x, y = y, x
        lu = [S.integrated_lufs(v[0], FS) for v in (x, y)]
        gain = [10 ** ((min(lu) - v) / 20) for v in lu]
        files = []
        arrs = []
        for side, (arr, name), g in (('prima', x, gain[0]), ('seconda', y, gain[1])):
            f = f'coppia_{n}_{side}.wav'
            z = finish(arr.copy(), g)
            wavfile.write(folder / 'audio' / f, FS, z)
            arrs.append(z)
            files.append(f'audio/{f}')
        checks[k] = {'lufs': [S.integrated_lufs(z / 32768.0, FS) for z in arrs], 'peak': int(max(np.abs(z.astype(int)).max() for z in arrs)),
                     'identical': bool(len(arrs[0]) == len(arrs[1]) and (arrs[0] == arrs[1]).all())}
        html.append(pair_html(n, files))
        key.append(f'coppia {n} ({k}): prima = {x[1]}, seconda = {y[1]}' + (' [duplicato nascosto di V, ordine invertito]' if k == 'D' else '') + (' [coppia identica]' if k == 'I' else ''))
    (folder / 'audio' / 'brano.wav').write_bytes((c / 'tempo_ramp_v1/piece_z.wav').read_bytes())
    (folder / 'ascolto.html').write_text(page(pkg, ''.join(html)), encoding='utf-8')
    (folder / 'LEGGIMI.txt').write_text(C.LEGGIMI.replace('ascolto cumulativo\n============================', 'ascolto di conferma\n============================='), encoding='utf-8', newline='\r\n')
    text = (folder / 'ascolto.html').read_text(encoding='utf-8')
    refs = sorted(set(re.findall(r'src="(audio/[^"]+)"', text)))
    visible = re.sub(r'<(script|style).*?</\1>', '', text, flags=re.S)
    labels = [w for w in ('V1', 'V2m', 'Do2', 'r8', 'senza rampa', 'duplicato', 'identica', 'candidat', 'oggi') if re.search(r'\b' + re.escape(w), visible)]
    t5 = all(abs(v['lufs'][0] - v['lufs'][1]) <= 0.1 and v['peak'] < 32767 for v in checks.values()) and checks['I']['identical'] and \
        not any(checks[k]['identical'] for k in 'KMVTRD')
    t6 = all((folder / f).exists() for f in refs) and len(refs) == 15 and not labels
    result = {'T5': bool(t5), 'T6': bool(t6), 'labels_found': labels, 'audio_files': len(refs), 'max_lufs_gap': max(abs(v['lufs'][0] - v['lufs'][1]) for v in checks.values())}
    if a.checks:
        Path(a.checks).write_text(json.dumps(result, indent=1) + '\n', encoding='utf-8', newline='\n')
    print(result)
    z = C.make_zip(folder)
    hyp = Path('docs/research/M2_5_CP3_Expert_Listening_Hypotheses.md')
    hh = hashlib.sha256(hyp.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    lines = [f'CHIAVE - ascolto CP-3 {a.date} - NON INVIARE', '=' * 48, '', f'Pacchetto: {pkg}', f'Commit: {a.commit}',
             f'Ipotesi: {hyp.as_posix()} SHA-256 (LF) {hh}', f'Seed di mescolamento: {seed}', '', 'brano = Z', *key, '', 'SHA-256 dei file:']
    lines += [f'  {C.sha(p)}  {p.relative_to(folder).as_posix()}' for p in sorted(folder.rglob('*')) if p.is_file()]
    lines += ['', f'SHA-256 dello zip: {C.sha(z)}  {z.name}']
    (out / f'CHIAVE_cp3_{a.date}_NON_INVIARE.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\r\n')
    print(f'{z} {z.stat().st_size / 1e6:.1f} MB')


if __name__ == '__main__':
    main()

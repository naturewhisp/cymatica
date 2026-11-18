"""Build the cumulative listening package of M2.5.3-4 (D-M2-50) and the conductor's key.

Design and hypotheses: docs/research/M2_5_Cumulative_Listening_Hypotheses.md (hash in progress.md before this script ran).
Inputs, from the same build:
  artifacts/cacp/cumulative_listening_v1/   (cymatica_game --cacp-run cumulative_listening_v1)
  artifacts/cacp/chiptune_ab_v1/            (cymatica_game --cacp-run chiptune_ab_v1)
Outputs (not versioned), under --out (default artifacts/expert_listening):
  cymatica_ascolto_cumulativo_<date>/ and .zip   -> for the listeners (one interactive page, audio files)
  CHIAVE_cumulativo_<date>_NON_INVIARE.txt       -> conductor only, never sent
The shuffle seed is drawn from the OS unless --seed is given, and is written only in the key file.
Usage: python -I make_cumulative_package.py --date 2026-11-18 --commit <sha>
"""
import argparse
import hashlib
import html
import json
import random
import secrets
import shutil
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'expert_music_guide'))
import _style as S  # noqa: E402  (CYMATICA palette and Chladni figures, DESIGN.md §13)


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def make_zip(folder):
    # No checksum list inside the package: identical hashes would reveal the identical pair and the duplicate.
    z = folder.with_suffix('.zip')
    with zipfile.ZipFile(z, 'w', zipfile.ZIP_DEFLATED) as zf:
        for p in sorted(folder.rglob('*')):
            if p.is_file():
                zf.write(p, (Path(folder.name) / p.relative_to(folder)).as_posix())
    return z


def radios(qid, options):
    return '<div class="opts">' + ''.join(
        f'<label class="opt"><input type="radio" name="{qid}" value="{html.escape(o)}"><span>{html.escape(o)}</span></label>'
        for o in options) + '</div>'


def scale(qid, low, high):
    cells = ''.join(f'<label class="opt n"><input type="radio" name="{qid}" value="{k}"><span>{k}</span></label>' for k in range(1, 8))
    return (f'<div class="scale"><div class="opts">{cells}</div>'
            f'<div class="ends"><span>{html.escape(low)}</span><span>{html.escape(high)}</span></div></div>')


def text(qid, rows=3, ph=''):
    return f'<textarea name="{qid}" rows="{rows}" placeholder="{html.escape(ph)}"></textarea>'


def line(qid, ph=''):
    return f'<input type="text" name="{qid}" placeholder="{html.escape(ph)}">'


def q(label, body, qid=None):
    tag = f'<span class="qid">{qid}</span>' if qid else ''
    return f'<div class="q">{tag}<p class="ql">{label}</p>{body}</div>'


def player(pid, src, sections=False):
    jumps = ''
    if sections:
        jumps = ('<div class="jumps"><span class="label">vai a</span>'
                 f'<button type="button" class="ghost" data-jump="{pid}" data-t="0">Parte 1 · 0:00</button>'
                 f'<button type="button" class="ghost" data-jump="{pid}" data-t="64">Parte 2 · 1:04</button>'
                 f'<button type="button" class="ghost" data-jump="{pid}" data-t="118.9">Parte 3 · 1:59</button></div>')
    return f'<audio id="{pid}" controls preload="none" src="{src}"></audio>{jumps}'


PAGE_CSS = f"""
:root {{ --bg:{S.BG}; --steel:{S.STEEL}; --steel-t:{S.STEEL_TXT}; --cyan:{S.CYAN}; --magenta:{S.MAGENTA}; --acid:{S.ACID}; --ink:{S.INK}; --card:#0a101d; }}
* {{ box-sizing:border-box; }}
html, body {{ margin:0; background:var(--bg); color:var(--ink); }}
body {{ font:16px/1.55 'Segoe UI','Bahnschrift',system-ui,sans-serif; }}
main {{ max-width:880px; margin:0 auto; padding:28px 16px 80px; }}
.label {{ font-family:'Bahnschrift','Segoe UI',sans-serif; font-variant:small-caps; letter-spacing:.14em; color:var(--steel-t); font-size:.82rem; }}
h1 {{ font-family:'Bahnschrift','Segoe UI',sans-serif; font-weight:300; font-size:2.1rem; letter-spacing:.02em; margin:10px 0 4px; }}
h1 span {{ color:var(--cyan); }}
h2 {{ font-family:'Bahnschrift','Segoe UI',sans-serif; font-weight:400; font-variant:small-caps; letter-spacing:.12em; font-size:1.2rem;
     color:var(--cyan); border-bottom:1px solid var(--steel); padding-bottom:6px; margin:40px 0 12px; }}
h2 .t {{ float:right; color:var(--steel-t); font-size:.85rem; letter-spacing:.08em; }}
p {{ margin:0 0 10px; }}
b {{ color:#fff; font-weight:600; }}
.hero {{ display:flex; gap:22px; align-items:center; margin-top:8px; }}
.hero svg {{ flex:0 0 auto; width:120px; height:120px; }}
.intro {{ font-size:1.05rem; }}
.card {{ border:1px solid var(--steel); background:var(--card); padding:14px 16px; margin:12px 0; }}
.card.a {{ border-left:3px solid var(--cyan); }} .card.b {{ border-left:3px solid var(--magenta); }} .card.c {{ border-left:3px solid var(--acid); }}
ol, ul {{ margin:0 0 8px; padding-left:22px; }} li {{ margin:3px 0; }} li::marker {{ color:var(--magenta); }}
audio {{ width:100%; margin:6px 0 4px; filter:invert(.9) hue-rotate(160deg); }}
.jumps {{ display:flex; flex-wrap:wrap; gap:6px; align-items:center; margin:4px 0 2px; }}
.q {{ margin:16px 0; position:relative; }}
.ql {{ margin:0 0 6px; }}
.qid {{ float:right; font-family:Consolas,monospace; color:var(--steel-t); font-size:.75rem; }}
.opts {{ display:flex; flex-wrap:wrap; gap:6px; }}
.opt {{ position:relative; cursor:pointer; }}
.opt input {{ position:absolute; opacity:0; }}
.opt span {{ display:inline-block; border:1px solid var(--steel); padding:5px 12px; color:var(--ink); background:#0c1322; }}
.opt.n span {{ min-width:38px; text-align:center; font-family:Consolas,monospace; }}
.opt input:checked + span {{ border-color:var(--cyan); color:var(--bg); background:var(--cyan); }}
.opt input:focus-visible + span {{ outline:2px solid var(--acid); outline-offset:2px; }}
.scale {{ display:inline-block; max-width:100%; }}
.scale .ends {{ display:flex; justify-content:space-between; gap:16px; font-size:.8rem; color:var(--steel-t); margin-top:4px; }}
textarea, input[type=text] {{ width:100%; font:inherit; color:var(--ink); background:#0c1322; border:1px solid var(--steel); padding:8px 10px; }}
textarea:focus, input[type=text]:focus {{ outline:none; border-color:var(--cyan); }}
button {{ font:inherit; cursor:pointer; padding:8px 16px; border:1px solid var(--cyan); background:var(--cyan); color:var(--bg); }}
button.ghost {{ background:transparent; color:var(--cyan); padding:3px 10px; font-size:.85rem; }}
button.mark {{ background:transparent; color:var(--acid); border-color:var(--acid); }}
.pair {{ display:grid; grid-template-columns:1fr 1fr; gap:12px; }}
.pair .label {{ display:block; margin-top:4px; }}
.grid2 {{ display:grid; grid-template-columns:1fr 1fr; gap:10px; }}
pre#out {{ white-space:pre-wrap; background:#0c1322; border:1px solid var(--steel); border-left:3px solid var(--acid); padding:10px; min-height:3em;
          font-family:Consolas,monospace; font-size:.85rem; }}
.muted {{ color:var(--steel-t); font-size:.9rem; }}
.saved {{ color:var(--acid); font-size:.85rem; margin-left:10px; }}
footer {{ margin-top:48px; border-top:1px solid var(--steel); padding-top:8px; font-family:Consolas,monospace; font-size:.75rem; color:var(--steel-t);
         display:flex; justify-content:space-between; flex-wrap:wrap; gap:8px; }}
@media (max-width:640px) {{ .pair, .grid2 {{ grid-template-columns:1fr; }} .hero svg {{ width:84px; height:84px; }} h1 {{ font-size:1.6rem; }} }}
"""

PAGE_JS = r"""
const KEY = 'cymatica-ascolto-' + document.body.dataset.pkg;
const form = document.getElementById('f');
function fields() { return Array.from(form.querySelectorAll('input[name], textarea[name]')); }
function save() {
  const data = {};
  for (const el of fields()) {
    if (el.type === 'radio') { if (el.checked) data[el.name] = el.value; }
    else if (el.value) data[el.name] = el.value;
  }
  try { localStorage.setItem(KEY, JSON.stringify(data)); const s = document.getElementById('saved'); if (s) s.textContent = 'salvato'; } catch (e) {}
}
function load() {
  let data = {};
  try { data = JSON.parse(localStorage.getItem(KEY) || '{}'); } catch (e) {}
  for (const el of fields()) {
    if (!(el.name in data)) continue;
    if (el.type === 'radio') el.checked = (el.value === data[el.name]); else el.value = data[el.name];
  }
}
function report() {
  const seen = new Set(); const out = ['CYMATICA - ascolto cumulativo - ' + document.body.dataset.pkg, ''];
  for (const el of fields()) {
    if (seen.has(el.name)) continue; seen.add(el.name);
    let v = '';
    if (el.type === 'radio') { const c = form.querySelector('input[name="' + el.name + '"]:checked'); v = c ? c.value : ''; }
    else v = el.value.trim();
    out.push(el.name + ': ' + (v || '-'));
  }
  return out.join('\n');
}
function mmss(t) { const m = Math.floor(t / 60), s = Math.floor(t % 60); return m + ':' + String(s).padStart(2, '0'); }
form.addEventListener('input', save);
form.addEventListener('change', save);
for (const b of document.querySelectorAll('[data-jump]')) {
  b.addEventListener('click', () => { const a = document.getElementById(b.dataset.jump); a.currentTime = parseFloat(b.dataset.t); a.play(); });
}
for (const b of document.querySelectorAll('[data-mark]')) {
  b.addEventListener('click', () => {
    const a = document.getElementById(b.dataset.mark); const t = document.querySelector('[name="' + b.dataset.into + '"]');
    t.value = (t.value ? t.value + ', ' : '') + mmss(a.currentTime); save();
  });
}
// One player at a time
for (const a of document.querySelectorAll('audio')) {
  a.addEventListener('play', () => { for (const o of document.querySelectorAll('audio')) if (o !== a) o.pause(); });
}
document.getElementById('show').addEventListener('click', () => { document.getElementById('out').textContent = report(); });
document.getElementById('copy').addEventListener('click', async () => {
  const t = report(); document.getElementById('out').textContent = t;
  try { await navigator.clipboard.writeText(t); document.getElementById('copied').textContent = 'copiato'; }
  catch (e) { document.getElementById('copied').textContent = 'copia non riuscita: selezionare il testo qui sotto'; }
});
document.getElementById('dl').addEventListener('click', () => {
  const blob = new Blob([report()], {type: 'text/plain;charset=utf-8'});
  const a = document.createElement('a'); a.href = URL.createObjectURL(blob); a.download = 'risposte_' + document.body.dataset.pkg + '.txt';
  document.body.appendChild(a); a.click(); a.remove();
});
load();
"""


def build_page(pkg, pairs_html):
    hero = S.chladni_svg(3, 5, 120, S.CYAN)
    P = []
    P.append(f'<!doctype html><html lang="it"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">'
             f'<title>CYMATICA ascolto</title><style>{PAGE_CSS}</style></head><body data-pkg="{pkg}"><main>')
    P.append(f'<div class="label">cymatica · ascolto cumulativo · M2.5</div>')
    P.append(f'<div class="hero">{hero}<div><h1>Ascolto <span>cumulativo</span></h1>'
             '<p class="intro">Circa 30 minuti. Non ci sono risposte giuste o sbagliate: anche un giudizio negativo o incerto è utile.</p></div></div>')
    P.append('<section class="card c"><p><b>Prima di tutto.</b> Estrai lo zip in una cartella (tasto destro, «Estrai tutto») e apri questa pagina '
             'dalla cartella estratta: dall\'anteprima dello zip l\'audio non si carica. Non serve essere online. '
             'Le risposte restano su questo computer: si salvano da sole mentre scrivi, e alla fine le copi o le scarichi.</p></section>')
    P.append('<section class="card"><ol>'
             '<li>Usa sempre le stesse cuffie o gli stessi diffusori.</li>'
             '<li>Nel mixer volume di Windows porta al 100 % il cursore del browser; annota il volume di sistema.</li>'
             '<li>Ascolta i primi 20 secondi del brano 1 e scegli un volume a cui il kick (la cassa) sia giudicabile. Poi non cambiarlo più.</li>'
             '<li>Segui i blocchi in ordine. Se ti stanchi o ti annoi, scrivilo alla fine: è un dato utile.</li>'
             '<li>Per favore non confrontare i file con strumenti (dimensioni, forme d\'onda): solo ascolto.</li></ol></section>')
    P.append('<form id="f" autocomplete="off">')
    P.append('<section class="card"><div class="grid2">'
             + q('Ascoltatore', radios('ascoltatore', ['esperto', 'secondo ascoltatore']))
             + q('Data e ora', line('data_ora'))
             + q('Cuffie / diffusori (modello)', line('apparecchiatura'))
             + q('Volume di sistema; cursore del browser al 100 %?', line('volume'))
             + '</div></section>')

    # Block 1
    P.append('<h2>1 · Brano 1<span class="t">8 min</span></h2>')
    P.append('<section class="card a"><p>Circa 3 minuti, in tre parti. Ascoltalo dall\'inizio alla fine senza guardare le domande, '
             'poi scrivi che cosa senti con parole tue.</p>' + player('b1', 'audio/brano_1.wav') + '</section>')
    P.append(q('Che cosa hai sentito? Descrivilo con parole tue.', text('1_descrizione', 5), 'B1.1'))
    P.append(q('Coerenza: le parti stanno insieme?', scale('1_coerenza', '1 = non stanno insieme', '7 = un brano unico'), 'B1.2'))
    P.append(q('Interesse: tiene l\'attenzione?', scale('1_interesse', '1 = annoia subito', '7 = fino alla fine'), 'B1.3'))
    P.append(q('Ripetitività', scale('1_ripetitivita', '1 = mai ripetitivo', '7 = ripetitivo in modo fastidioso'), 'B1.4'))

    # Block 2
    P.append('<h2>2 · Il brano 1, parte per parte<span class="t">5 min</span></h2>')
    P.append('<section class="card a"><p>Puoi riascoltare le parti che ti servono con i pulsanti. Non serve riascoltare tutto.</p>'
             + player('b1b', 'audio/brano_1.wav', sections=True) + '</section>')
    P.append(q('Riesci a battere il piede a tempo? Parte 1', radios('2_tempo_p1', ['sempre', 'a tratti', 'mai']), 'B2.1'))
    P.append(q('Parte 2', radios('2_tempo_p2', ['sempre', 'a tratti', 'mai'])))
    P.append(q('Parte 3', radios('2_tempo_p3', ['sempre', 'a tratti', 'mai'])))
    P.append(q('Nella parte 2 c\'è un punto in cui il ritmo sembra spostarsi?',
               radios('2_spostamento', ['sì, e sembra voluto', 'sì, e sembra un errore', 'non l\'ho notato']), 'B2.2'))
    P.append(q('Il passaggio dalla parte 1 alla parte 2 è…', radios('2_passaggio', ['brusco', 'naturale', 'poco marcato']), 'B2.3'))
    P.append(q('La parte 3 ti sembra un ritorno della parte 1?', radios('2_ritorno', ['sì', 'in parte', 'no']), 'B2.4'))
    P.append(q('La melodia ti sembra…', radios('2_melodia_forma', ['una frase con un inizio e una fine', 'una figura che si ripete']), 'B2.5'))
    P.append(q('Riesci a seguire la melodia?', radios('2_melodia_segue', ['sì', 'a tratti si confonde con l\'arpeggio', 'no'])))
    P.append(q('Le note più acute della melodia sono…', radios('2_melodia_acuti', ['gradevoli', 'stridenti', 'non ci ho fatto caso'])))
    P.append(q('La melodia è prevedibile?', scale('2_melodia_prevedibile', '1 = mai', '7 = sempre'), 'B2.6'))
    P.append(q('L\'arpeggio di sottofondo…', radios('2_arpeggio_livello', ['copre il resto', 'è giusto', 'si perde']), 'B2.7'))
    P.append(q('Il suono dell\'arpeggio è fastidioso?', radios('2_arpeggio_timbro', ['no', 'un po\'', 'sì'])))
    P.append(q('Il kick (la cassa)…', radios('2_kick', ['è debole', 'è giusto', 'rimbomba o è troppo lungo']), 'B2.8'))
    P.append(q('Alcuni strumenti entrano ed escono. Ti sembra che…',
               radios('2_entrate', ['diano forma al brano', 'lo interrompano', 'non me ne sono accorto']), 'B2.9'))

    # Block 3
    P.append('<h2>3 · Brano 2<span class="t">7 min</span></h2>')
    P.append('<section class="card b"><p>Un altro brano, sempre circa 3 minuti. Mentre ascolti, premi <b>Segna adesso</b> nel momento '
             'in cui l\'interesse cala, se cala (puoi premerlo più volte).</p>' + player('b2', 'audio/brano_2.wav')
             + '<div class="jumps"><button type="button" class="mark" data-mark="b2" data-into="3_calo">Segna adesso</button></div></section>')
    P.append(q('Istanti in cui l\'interesse cala (minuti:secondi)', line('3_calo', 'es. 1:42'), 'B3.1'))
    P.append(q('Rispetto al brano 1, il brano 2 ti sembra…',
               radios('3_confronto', ['lo stesso brano', 'una variazione dello stesso brano', 'un brano diverso']), 'B3.2'))
    P.append(q('Quale preferisci, e perché?', radios('3_preferito', ['brano 1', 'brano 2', 'uguali']) + text('3_perche', 2), 'B3.3'))
    P.append(q('Nella parte centrale del brano 2 c\'è un accordo o un passaggio che suona fuori posto? Se sì, dove (mm:ss)?',
               radios('3_fuori_posto', ['no', 'sì']) + line('3_fuori_posto_dove', 'mm:ss'), 'B3.4'))

    # Block 4
    P.append('<h2>4 · Tre coppie<span class="t">6 min</span></h2>')
    P.append('<section class="card"><p>Ogni coppia ha due estratti brevi dello stesso passaggio. Ascoltali quante volte vuoi, in qualsiasi ordine. '
             'Per ciascuna coppia rispondi: senti una differenza? Quale preferisci? Perché?</p></section>')
    P.append(pairs_html)

    # Block 5
    P.append('<h2>5 · Domande finali<span class="t">3 min</span></h2>')
    P.append(q('Hai sentito scatti, click o fruscii? Dove (brano, mm:ss)?', line('5_scatti'), 'B5.1'))
    P.append(q('Gli acuti del suono complessivo sono…', radios('5_acuti', ['spenti', 'equilibrati', 'stridenti']), 'B5.2'))
    P.append(q('Osservazioni libere: musicalità, direzione artistica, quello che vuoi.', text('5_osservazioni', 5), 'B5.3'))
    P.append(q('Ti sei stancato o annoiato? Se sì, da quando?', line('5_affaticamento'), 'B5.4'))
    P.append(q('Durata effettiva della sessione', line('5_durata', 'es. 32 minuti'), 'B5.5'))

    # Second listener
    P.append('<h2>Per il secondo ascoltatore<span class="t">facoltativo</span></h2>')
    P.append('<section class="card c"><p>Per chi non conosce la musica. Ascolta il <b>brano 1</b> (blocco 1) e la <b>coppia 1</b> (blocco 4), '
             'poi rispondi qui. Nessuna risposta è sbagliata; puoi scrivere «non so». Usa la stessa pagina dopo aver scaricato '
             'le risposte dell\'esperto, oppure un altro computer.</p></section>')
    P.append(q('1. Che cosa hai sentito? Descrivilo con parole tue.', text('S_descrizione', 3)))
    P.append(q('2. Ti è piaciuto?', scale('S_piaciuto', '1 = per niente', '7 = moltissimo')))
    P.append(q('3. Ti sei annoiato? Se sì, più o meno quando?', line('S_noia')))
    P.append(q('4. Hai sentito dei cambi di «parte»? Quanti, e più o meno quando?', line('S_parti')))
    P.append(q('5. Riuscivi a battere il piede a tempo?', radios('S_tempo', ['sempre', 'a tratti', 'mai'])))
    P.append(q('6. Hai sentito rumori strani, scatti o fruscii? Quando?', line('S_rumori')))
    P.append(q('7. Coppia 1: le due versioni ti sembrano uguali o diverse? Se diverse, quale preferisci?',
               radios('S_coppia1', ['uguali', 'diverse, preferisco la prima', 'diverse, preferisco la seconda', 'non so'])))
    P.append('</form>')

    P.append('<h2>Fine</h2><section class="card a"><p>Grazie. Copia le risposte e incollale in una mail, oppure scarica il file di testo e allegalo.</p>'
             '<p><button type="button" id="copy">Copia le risposte</button> <button type="button" id="dl" class="ghost">Scarica .txt</button> '
             '<button type="button" id="show" class="ghost">Mostra</button><span class="saved" id="copied"></span><span class="saved" id="saved"></span></p>'
             '<pre id="out"></pre></section>')
    P.append(f'<footer><span>CYMATICA · M2.5 · ascolto cumulativo</span><span>{pkg}</span></footer>')
    P.append(f'</main><script>{PAGE_JS}</script></body></html>')
    return ''.join(P)


def pair_html(n, files):
    a, b = files
    return (f'<section class="card b"><div class="label">coppia {n}</div><div class="pair">'
            f'<div><span class="label">prima</span><audio controls preload="none" src="{a}"></audio></div>'
            f'<div><span class="label">seconda</span><audio controls preload="none" src="{b}"></audio></div></div>'
            + q('Senti una differenza?', radios(f'4_c{n}_differenza', ['sì', 'no', 'non so']), f'B4.{n}')
            + q('Quale preferisci?', radios(f'4_c{n}_preferita', ['la prima', 'la seconda', 'nessuna']))
            + q('Quale funziona meglio musicalmente, e perché?', text(f'4_c{n}_perche', 2))
            + '</section>')


LEGGIMI = """CYMATICA - ascolto cumulativo
============================

1. Estrai lo zip in una cartella (tasto destro, "Estrai tutto").
2. Apri ascolto.html dalla cartella estratta, con Edge, Chrome o Firefox.
3. Segui la pagina. Le risposte si salvano mentre scrivi; alla fine usa
   "Copia le risposte" o "Scarica .txt" e manda il testo a chi ti ha
   inviato il pacchetto.

Se il browser non riproduce l'audio, i file sono nella cartella audio/.
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--listening', default='artifacts/cacp/cumulative_listening_v1')
    ap.add_argument('--chiptune', default='artifacts/cacp/chiptune_ab_v1')
    ap.add_argument('--out', default='artifacts/expert_listening')
    ap.add_argument('--date', required=True)
    ap.add_argument('--commit', required=True)
    ap.add_argument('--seed', type=int, default=None)
    a = ap.parse_args()

    lis, chip, out = Path(a.listening), Path(a.chiptune), Path(a.out)
    man = json.loads((lis / 'cumulative_listening_v1_manifest.json').read_text(encoding='utf-8'))
    if man.get('status') != 'MEASURED':
        sys.exit('cumulative_listening_v1 is not MEASURED')
    seed = a.seed if a.seed is not None else secrets.randbits(32)
    rng = random.Random(seed)

    pkg = f'cymatica_ascolto_cumulativo_{a.date}'
    folder = out / pkg
    if folder.exists():
        shutil.rmtree(folder)
    (folder / 'audio').mkdir(parents=True)

    sources = {
        'brano_1.wav': lis / man['piece_1_reference']['file'],
        'brano_2.wav': lis / man['piece_2_second_path']['file'],
    }
    chip_prod, chip_chip = chip / 'chip_B_production.wav', chip / 'chip_B_chiptune_aligned.wav'
    ident = chip / 'chip_Aprime_production.wav'
    # Pairs (hypotheses §2): position 1 and 3 are the chiptune pair and its duplicate with the order reversed, never
    # contiguous; position 2 is the identical pair. Random: which version comes first in pair 1.
    first_chip = rng.random() < 0.5
    p1 = (chip_chip, chip_prod) if first_chip else (chip_prod, chip_chip)
    p3 = (p1[1], p1[0])
    pairs = {1: (p1, 'chiptune vs produzione, sezione B, frasi 3-4'),
             2: ((ident, ident), 'coppia identica (B0): produzione, sezione A\', frasi 3-4'),
             3: (p3, 'duplicato nascosto (B0) della coppia 1, ordine invertito')}
    key_pairs = []
    for n, ((x, y), what) in pairs.items():
        fa, fb = f'audio/coppia_{n}_prima.wav', f'audio/coppia_{n}_seconda.wav'
        sources[fa.split('/')[1]] = x
        sources[fb.split('/')[1]] = y
        def name(p):
            return {chip_prod: 'produzione', chip_chip: 'chiptune', ident: 'produzione'}[p]
        key_pairs.append(f'coppia {n}: {what}; prima = {name(x)} ({x.name}), seconda = {name(y)} ({y.name})')
    for dst, src in sources.items():
        shutil.copyfile(src, folder / 'audio' / dst)

    pairs_html = ''.join(pair_html(n, (f'audio/coppia_{n}_prima.wav', f'audio/coppia_{n}_seconda.wav')) for n in (1, 2, 3))
    (folder / 'ascolto.html').write_text(build_page(pkg, pairs_html), encoding='utf-8')
    (folder / 'LEGGIMI.txt').write_text(LEGGIMI, encoding='utf-8', newline='\r\n')
    z = make_zip(folder)

    hyp = Path('docs/research/M2_5_Cumulative_Listening_Hypotheses.md')
    hyp_hash = hashlib.sha256(hyp.read_bytes().replace(b'\r\n', b'\n')).hexdigest() if hyp.exists() else 'n/a'
    p1m, p2m = man['piece_1_reference'], man['piece_2_second_path']
    key = [f'CHIAVE - ascolto cumulativo {a.date} - NON INVIARE', '=' * 48, '',
           f'Pacchetto: {pkg}', f'Commit della build: {a.commit}', f'Ipotesi: {hyp} SHA-256 (LF) {hyp_hash}',
           f'Seed di mescolamento: {seed}', '',
           f'brano 1 = riferimento, seed {p1m["seed"]}, percorso {json.dumps(p1m["variant"])}, golden PCM {p1m["golden_pcm_status"]}, '
           f'{p1m["integrated_lufs"]:.2f} LUFS',
           f'brano 2 = seed {p2m["seed"]}, percorso {json.dumps(p2m["variant"])}, golden PCM {p2m["golden_pcm_status"]}, '
           f'{p2m["integrated_lufs"]:.2f} LUFS', '', *key_pairs, '', 'SHA-256 dei file del pacchetto:']
    for p in sorted(folder.rglob('*')):
        if p.is_file():
            key.append(f'  {sha(p)}  {p.relative_to(folder).as_posix()}')
    key += ['', f'SHA-256 dello zip: {sha(z)}  {z.name}']
    (out / f'CHIAVE_cumulativo_{a.date}_NON_INVIARE.txt').write_text('\n'.join(key) + '\n', encoding='utf-8', newline='\r\n')
    print(f'package {folder}\nzip {z} ({z.stat().st_size / 1e6:.1f} MB)\nkey {out / f"CHIAVE_cumulativo_{a.date}_NON_INVIARE.txt"}')


if __name__ == '__main__':
    main()

"""Builds the F7 instructions PDF in the CYMATICA 'Sintetico' style (DESIGN.md §13): HTML + inline SVG, printed by Edge.

Usage: python make_f7_pdf.py <output_dir>   (e.g. artifacts/expert). Needs numpy and Microsoft Edge; no other dependency.
The text mirrors F7_istruzioni_per_l_esperto.md: change both together. The Chladni figures are the nodal lines of
cos(n pi x) cos(m pi y) - cos(m pi x) cos(n pi y), drawn by marching squares; the Lissajous trace is (sin 3t, sin 2t).
"""
import math
import os
import subprocess
import sys

import numpy as np

OUT_DIR = sys.argv[1]
HTML = os.path.join(OUT_DIR, 'F7_istruzioni.html')
PDF = os.path.join(OUT_DIR, 'F7_istruzioni_melodia.pdf')

BG = '#070b14'        # nero-blu profondo
STEEL = '#26354d'     # blu acciaio scuro: struttura e grani
STEEL_TXT = '#7d8fae'
CYAN = '#2ee6ff'      # cyan elettrico
MAGENTA = '#ff3dc1'
ACID = '#d9ff3a'      # giallo acido
INK = '#dfe7f5'       # testo: tinto, sotto la luminanza del Seme


def chladni_svg(n, m, size, colour, grains=True, stroke=1.6):
    """Nodal lines of cos(n pi x) cos(m pi y) - cos(m pi x) cos(n pi y) on [0,1]^2, by marching squares."""
    N = 220
    xs = np.linspace(0.0, 1.0, N)
    X, Y = np.meshgrid(xs, xs)
    Z = np.cos(n * math.pi * X) * np.cos(m * math.pi * Y) - np.cos(m * math.pi * X) * np.cos(n * math.pi * Y)
    segs = []
    s = size / (N - 1)

    def interp(p1, p2, v1, v2):
        t = v1 / (v1 - v2) if v1 != v2 else 0.5
        return (p1[0] + t * (p2[0] - p1[0]), p1[1] + t * (p2[1] - p1[1]))

    for i in range(N - 1):
        for j in range(N - 1):
            c = [(j, i), (j + 1, i), (j + 1, i + 1), (j, i + 1)]
            v = [Z[i, j], Z[i, j + 1], Z[i + 1, j + 1], Z[i + 1, j]]
            pts = []
            for k in range(4):
                a, b = k, (k + 1) % 4
                if (v[a] > 0) != (v[b] > 0):
                    pts.append(interp(c[a], c[b], v[a], v[b]))
            if len(pts) >= 2:
                segs.append((pts[0], pts[1]))
            if len(pts) == 4:
                segs.append((pts[2], pts[3]))
    d = ' '.join(f'M{a[0]*s:.1f} {a[1]*s:.1f}L{b[0]*s:.1f} {b[1]*s:.1f}' for a, b in segs)
    dots = ''
    if grains:
        rng = np.random.default_rng(7)
        pts = []
        while len(pts) < 700:
            x, y = rng.random(2)
            z = math.cos(n * math.pi * x) * math.cos(m * math.pi * y) - math.cos(m * math.pi * x) * math.cos(n * math.pi * y)
            if abs(z) < 0.025 and (x - 0.5) ** 2 + (y - 0.5) ** 2 > 0.012:
                pts.append((x * size, y * size))
        dots = ''.join(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="0.9"/>' for x, y in pts)
    return (f'<svg viewBox="0 0 {size} {size}" width="{size}" height="{size}" xmlns="http://www.w3.org/2000/svg">'
            f'<rect x="0.5" y="0.5" width="{size-1}" height="{size-1}" fill="none" stroke="{STEEL}" stroke-width="1"/>'
            f'<g fill="{STEEL_TXT}" opacity="0.55">{dots}</g>'
            f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="{stroke}" stroke-linecap="round" opacity="0.95"/></svg>')


def lissajous_svg(w, h):
    pts = []
    for k in range(0, 1201):
        t = 2 * math.pi * k / 1200
        x = w / 2 + (w / 2 - 6) * math.sin(3 * t + math.pi / 4)
        y = h / 2 + (h / 2 - 4) * math.sin(2 * t)
        pts.append(f'{x:.1f},{y:.1f}')
    return (f'<svg viewBox="0 0 {w} {h}" width="{w}" height="{h}" xmlns="http://www.w3.org/2000/svg">'
            f'<polyline points="{" ".join(pts)}" fill="none" stroke="{MAGENTA}" stroke-width="1.2" opacity="0.8"/></svg>')


def grid_svg():
    """One phrase: 4 bars x 16 sixteenths, with the meaning of 'inizio' and 'durata' (format only, not music)."""
    W, cell, top, left = 700, 10, 34, 20
    h = 150
    parts = [f'<svg viewBox="0 0 {W} {h}" width="100%" xmlns="http://www.w3.org/2000/svg" font-family="Consolas, monospace">']
    for s in range(65):
        x = left + s * cell
        strong = s % 16 == 0
        beat = s % 4 == 0
        colour = STEEL_TXT if strong else (STEEL if beat else '#141d2c')
        parts.append(f'<line x1="{x}" y1="{top}" x2="{x}" y2="{top+60}" stroke="{colour}" stroke-width="{1.6 if strong else 1}"/>')
        if strong and s < 64:
            parts.append(f'<text x="{x+3}" y="{top-8}" fill="{STEEL_TXT}" font-size="11">BATTUTA {s//16+1}</text>')
        if beat:
            parts.append(f'<text x="{x-3}" y="{top+76}" fill="{STEEL_TXT}" font-size="9">{s}</text>')
    parts.append(f'<rect x="{left}" y="{top}" width="{64*cell}" height="60" fill="none" stroke="{STEEL}"/>')
    # example note: inizio 4, durata 4
    x0 = left + 4 * cell
    parts.append(f'<rect x="{x0}" y="{top+22}" width="{4*cell}" height="14" fill="{CYAN}" opacity="0.9"/>')
    parts.append(f'<line x1="{x0}" y1="{top+92}" x2="{x0}" y2="{top+40}" stroke="{ACID}" stroke-dasharray="3 3"/>')
    parts.append(f'<text x="{x0-2}" y="{top+106}" fill="{ACID}" font-size="11">inizio = 4</text>')
    parts.append(f'<path d="M{x0} {top+18}L{x0+4*cell} {top+18}" stroke="{MAGENTA}" stroke-width="1.4"/>')
    parts.append(f'<rect x="{x0+4*cell+3}" y="{top+10}" width="232" height="14" fill="{BG}"/>')
    parts.append(f'<text x="{x0+4*cell+6}" y="{top+21}" fill="{MAGENTA}" font-size="11">durata = 4 sedicesimi (un quarto)</text>')
    parts.append(f'<text x="{left+64*cell-150}" y="{top+106}" fill="{STEEL_TXT}" font-size="11">fine frase = 63</text>')
    parts.append('</svg>')
    return ''.join(parts)


CSS = f"""
@page {{ size: A4; margin: 0; }}
* {{ box-sizing: border-box; }}
html, body {{ margin: 0; background: {BG}; color: {INK}; -webkit-print-color-adjust: exact; print-color-adjust: exact; }}
body {{ font-family: 'Segoe UI', 'Bahnschrift', sans-serif; font-size: 10.5pt; line-height: 1.5; }}
.page {{ width: 210mm; height: 297mm; overflow: hidden; padding: 18mm 18mm 16mm 18mm; position: relative; page-break-after: always; }}
.page:last-child {{ page-break-after: auto; }}
.label {{ font-family: 'Bahnschrift', 'Segoe UI', sans-serif; font-variant: small-caps; letter-spacing: 0.14em; color: {STEEL_TXT}; font-size: 9pt; }}
h1 {{ font-family: 'Bahnschrift', 'Segoe UI', sans-serif; font-weight: 300; font-size: 30pt; letter-spacing: 0.02em; margin: 6mm 0 2mm 0; color: {INK}; }}
h1 span {{ color: {CYAN}; }}
h2 {{ font-family: 'Bahnschrift', 'Segoe UI', sans-serif; font-weight: 400; font-variant: small-caps; letter-spacing: 0.12em;
     font-size: 13pt; color: {CYAN}; border-bottom: 1px solid {STEEL}; padding-bottom: 1.5mm; margin: 8mm 0 3mm 0; }}
p {{ margin: 0 0 2.5mm 0; }}
ul {{ margin: 0 0 3mm 0; padding-left: 5mm; }}
li {{ margin-bottom: 1.6mm; }}
li::marker {{ color: {MAGENTA}; }}
b {{ color: #ffffff; font-weight: 600; }}
code, .mono {{ font-family: 'Consolas', 'Cascadia Mono', monospace; font-variant-numeric: tabular-nums; }}
code {{ color: {ACID}; font-size: 9.5pt; }}
pre {{ font-family: 'Consolas', monospace; background: #0c1322; border: 1px solid {STEEL}; border-left: 3px solid {ACID};
      padding: 3mm 4mm; color: {INK}; font-size: 9.5pt; margin: 2mm 0 3mm 0; }}
.hero {{ display: flex; gap: 10mm; align-items: center; margin-top: 6mm; }}
.intro {{ font-size: 12pt; color: {INK}; }}
.card {{ border: 1px solid {STEEL}; padding: 4mm 5mm; margin: 3mm 0; background: #0a101d; }}
.card.a {{ border-left: 3px solid {CYAN}; }}
.card.b {{ border-left: 3px solid {MAGENTA}; }}
.tag {{ display: inline-block; font-family: 'Bahnschrift', sans-serif; font-variant: small-caps; letter-spacing: 0.12em;
       font-size: 9pt; padding: 0.5mm 2mm; border: 1px solid; margin-right: 2mm; }}
.tag.a {{ color: {CYAN}; border-color: {CYAN}; }} .tag.b {{ color: {MAGENTA}; border-color: {MAGENTA}; }}
table {{ border-collapse: collapse; width: 100%; font-size: 9.5pt; margin: 2mm 0; }}
td, th {{ border-bottom: 1px solid {STEEL}; padding: 1.6mm 2mm; text-align: left; vertical-align: top; }}
th {{ font-variant: small-caps; letter-spacing: 0.1em; color: {STEEL_TXT}; font-weight: 400; }}
.footer {{ position: absolute; left: 18mm; right: 18mm; bottom: 9mm; display: flex; justify-content: space-between;
          font-family: 'Consolas', monospace; font-size: 8pt; color: {STEEL_TXT}; border-top: 1px solid {STEEL}; padding-top: 2mm; }}
.fig {{ margin: 3mm 0 1mm 0; }}
.caption {{ font-size: 8.5pt; color: {STEEL_TXT}; }}
.chladni-row {{ display: flex; gap: 6mm; margin-top: 4mm; }}
"""


def page(body, n, total):
    return (f'<section class="page">{body}<div class="footer"><span>CYMATICA · M2.5.1c · CONTROLLO POSITIVO F7</span>'
            f'<span>{n} / {total}</span></div></section>')


p1 = f"""
<div class="label">Cymatica · richiesta all'ascoltatore esperto</div>
<h1>Una melodia che <span>si sviluppa</span></h1>
<div class="hero">
  <div style="flex:1">
    <p class="intro">Grazie per l'aiuto. Ti chiediamo di scrivere una melodia, nel modo in cui la scriveresti per un brano vero.
    Ci serve come esempio umano di «sviluppo» per controllare uno strumento di analisi del nostro gioco musicale.</p>
    <p>Non serve conoscere lo strumento: anzi, è meglio di no. Scrivi come ti viene naturale.</p>
  </div>
  <div>{chladni_svg(2, 5, 190, CYAN)}</div>
</div>

<h2>Cosa scrivere</h2>
<ul>
  <li><b>Una sola linea melodica</b>, come un lead synth o una voce: una nota alla volta, senza accordi.</li>
  <li><b>Circa 24 frasi</b>, ognuna di <b>4 battute in 4/4</b>. In tutto circa 96 battute.</li>
  <li>Parti da <b>un motivo breve</b> e <b>sviluppalo</b> nel corso del brano, come faresti tu: variazioni, trasformazioni,
      ritorni, contrasti. Non deve essere un loop identico e non deve essere casuale.</li>
  <li>Tonalità, tempo e stile sono liberi. Un riferimento possibile: Re dorico, intorno ai 120 BPM.</li>
  <li>La griglia più fine che leggiamo sono i <b>sedicesimi</b>: le note più veloci verranno arrotondate al sedicesimo.</li>
</ul>

<h2>La griglia di una frase</h2>
<div class="fig">{grid_svg()}</div>
<p class="caption">Una frase = 4 battute × 16 sedicesimi, posizioni da 0 a 63. La barra di esempio mostra solo come si leggono
«inizio» e «durata»: non è un suggerimento musicale.</p>
<div class="chladni-row">
  <div>{chladni_svg(1, 4, 120, MAGENTA, grains=False, stroke=1.3)}</div>
  <div>{chladni_svg(2, 7, 120, ACID, grains=False, stroke=1.1)}</div>
  <div>{chladni_svg(4, 5, 120, CYAN, grains=False, stroke=1.1)}</div>
  <div style="align-self:center">{lissajous_svg(150, 110)}</div>
</div>
"""

p2 = f"""
<div class="label">Consegna</div>
<h2 style="margin-top:4mm">Come consegnarla</h2>
<p>Scegli una delle due strade.</p>
<div class="card a"><span class="tag a">A · consigliata</span><b>File MIDI</b>
  <p style="margin-top:2mm">Scrivi la melodia in qualsiasi programma (MuseScore, che è gratuito, oppure la tua DAW) ed esporta un file
  <code>.mid</code> con una sola traccia. Ogni frase deve iniziare su una battuta multipla di 4: battuta 1, 5, 9, e così via.</p>
</div>
<div class="card b"><span class="tag b">B</span><b>Tabella di testo</b>
  <p style="margin-top:2mm">Un file di testo, una nota per riga, con quattro colonne separate da virgole.</p>
<pre>frase, inizio, nota, durata
1, 0, D4, 4
1, 4, F4, 4
2, 0, D4, 2</pre>
  <table>
    <tr><th>Colonna</th><th>Significato</th></tr>
    <tr><td class="mono">frase</td><td>numero della frase, da 1 a 24 circa</td></tr>
    <tr><td class="mono">inizio</td><td>in sedicesimi dall'inizio della frase, da 0 a 63 (4 battute × 16 sedicesimi)</td></tr>
    <tr><td class="mono">nota</td><td>nome con l'ottava (C4 è il Do centrale; diesis con <code>#</code>, bemolli con <code>b</code>: <code>F#4</code>, <code>Bb3</code>)</td></tr>
    <tr><td class="mono">durata</td><td>in sedicesimi (4 = un quarto, 2 = un ottavo, 16 = una battuta intera)</td></tr>
  </table>
  <p class="caption">Le righe dell'esempio mostrano solo il formato, non un suggerimento musicale.</p>
</div>

<h2>Una nota facoltativa, in un file separato</h2>
<p>Se vuoi, scrivi in poche righe come hai sviluppato il motivo: per esempio «frasi 5–8: inversione; frase 17: ritorno del tema».
La teniamo <b>separata</b> e la apriamo solo dopo la misura, per confrontare quello che hai fatto con quello che lo strumento vede.</p>

<div style="display:flex; gap:8mm; align-items:flex-start">
<div style="flex:1">
<h2>Cosa ne faremo</h2>
<ul>
  <li>Convertiamo la melodia nel formato dello strumento, senza modificarne le note.</li>
  <li>La misura viene confrontata con quello che ci aspettiamo da uno sviluppo reale. Le aspettative sono state fissate
      prima di ricevere il tuo file.</li>
  <li>Il tuo nome non compare nei documenti del progetto.</li>
</ul>
</div>
<div style="margin-top:10mm">{chladni_svg(5, 8, 110, MAGENTA, grains=False, stroke=1.0)}</div>
</div>
"""

html = (f'<!doctype html><html lang="it"><head><meta charset="utf-8"><title>CYMATICA — Una melodia che si sviluppa</title>'
        f'<style>{CSS}</style></head><body>{page(p1, 1, 2)}{page(p2, 2, 2)}</body></html>')
os.makedirs(OUT_DIR, exist_ok=True)
with open(HTML, 'w', encoding='utf-8') as f:
    f.write(html)
edge = r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe'
subprocess.run([edge, '--headless=new', '--disable-gpu', '--no-pdf-header-footer', f'--print-to-pdf={os.path.abspath(PDF)}',
                'file:///' + os.path.abspath(HTML).replace('\\', '/')], check=True, timeout=120)
print(PDF, os.path.getsize(PDF))

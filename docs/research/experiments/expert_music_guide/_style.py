"""Shared CYMATICA PDF style (DESIGN.md §13, Sintetico palette), copied from m25_ensemble_tools/make_f7_pdf.py."""
import math

import numpy as np

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



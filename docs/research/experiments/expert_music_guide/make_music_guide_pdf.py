"""Builds the expert music guide PDF (mood of CYMATICA, the piece of today, what we ask: F7 and authored tables).

Usage: python make_music_guide_pdf.py <output_dir>   (e.g. artifacts/expert). Needs numpy and Microsoft Edge.
The text follows the owner's Google Doc "Cymatica Audio Design" (2026-11-18), corrected against DESIGN.md, plus the F7
instructions of m25_ensemble_tools/F7_istruzioni_per_l_esperto.md: change them together. No name of the expert here.
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from _style import ACID, BG, CSS, CYAN, INK, MAGENTA, STEEL, STEEL_TXT, chladni_svg, grid_svg, lissajous_svg  # noqa: E402

OUT_DIR = sys.argv[1]
HTML = os.path.join(OUT_DIR, 'CYMATICA_guida_musicale.html')
PDF = os.path.join(OUT_DIR, 'CYMATICA_guida_musicale.pdf')

EXTRA_CSS = f"""
.small {{ font-size: 9.5pt; }}
td.k {{ color: {CYAN}; white-space: nowrap; font-family: 'Bahnschrift', sans-serif; }}
.quote {{ font-size: 12.5pt; color: {INK}; border-left: 3px solid {CYAN}; padding: 1mm 0 1mm 4mm; margin: 3mm 0; }}
"""

TOTAL = 6


def page(body, n):
    return (f'<section class="page">{body}<div class="footer"><span>CYMATICA · GUIDA MUSICALE PER IL COMPOSITORE</span>'
            f'<span>{n} / {TOTAL}</span></div></section>')


p1 = f"""
<div class="label">Cymatica · guida per chi scrive la musica</div>
<h1>La musica che <span>diventa materia</span></h1>
<div class="hero">
  <div style="flex:1">
    <p class="intro">In CYMATICA la musica non fa da colonna sonora e non è un binario passivo: è la sorgente fisica che genera
    la geometria, i colori, gli ostacoli e le zone franche dell'arena di gioco.</p>
    <p>Il gioco si ispira alle <b>figure di Chladni</b> e alla <b>cimatica</b>: le forme che la sabbia assume su una piastra
    metallica che vibra sotto un suono. Quando il brano suona, lo schermo <i>è</i> la piastra: le frequenze muovono la
    materia, creando nodi (linee di quiete e di sicurezza) e ventri di vibrazione (onde e scariche di energia).</p>
  </div>
  <div>{chladni_svg(3, 5, 180, CYAN)}</div>
</div>

<h2>1 · Il mood</h2>
<p class="quote">Un esperimento acustico osservato da vicino, che diventa opera d'arte astratta.</p>
<ul>
  <li><b>Riferimenti sonori:</b> il sound design materico e aggressivo di Mick Gordon, l'elettronica generativa e
      architettonica di Max Cooper e Disasterpeace, il minimalismo di processo di Steve Reich (mutamenti graduali e
      percepibili, sfasamenti, poliritmi).</li>
  <li><b>Non un rhythm game generico:</b> molti giochi musicali allineano ostacoli su una cassa in quattro o su campioni
      rétro. CYMATICA chiede una personalità compositiva riconoscibile, organica e rigorosa, in cui il giocatore sente che
      ogni cambiamento sullo schermo ha una causa acustica.</li>
  <li><b>Il Seme:</b> il giocatore è una frequenza pura intrappolata in una piastra instabile. Non spara: sopravvive
      accordandosi al campo.</li>
  <li><b>Purezza e dissonanza:</b> al posto della salute c'è la dissonanza. Quando cresce, il suono perde nitidezza per gradi
      (purezza, disturbo, instabilità, collasso). La musica ha bisogno di un punto di partenza nitido e consonante, da cui
      allontanarsi e a cui tornare.</li>
  <li><b>Il ritmo aiuta a prevedere:</b> il giocatore usa il Pulso per anticipare i pericoli. Un ritmo che si sente e si può
      contare è parte del gioco.</li>
  <li><b>Il silenzio è uno strumento:</b> prima di un culmine, una sottrazione breve rende tutto più leggibile e aumenta
      l'attesa.</li>
</ul>
<p class="small"><b>Riferimenti visivi:</b> Chladni, <i>Cymatics</i> di Hans Jenny, Oskar Fischinger, Norman McLaren, John Whitney,
figure di Lissajous, sabbia vibrante. <b>Da evitare:</b> synthwave e outrun, glitch decorativo.
<b>Parole chiave:</b> materia, precisione, risonanza, tensione e ritorno.</p>
"""

p2 = f"""
<div class="label">Come scrivere per il gioco</div>
<h2 style="margin-top:4mm">2 · I quattro ruoli</h2>
<p>Scrivere per CYMATICA significa scrivere una partitura che è anche il livello. Il gioco legge la musica in quattro ruoli,
che devono restare distinti e leggibili:</p>
<table>
  <tr><th>Ruolo</th><th>In musica</th><th>Nel gioco</th></tr>
  <tr><td class="k">Pulso</td><td>batteria, transienti: l'àncora metrica</td><td>onde d'urto, flash, finestre per agire. Transienti puliti</td></tr>
  <tr><td class="k">Corpo</td><td>accordi, pad e basso: gravità e struttura</td><td>le note tenute creano le linee nodali stabili, i muri del labirinto. Meno note, più leggibilità</td></tr>
  <tr><td class="k">Trama</td><td>arpeggi, figure ritmiche, tessiture</td><td>la pioggia di micro-proiettili. Si incastra con il Pulso senza ingolfare il mix</td></tr>
  <tr><td class="k">Vettore</td><td>la melodia: lead, voce, solista</td><td>anima l'entità che insegue e ne guida le traiettorie</td></tr>
</table>

<h2>Sviluppo, non loop</h2>
<ul>
  <li><b>Un motivo chiaro:</b> un'idea tematica riconoscibile per il Vettore.</li>
  <li><b>Trasformazioni che ne conservano l'identità:</b> trasposizione, cambio di registro, varianti della coda,
      aumentazione e diminuzione, invece di materiale slegato. Nel gioco la trasformazione del tema diventa una
      trasformazione geometrica dei proiettili.</li>
  <li><b>Una forma con un ritorno:</b> esposizione (A), contrasto e tensione con un cambio di tempo o di metro (B), ritorno
      trasformato del tema prima della risoluzione (A′).</li>
</ul>

<h2>Respiro, preavviso, dinamica</h2>
<ul>
  <li><b>Niente muro di suono:</b> un brano iper-compresso a 0 dBFS trasforma il livello in una schermata bianca e cancella
      il contrasto fra tensione e recupero. Servono margini dinamici e un respiro percussivo (ducking ritmico): ogni colpo di
      cassa abbassa per un attimo pad e synth, e la piastra pulsa.</li>
  <li><b>Annuncia i cambiamenti:</b> un fill, un piatto al contrario, un filtro che si apre o una sospensione armonica prima
      di un drop. Il gioco li usa per mostrare in anticipo dove arriverà il pericolo.</li>
</ul>
<div style="display:flex; gap:6mm; margin-top:3mm">{chladni_svg(1, 4, 100, MAGENTA, grains=False, stroke=1.2)}{chladni_svg(2, 7, 100, ACID, grains=False, stroke=1.0)}{chladni_svg(4, 5, 100, CYAN, grains=False, stroke=1.0)}<div style="align-self:center">{lissajous_svg(140, 100)}</div></div>
"""

p3 = f"""
<div class="label">Gli stati della piastra</div>
<h2 style="margin-top:4mm">3 · Gli archetipi</h2>
<p>Il gioco conosce cinque stati fisici e comportamentali della piastra. Non sono generi musicali: i generi indicano solo
un'affinità. Più avanti ogni archetipo avrà la sua palette di strumenti.</p>
<table>
  <tr><th>Archetipo</th><th>Affinità</th><th>Nel gioco</th><th>Nel suono</th></tr>
  <tr><td class="k">Sintetico<br>Matriziale</td><td>EDM, techno, chiptune</td><td>quantizzazione rigida, griglie a 90°, ritmo euclideo</td><td>transienti definiti, sequenze, arpeggi, sidechain, timbri digitali con dettaglio analogico</td></tr>
  <tr><td class="k">Organico<br>Concentrico</td><td>classica, barocca, acustica</td><td>cerchi concentrici, curve, l'arena che si espande e si contrae con la dinamica</td><td>corde e modelli fisici, pad che respirano, dinamica ampia, fraseggio espressivo</td></tr>
  <tr><td class="k">Fratturato<br>Caotico</td><td>metal, noise, industrial</td><td>muri che si crepano in schegge sui colpi di distorsione</td><td>saturazione, distorsione controllata, percussioni fisiche, rumore intonato</td></tr>
  <tr><td class="k">Etereo<br>Sospeso</td><td>ambient, drone, minimal</td><td>masse lente che ruotano, bassa densità</td><td>tessiture granulari, drone, sub, riverbero lungo ma non confuso, eventi radi</td></tr>
  <tr><td class="k">Sincopato<br>Spostato</td><td>IDM, math rock, jazz</td><td>tempi dispari, poliritmi, la piastra che ruota sugli accenti spostati</td><td>sincopi, polimetri, timbri acustici ed elettronici misti, accenti secchi</td></tr>
</table>
<p class="small">Sul Sintetico stiamo provando un timbro 8-bit chiptune. Se scrivendo un motivo o un giro ti viene in mente a quale
archetipo si addice, scrivilo accanto.</p>

<h2>4 · Il brano di oggi</h2>
<p>Il materiale che ti chiediamo si innesta sul brano procedurale che il gioco produce oggi.</p>
<table>
  <tr><td class="k">Forma</td><td>A, B, A′ (ritorno di A); 96 battute, circa 3 minuti; frasi di 4 battute in 4/4, un accordo per battuta</td></tr>
  <tr><td class="k">A e A′</td><td>Re dorico, 120 BPM; giro Re m, Sol, La m, Re m</td></tr>
  <tr><td class="k">B</td><td>Sol eolio, 140 BPM; giro Sol m, Mi♭, Fa, Sol m; a metà il kick si sposta di un ottavo per due frasi</td></tr>
  <tr><td class="k">Ruoli</td><td>Pulso: il kick. Corpo: accordi a due voci con il basso. Trama: arpeggi rapidi. Vettore: la melodia, sopra gli accordi</td></tr>
  <tr><td class="k">Melodia</td><td>un motivo di una battuta che a ogni battuta si sposta sulla fondamentale dell'accordo e cambia di frase in frase, con note di passaggio aggiunte o tolte. Oggi in A: Re, Fa, La, Sol</td></tr>
</table>
<p>Il gioco sceglie fra alternative scritte da una persona e le combina in modo diverso a ogni partita. Più alternative valide
ci sono, più ogni partita suona diversa senza diventare casuale.</p>
"""

p4 = f"""
<div class="label">Cosa ti chiediamo</div>
<h2 style="margin-top:4mm">5 · Due richieste</h2>
<div class="card a"><span class="tag a">A</span><b>La melodia F7</b>
  <p style="margin-top:2mm">La richiesta già inviata, senza cambiamenti: una sola linea melodica di circa 24 frasi da 4 battute
  in 4/4, che parte da un motivo breve e lo sviluppa come faresti tu. Le istruzioni complete sono nelle pagine 5 e 6. Il mood
  descritto qui serve solo come ispirazione: per F7 conta che sia una melodia che si sviluppa, scritta come per un brano vero.</p>
</div>
<div class="card b"><span class="tag b">B</span><b>Alternative per il brano del gioco</b>
  <p style="margin-top:2mm">Questo materiale entra direttamente nelle tabelle da cui il gioco sceglie. Qualunque quantità aiuta,
  anche una sola voce per tipo.</p>
  <ul>
    <li><b>Motivi d'apertura:</b> due o tre per sezione, ciascuno di <b>una battuta di 4/4</b> (16 sedicesimi), una nota alla
        volta, nel modo della sezione: Re dorico per A, Sol eolio per B. Conviene che la prima nota sia la fondamentale o la
        quinta dell'accordo. Pensali come figure che reggono la ripetizione su accordi diversi e qualche variazione.</li>
    <li><b>Giri d'accordi:</b> uno o due alternativi per sezione, quattro accordi per frase, uno per battuta, nel modo della
        sezione. Se usi note fuori dal modo, per esempio la sensibile, indicalo.</li>
    <li><b>Facoltativi:</b> schemi d'arpeggio di una battuta per la Trama, con le note dell'accordo indicate come 1, 3, 5 e 8,
        e linee di basso di una frase.</li>
  </ul>
</div>
<h2>Formato delle alternative</h2>
<p>Lo stesso della tabella di F7 (pagina 6): una nota per riga. Per un motivo d'apertura la «frase» è il numero del motivo e
l'inizio va da 0 a 15. Per i giri basta il nome dell'accordo per battuta.</p>
<pre>motivo, inizio, nota, durata        giro A2: Re m, Do, Sol, Re m
1, 0, D4, 4                         giro B2: Sol m, Fa, Mi♭, Re
1, 4, F4, 2</pre>
<p class="caption">Le righe mostrano solo il formato, non un suggerimento musicale.</p>
<p>Il gioco usa il materiale così come lo scrivi: lo sposta sugli accordi, lo varia e lo combina con il resto.
Il tuo nome non compare nei documenti del progetto.</p>
"""

p5 = f"""
<div class="label">Istruzioni F7 · 1 di 2</div>
<h2 style="margin-top:4mm">Una melodia che si sviluppa</h2>
<p>Ti chiediamo di scrivere una melodia, nel modo in cui la scriveresti per un brano vero. Ci serve come esempio umano di
«sviluppo» per controllare uno strumento di analisi del gioco. Non serve conoscere lo strumento: anzi, è meglio di no.</p>
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
<h2>Una nota facoltativa, in un file separato</h2>
<p>Se vuoi, scrivi in poche righe come hai sviluppato il motivo: per esempio «frasi 5–8: inversione; frase 17: ritorno del
tema». La teniamo <b>separata</b> e la apriamo solo dopo la misura, per confrontare quello che hai fatto con quello che lo
strumento vede.</p>
"""

p6 = f"""
<div class="label">Istruzioni F7 · 2 di 2</div>
<h2 style="margin-top:4mm">Come consegnarla</h2>
<p>Scegli una delle due strade.</p>
<div class="card a"><span class="tag a">A · consigliata</span><b>File MIDI</b>
  <p style="margin-top:2mm">Scrivi la melodia in qualsiasi programma (MuseScore, che è gratuito, oppure la tua DAW) ed esporta un
  file <code>.mid</code> con una sola traccia. Ogni frase deve iniziare su una battuta multipla di 4: battuta 1, 5, 9, e così via.</p>
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

pages = [p1, p2, p3, p4, p5, p6]
assert len(pages) == TOTAL
html = (f'<!doctype html><html lang="it"><head><meta charset="utf-8"><title>CYMATICA · Guida musicale</title>'
        f'<style>{CSS}{EXTRA_CSS}</style></head><body>' + ''.join(page(p, i + 1) for i, p in enumerate(pages)) + '</body></html>')
os.makedirs(OUT_DIR, exist_ok=True)
with open(HTML, 'w', encoding='utf-8') as f:
    f.write(html)
edge = r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe'
subprocess.run([edge, '--headless=new', '--disable-gpu', '--no-pdf-header-footer', f'--print-to-pdf={os.path.abspath(PDF)}',
                'file:///' + os.path.abspath(HTML).replace('\\', '/')], check=True, timeout=120)
print(PDF, os.path.getsize(PDF))

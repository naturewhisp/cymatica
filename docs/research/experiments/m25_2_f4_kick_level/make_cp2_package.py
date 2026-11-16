"""Build the CP-2 listening packages for F4a (sessions 1 and 2) and the conductor's key.

Design and hypotheses: docs/research/CP2_S1_Listening_Hypotheses.md (hash in progress.md before this script ran).
Inputs: the clips of `cymatica_game --cacp-run kick_level_ab_v1` (artifacts/cacp/kick_level_ab_v1).
Outputs (not versioned), under --out (default artifacts/expert_listening):
  cymatica_ascolto_CP2_sessione1_<date>/ and .zip   -> for the listeners
  cymatica_ascolto_CP2_sessione2_<date>/ and .zip   -> for the listeners (ABX, another day)
  CHIAVE_CP2_<date>_NON_INVIARE.txt                 -> conductor only, never sent
The shuffle seed is drawn from the OS unless --seed is given, and is written only in the key file.
Usage: python -I make_cp2_package.py --clips artifacts/cacp/kick_level_ab_v1 --date 2026-11-16
"""
import argparse
import hashlib
import random
import secrets
import shutil
import zipfile
from pathlib import Path

OFFSETS = ['+00dB', '+03dB', '+06dB', '+09dB', '+12dB']
SECTIONS = ['A', 'B', 'Aprime']
SECTION_LABEL = {'A': "A (Re dorico, 120 BPM)", 'B': "B (Sol eolio, 140 BPM)", 'Aprime': "A' (120 BPM)"}
ABX_TRIALS = 16


def clip(clips, section, offset):
    return clips / f'f4a_{section}_{offset}_aligned.wav'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def make_zip(folder):
    # No per-file checksum list inside the package: identical hashes would reveal the hidden duplicate, the
    # identical pair and the ABX answers. Integrity is checked with the zip hash, written in the key only.
    z = folder.with_suffix('.zip')
    with zipfile.ZipFile(z, 'w', zipfile.ZIP_DEFLATED) as zf:
        for p in sorted(folder.rglob('*')):
            if p.is_file():
                zf.write(p, (Path(folder.name) / p.relative_to(folder)).as_posix())
    return z


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\r\n')


LEGGIMI_1 = """CYMATICA — ascolto CP-2, sessione 1 (livello del kick)
=====================================================

Grazie per l'ascolto. Durata prevista: circa 40 minuti, con una pausa a metà.
Se ti stanchi o ti annoi, scrivilo nel modulo: è un dato utile, non un problema.

CHE COSA È CAMBIATO rispetto all'ascolto del 3 novembre
- Nel brano completo è cambiato solo il livello del kick.
- Nelle classifiche e nelle coppie le clip dello stesso gruppo differiscono SOLO
  per il livello del kick. Hanno tutte la stessa loudness complessiva, quindi
  non scegliere in base al volume generale.

PRIMA DI INIZIARE
1. Usa sempre le stesse cuffie o gli stessi diffusori e annota il modello.
2. Nel mixer volume di Windows porta al 100 % il cursore del lettore audio che usi.
   Annota il volume di sistema.
3. Ascolta i primi 20 secondi di 01_brano_completo e scegli un volume a cui il
   kick sia giudicabile. Poi NON cambiarlo più per tutta la sessione.

ORDINE
1. 01_brano_completo/brano.wav (circa 3 minuti), primo ascolto.
   Prima scrivi con parole tue che cosa senti, senza guardare le domande.
   Poi dai le tre valutazioni da 1 a 7 (vedi MODULO.txt).
2. 02_classifiche: tre gruppi. Per ogni gruppo, ascolta le clip quante volte
   vuoi, in qualsiasi ordine, poi scrivi:
   - quali clip non riesci a distinguere tra loro, se ce ne sono;
   - la classifica, dalla preferita alla meno gradita;
   - perché la prima ti convince e l'ultima no.
3. PAUSA di qualche minuto.
4. 01_brano_completo/brano.wav, secondo ascolto. Segna minuto e secondo in cui
   l'interesse cala, se cala. Per ciascuna sezione rispondi: il tempo forte si
   percepisce? Le sezioni nel brano sono:
     A   0:00 – 1:04
     B   1:04 – 1:59
     A'  1:59 – fine
   Se la percezione del tempo forte cambia, indica dove.
5. 03_coppie: quattro coppie (prima.wav, seconda.wav). Per ciascuna:
   - senti una differenza? sì / no / non so;
   - quale preferisci?
   - quale funziona meglio musicalmente, e perché?
6. Domande finali nel modulo: artefatti, timbro e livelli, osservazioni libere.

Le viste e i nomi delle versioni NON sono nel pacchetto: chi conduce ha la
chiave e la rivela a fine sessione.

04_secondo_ascoltatore contiene le domande semplici e una coppia per un
eventuale secondo ascoltatore, che non deve conoscere la musica.

Per favore non confrontare i file con strumenti (dimensioni, hash, forme
d'onda): il giudizio deve venire solo dall'ascolto.
"""

LEGGIMI_2 = """CYMATICA — ascolto CP-2, sessione 2 (prove ABX)
==============================================

Da fare in un giorno diverso dalla sessione 1. Durata: circa 20 minuti.
Stesse cuffie o diffusori della sessione 1, cursore del lettore al 100 %,
volume fisso scelto all'inizio.

Ci sono due cartelle, abx_1 e abx_2. In ciascuna:
- riferimento_1.wav e riferimento_2.wav sono due versioni che differiscono
  solo per il livello del kick, alla stessa loudness complessiva;
- prove/prova_01.wav ... prova_16.wav: ogni prova è uguale a uno dei due
  riferimenti.

Per ogni prova ascolta quante volte vuoi la prova e i due riferimenti, poi
scrivi 1 o 2 nel modulo. Non confrontare i file con strumenti: solo ascolto. Se non sai, scegli comunque: il caso viene contato
nel calcolo. Fai una breve pausa fra abx_1 e abx_2.
"""

MODULO_1 = """MODULO DI RISPOSTA — CP-2, sessione 1
(nessun nome nei documenti: scrivi solo «esperto» o «secondo ascoltatore»)

Data e ora, durata effettiva:
Ascoltatore (esperto / secondo ascoltatore):
Apparecchiatura (cuffie / diffusori / interfaccia):
Volume di sistema e cursore del lettore (verificato al 100 %):
Pacchetto (nome della cartella):

1. BRANO COMPLETO, primo ascolto
Descrizione libera:

Coerenza 1–7 (1 = le parti non stanno insieme, 7 = suonano come un brano unico):
Interesse 1–7 (1 = annoia subito, 7 = tiene l'attenzione fino alla fine):
Ripetitività 1–7 (1 = mai ripetitivo, 7 = ripetitivo in modo fastidioso):

2. CLASSIFICHE
gruppo_1 — clip indistinguibili:            classifica:            perché prima / ultima:
gruppo_2 — clip indistinguibili:            classifica:            perché prima / ultima:
gruppo_3 — clip indistinguibili:            classifica:            perché prima / ultima:

4. BRANO COMPLETO, secondo ascolto
Istante del calo d'interesse (mm:ss), se c'è:
Q3 — il tempo forte si percepisce?   A:          B:          A':
     dove cambia, se cambia:

5. COPPIE (per ciascuna: differenza sì/no/non so — preferita — perché)
coppia_1:
coppia_2:
coppia_3:
coppia_4:

6. DOMANDE FINALI
Q1 — crackle, click o artefatti? dove (mm:ss)?
Q6 — timbro e livelli relativi degli strumenti; il kick ora come sta nel mix?
Q8 — osservazioni libere (musicalità, direzione artistica):

Affaticamento o noia (se sì, da quando):
Esito complessivo (libero, anche negativo o inconcludente):
Chiave rivelata a fine sessione (la compila chi conduce):
"""

SECONDO = """DOMANDE PER IL SECONDO ASCOLTATORE
(nessuna risposta è sbagliata; si può rispondere «non so»)

Ascolta ../01_brano_completo/brano.wav, poi rispondi:
1. Che cosa hai sentito? Descrivilo con parole tue.
2. Ti è piaciuto? Da 1 a 7.
3. Ti sei annoiato? Se sì, più o meno quando?
4. Hai sentito dei cambi di «parte» nel brano? Quanti, e più o meno quando?
5. Riuscivi a battere il piede a tempo? Sempre, a tratti, mai?
6. Hai sentito rumori strani, scatti o fruscii? Quando?

Poi ascolta la coppia in questa cartella (prima.wav, seconda.wav):
7. Le due versioni ti sembrano uguali o diverse? Se diverse, quale preferisci?

Durata effettiva:          Apparecchiatura:          Volume (fisso):
"""

MODULO_2 = """MODULO DI RISPOSTA — CP-2, sessione 2 (ABX)

Data e ora, durata effettiva:
Ascoltatore (esperto / secondo ascoltatore):
Apparecchiatura e volume (come nella sessione 1?):

abx_1 — risposta (1 o 2) per prova 01..16:
01:   02:   03:   04:   05:   06:   07:   08:
09:   10:   11:   12:   13:   14:   15:   16:
Sensazione (facile / difficile / a caso):

abx_2 — risposta (1 o 2) per prova 01..16:
01:   02:   03:   04:   05:   06:   07:   08:
09:   10:   11:   12:   13:   14:   15:   16:
Sensazione (facile / difficile / a caso):

Affaticamento (se sì, da quando):
"""

ap = argparse.ArgumentParser()
ap.add_argument('--clips', required=True)
ap.add_argument('--out', default='artifacts/expert_listening')
ap.add_argument('--date', required=True)
ap.add_argument('--seed', type=int, default=None)
args = ap.parse_args()

clips = Path(args.clips)
out = Path(args.out)
seed = args.seed if args.seed is not None else secrets.randbits(64)
rng = random.Random(seed)

s1 = out / f'cymatica_ascolto_CP2_sessione1_{args.date}'
s2 = out / f'cymatica_ascolto_CP2_sessione2_{args.date}'
for d in (s1, s2):
    if d.exists():
        raise SystemExit(f'{d} exists: refusing to overwrite')
key = [f'Chiave dei pacchetti CP-2 F4a del {args.date} (NON inviare agli ascoltatori)',
       f'Seed del mescolamento (solo qui): {seed}',
       'Ipotesi: docs/research/CP2_S1_Listening_Hypotheses.md',
       'Livelli = offset sul guadagno di bus di Pulso (+00dB = produzione, -6 dB). Clip allineate in loudness all\'offset 0.', '']


def rel(p):
    return p.relative_to(out).as_posix()


def add(src, dst, label):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(src, dst)
    key.append(f'{rel(dst)}  =  {label}  [{src.name}]  sha256 {sha(dst)}')


# Session 1 ---------------------------------------------------------------------------------------------------
key.append('== Sessione 1 ==')
add(clips / 'f4a_full_+06dB_raw.wav', s1 / '01_brano_completo' / 'brano.wav', 'brano completo, kick +6 dB (grezzo)')
groups = SECTIONS[:]
rng.shuffle(groups)
for gi, sec in enumerate(groups, 1):
    items = [(o, clip(clips, sec, o)) for o in OFFSETS]
    if sec == 'B':
        dup = rng.choice(OFFSETS)
        items.append((dup + ' DUPLICATO NASCOSTO (B0)', clip(clips, sec, dup)))
    rng.shuffle(items)
    # B0: the hidden duplicate is never adjacent to its original (adjacent copies would be trivially spotted)
    while sec == 'B' and abs([i for i, (_, s) in enumerate(items) if s == clip(clips, sec, dup)][0] -
                             [i for i, (_, s) in enumerate(items) if s == clip(clips, sec, dup)][1]) < 2:
        rng.shuffle(items)
    key.append(f'-- gruppo_{gi}: sezione {SECTION_LABEL[sec]}')
    for ci, (o, src) in enumerate(items, 1):
        add(src, s1 / '02_classifiche' / f'gruppo_{gi}' / f'clip_{ci}.wav', f'sezione {sec}, kick {o}')
pairs = [('A', '+00dB', '+03dB', 'L2'), ('Aprime', '+00dB', '+03dB', 'L2'), ('B', '+06dB', '+09dB', 'S1'),
         ('Aprime', '+06dB', '+06dB', 'COPPIA IDENTICA (B0)')]
rng.shuffle(pairs)
for pi, (sec, a, b, tag) in enumerate(pairs, 1):
    first, second = (a, b) if rng.random() < 0.5 else (b, a)
    key.append(f'-- coppia_{pi}: sezione {sec}, {tag}')
    add(clip(clips, sec, first), s1 / '03_coppie' / f'coppia_{pi}' / 'prima.wav', f'sezione {sec}, kick {first}')
    add(clip(clips, sec, second), s1 / '03_coppie' / f'coppia_{pi}' / 'seconda.wav', f'sezione {sec}, kick {second}')
first, second = ('+00dB', '+09dB') if rng.random() < 0.5 else ('+09dB', '+00dB')
key.append('-- secondo ascoltatore: sezione A, 0 contro +9 dB')
add(clip(clips, 'A', first), s1 / '04_secondo_ascoltatore' / 'prima.wav', f'sezione A, kick {first}')
add(clip(clips, 'A', second), s1 / '04_secondo_ascoltatore' / 'seconda.wav', f'sezione A, kick {second}')
write(s1 / 'LEGGIMI.txt', LEGGIMI_1)
write(s1 / 'MODULO.txt', MODULO_1)
write(s1 / '04_secondo_ascoltatore' / 'DOMANDE.txt', SECONDO)

# Session 2 (ABX) ---------------------------------------------------------------------------------------------
key.append('')
key.append('== Sessione 2 (ABX, criterio L2: almeno 12/16 corrette su ciascun estratto) ==')
abx = ['A', 'Aprime']
rng.shuffle(abx)
for ai, sec in enumerate(abx, 1):
    refs = ['+00dB', '+03dB']
    rng.shuffle(refs)
    key.append(f'-- abx_{ai}: sezione {sec}; riferimento_1 = {refs[0]}, riferimento_2 = {refs[1]}')
    for ri, o in enumerate(refs, 1):
        add(clip(clips, sec, o), s2 / f'abx_{ai}' / f'riferimento_{ri}.wav', f'sezione {sec}, kick {o}')
    answers = []
    for t in range(1, ABX_TRIALS + 1):
        x = rng.randint(1, 2)
        answers.append(str(x))
        add(clip(clips, sec, refs[x - 1]), s2 / f'abx_{ai}' / 'prove' / f'prova_{t:02d}.wav', f'X = riferimento_{x} ({refs[x - 1]})')
    key.append(f'   risposte corrette abx_{ai} (prove 01..16): ' + ' '.join(answers))
write(s2 / 'LEGGIMI.txt', LEGGIMI_2)
write(s2 / 'MODULO.txt', MODULO_2)

z1, z2 = make_zip(s1), make_zip(s2)
key += ['', f'{z1.name}  sha256 {sha(z1)}', f'{z2.name}  sha256 {sha(z2)}']
keyfile = out / f'CHIAVE_CP2_{args.date}_NON_INVIARE.txt'
if keyfile.exists():
    raise SystemExit(f'{keyfile} exists: refusing to overwrite')
write(keyfile, '\n'.join(key) + '\n')
print(z1, z1.stat().st_size // 1_000_000, 'MB')
print(z2, z2.stat().st_size // 1_000_000, 'MB')
print(keyfile)

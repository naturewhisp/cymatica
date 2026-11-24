# M2.5 CP-3 — I8, accenti comuni fra batteria e basso: indagine. Pre-registrazione

- **Stato:** scritto il 2026-11-24, **prima** di codice e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Origine:** [ascolto dell'esperto](../m2_5_cp3_expert_listening_2026-11-23.md), voci di ADR-0007 §8.1: accenti scelti per misura, basso e batteria legati, «troppa sincope tra il kick e il basso».
- **Già noto:** in Z 22–25 % dei kick ha un attacco di basso; il basso dell'esperto coincide col kick nel 36 % dei casi e cade sui quarti nel 61 % (I3). Queste cifre non sono previsioni.
- **Fonti lette** (pratica di docenti, non esperimenti): vedi §4.
- **Materiale:** MIDI dell'esperto fuori dal repository (solo statistiche aggregate); piano di oggi e piano Z, seed 1337, con le rispettive linee V2. Griglia di 16 sedicesimi; fra Pulso e basso tolleranza di mezzo sedicesimo. Nostre frasi «pedale» e mute escluse; per l'esperto le battute con almeno un attacco di batteria **e** uno di basso.

## 1. Misure e previsioni

| ID | Misura | Previsione |
|---|---|---|
| A1 | Quota dei kick con un attacco di basso, meno il caso (media dei 15 spostamenti circolari dentro la battuta) | esperto ≥ 15 punti sopra il caso; Z ≤ 8 |
| A2 | Quota degli attacchi di basso su kick **o rullante** | esperto ≥ 50 % |
| A3 | Sincope per battuta (Longuet-Higgins e Lee, pesi 0 −1 −2 −3 −4 sui livelli metrici, somma diviso attacchi) di kick+basso uniti | Z > esperto |
| A4 | Velocity mediana degli attacchi di basso dell'esperto su kick meno quella degli altri | ≥ +5 (scala 1–127) |
| A5 | Correlazione di Spearman per battuta fra numero di attacchi di batteria e di basso | esperto ≤ −0,2 («se uno è complesso l'altro è semplice»); Z fra −0,2 e 0,2 |
| A6 | Posizioni della battuta in cui **entrambi** attaccano in almeno metà delle battute | esperto ≥ 2; Z ≤ 1 |

## 2. Regola

- Un prototipo con accenti comuni è **giustificato dalle misure** se valgono almeno tre fra A1, A3, A5 e A6.
- Se valgono meno di tre, il prototipo si può fare solo come **scelta d'autore sulla parola dell'esperto**, dichiarata: il suo brano non prova la sua regola.
- L'indagine non decide livelli, numero di accenti né percussioni.

## 3. Limiti

- Un brano dell'esperto, 162 BPM e altro genere; un nostro seed. Il suo MIDI ha kick e rullante, il nostro solo kick: A2 non si confronta.
- La sincope è una misura simbolica, non un giudizio; A4 dipende da come l'808 è stato suonato.

## 4. Fonti, lette il 2026-11-24

- **Sandlin**, «How To Lock In With Drums On Bass» (letta per intero, senza le immagini): mappare kick e rullante su una griglia di sedicesimi; il basso ha un rapporto chiaro e costante con loro, senza copiarli; occupa gli spazi. Nessuna regola su accenti, sincope o durata.
- **Heath**, «How Bass and Drums Lock Together» (letta per intero, figure illeggibili): nota di basso e kick che cadono insieme partono insieme; accentare le note sui tempi 1 e 3; agganciare prima i tempi forti e aggiungere i levare uno alla volta; fermare la nota sul rullante oppure farla risuonare, per scelta. Cita un libro e un elenco di rudimenti, non esperimenti.
- Nessuna delle due dà soglie misurabili. Sono opinioni di docenti, coerenti con l'esperto, ma non indipendenti dalla pratica comune.

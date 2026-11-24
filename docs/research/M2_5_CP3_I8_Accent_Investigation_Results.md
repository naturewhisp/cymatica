# M2.5 CP-3 — I8, accenti comuni fra batteria e basso: risultati dell'indagine

- **Pre-registrazione:** [documento](M2_5_CP3_I8_Accent_Investigation_Preregistration.md), SHA-256 (LF) `d4c266e4c328651ad0bf33d293ea8c13d723b39eb350abb664a1d60d2cef349d`.
- **Esecuzione:** 2026-11-24. **Dati:** [esperimento](experiments/m25_cp3_accents/README.md), solo statistiche aggregate. Nessun cambiamento alla musica.

| ID | Esperto (808) | Sub | Z | Esito |
|---|---|---|---|---|
| A1, kick con basso sopra il caso | 33 % contro 15 %: **+17,7 punti** | +22,2 | 25 % contro 21 %: **+4,0** | Confermata |
| A2, basso su kick o rullante | **70 %** | 65 % | — | Confermata |
| A3, sincope per attacco, kick+basso | 0,292 | 0,297 | 0,353 | Confermata, **scarto piccolo** |
| A4, velocity del basso sul kick | 0 punti in più | 0 | — | **Mancata** |
| A5, complessità batteria/basso | **+0,29** | +0,18 | +0,02 | **Mancata**: nel suo brano crescono insieme |
| A6, posizioni condivise in metà delle battute | 1 (il primo sedicesimo) | 1 | 1 | **Mancata**: non distingue |

## 1. Regola

Ne valgono **due su quattro** (A1 e A3; ne servivano tre): il prototipo **non è giustificato dalle misure**. Si può fare solo come **scelta d'autore sulla parola dell'esperto**, dichiarata.

## 2. Lettura

- **Il legame esiste ed è la differenza più netta:** rispetto al caso, il suo basso incontra il kick 17,7 punti in più, il nostro 4. Il nostro basso, scelto per coincidere al 36 %, è quasi indipendente dal kick.
- **Il 36 % ingannava.** La quota degli attacchi di basso sul kick è quasi uguale (36,5 contro 35,9 %), ma i nostri kick sono più fitti e il caso è più alto. Contava lo scarto dal caso, non la quota.
- **La sua regola «complesso contro semplice» non si vede nel suo brano:** la correlazione è positiva. È una pratica di improvvisazione, e il brano potrebbe non essere il posto dove cercarla. Non la smentisce e non la prova.
- **Gli accenti non sono nella velocity del basso:** non ci sono (A4), quindi sono nella collocazione e nei suoni, o non sono in quel MIDI.
- **Il rullante conta:** due terzi abbondanti del suo basso cadono su kick o rullante. A noi manca del tutto: la richiesta di percussioni e quella degli accenti sono legate.
- **Osservato dopo i dati, non previsto:** presi da soli, i nostri kick e basso sono molto più sincopati dei suoi (kick 0,87 contro 0,40; basso 0,74 contro 0,50). È coerente con «squadrare di più i ritmi», più di A3, che li somma. Da pre-registrare da capo prima di usarlo.

## 3. Proposta

Prototipo offline dichiarato come scelta d'autore, con una sua pre-registrazione:
1. **Accenti per misura:** per ogni frase una maschera di 2–4 posizioni, la prima sempre l'uno, comune a kick e basso. Il kick le suona tutte, il basso almeno metà; il basso riempie fuori solo se il kick è semplice.
2. **Kick meno sincopato:** criterio sull'indice da solo (0,87 oggi), con soglia scritta prima.
3. **Criteri:** A1 sopra il caso (≥ 15 punti), non la quota; sincope di ciascuno; J3 e J4 di I1–I3.
4. **Percussioni:** indagine separata, perché toccano ruoli e voci (ADR). Il rullante va deciso prima o insieme.

## 4. Limiti

- Un brano dell'esperto, altro genere e tempo; un nostro seed; 64 battute nostre contro 63 sue (34 per il Sub).
- Il nostro modello non ha rullante: A2 non è un confronto.
- La sincope di Longuet-Higgins e Lee è un modello, non un giudizio. Implementazione scritta per questa indagine, senza test automatico; quattro sui quarti danno 0 e i levare 1,75, calcolati a mano.

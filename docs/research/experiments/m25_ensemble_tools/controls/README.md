# Controlli F8 e F9

Convertiti con [`convert_control.py`](../convert_control.py) secondo le regole fissate nell'[aggiunta F8/F9](../../../M2_5_1c_Ensemble_Tools_Preregistration_Addendum_F8_F9.md); il rapporto di conversione è dentro ogni JSON (`conversion_report`). Lo scenario `ensemble_tools_v1` li misura solo se l'hash dell'aggiunta coincide.

## F8 — J. S. Bach, Partita I per violino solo, BWV 1002

- **Fonte:** Mutopia Project, `https://www.mutopiaproject.org/ftp/BachJS/BWV1002/partita-1/partita-1-mids.zip`, scaricato il 2026-11-12 con il consenso del titolare; 24.933 byte, SHA-256 `cc18258061accad9c3fab474d7fab0b3c3e7cbb56958eb365b3b6c1c359a505c`. Licenza: **pubblico dominio** (Mutopia, «Reproduction of Autograph»). MIDI generati da GNU LilyPond 2.16.1.
- **Movimento:** `violin-7.mid`, Double della Tempo di Borea, salvato come `F8_bach_bwv1002_borea_double_violin.mid` (SHA-256 `a873edff6f591b0da147650bcc60bc029f1283d35b2a0a14a2f2d7f33d8481ef`).
- **Scelta, regola dichiarata:** l'aggiunta preferiva circa 24 frasi; nessun movimento dell'archivio le raggiunge. Si è preso il movimento più lungo con al più il 5 % di note sovrapposte: 68 battute in 2/2, 17 frasi da 64 sedicesimi, 4 % di sovrapposizioni.
- **Conversione:** 554 note lette, nessun attacco spostato (partitura incisa), 13 note più basse di bicordi scartate, 17 frasi tutte con note. Tutte le note sono ottavi: la M4 vede solo gli intervalli.

## F9 — subagente alla cieca (non umano)

- **Procedura:** un subagente nuovo ha ricevuto le istruzioni scritte per l'esperto, con l'ordine di non usare strumenti né leggere file, e ha risposto con la tabella (`F9_subagent_table.csv`) e una nota d'intento (`F9_subagent_intent.md`). Il registro dell'esecuzione riporta 1 uso di strumenti, presumibilmente la consegna del risultato; non è stato possibile verificarlo dall'agente principale.
- **Conversione:** 24 frasi, 436 note, nessuna modifica.

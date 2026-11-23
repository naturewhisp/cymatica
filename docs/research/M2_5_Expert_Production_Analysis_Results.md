# M2.5 — Analisi della produzione inviata dall'esperto. Risultati

- **Pre-registrazione:** [M2_5_Expert_Production_Analysis_Preregistration.md](M2_5_Expert_Production_Analysis_Preregistration.md), SHA-256 (LF) `9ba02618794e71f6228480fbcf6d48c426bc8f8f530daeac456cbf08cfa2d81e`.
- **Esecuzione:** 2026-11-20.
- **Dati:** [esperimento](experiments/expert_production_analysis/README.md), con statistiche aggregate soltanto. Il materiale dell'esperto resta fuori dal repository.
- **Decodifica MP3:** FFmpeg già installato sul PC (build `2026-02-09-git-9bfa1635ae`), approvato dal titolare solo per questa analisi. Non è una dipendenza di CYMATICA.

## 1. Esiti

| ID | Previsione | Misura | Esito |
|---|---|---|---|
| D1 | Backbeat su 2 e 4 in ≥ 75 % delle battute con rullante | **32 %** (36 su 112). I colpi si addensano sui sedicesimi 4, 7 e 10: uno schema da breakbeat, non un backbeat dritto | **Mancata** |
| D2 | Kick sul tempo 1 in ≥ 75 % delle battute | 81 % (92 su 113); mediana 3 kick per battuta | Confermata |
| D3 | Ghost notes ≥ 10 % | 35 % con il rullante sulle note 37 e 38, la regola scritta nel codice prima dei dati; 17 % con la sola 38 | Confermata |
| D4 | Basso ≥ 90 % in Fa frigio, Fa più frequente | 98,8 %, Fa più frequente; sub-basso 98,5 % | Confermata |
| D5 | Arpeggio ≥ 90 % a sedicesimi | **Non misurabile.** Il MIDI contiene gli accordi d'ingresso all'arpeggiatore di Ableton, uno ogni 4 battute, e non le note arpeggiate | Non misurabile |
| D6 | Periodo del basso, descrittivo | Nessuna battuta ripete la precedente. Ripetizione a 2 battute 9 %, a 4 battute 16 %, a 8 battute 40 %. Ogni finestra di 4 battute differisce dalla precedente | Descrittivo |
| S1 | Intervallo mediano fra cambiamenti ≤ 8 battute | **1 battuta** (quartili 1–2); l'81 % delle finestre di 8 battute contiene un cambiamento | Confermata |
| S2 | Sidechain sul basso ≥ 6 dB | Prima della correzione la misura non è valida (vedi sotto). Dopo la correzione: **+1,7 dB** (quartili 0,9–2,4) | **Mancata** |
| S3 | Loudness del mix fra −8,5 e −5,5 LUFS | **−8,41 LUFS** | Confermata, al margine. L'esperto dichiara circa −7 |
| S4 | Quota sotto 100 Hz ≤ 5 % negli stem non di basso | Mediana 0,00008 %; massimo 0,04 % | Confermata. Basso 808 al 90 % sotto 100 Hz, sub-basso al 99 % |

**Correzione dichiarata dopo i dati (S2).** Il controllo d'allineamento, scritto prima della misura, è fallito: subito dopo i kick del MIDI l'energia dello stem della batteria scendeva di 7,5 dB in mediana, invece di salire. Il MIDI era quindi spostato rispetto all'audio.
- Lo scarto che rende massima l'energia d'attacco ai kick è di **69 ms** (0,19 beat); con lo scarto l'energia sale di 4,6 dB.
- Ho applicato soltanto questa correzione: definizioni e soglie restano quelle della pre-registrazione.
- Probabilmente lo stem del basso è esportato prima del sidechain, oppure il pumping agisce sul gruppo o sul master. La nota dell'esperto non permette di distinguere.

**Verifica dell'implementazione BS.1770.** Sul brano di riferimento di CYMATICA, lo script Python dà −19,60 LUFS, contro −19,601 della misura C++ del motore.

## 2. Confronto con CYMATICA

Stessa definizione di S1 sugli stem dei quattro ruoli.

| Misura | Esperto | CYMATICA, riferimento | CYMATICA, seed 34 |
|---|---|---|---|
| Intervallo mediano fra cambiamenti | 1 battuta | **12 battute** | **10 battute** |
| Finestre di 8 battute con almeno un cambiamento | 81 % | 50 % | 50 % |
| Cambiamenti, cioè entrate o uscite, per strato e per 8 battute | 0,22 (24 stem) | 0,15 (4 ruoli) | 0,15 |
| Ritmo armonico | 1 accordo ogni 4 battute | 1 accordo per battuta | 1 accordo per battuta |
| Il basso ripete la finestra di 4 battute precedente | mai | sì, dal piano (non misurato): stesso giro per 32 battute | dal piano (non misurato): sì, salvo la frase cadenzale 15 di B |
| Quota sotto 100 Hz della voce più grave non percussiva | basso 808 90 %, sub 99 % | Corpo **4,8 %** | Corpo 2,8 % |
| Loudness integrata | −8,4 LUFS | −19,6 LUFS | −20,1 LUFS |

**Letture:**
1. **«Ogni 8 battute la canzone dovrebbe cambiare».** In numeri, il brano dell'esperto cambia qualcosa quasi a ogni battuta, e in 8 battute su 10 c'è almeno un cambiamento; CYMATICA cambia ogni 10–12 battute. La differenza non sta nell'armonia: l'esperto cambia accordo *più lentamente* di noi. Sta negli strati che entrano ed escono, nella batteria e nel basso, che non ripete mai uguale una finestra di 4 battute. Dipende in parte dal numero di strati (24 contro 4), ma anche per strato l'esperto cambia di più (0,22 contro 0,15).
2. **«Manca un basso».** La voce più grave di Corpo suona la fondamentale un'ottava sotto la tonica della sezione, Re3 (147 Hz) in A e Sol2 (98 Hz) in B. Sotto 100 Hz cade il 3–5 % della sua energia, contro il 90–99 % dei bassi dell'esperto. Il registro del basso, cioè le ottave 1 e 2, in CYMATICA lo occupa solo il kick.
3. **Batteria.** L'esperto mette il kick sul tempo 1 quasi sempre e il rullante in uno schema da breakbeat con molte ghost notes. È la sua estetica jungle: conferma la proposta di backbeat e ghost notes per F1 (ADR-0007 §8.1) come principio, non come pattern da copiare.
4. **Loudness.** −8,4 LUFS è una loudness da club. CYMATICA sta a −19,6, con un obiettivo diverso (gioco, ascolto lungo); non è un modello per noi.

## 3. Limiti

- Un solo brano, di un genere diverso; MP3 con perdite.
- S1 dipende dalla soglia di −40 dB e dal numero di strati.
- Il sidechain potrebbe non essere visibile nello stem esportato.
- Per S1 assumo che la griglia delle battute parta dall'inizio dell'audio. Lo scarto del MIDI (69 ms, meno di un quinto di beat) non sposta i confini delle battute in modo rilevante.

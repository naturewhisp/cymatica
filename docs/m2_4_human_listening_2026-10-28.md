# M2.4 — Ascolto umano esplorativo della fixture (2026-10-28)

- **Stato:** evidenza qualitativa **supplementare**, non un gate. Non sostituisce l'ascolto formale del gate M2.5 né il pilot del [protocollo di valutazione](research/M2_4_Evaluation_Protocol.md) §3–§4.
- **Materiale:** `fixture_probe_v1.wav` (fixture-m2-4-v1, seed 1337, 182,9 s, `pcm_content_hash` 15126969907236845529, invariato da Inc-4 fino a `d86f6bb`).
- **Ascoltatore:** un esperto di musica esterno al progetto. Il nominativo è noto al titolare; qui non è riportato. Le risposte sono state raccolte dal titolare il 2026-10-28 con la scheda di 12 domande proposta dall'agente.
- **Condizioni:**
  - non documentate: dispositivo (le risposte citano l'ascolto in cuffia), livello, numero di ascolti, conoscenza preventiva del progetto;
  - ascoltatore singolo, ascolto non alla cieca rispetto al materiale.

  Le risposte sono opinioni personali dichiarate come tali.
- **Metodo dell'agente:** ogni osservazione verificabile è stata confrontata con il piano, cioè gli intent di [fixture_intents_seed1337.csv](research/experiments/cacp_methods/fixture_intents_seed1337.csv), con il codice e con le misure. Un riscontro conferma che un fenomeno esiste nel piano o nel segnale; non conferma il giudizio estetico.

## 1. Sintesi delle risposte

| Tema | Risposta (sintesi) |
|---|---|
| Parti percepite | 4: 00:00, 00:31, 01:04, 01:58 |
| Cosa cambia | tempo, modo e riferimento ritmico: il kick passa da un ritmo «frenetico» a un «4 on the floor» |
| Contrasto | talvolta spiazzante, soprattutto fra la parte 1 e la 2 |
| Ritorno finale | la melodia suona come quella della prima parte, ma «invertita» |
| Transizioni | «molto brusche»; le parti suonano come loop di 4 battute giustapposti. Suggerisce rampe di BPM o modulazioni ritmiche sulle percussioni |
| Strati | due synth saw filtrati passa-basso, un kick, una percussione FM «nell'orecchio sinistro» |
| Strato che entra | la percussione FM, dalla seconda parte fino alla fine |
| Strato coperto | «il kick è fin troppo basso» |
| Melodia | direzione riconoscibile |
| Difetti | il brano «parte senza il primo colpo di kick»: atteso `K--K-K--K--K-K--`, sentito `---K-K--K--K-K--`. Il panning della percussione FM a sinistra può risultare fastidioso in cuffia |
| Valutazioni 1–7 | coerenza **6**, interesse **4**, ripetitività **7** |
| Calo d'attenzione | in ogni parte, dopo il secondo loop per un ascoltatore medio, intorno al quarto per un ascoltatore attento |
| Una cosa da cambiare | il mix, a parte la variazione delle parti |
| Suggerimenti | bilanciamento con rumore rosa e curve isofoniche; riverberi di durate diverse; modulazioni (LFO sul filtro, sull'attacco); saturazione sul master; compressione solo come scelta stilistica; chorus, flanger e delay. La percussione FM gli risulta «un po' dissonante» |

## 2. Riscontri sul piano e sul segnale

| ID | Osservazione | Riscontro dell'agente | Esito |
|---|---|---|---|
| H1 | 4 parti a 00:00, 00:31, 01:04, 01:58 | Il piano ha 3 sezioni: A a 0 s, B a 64,0 s (01:04), A′ a 118,86 s (01:58). A è divisa in due metà: frasi 0–3 senza Trama e frasi 4–7 con Trama, dalla battuta 16 = 32,0 s (00:32). Anche il kick cambia schema a metà di A (H2). | **Confermata**: le 4 parti corrispondono ai confini del piano entro ~1 s. Le sezioni sono distinguibili all'ascolto. |
| H2 | Kick da ritmo «frenetico» a «4 on the floor» | Pulso, passi da sedicesimo per battuta: A1 (battute 0–15) `2,5,7,10,13,15`, cioè E(6,16) ruotato; A2 (16–31) `0,4,8,12`; B e A′ `2,6,10,14`. | **Confermata** per A1 → A2. |
| H3 | «Manca il primo colpo di kick» | Non è un difetto di rendering: la catena O3 di 5a dimostra che tutti i NoteOn pianificati sono applicati al frame previsto. È il **piano**: il pattern euclideo di Pulso ha rotazione 2, quindi nessun colpo sul primo passo della battuta. Il kick cade sul **tempo forte in solo 16 battute su 96**: in A1 non c'è colpo sul downbeat, e in B e A′ i colpi cadono sul controtempo di ottavo (`2,6,10,14`). La rotazione viene dalla ricetta del candidato (`pulsoRotation = k % 4`, [music_planner.cpp](../engine/audio/music_planner.cpp)) e il punteggio non premia il downbeat. | **Confermata come proprietà del generatore** (finding F1). |
| H4 | Kick troppo basso | Telemetria post-bus: energia massima di Pulso simile a quella di Corpo (circa 0,07–0,08 per sezione). Pulso è uno sweep 140 → 50 Hz: a parità di livello elettrico, le basse frequenze sono percepite meno forte (curve isofoniche, ISO 226). | **Plausibile**; da misurare in 5e con loudness per stem e da tarare in M2.5. |
| H5 | Percussione FM a sinistra, fastidiosa in cuffia | La Trama ha un micro-delay L 11 ms / R 17 ms: il canale sinistro arriva circa 6 ms prima, e l'effetto di precedenza (Haas) lateralizza la sorgente a sinistra. Nessun panning esplicito. | **Spiegata dal codice** (finding F2). |
| H6 | Ritorno finale con melodia «invertita» | Le ricette di A′ includono `ContourInversion` (pivot 2) accanto a identità, ottava e variazioni ritmiche. | **Compatibile con il piano**; quale trasformazione ha vinto in ogni frase si verifica sulla trace. |
| H7 | Transizioni brusche, loop di 4 battute, ripetitività 7/7 | Il cambio di tempo è istantaneo per contratto (`SetTempoMap` al confine, D-M2-05). Le frasi riusano poche ricette fisse e il seed agisce solo sui pareggi (nota di ADR-0005). | **Coerente** con i limiti noti della varietà (F3). |
| H8 | Due synth saw filtrati | Corpo (PolyBLEP saw + SVF) e Vettore (PolyBLEP saw con glide e vibrato). | **Confermata.** |

## 3. Finding e proposte (non decisioni)

- **F1 — Pulso senza appoggio metrico (rilevante).** Il ruolo dichiarato di Pulso è «Rhythmic anchor» ([music_intent.h](../engine/audio/music_intent.h)), ma in 80 battute su 96 il kick non suona sul tempo forte. Il generatore sceglie la rotazione senza un vincolo o un punteggio sul downbeat. Correggerlo è una modifica del generatore: richiede una decisione documentata, oltre il perimetro di Inc-5, che non modifica il generatore. Il momento naturale è l'ingresso di M2.5 o M2-F.
- **F2 — Lateralizzazione della Trama.** Il delay asimmetrico fisso produce una sorgente stabilmente a sinistra. È una candidata al tuning di M2.5: ritardi più vicini fra loro, alternanza o modulazione dei ritardi, mix wet più basso. Il Side/Mid di 5b la rende misurabile.
- **F3 — Ripetitività e transizioni.** Il giudizio conferma con un ascoltatore umano il limite già noto: poche ricette, seed poco influente, cambi di tempo istantanei. Le metriche multi-seed di 5e misureranno la diversità; rampe di tempo e variazioni ritmiche di transizione richiederebbero un'estensione di `MusicClock` e del planner, da decidere dopo Inc-5.
- **F4 — Mix e timbro (H4, suggerimenti di mixaggio, FM «dissonante»).** Sono proposte di tuning per M2.5. Riverberi, chorus, flanger, saturazione ed effetti con feedback hanno costo DSP e vanno valutati contro il budget della callback (D-M2-10) e la protezione del master (ADR-0004). Nessuna è adottata ora.

## 4. Limiti

- **Campione:** un solo ascoltatore, condizioni d'ascolto non controllate e non documentate, nessuna randomizzazione né confronto A/B.
- **Valutazioni:** sono indicative, non statistiche.
- **Riscontri:** confermano fenomeni presenti nel piano o nel segnale; il giudizio di qualità resta dell'ascoltatore.
- **Gate:** l'ascolto formale resta al gate M2.5 (specifica §32), con device reale e protocollo.

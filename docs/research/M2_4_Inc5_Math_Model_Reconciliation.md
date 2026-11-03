# Modello matematico del motore — riconciliazione delle indagini A e B

- **Data:** 2026-11-03
- **Oggetto:**
  - indagine A, Claude: [M2_4_Inc5_Math_Model_Claude.md](M2_4_Inc5_Math_Model_Claude.md);
  - indagine B, Antigravity: [M2_4_Inc5_Math_Model_Antigravity.md](M2_4_Inc5_Math_Model_Antigravity.md);
  - entrambe al commit `df0bcbe` (SHA completo `df0bcbef6947e28269313b1df85c960fc54a6373`).
- **Protocollo:** indagini indipendenti e riconciliazione dopo la consegna di entrambe ([progress.md](../progress.md), «Consolidamento del modello matematico»). L'indagine A è stata scritta senza aprire B; B dichiara di non aver aperto A.
- **Conflitto d'interessi dichiarato:** la riconciliazione è scritta dall'autore dell'indagine A. Per limitarne l'effetto:
  - ogni rilievo, di A o di B, è accettato solo se verificato sul codice del commit o con uno strumento eseguito;
  - le correzioni ad A sono registrate qui e segnalate nel dossier A con un erratum;
  - la revisione indipendente del titolare resta il giudizio finale.
- **Status:** descrittivo, non normativo. Nessuna modifica al codice.

## 1. Esito in sintesi

- **Accordo forte su sei punti**, trovati in modo indipendente dalle due indagini:
  - la diversità fra seed è nulla per costruzione;
  - la coerenza delle frasi si rompe se cambia `barsPerPhrase`;
  - la trasposizione al bordo del registro rompe il contorno;
  - la densità SPSC dei pad incrociati è imprecisa;
  - golden e hash dipendono dalla toolchain;
  - M-NORM.
- **Un rilievo di B corregge A:** AG-MM-07. A 140 BPM la frammentazione dei gruppi musicali è numerica (±1 campione), non dovuta al voice leading. Ha conseguenze sul pacchetto `evaluation_pack_v2` (§4).
- **Quattro rilievi di B sono smentiti o ridimensionati:**
  - AG-MM-10, overshoot del limiter: smentito da una misura;
  - AG-MM-08, ordine fra rampa e limiter: già deciso in ADR-0004;
  - AG-MM-09, buffer «mai letti né scritti»: falso, è solo sovradimensionato;
  - AG-MM-14: irraggiungibile, con parametri errati.
- **Affidabilità di B:** il dossier contiene numerosi identificatori, SHA, domini e parametri che non esistono nel repository (§2). Le sue affermazioni sostanziali vanno quindi prese solo dove verificate, come fatto qui.

## 2. Affidabilità fattuale dell'indagine B

Verifiche puntuali sul commit `df0bcbe` (`grep` su `engine/` e `docs/`):

| Affermazione di B | Realtà nel repository |
|---|---|
| SHA del commit `df0bcbe66bc61bb9…` (dossier) e `df0bcbe912e32d8d…` (riepilogo) | `df0bcbef6947e28269313b1df85c960fc54a6373`: entrambi gli SHA citati sono inesistenti |
| `isValidTempoMap`, `beatFactorRational`, `MusicClock::init`, `DeterministicRng` / `deterministic_rng.h`, `randomUniform01`, `voiceChordRootFifth`, `applyMotifTransform`, `synthesizeVoiceSample`, `sampleToDeviceFrame`, `processCommands` | nessuno esiste nel codice. Esistono `isSupportedTempoMap`, `stable_random.h`, `randomF32`, `voiceTriad`, `applyRecipe` e altri |
| Domini del tempo: BPM ∈ [20, 300], ts_num ≤ 32, ts_den fino a 32, suddivisioni e battute per frase ≤ 64 | `isSupportedTempoMap`: ts_den ∈ {2, 4, 8, 16}, numeratore ≤ 16, limiti 65.535, solo il vincolo D ≤ N sul BPM |
| Budget del motore: 64 eventi per blocco | `kMaxScheduledEventsPerBlock = 256` (`audio_engine.h:116`) |
| Finestra di lookahead [t, t + 4.800) | 7.200 frame (`AudioSchedulerConfig`) |
| Build con `/W4 /WX` | `/utf-8 /W4 /permissive-`, senza `/WX` (`CMakeLists.txt`) |
| ADR-0002 = `0002-seeded-rng-contract.md` | `0002-m1-timing-random-concurrency.md` |
| Finding «F2 `L_DOMINANT_PRECEDENCE_RISK`» | non presente nel repository |
| Limiter: «predittore quadratico attivo solo se d1·d2 < 0», guadagni `prevL_`/`prev2L_` | nessuna condizione del genere; il detector è picco campione + 3 interpolatori + curvatura, come descritto in ADR-0004 §4.3 |

Molti rimandi `file:riga` di B puntano a funzioni inesistenti e non sono stati controllati uno per uno. Le formule generali che B riporta per clock, fixed step, mixer, PolyBLEP, SVF e statistica coincidono con il codice dove sono state confrontate.

**Lettura:** B è utile come seconda prospettiva e ha trovato almeno un difetto reale che A non aveva visto, ma non è citabile come fonte primaria.

## 3. Accordi (trovati indipendentemente da A e B)

| Tema | A | B | Verifica | Esito |
|---|---|---|---|---|
| Diversità fra seed nulla per costruzione: il seed entra solo nello spareggio, i pareggi sono 0 | CL-MM-06, Teorema D, `tie_probe` (0/24 e 0/2 pareggi, scarto ≥ 300, contenuto identico sui seed 1..64) | AG-MM-06 (stessa dimostrazione, `tiedTopCandidates == 1`) | codice `music_planner.cpp` e misura di A | **confermato** |
| Indici di frase incoerenti se `barsPerPhrase` cambia fra segmenti | CL-MM-01 | AG-MM-01 (stesso meccanismo; aggiunge l'identità `positionAtFrame(frameAtPhrase(P)).phrase == P` come invariante da testare) | `music_clock.cpp` | **confermato, latente**; B ne alza la severità a MED, A la teneva LOW perché la fixture non lo attiva |
| Trasposizione modale ripiegata nota per nota | CL-MM-02 | AG-MM-04 | `applyModalTranspose` | **confermato, latente** |
| Densità SPSC dei pad incrociati contata alla fine della frase | CL-MM-04 | AG-MM-05 | `validateHardConstraints` contro `planPhrase` | **confermato** |
| Hash canonico e golden dipendenti da `std::pow` e dal CRT | §2.3 di A | AG-MM-03 | `plan_canonical_hash.h` lo dichiara | **confermato** (già noto e dichiarato) |
| M-NORM: soglia degli onset legata al massimo dell'intero stem, rilascio di 2 ms come causa del massimo | CL-MM-09, CL-MM-13 | AG-MM-13 | 5e.3b | **confermato** (evidenza meccanicistica retrospettiva sul set B) |
| Conformità di clock, fixed step, mixer, Lemire, PolyBLEP, SVF, O3, loudness, ricostruzione degli stem | §1–§7 di A | AG-MM-16..20 | test esistenti | **confermato** |

## 4. Divergenze e correzioni

### 4.1 AG-MM-07: gruppi musicali frammentati a 140 BPM — **B corretto, A corretto da B**

**Fatto.**
- Il fingerprint per frase usa i frame relativi all'inizio della frase. A 140 BPM un beat dura 20.571,43 frame e una frase 329.142,86, quindi i confini relativi oscillano di ±1 campione da frase a frase, con periodo 7 frasi.
- Verifica dell'agente A (script, griglia di 64 passi per frase della sezione B): 7 griglie relative distinte fra le 8 frasi; le frasi 9–14 differiscono dalla 8 in 54, 45, 36, 27, 18 e 9 passi, sempre di 1 campione; la frase 15 coincide con la 8.
- I gruppi registrati in 5e.2 ([seed_diversity_and_groups_2026-11-02.json](experiments/inc5_evaluation_pack_v2/seed_diversity_and_groups_2026-11-02.json)) lo confermano:

  | Sezione | Frasi | Gruppi |
  |---|---|---|
  | A (120 BPM) | 0–3 / 4–6 / 7 | 0 / 1 / 2 |
  | B (140 BPM) | 8–15 | 3–10, uno per frase |
  | A' (120 BPM) | 16 / 17–22 / 23 | 11 / 12 / 13 |

  Le frasi 9–14 di B, simbolicamente uguali come le 17–22 di A', ricevono 6 gruppi distinti.
- Con una griglia simbolica i gruppi sarebbero 9, non 14.

**Correzione ad A.** Il §4.5 (CL-MM-05) attribuiva la varietà fra frasi della stessa sezione allo stato di voice leading e ai flag di frase. Nella sezione B la distinzione è invece un artefatto di arrotondamento di 20,8 µs, musicalmente nullo. Il dossier A riceve un erratum che rimanda qui.

**Conseguenza.** È la più rilevante della riconciliazione.
- La regola di raggruppamento di `evaluation_pack_v2` (ADR-0006, precisazione DIF-5e.0-02) **sovrastima i gruppi indipendenti e quindi `n_eff`** per le finestre della sezione B.
- Gli `n_eff` attesi registrati in 5e.2 (10,9 positivi, 8,9 negativi) sono quindi ottimistici.
- Vale prima della sessione alla cieca di Antigravity (5e-10), che usa proprio quel pacchetto.
- Si somma all'ipotesi d'indipendenza fra gruppi con materiale imparentato (CL-MM-10, solo A).

### 4.2 AG-MM-10: overshoot del limiter a f_s/4 — **smentito**

- **Affermazione di B:** con un seno a 12 kHz e fase −π/4 (campioni ±C a −1 dBFS) il limiter non interverrebbe, «perché d1·d2 = 0», e il true peak salirebbe a +1,83 dBTP.
- **Codice:** questa condizione non esiste. Con campioni (+C, +C, −C, −C) il termine di curvatura vale √(1 + 0,35·4)·C ≈ 1,55·C, oppure √5·C ≈ 2,24·C, e supera la soglia.
- **Misura** (programma `limiter_probe` collegato a `cymatica_audio.lib` del commit; true peak con `analysis::estimateTruePeakDbtp`, BS.1770 Allegato 2):

  | Segnale | TP in ingresso | TP in uscita | Riduzione massima |
  |---|---|---|---|
  | f_s/4, fase −π/4, campioni ±C | +2,69 dBTP | **−4,81 dBTP** | 9,1 dB |
  | f_s/4, fase −π/4, ampiezza 2,0 | +6,70 dBTP | −4,81 dBTP | 13,1 dB |
  | 11,9 kHz, ampiezza 1,0 | +0,22 dBTP | −2,94 dBTP | 5,7 dB |
  | 1 kHz, ampiezza 1,0 | +0,01 dBTP | −1,78 dBTP | 1,8 dB |

- **Esito:** il limiter resta sotto −1 dBTP con margine; ad alta frequenza è anzi conservativo. Resta valida l'osservazione di B che il test esistente su 12 kHz usa una soglia lasca (< 2,0 dBTP): un vettore a f_s/4 con fase −π/4 sarebbe un buon test di caratterizzazione.

### 4.3 AG-MM-08: ordine fra rampa di trasporto e limiter — **smentito come divergenza**

ADR-0004 §4.3 registra come decisione autoritativa: «Rampa di trasporto: a monte (nel `VoiceAllocator`) — equivalente per il silenzio grazie a L = 0». Codice e ADR sono allineati; B ha confrontato la versione superata.

### 4.4 AG-MM-09: buffer «morto» del limiter — **ridimensionato**

`historyL_` e `historyR_` sono scritti e letti a ogni campione (`estimatePeakCausal`). Dei 12 slot ne servono però solo 3: il buffer è sovradimensionato, non morto, e il commento «12-sample polyphase» descrive un detector non più in uso (vedi ADR-0004 §4.3). Severità INFO.

### 4.5 AG-MM-14: NoteOff differito oltre la scadenza della durata — **irraggiungibile, parametri errati**

- **Parametri:** il budget è di 256 eventi per blocco, non 64.
- **Raggiungibilità:** il vincolo hard del planner (≤ 128 eventi SPSC per finestra di 7.200 frame) e blocchi ≤ 1.024 frame tengono ogni blocco ben sotto il budget, quindi il caso non si verifica con il materiale del planner.
- **Meccanismo:** è valido in astratto. Un NoteOff differito oltre la scadenza troverebbe la voce già rilasciata dalla guardia di sicurezza e sarebbe Ignored. È una nota di robustezza, non un rilievo.

## 5. Punti trovati da una sola indagine

### Solo da A, non contraddetti da B

CL-MM-03, CL-MM-05, CL-MM-07, CL-MM-08, CL-MM-10, CL-MM-11, CL-MM-12, CL-MM-13:
- periodicità di una battuta;
- punteggi uniformi nella sezione, al netto della correzione del §4.1;
- regola 1 del tracker contro quella del motore;
- commento «tempo-synced»;
- indipendenza fra gruppi;
- degenerazione delle metriche di diversità per seed;
- impacchettamento di `PolicyVersionId`;
- modello delle pendenze di rilascio.

### Solo da B, verificati dall'agente A

| ID | Contenuto | Verifica | Esito |
|---|---|---|---|
| AG-MM-02 | `phrasePhase` senza clamp: l'arrotondamento in float32 può dare 1,0f | Calcolo in float32: per una frase di 16 beat il caso compare solo sotto ~1,4 BPM; con frasi lunghe (per esempio 64 battute) già intorno a 20 BPM. A 20–300 BPM e 4 battute il valore massimo è 0,99999 | **confermato, latente** (tempi estremi o frasi lunghe); INFO |
| AG-MM-11 | Il primo campione di ogni nota è muto: l'inviluppo è applicato prima del passo ADSR | Ordine nel ciclo di `renderBlock` (campione con `envelopeLevel` = 0, poi passo) | **confermato**; ritardo di 1 campione (20,8 µs), musicalmente irrilevante; spiega le latenze di 529/817 campioni |
| AG-MM-12 | Immagine della Trama spostata a sinistra: lo slot 4 ha guadagni L:R = 1:0,5 (+6,02 dB a sinistra); con note non sovrapposte vince sempre lo slot 4 (regola 0); il tap sinistro del delay arriva 6 ms prima | Codice `voice_allocator.cpp` (spread per slot), `role_bus_processor.cpp` (11 e 17 ms); i guadagni assoluti 0,60/0,30 di B sono inesatti, il rapporto è corretto | **confermato analiticamente**; l'effetto percettivo (precedenza di Haas) non è verificato e passa all'ascolto dell'esperto |
| AG-MM-15 | La fusione per distanza minima concatena picchi crescenti: una catena può superare la distanza minima collassando in un solo picco | Codice `pickOnsetFrames` (`out.back() = i`) | **confermato**; effetto minore sul materiale attuale (P4 di 5e.3b: nessuna fusione sui mancati); lacuna di test |

## 6. Punti aperti consolidati

Ordinati per conseguenza.

| # | Punto | Fonte | Quando |
|---|---|---|---|
| R-01 | **Raggruppamento musicale di `evaluation_pack_v2`:** gruppi in eccesso a 140 BPM (n_eff sovrastimato) e indipendenza fra gruppi con materiale imparentato (n_eff anti-conservativo) | AG-MM-07, CL-MM-10 | **prima della sessione alla cieca** di 5e-10: decisione del titolare (§7) |
| R-02 | Il seed non genera varietà (Teorema D): servono sorgenti reali di variazione, cioè stato lungo la sezione, varianti di motivo e il «direttore musicale» F5; misurare la varietà su assi diversi dal seed | CL-MM-06/11, AG-MM-06 | M2.5 (F5) |
| R-03 | M-NORM: normalizzazione robusta o locale in un `descriptors_v2`; ricalibrare δ dopo la scelta del rilascio | CL-MM-09, AG-MM-13 | M2.5 |
| R-04 | Rilascio per ruolo guidato dal rapporto delle pendenze e dall'ascolto | CL-MM-13, 5e.3 | M2.5 (decisione) |
| R-05 | Immagine stereo della Trama e precedenza: verifica d'ascolto | AG-MM-12 | ascolto dell'esperto (fine Inc-5) |
| R-06 | Guasti latenti e test di caratterizzazione: `barsPerPhrase`, trasposizione al bordo, densità dei pad incrociati, `phrasePhase`, catena di fusione, tracker contro motore, vettore f_s/4 nel test del limiter | CL-MM-01/02/04/07, AG-MM-01/02/04/05/15, §4.2 | M2.5 o micro-fase di test, su decisione del titolare |
| R-07 | Pulizie: commento «tempo-synced», buffer del limiter sovradimensionato, `PolicyVersionId::toU32` senza maschera | CL-MM-08/12, AG-MM-09 | quando si toccano quei file |

## 7. Decisione richiesta al titolare (R-01)

La sessione alla cieca di Antigravity (5e-10) valuta il valutatore con `presence_stats_v1` su `evaluation_pack_v2`, i cui gruppi nella sezione B sono gonfiati da un artefatto numerico.

Opzioni:

- **(a) Correggere prima della sessione** (raccomandata). Una micro-fase di Inc-5 nel perimetro di ADR-0006:
  - regola di raggruppamento su griglia simbolica, cioè passi della griglia della frase invece dei frame;
  - con il gruppo esteso alle frasi con lo stesso materiale trasposto, se si vuole affrontare anche CL-MM-10;
  - nuova versione del pacchetto, con v1 e v2 ancora leggibili;
  - `n_eff` atteso ricalcolato.
- **(b) Mantenere v2 e dichiarare il limite:** gli intervalli della sezione B sono riportati come ottimistici e il risultato della sessione è letto di conseguenza.

**Esito di R-01 (2026-11-03):** il titolare ha scelto l'opzione (a), attuata nella fase 5e.3c con `evaluation_pack_v3`. La regola simbolica dà 8 gruppi, come previsto, e `n_eff` atteso 4,93 per i positivi e 3,77 per i negativi. Con i negativi sotto 4 la specificità non è misurabile su questa fixture; dettagli in ADR-0006 «5e.3c» e in `progress.md`.

## 8. Lezioni di processo

- **Indipendenza utile.** La seconda indagine ha trovato un difetto reale (AG-MM-07) sfuggito alla prima, e ha confermato in modo indipendente il risultato principale (Teorema D).
- **Verificabilità insufficiente.** Il dossier B mescola fatti verificati e dettagli inventati: identificatori, SHA, parametri e finding inesistenti.

Per le prossime consegne di Antigravity si propone di chiedere:
1. l'output grezzo dei comandi dichiarati;
2. per ogni rilievo, un estratto di codice copiato;
3. il divieto di citare file, funzioni o finding non trovati con una ricerca nel repository.

## 9. Riproducibilità

- **Commit:** `df0bcbe`.
- **Strumenti dell'agente A** (scratchpad della sessione, non versionati):
  - `tie_probe.cpp` (pareggi e seed, dossier A §4.2);
  - `limiter_probe.cpp`: compilato con `cl /std:c++20 /O2 /EHsc /MD` contro `cymatica_audio.lib` e `cymatica_core.lib` di `build/x64-release`; applica `MasterLimiter` a 1 s di segnale stereo e misura il true peak prima e dopo;
  - script Python per la griglia a 140 BPM (§4.1) e per `phrasePhase` in float32 (§5).
- **Evidenze del repository:**
  - gruppi musicali di 5e.2 ([seed_diversity_and_groups_2026-11-02.json](experiments/inc5_evaluation_pack_v2/seed_diversity_and_groups_2026-11-02.json));
  - ADR-0004 §4.3;
  - manifest di 5e.3 e 5e.3b.

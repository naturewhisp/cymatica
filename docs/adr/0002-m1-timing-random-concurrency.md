# ADR-0002 — Profilo temporale, generazione deterministica e concorrenza (Milestone 1)

- **Data:** 2026-10-06
- **Stato:** accepted for implementation
- **Milestone:** M1 — Tempo, seed e contratti deterministici
- **Documenti correlati:** `CYMATICA_Specifica_Agentica_Sviluppo.md` §7.5, §15, §20, §22, §32, §36; `DESIGN.md` §6.2.1, §17.2

---

## 1. Contesto

La Milestone 1 fonda il runtime non deterministico riproducibile di CYMATICA. Richiede di definire:
1. la frequenza di campionamento logica interna e la frequenza di simulazione fissa (Spec §7.5, §20.3, §36 Q3);
2. l'algoritmo di random mixing e derivazione dei seed con test vector invarianti (Spec §15.3, §15.4, §15.5, §36 Q4);
3. il protocollo di scambio lock-free e wait-free tra il thread audio realtime e il game thread per telemetria, comandi e controlli continui (Spec §20.5, §20.6, §20.7);
4. la libreria di serializzazione JSON per configurazioni e RunRecord (Spec §28.2, §36 Q2 / D-M0-05);
5. il profilo minimo del giocatore per il validatore M4 (Spec §32, DESIGN §6.2.1).

---

## 2. Decisioni

### D-M1-01: Profilo temporale e gestione catch-up (Spec §7.5, §20.3, §36 Q3)

- **Frequenza audio interna:** 48.000 Hz timeline logica continua. I device con frequenze diverse (es. 44.100 Hz) utilizzano un fattore di conversione razionale esatto `160/147` con gestione del resto sull'indice assoluto dei frame.
- **Frequenza di simulazione:** 120 Hz (`dt = 1/120 s`), corrispondente esattamente a 400 sample frame per tick di simulazione (`48000 / 120 = 400`).
- **Aritmetica del clock musicale:** i BPM sono rappresentati da una frazione intera `RationalBpm{numerator, denominator}` (es. 120/1 o 1275/10). Le coordinate di frame assoluto per beat, battuta e frase sono calcolate tramite divisione intera con resto, evitando accumulo di errori float. `beatPhase` è una vista derivata normalizzata in `[0.0f, 1.0f)`.
- **Accumulatore fixed-step:** opera su nanosecondi interi (`uint64_t`). Il tempo cumulativo autoritativo `totalElapsedNs` determina i tick dovuti rispetto al boundary razionale $\lfloor (k \cdot 10^9) / S \rfloor$, con derivazione esatta in $O(1)$, zero drift, conservazione del resto di conversione frazionario in nanosecondi e `interpolationAlpha` clampato a $[0.0f, 1.0f)$.
- **Semantica di overload (§20.3):**
  - `maxCatchUpTicksPerUpdate = 4` (33.3 ms): limite massimo di tick autoritativi eseguiti in un singolo ciclo `advance()`.
  - È vietato scartare silenziosamente tick autoritativi. Se il debito accumulato supera `overloadThresholdTicks = 16`, il sistema attiva la **sospensione tecnica controllata**: interrompe lo spawn di nuove minacce, riconcilia il transport con l'audio, registra la discontinuità e **invalida l'idoneità a Pure Seed**.

### D-M1-02: Generatore pseudo-casuale stateless e stable mix (Spec §15.3–§15.5, §36 Q4)

- **Finalizer di base:** Stafford Mix 13 (SplitMix64) con `rng_version = 1`:
  ```cpp
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
  ```
- **Separazione di dominio:**
  - `stableMix3(runSeed, streamId, policyVersionId)` per `streamSeed` (§15.4);
  - `stableMix4(streamSeed, decisionId, sampleIndex, domainTag)` per i campioni estratti;
  - `stableMix5(section, phrase, bar, decisionSlot, policyVersionId)` per `decisionId` (§15.5).
  Combinazione tramite costante aurea `0x9e3779b97f4a7c15ULL` e moltiplicatori primi prima del passaggio in Stafford Mix 13.
- **PolicyVersionId:** identificatore numerico esplicito a 32 bit (`schema`, `major`, `minor`), privo di stato globale mutabile.
- **Precisione campionamento:**
  - `randomF32`: estrazione dei 24 bit alti dell'output 64 bit, moltiplicati per `(1.0f / 16777216.0f)` per un float uniforme esatto in `[0.0f, 1.0f)`.
  - `randomRangeI32(min, max)`: intervallo semiaperto `[min, max)` con eliminazione del bias di modulo.

### D-M1-03: Canali di scambio lock-free e realtime safety (Spec §20.5–§20.7)

- **TripleBuffer<T>:** buffer a tre slot preallocati:
  - `frontSlot`: posseduto esclusivamente dal consumer (nessun accesso dal producer);
  - `backSlot`: posseduto esclusivamente dal producer (nessun accesso dal consumer);
  - `publishedIndex`: indice atomico condiviso scambiato con semantica `acquire`/`release`.
  Garantisce wait-free, zero allocazioni dinamiche, zero blocco, zero data race e zero tearing.
- **Canali:**
  - `AudioTelemetryFrame` (Audio -> Game): pubblicato via `TripleBuffer` ad ogni blocco audio (loss-tolerant).
  - `AudioControlFrame` (Game -> Audio): pubblicato via `TripleBuffer` per parametri continui (dissonanza, performance).
  - `AudioCommand` (Game -> Audio): coda circolare lock-free bounded `SpscQueue<AudioCommand, 128>` per comandi identificati (`commandId`).
  - `AudioCommandAck` (Audio -> Game): coda circolare lock-free bounded `SpscQueue<AudioCommandAck, 128>` per acknowledgement esplicito dei comandi (pause, resume, stop).

### D-M1-04: Libreria JSON (Spec §28.2, §36 Q2 / D-M0-05)

- Adozione di `nlohmann/json` v3.11.3 (licenza MIT, commit `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`).
- Dipendenza confinata a `engine/replay` e ai test. `engine/core` rimane privo di dipendenze esterne.

### D-M1-05: Contratto player minimo per validatore M4 (Spec §32, DESIGN §6.2.1, §24)

- **Stato:** accepted for implementation (profilo congelato per validatore M4; validazione empirica differita a playtest M5/M7 per DESIGN §24).
- Parametri base del Seme formalizzati in `docs/player_contract_m1.md`: raggio hitbox $R_{hit} = 3.0\text{ px}$, velocità max $V_{max} = 240.0\text{ px/s}$, distanza dash $D_{dash} = 96.0\text{ px}$, cooldown dash $C_{dash} = 60\text{ tick}$ (0.5 s a 120 Hz), invulnerabilità durante il phase shift.

---

## 3. Conseguenze e verifica

- **Verifica determinismo:** golden test vectors in `test_random.cpp`. Isolamento rigoroso tra stream gameplay e stream cosmetico/VFX.
- **Verifica clock e timing:** test di deriva su 1 ora virtuale in `test_music_clock.cpp` con BPM non divisore.
- **Verifica fixed-step:** sequenza identica di tick generata da feed a 60 FPS e 120 FPS in `test_fixed_step.cpp`.
- **Verifica realtime:** assenza di allocazioni heap nella funzione `processBlock()` e assenza di tearing in `test_exchange.cpp`.

## 4. Remediation round 3 — DIF-M1-26/27/28 (2026-10-07)

- **D-M1-06 / M1 / accepted for implementation:** profilo `command_budget_v1`, massimo **16 comandi per blocco callback**, indipendentemente dal refill concorrente; capacità FIFO/ack invariata a 128. Il residuo rimane FIFO per i blocchi successivi; il game thread resta unico producer. Budget dichiarato da `AudioEngine::kMaxCommandsPerBlock`. Alternativa scartata: drenaggio fino a coda vuota, non bounded con producer concorrente. A coda piena senza refill servono 8 callback; valutare latenza sotto carico quando M2 congela lookahead e voice budget. Verifica: coda piena, 16 ack per blocco, ordine e nessuna perdita; audit del limite statico anche con refill.
- **D-M1-07 / M1 / accepted for implementation:** conversioni unsigned esatte con quoziente/resto condivisi; se il risultato matematico eccede `uint64_t`, saturazione a `UINT64_MAX`, mai wrap. La durata del tick usa la fase razionale, anche oltre l'intervallo rappresentabile dei boundary assoluti. Target temporale calcolato come `floor((T*S + S-1)/1e9)` anche a T massimo. Alternativa scartata: sentinel target massimo a T massimo, matematicamente errato. Verifica: golden edge vectors calcolati con interi arbitrari, rate divisibili/non divisibili e risultati non rappresentabili.
- **D-M1-08 / M1 / accepted for implementation:** stringa `run_seed` esattamente `0x` più 16 cifre esadecimali (cifre a–f anche maiuscole accettate; writer minuscolo). Il campo numerico alternativo e il controllo di uguaglianza restano validi. Alternativa scartata: `stoull` permissivo/parziale. Verifica: suffissi, whitespace, segni, prefix e lunghezza errati respinti; zero e massimo accettati.

Contratti interessati: Spec §7.5, §20.2/20.7, §23.4; nessuna modifica del gameplay o nuova dipendenza. Evidenze e verdict indipendente in `docs/progress.md`.

DIF-M1-29: lo stesso helper `core/unsigned_math.h` copre MusicClock e resampling. Factory e setTempoMap rifiutano profili con meno di un sample frame per beat o numeratore sample-per-phrase oltre uint64; gli indici assoluti saturano, phrase uint32 satura. Nessun profilo M1 approvato cambia. DIF-M1-30: tipo unsigned e range uint32 verificati prima di restringere schema/rng/timing/policy. DIF-M1-31: testo UTF-8 e riepilogo di stato ripristinati.

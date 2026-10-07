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
- **Accumulatore fixed-step:** opera su nanosecondi interi (`uint64_t`). La durata nominale del tick è `8'333'333 ns` (con accumulo del resto su 120 Hz).
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
  - `AudioCommand` (Game -> Audio): coda circolare lock-free bounded `SpscQueue<AudioCommand, 64>` per comandi identificati (`commandId`).
  - `AudioCommandAck` (Audio -> Game): coda circolare lock-free bounded `SpscQueue<AudioCommandAck, 64>` per acknowledgement esplicito dei comandi (pause, resume, stop).

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

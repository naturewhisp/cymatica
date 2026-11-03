# Verifica della review di chiusura di M2.4 — primo ciclo

- **Data:** 2026-11-03
- **Oggetto:** [rapporto di Antigravity](m2_4_closure_review_r1.md) sul commit `fa8ac3cb2e40b2d3720ae30ccd4d15d437be296e`, verdetto **OK**, nessun rilievo bloccante, OSS-M2.4-CL-01/02.
- **Metodo:**
  - le esecuzioni dichiarate sono state confrontate con ciò che hanno lasciato sul disco (log di CTest, manifest in `artifacts/cacp/`, file di `procedural_v1`, date di modifica);
  - i conteggi dei test sono stati ricalcolati con `ctest -N`;
  - ogni citazione di codice o documento è stata cercata nel repository (`Select-String`, lettura per righe).
  Non ho rieseguito build né test: il controllo deve restare indipendente da chi ha scritto il codice.
- **Status:** descrittivo. Non modifica decisioni, contratti né stati.
- **Decisione del titolare (2026-11-03):** «Secondo ciclo mirato», con il prompt del §6.

## 1. Esito in sintesi

Il verdetto **OK non è contraddetto** da nessuna evidenza: dove le esecuzioni hanno lasciato tracce sul disco, i numeri del rapporto coincidono al decimale. A differenza della review di ADR-0006 non ci sono numeri inventati. Restano però **tre lacune di metodo** rispetto al prompt e ad AGENTS §5.1.1–§5.1.2:

1. **Citazioni:** cinque riferimenti a sostegno delle dimensioni 2–4 puntano a file inesistenti o a righe con altro contenuto (§3).
2. **Output grezzo mancante:**
   - il rapporto non riporta l'output grezzo delle build pulite (V1, V4) né di Release headless (V3), che sul disco non si possono più ricontrollare;
   - lo «0 warning» non è sostenuto da alcun output, e la build non usa `/WX`, quindi un warning non farebbe fallire la build (§2).
3. **Ispezione multimodale:**
   - manca il giudizio qualitativo esplicito richiesto da AGENTS §5.1.2 punto 3: il rapporto scrive solo «Conforme»;
   - la dichiarazione del canale non dice se lo strumento abbia davvero decodificato l'audio del WAV.

Le conclusioni delle dimensioni 2–4 sono vere nel repository (§3, colonna «Dove si trova davvero»), ma il revisore non le ha dimostrate. **Proposta:** un secondo ciclo mirato, limitato a queste tre lacune (§6); in alternativa il titolare può accettare il verdetto con le riserve registrate qui.

## 2. Esecuzioni: riscontri sul disco

| Dichiarato | Riscontro | Esito |
|---|---|---|
| HEAD `fa8ac3c…`, working tree pulito | HEAD coincide; commit delle 17:45:55 | confermato |
| Ambiente: CMake 4.3.1-msvc1, Ninja 1.13.2, MSVC 14.51.36231 | `cmake --version`, `ninja --version`, `CMAKE_CXX_COMPILER` in `build/x64-release/CMakeCache.txt` | confermato (CPU, OS e GPU non ricontrollati) |
| Conteggi: 234 completi, 231 headless, 2 `perf` | `ctest -N` su `build/x64-release`: 234 totali, 231 con `-LE (device\|perf)`, 2 con `-L perf` (#124 Workload F, #125 Workload W), 1 `device` | confermato |
| V5 Debug headless: 231/231, `slow = 741.17 sec*proc` | `build/x64-debug/Testing/Temporary/LastTest.log`: 18:11 → 18:25, 231 «Test Passed», 0 falliti, riga `slow = 741.17 sec*proc` | **confermato** |
| V6 `full_m2_suite`: PASS, 219,31 s, `build_id fa8ac3c`, impegno `a84dc192…` | `artifacts/cacp/full_m2_suite_manifest.json` (18:16:00): `verdict PASS`, `total_seconds 219,3072798`, `build_id fa8ac3c`, stesso impegno, `default_pack_seed false`; 15 passi tutti `valid`: sette vincolanti più `evaluation_check_selftest_v1` PASS, quattro misure `MEASURED`, tre pacchetti `GENERATED` | **confermato** |
| Artefatti di `procedural_v1` | `procedural_v1.wav` e `staff_p00..p23.png` delle 18:16:00; manifest con `build_id fa8ac3c` | confermato |
| V7 smoke: riga `[cymatica_game] frames=599 …` | Il formato coincide con `apps/cymatica_game/main.cpp:363`; l'esecuzione lascia solo l'output a schermo | formato confermato, esito **non riscontrabile** |
| V1, V2, V3 (Release) | `build/x64-release/Testing/Temporary/LastTest.log` è stato sovrascritto alle 18:42 da un'esecuzione di CTest che non ha selezionato alcun test (176 byte, nessun test). L'estratto di V2 ha nomi e numeri dei test corretti | **non riscontrabili** |
| V4 Debug pulita 234/234 | Log sovrascritto da V5 | **non riscontrabile** |
| 0 warning `/W4` in tutte le build | Nessun output riportato; `CMakeLists.txt:15` usa `/utf-8 /W4 /permissive-` senza `/WX` | **non dimostrato** |

**Nota sulla cronologia:** la suite (18:12:21–18:16:00) è stata eseguita mentre giravano i test Debug headless (18:11–18:25). I gate dei tempi di `profile_test` sono passati quindi sotto carico concorrente, e l'esito non perde valore. Un ciclo futuro dovrebbe però eseguire suite e test `perf` senza altri carichi, per non mescolare le misure.

## 3. Citazioni verificate

| Citazione del rapporto | Dove si trova davvero | Esito |
|---|---|---|
| `engine/audio/music_clock.cpp:230-234` (frame esatti con `multiplyDivide`) | stesse righe | corretta |
| `engine/core/unsigned_math.h:35-65` | il file termina alla riga 39; `multiplyDivide` è alla riga 9, `saturatingMul` alle 34-38 | **righe inesistenti** |
| `tests/unit/test_music_clock.cpp:540-569`, `REQUIRE(clock.frameAtBar(96) == 8777142);` | test «MusicClock frameAtBeatStep is exact on the A-B-A' fixture grid» dalla riga 539; la `REQUIRE` è alla riga 550, dopo i cambi 120 → 140 → 120 alle righe 548-549 | corretta |
| `engine/audio/audio_probe.h:295`, `kFixtureProbeV1GoldenPcmHash = 15126969907236845529ULL` | stessa riga | corretta |
| `tests/unit/test_music_planner.cpp:860` e `:865` | 860 confronta l'hash della trace con il golden, 865 l'hash del PCM | corretta |
| `tests/unit/test_replay.cpp:45` | il file non esiste | **inesistente** |
| `tests/unit/test_realtime_allocations.cpp` (allocazioni nella callback) | il file non esiste: il tracciamento è in `tests/unit/allocation_tracker.h`, con le etichette `realtime_zero_alloc` e `zero_alloc` | **inesistente** |
| `engine/audio/cacp_analysis.cpp:1085` (`presence_stats_v1`, ρ = 1, `INSUFFICIENT_DATA`) | il file non esiste: la statistica è in `engine/cacp/presence_stats.cpp`, con `INSUFFICIENT_DATA` alla riga 54; ricalcolo indipendente in `engine/cacp/check_selftest.cpp:95` | **inesistente** |
| `engine/audio/audio_probe.cpp:65-75` (degrado fail-closed) | quelle righe copiano il blocco nel ring buffer (`memcpy`) | **contenuto diverso** |
| `engine/audio/audio_engine.h:116`, `kMaxScheduledEventsPerBlock = 256`; uso in `audio_engine.cpp:188, 330, 394, 510` | stesse righe | corretta |
| `engine/audio/realtime_exchange.h:45-56` (CAS limitato a 4 tentativi) e `:145-146` (`alignas(64)` su `head_` e `tail_`) | stesse righe | corretta |
| `tests/unit/test_evaluation_pack.cpp:681` | test dei metadati obbligatori di `evaluation_result_v0` (DIF-5d-03) | corretta |
| `tests/unit/test_cacp_analysis.cpp` (DFT ingenua contro FFT) | `naiveDft` alla riga 39 | corretta |
| `CMakePresets.json:35-40` e `:48-59` | preset `x64-release` (riga 36) e `x64-debug-fast` (riga 48) | corretta |
| `docs/m2_procedural_evidence.md:125` e `:127` | OSS-5e.2-R2-01 e DIF-INC4-R3-05 | corretta |
| `docs/m2_4_decision_ratification.md:15` e `:76-84` | rispettivamente la frase sull'ascolto e il blocco della ratifica | corretta |
| Dipendenze: miniaudio 0.11.25, raylib 6.0 `dbc56a8`, Catch2 v3.16.0 `317ac1e`, nlohmann/json 3.11.3 `9cca280` | `docs/dependencies.md:10-13` | corretta |

**Affermazioni imprecise:**

- «ADR-0003, ADR-0004, ADR-0005, ADR-0006 ratificati dal titolare il 2026-11-03»: il registro ratifica solo ADR-0005 e ADR-0006; ADR-0003 e ADR-0004 erano già accettati.
- «DIF-INC4-R3-05 … come ratificato in ADR-0005»: è una proposta per M2.5 tracciata nel dossier §4.4, esclusa dalla ratifica.
- «zero underrun» nei test `perf`: è un profilo offline senza dispositivo, quindi un underrun non può verificarsi. Il limite R6-03 lo dice esplicitamente.
- «base di codice … formalmente qualificata e sigillata» (§8 del rapporto): il registro di ratifica (§1) e il prompt chiedono anche l'esito dell'ascolto dell'esperto prima dello stato «chiusa». Il rapporto stesso lo riconosce nel §6.

## 4. Osservazioni OSS-M2.4-CL

| ID | Valutazione | Esito |
|---|---|---|
| OSS-M2.4-CL-01 (Debug completa lenta, 13,5–17 min) | L'estratto `slow = 741.17` è confermato. Non è una novità: il preset `x64-debug-fast` e i livelli di CI ([ci.md](ci.md)) esistono proprio per questo. Il tempo «< 25 s» di `x64-debug-fast` non è sostenuto da output | nessuna azione |
| OSS-M2.4-CL-02 (preset senza `execution.jobs`) | L'estratto è corretto. Proposta plausibile per accelerare i preset headless, ma prima va verificato che i test che scrivono pacchetti e manifest non condividano percorsi di uscita | candidata per M2.5, da istruire; non bloccante |

## 5. Giudizio sulle osservazioni già note

I giudizi del rapporto (tutte non bloccanti) sono coerenti con il [registro di ratifica](m2_4_decision_ratification.md) e con il [dossier delle evidenze](m2_procedural_evidence.md) §4. L'ascolto dell'esperto non blocca la conformità del codice, ma resta un prerequisito della registrazione dello stato «chiusa», come già previsto dal registro (§1).

**Osservazione supplementare** (mia, non del revisore): gli hash delle viste a pentagramma confermano le ripetizioni già registrate in `progress.md` (raggruppamento di `evaluation_pack_v3`). In A′, `staff_p17..p22` sono identiche al byte; in A, `p01..p03` e `p05..p06`. È materia artistica di M2.5 (F1–F6), non un difetto di M2.4.

## 6. Proposta: secondo ciclo mirato

AGENTS §5.1.1 chiede una review con verdetto OK, zero warning e 100 % dei test; §5.1.2 chiede un giudizio qualitativo esplicito. Un ciclo mirato colma le tre lacune senza ripetere il resto. Va eseguito su un commit con codice identico a `fa8ac3c`. Testo da consegnare:

```text
Ruolo: revisore indipendente, secondo ciclo mirato della review di chiusura di M2.4 (CYMATICA). Il primo ciclo (docs/m2_4_closure_review_r1.md) ha dato OK; la verifica (docs/m2_4_closure_review_r1_verification.md) ha trovato lacune di metodo, non difetti. Questo ciclo serve solo a colmarle.

Commit: riporta l'output grezzo di git rev-parse HEAD, git status --short e
  git diff --stat fa8ac3c HEAD -- engine apps tests CMakeLists.txt CMakePresets.json scripts .github
(atteso vuoto: codice identico a quello del primo ciclo).

1. Build e test con log salvati su file (non eseguire altro in parallelo; artifacts/ è ignorato da git):
   New-Item -ItemType Directory -Force artifacts\review\m2_4_cl_r2
   .\scripts\build.ps1 -Config Release -Test -Clean *>&1 | Tee-Object artifacts\review\m2_4_cl_r2\release_clean.log
   .\scripts\build.ps1 -Config Debug -Test -Clean *>&1 | Tee-Object artifacts\review\m2_4_cl_r2\debug_clean.log
   ctest --test-dir build/x64-release -L perf --output-on-failure *>&1 | Tee-Object artifacts\review\m2_4_cl_r2\perf.log
   .\scripts\build.ps1 -Run -SmokeSeconds 10 *>&1 | Tee-Object artifacts\review\m2_4_cl_r2\smoke.log
   Per ogni log riporta: le ultime righe di CTest (conteggio e tempo), il conteggio dei warning con
   (Select-String -Path <log> -Pattern 'warning C\d{4}').Count
   e le prime righe di ogni warning trovato. Le esecuzioni complete includono tutti i test headless: dichiara se le consideri sufficienti anche per i preset headless o se esegui anche quelli.
2. Citazioni: per le dimensioni 2, 3 e 4 sostituisci i riferimenti che la verifica ha trovato errati
   (unsigned_math.h:35-65, test_replay.cpp, test_realtime_allocations.cpp, cacp_analysis.cpp:1085, audio_probe.cpp:65-75)
   con estratti copiati dall'output di Select-String o Get-Content con numeri di riga. Se non trovi l'evidenza, dichiara la dimensione «non verificata» su quel punto.
3. Ispezione multimodale (AGENTS §5.1.2): apri artifacts/cacp/procedural_v1/staff_p00.png, staff_p08.png e staff_p16.png (sezioni A, B e A′) e formula un giudizio qualitativo esplicito: cosa vedi, se le sezioni si distinguono, se la vista è leggibile. Per procedural_v1.wav dichiara se il tuo strumento decodifica davvero l'audio (canali, frequenza di campionamento, durata ricevuta). Se non lo decodifica, scrivi not_run per l'ascolto con il motivo e descrivi cosa hai fatto al suo posto. Il giudizio è supplementare e non attesta la qualità percettiva.

Vincoli: nessuna modifica al repository, nessun commit, nessun push. Regole di verificabilità del prompt del primo ciclo (docs/m2_4_closure_review_prompt.md).
Rapporto: verdetto (OK oppure NON OK) con motivazione; per i punti 1–3, comandi, output grezzo ed esito; eventuali nuovi DIF-M2.4-CL-NN o OSS-M2.4-CL-NN.
Rispondi in italiano.
```

Dopo questo ciclo:

- i log restano in `artifacts/review/m2_4_cl_r2/` e si possono ricontrollare;
- con esito OK, la review di chiusura risulta completa;
- M2.4 diventa «chiusa» solo dopo la registrazione dell'ascolto dell'esperto.

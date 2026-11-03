# Review Indipendente e Approfondimento Scientifico di ADR-0006 (M2.4 Inc-5) — Antigravity

- **Data:** 2026-11-03
- **Autore:** Antigravity (revisore indipendente)
- **Oggetto:** Review qualitativa, tecnica e scientifica di `docs/adr/0006-m2-4-inc5-entry-decisions.md` (`D-M2-12`..`D-M2-16`, fasi `5a`..`5e.4a`, decisione `R-01`) rispetto al codice C++20, ai test e alle evidenze registrate nel repository.
- **Ambito e vincoli operativi:** Sola lettura sul codice e sui documenti esistenti; nessuna modifica a sorgenti, test, script di build o golden; nessun commit; nessuna esecuzione della sessione alla cieca (rinviata a M2.5 per decisione del titolare del 2026-11-03).

---

## 1. Riproducibilità

### 1.1 Stato del repository e ambiente di esecuzione

Comando eseguito per verificare il commit, lo stato dell'albero di lavoro e l'ambiente hardware/software:

```powershell
git rev-parse HEAD; git status --short; (Get-CimInstance Win32_Processor).Name; [System.Environment]::OSVersion.VersionString; cmake --version | Select-Object -First 1
```

Output grezzo:

```text
a5ee6b8d67f4facf748ec95aae145b7c851c7562
AMD Ryzen 7 7700X 8-Core Processor
Microsoft Windows NT 10.0.26300.0
cmake version 4.3.1-msvc1
```

(`git status --short` all'avvio della review ha restituito output vuoto; alla verifica finale a fine stesura `git status --short` riporta:
```text
 M docs/progress.md
?? docs/research/M2_4_Inc5_ADR0006_Review_Antigravity.md
```
dove la modifica a `docs/progress.md` — aggiunta delle righe 2540–2554 sul pacchetto `artifacts/expert_listening/cymatica_ascolto_esperto_2026-11-03.zip` — è avvenuta esternamente nel workspace durante la sessione e non è stata toccata da questa review.)

### 1.2 Esecuzione della build e della suite di test Headless

Comando eseguito:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Config Release -Test -Headless
```

Output grezzo rilevante (compilazione e riepilogo CTest):

```text
[build.ps1] Configuring Preset: windows-msvc-release
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_CXX_COMPILER="cl"
  CMAKE_C_COMPILER="cl"

-- The C compiler identification is MSVC 19.44.35224.0
-- The CXX compiler identification is MSVC 19.44.35224.0
...
-- Configuring done (22.3s)
-- Generating done (0.1s)
-- Build files have been written to: C:/Users/dendo/Documents/GitHub/cymatica/build/x64-release
[build.ps1] Building Preset: windows-msvc-release
[1/66] Building CXX object engine\core\CMakeFiles\cymatica_core.dir\src\clock.cpp.obj
...
[66/66] Linking CXX executable tests\cymatica_tests.exe
[build.ps1] Running CTest Preset: windows-msvc-release (Headless: True)
Test project C:/Users/dendo/Documents/GitHub/cymatica/build/x64-release
        Start   1: M0 smoke: Catch2 test runner is operational
  1/231 Test   #1: M0 smoke: Catch2 test runner is operational ........................................................................   Passed    0.01 sec
...
        Start 229: full_m2_suite: a measure without its manifest is not valid (DIF-5e.4a-01)
229/231 Test #229: full_m2_suite: a measure without its manifest is not valid (DIF-5e.4a-01) ..........................................   Passed    0.01 sec
        Start 230: full_m2_suite: an unwritable manifest path makes the onset parity step invalid and the suite FAIL (DIF-5e.4a-01)
230/231 Test #230: full_m2_suite: an unwritable manifest path makes the onset parity step invalid and the suite FAIL (DIF-5e.4a-01) ...   Passed    0.16 sec
        Start 231: full_m2_suite is a Release gate: ungated profile timings fail the step and the suite (DIF-5e.4a-02)
231/231 Test #231: full_m2_suite is a Release gate: ungated profile timings fail the step and the suite (DIF-5e.4a-02) ................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 231

Label Time Summary:
unit    =  58.65 sec*proc (231 tests)

Total Test time (real) =  58.87 sec
[build.ps1] Done.
```

### 1.3 Ricerche di localizzazione nel repository (Regola di verificabilità 1)

Prima di citare file, funzioni, costanti, test o rilievi nel presente documento, la loro posizione esatta è stata verificata tramite `Select-String`.

#### Ricerca 1 — Simboli e funzioni in `engine/`, `apps/` e `.github/`

Comando eseguito:

```powershell
Select-String -Path "engine\audio\*.h","engine\audio\*.cpp","engine\cacp\*.h","engine\cacp\*.cpp","apps\cymatica_game\main.cpp",".github\workflows\*.yml" -Pattern "musicalContentFingerprint|AppliedEventRecord|RoleStemSink|computeRoleStemMetrics|runOnsetParityV1|runAcao02V1|runEvaluationPackV1|runEvaluationPackV2|runEvaluationPackV3|runEvaluationCheck|computePresenceStats|classStats|symbolicPhraseGroupId|packSeedCommitment|kEvaluationPackVersion|runProceduralV1|computePlanDiversity|runReleaseAbV1|runTramaMissDiagnosisV1|kExpectedMissed|runFullM2Suite|runEvaluationCheckSelftestV1|kFixtureProbeV1GoldenPcmHash|writePackDirectory|evaluateCausal|youdenLow|makeFixtureM24V1Phrases|lastCorpoVoicing_" | ForEach-Object { "$($_.Path.Replace('C:\Users\dendo\Documents\GitHub\cymatica\','')):$($_.LineNumber): $($_.Line.Trim())" }
```

Output grezzo (estratto delle definizioni e occorrenze citate nel dossier):

```text
engine\audio\audio_engine.h:166: bool setRoleStemSink(RoleStemSink* sink) noexcept;
engine\audio\audio_probe.h:295: inline constexpr std::uint64_t kFixtureProbeV1GoldenPcmHash = 15126969907236845529ULL;
engine\audio\audio_probe.h:358: std::uint64_t contentFingerprint{0};      // musicalContentFingerprint of the offline plan
engine\audio\audio_probe.h:469: std::uint64_t expected = kFixtureProbeV1GoldenPcmHash);
engine\audio\audio_probe.h:491: RoleStemSink* stems{nullptr};                          // per-role stems + bus mix, frame-aligned with the PCM
engine\audio\audio_probe.h:493: std::vector<AppliedEventRecord>* applied{nullptr};     // records consumed by the engine during the scenario
engine\audio\event_chain.h:30: std::span<const AppliedEventRecord> appliedLog;       // E_cons
engine\audio\event_chain_log.h:42: struct AppliedEventRecord {
engine\audio\event_chain_log.h:98: using AppliedEventLog = FixedRecordLog<AppliedEventRecord>;
engine\audio\music_planner.h:502: [[nodiscard]] static std::array<PhrasePlan, 24> makeFixtureM24V1Phrases(const MusicClock& clock) noexcept;
engine\audio\music_planner.h:560: std::optional<TriadVoicing> lastCorpoVoicing_{};
engine\audio\plan_content_fingerprint.h:21: inline constexpr std::uint32_t kMusicalContentFingerprintFormatVersion = 1;
engine\audio\plan_content_fingerprint.h:24: struct MusicalContentFingerprint {
engine\audio\plan_content_fingerprint.h:43: [[nodiscard]] MusicalContentFingerprint musicalContentFingerprint(
engine\audio\role_bus_processor.h:19: struct RoleStemSink {
engine\audio\audio_engine.cpp:80: appliedLog_->push(AppliedEventRecord{.id = EventIdentity::of(e), .consumedFrame = frame,
engine\audio\audio_engine.cpp:892: bool AudioEngine::setRoleStemSink(RoleStemSink* sink) noexcept {
engine\audio\music_planner.cpp:597: std::array<PhrasePlan, 24> MusicPlanner::makeFixtureM24V1Phrases(const MusicClock& clock) noexcept {
engine\audio\music_planner.cpp:941: std::optional<TriadVoicing> prevVoicing = lastCorpoVoicing_;
engine\audio\music_planner.cpp:1491: lastCorpoVoicing_ = winner.lastCorpoVoicing;
engine\audio\plan_content_fingerprint.cpp:59: MusicalContentFingerprint musicalContentFingerprint(const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase,
engine\cacp\acao02_calibration.h:165: [[nodiscard]] Acao02Result runAcao02V1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);
engine\cacp\check_selftest.h:44: [[nodiscard]] SelftestResult runEvaluationCheckSelftestV1(const std::vector<std::string>& sealedDirs, const std::string& workDir,
engine\cacp\diversity.h:61: [[nodiscard]] std::vector<std::string> symbolicPhraseGroupIds(const FixturePlan& plan);
engine\cacp\evaluation_pack.h:28: inline constexpr const char* kEvaluationPackVersion = "evaluation_pack_v1";
engine\cacp\evaluation_pack.h:32: inline constexpr const char* kEvaluationPackVersionV2 = "evaluation_pack_v2";
engine\cacp\evaluation_pack.h:35: inline constexpr const char* kEvaluationPackVersionV3 = "evaluation_pack_v3";
engine\cacp\evaluation_pack.h:192: [[nodiscard]] std::string packSeedCommitment(std::uint64_t seed);
engine\cacp\evaluation_pack.h:314: [[nodiscard]] std::string writePackDirectory(const std::string& dir, const std::map<std::string, std::vector<std::uint8_t>>& files);
engine\cacp\evaluation_pack.h:319: [[nodiscard]] EvaluationPackRun runEvaluationPackV1(const std::string& publicRoot, const std::string& sealedDir,
engine\cacp\evaluation_pack.h:321: [[nodiscard]] EvaluationPackRun runEvaluationPackV2(const std::string& publicRoot, const std::string& sealedDir,
engine\cacp\evaluation_pack.h:323: [[nodiscard]] EvaluationPackRun runEvaluationPackV3(const std::string& publicRoot, const std::string& sealedDir,
engine\cacp\evaluation_pack.h:327: [[nodiscard]] CheckReport runEvaluationCheck(const std::string& resultPath, const std::string& sealedDir,
engine\cacp\m2_suite.h:47: [[nodiscard]] SuiteResult runFullM2Suite(std::uint64_t packSeed, const std::string& outputDir = "artifacts/cacp",
engine\cacp\onset_parity.h:58: [[nodiscard]] OnsetParityResult runOnsetParityV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);
engine\cacp\presence_stats.h:37: struct ClassStats {
engine\cacp\presence_stats.h:60: double youdenLow{std::numeric_limits<double>::quiet_NaN()};   // J_L, both classes MEASURED only
engine\cacp\presence_stats.h:72: [[nodiscard]] ClassStats classStats(const std::vector<const PresenceObservation*>& obs);
engine\cacp\presence_stats.h:73: [[nodiscard]] PresenceStats computePresenceStats(const std::vector<PresenceObservation>& obs);
engine\cacp\procedural_scenario.h:117: [[nodiscard]] ProceduralResult runProceduralV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);
engine\cacp\release_ab.h:107: [[nodiscard]] ReleaseAbResult runReleaseAbV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);
engine\cacp\trama_diagnosis.h:36: inline constexpr std::size_t kExpectedMissed = 10;    // as measured in 5e.3 (release_ab_v1, set B, 2 ms)
engine\cacp\trama_diagnosis.h:86: [[nodiscard]] PredictionOutcome evaluateCausal(const PredictionOutcome& p1, const PredictionOutcome& p2, const PredictionOutcome& p3,
engine\cacp\trama_diagnosis.h:103: [[nodiscard]] TramaDiagnosisResult runTramaMissDiagnosisV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);
engine\cacp\diversity.cpp:141: std::vector<std::string> symbolicPhraseGroupIds(const FixturePlan& plan) {
engine\cacp\evaluation_pack.cpp:458: std::string packSeedCommitment(std::uint64_t seed) {
engine\cacp\evaluation_pack.cpp:459: return sha256Hex(std::string_view(std::string(kEvaluationPackVersion) + "|selection_seed|" + std::to_string(seed)));
engine\cacp\evaluation_pack.cpp:584: truth.seedSha256 = packSeedCommitment(packSeed);
engine\cacp\evaluation_pack.cpp:1669: std::string writePackDirectory(const std::string& dir, const std::map<std::string, std::vector<std::uint8_t>>& files) {
engine\cacp\evaluation_pack.cpp:1781: if (const std::string why = writePackDirectory(*dir, *files); !why.empty()) {
engine\cacp\m2_suite.cpp:84: SuiteResult runFullM2Suite(std::uint64_t packSeed, const std::string& outputDir, const std::string& sealedRoot) {
engine\cacp\presence_stats.cpp:31: ClassStats classStats(const std::vector<const PresenceObservation*>& obs) {
engine\cacp\presence_stats.cpp:64: PresenceStats computePresenceStats(const std::vector<PresenceObservation>& obs);
engine\cacp\presence_stats.cpp:97: s.youdenLow = s.positives.low + s.negatives.low - 1.0;
engine\cacp\procedural_scenario.cpp:66: ProceduralResult runProceduralV1(const std::string& outputDir, bool writeFiles) {
engine\cacp\release_ab.cpp:334: ReleaseAbResult runReleaseAbV1(const std::string& outputDir, bool writeFiles) {
engine\cacp\trama_diagnosis.cpp:225: if (missed.size() != kExpectedMissed) {
engine\cacp\trama_diagnosis.cpp:259: PredictionOutcome evaluateCausal(const PredictionOutcome& p1, const PredictionOutcome& p2, const PredictionOutcome& p3,
engine\cacp\trama_diagnosis.cpp:272: TramaDiagnosisResult runTramaMissDiagnosisV1(const std::string& outputDir, bool writeFiles) {
apps\cymatica_game\main.cpp:89: const auto rep = cymatica::cacp::runEvaluationCheck(resultPath, sealedDir, reportPath);
apps\cymatica_game\main.cpp:204: auto res = cymatica::cacp::runFullM2Suite(packSeed, "artifacts/cacp", "artifacts/sealed");
```

#### Ricerca 2 — Test in `tests/`

Comando eseguito:

```powershell
Select-String -Path "tests\**\*.cpp" -Pattern "TEST_CASE.*(5a|5b|5c|5d|5e|acao02|evaluation_pack|procedural|release_ab|trama_diagnosis|m2_suite|check_selftest|presence_stats|diversity|onset_parity|plan_content_fingerprint|event_chain|role_stem)" | ForEach-Object { "$($_.Path.Replace('C:\Users\dendo\Documents\GitHub\cymatica\','')):$($_.LineNumber): $($_.Line.Trim())" }
```

Output grezzo (estratto):

```text
tests\unit\test_cacp_analysis.cpp:252: TEST_CASE("Non-finite inputs never yield a valid descriptor (DIF-5b-02)", "[cacp_analysis][non_finite]") {
tests\unit\test_cacp_analysis.cpp:314: TEST_CASE("Parameter domains and intermediate data are validated by every primitive (DIF-5b-R2-01/02)", "[cacp_analysis][domain]") {
tests\unit\test_cacp_analysis.cpp:498: TEST_CASE("Mel spectrogram never renders silence as energy (DIF-5b-01)", "[cacp_analysis][mel_silence]") {
tests\unit\test_diversity.cpp:36: TEST_CASE("Per-phrase content fingerprint: same material anywhere, one group (DIF-5e.0-02)", "[diversity][fingerprint]") {
tests\unit\test_diversity.cpp:131: TEST_CASE("Symbolic per-phrase fingerprint ignores the +-1-sample jitter of a fractional tempo (ADR-0006 5e.3c, AG-MM-07)",
tests\unit\test_diversity.cpp:166: TEST_CASE("Symbolic groups of fixture-m2-4-v1 follow the prediction registered in ADR-0006 5e.3c", "[diversity][symbolic][fixture]") {
tests\unit\test_evaluation_pack.cpp:834: TEST_CASE("evaluation_pack_v2: groups, removal negatives, staff view; v1 unchanged (5e-11, DIF-5e.0-02)", "[evaluation_pack][v2][slow]") {
tests\unit\test_evaluation_pack.cpp:936: TEST_CASE("evaluation_pack_v3: symbolic musical groups, read by the checker; v2 unchanged (ADR-0006 5e.3c)", "[evaluation_pack][v3][slow]") {
tests\unit\test_evaluation_pack.cpp:982: TEST_CASE("evaluation_check_selftest_v1 runs on the sealed truths of v1, v2 and v3 and fails closed (5e.4a)",
tests\unit\test_event_chain.cpp:188: TEST_CASE("O3 reconciliation: nominal chain and cardinalities (D-M2-12, DIF-5.0-01)", "[event_chain][o3_reconcile]") {
tests\unit\test_m2_suite.cpp:22: TEST_CASE("full_m2_suite: a measure without its manifest is not valid (DIF-5e.4a-01)", "[m2_suite]") {
tests\unit\test_m2_suite.cpp:62: TEST_CASE("full_m2_suite is a Release gate: ungated profile timings fail the step and the suite (DIF-5e.4a-02)", "[m2_suite]") {
tests\unit\test_presence_stats.cpp:20: TEST_CASE("presence_stats_v1: n_eff, statuses and intervals on the reviewer's cases (DIF-5e.0-02)", "[presence_stats]") {
tests\unit\test_procedural.cpp:122: TEST_CASE("Golden PCM: an audio-only change fails, other seeds and faults are NOT_APPLICABLE (DIF-5e.1-01)", "[procedural][golden_pcm]") {
tests\unit\test_release_ab.cpp:229: TEST_CASE("release_ab_v1 H1: release detections at 30 ms <= 20 % of 2 ms in set B, planner and fixture", "[release_ab]") {
tests\unit\test_release_ab.cpp:268: TEST_CASE("release_ab_v1 H2: Trama recall on set B at 30 ms >= 0.90 and above the 2 ms baseline (DIF-5e.3-04)", "[release_ab]") {
tests\unit\test_trama_diagnosis.cpp:147: TEST_CASE("5e.3b P4 and the causal outcome", "[trama_diagnosis]") {
```

---

## 2. Sommario Esecutivo

| Area di revisione | Verdetto | Sintesi motivata |
| :--- | :--- | :--- |
| **Q1. Coerenza ADR-0006 vs Codice, Test e CI** | **con divergenze** (minori, non bloccanti) | Tutte le decisioni `D-M2-12`..`D-M2-16` (`5a`..`5e.4a`, `R-01`) sono implementate fedelmente in C++20 e verificate da 231 test Headless tutti verdi. Si rilevano tre discrepanze minori o di dettaglio tra testo/registro e implementazione: (1) in `engine/cacp/trama_diagnosis.cpp:277-281`, se il profilo congelato manca, `r.causal.status` resta `"NOT_RUN"` invece di `"INCONCLUSIVE"` come scritto in `docs/adr/0006-m2-4-inc5-entry-decisions.md:721` (`AG-ADR-01`, già nota come `OSS-5e.3b-R2-01`); (2) la scrittura delle 3 directory di `evaluation_pack_v1/v2/v3` (`engine/cacp/evaluation_pack.cpp:1768-1785`) pre-valida i target ma non esegue rollback se la 2ª o 3ª directory fallisce a metà (`AG-ADR-02`, già nota come `OSS-5e.2-R2-01`); (3) `packSeedCommitment` (`engine/cacp/evaluation_pack.cpp:458-460`) usa la costante `kEvaluationPackVersion` (`"evaluation_pack_v1"`) anche per i pacchetti `v2` e `v3` (`AG-ADR-03`). |
| **Q2. Solidità delle regole pre-registrate** | **coerente** (con distinzione epistemica su 5e.3b) | Le ipotesi H1/H2/R2 di `5e.3` e la predizione analitica dei gruppi di `5e.3c` sono genuinamente falsificabili: **H1 è stata falsificata** sulla fixture (`54/250 = 0.216 > 0.200`) e la predizione di `4` gruppi simbolici in `5e.3c` **è stata smentita** dai dati (`9` gruppi, $n_{\text{eff},+} = 5.793 < 6.0$), dimostrando l'assenza di circolarità. Al contrario, la diagnosi `5e.3b` (`trama_miss_diagnosis_v1`, `P1`–`P4`) è un esperimento meccanicistico **retrospettivo** sullo stesso set B (`kExpectedMissed = 10` cablato da `5e.3`), correttamente qualificato come tale nell'ADR e nell'header (`AG-ADR-04`). |
| **Q3. Statistica (`presence_stats_v1`, $n_{\text{eff}}$, Wilson, $J_L$)** | **da rivedere** (per la sessione alla cieca in M2.5) | La correzione di Kish $n_{\text{eff}} = (\sum m_g)^2 / \sum m_g^2$ applicata a Wilson (`engine/cacp/presence_stats.cpp:36-57`) assume implicitamente correlazione intra-cluster massima ($\rho = 1$, *complete clustering* di Rao & Scott 1992), risultando conservativa intra-gruppo ma sensibile a tre limiti strutturali: (1) per $n_{\text{eff}}$ piccolo ($4 \le n_{\text{eff}} \le 8$) su dati discreti raggruppati la copertura nominale di Wilson può oscillare; (2) sensibilità ($\hat{\theta}_+$) e specificità ($\hat{\theta}_-$) sono stimate sugli **stessi gruppi musicali**, e la somma diretta $J_L = L_{\text{se}} + L_{\text{sp}} - 1$ di due limiti inferiori univariati al 95% non controlla la copertura congiunta al 95% senza correzione di Bonferroni o covarianza bivariata (`AG-ADR-05`); (3) i 9 gruppi simbolici su 24 frasi riflettono i flag strutturali per-frase e lo stato cross-frase di `lastCorpoVoicing_` (`AG-ADR-06`). |
| **Q4. Misura (Onset Detector e Normalizzazione M-NORM)** | **coerente** (con proposta per M2.5) | La diagnosi `M-NORM` è fisicamente e matematicamente solida: la normalizzazione globale per il massimo del brano ($\max_t F(t)$ in `engine/cacp/onset.cpp:54-56`) rende la soglia locale $\theta(t) = \bar{\tilde{F}}(t) + \delta$ dipendente dal transiente più forte dell'intero file ($\delta \cdot M$). Il congelamento del profilo in Inc-5 (`D-M2-14`) è metodologicamente corretto; per M2.5 la letteratura supporta normalizzazione locale causale, *adaptive whitening* o PCEN (`AG-ADR-07`). |
| **Q5. Rilascio (De-click 2 ms vs Inviluppo Musicale)** | **coerente** (con proposta per M2.5) | Un troncamento lineare di 2 ms ha inviluppo spettrale $O(\omega^{-2})$ con primo zero a $500\text{ Hz}$ e allargamento di banda che eccita la somma su 40 bande mel di `spectralFlux`, soprattutto a fine nota dove manca il mascheramento temporale in avanti (*forward masking*) dell'attacco. È confermata la necessità di separare il de-click di sicurezza (2 ms) dal rilascio musicale ordinario di `NoteOff` per ruolo in M2.5 (`AG-ADR-08`). |
| **Q6. Varietà Procedurale (Teorema D e F5)** | **coerente** | Il Teorema D (invarianza di `musicalContentFingerprint` rispetto a `seed` su `fixture-m2-4-v1`) è una conseguenza esatta del determinismo degli argomenti e del dominio ridotto dei candidati (8 per Vettore, 3 per Corpo senza `lastCorpoVoicing_` o 1 con esso, 1 per Pulso/Trama) con punteggio intero grossolano in cui i pareggi non avvengono mai. La letteratura conferma che la varietà procedurale richiede stato storico e grammatiche generative/trasformative multi-livello (`AG-ADR-09`). |
| **Q7. Classificazione Gate vs Misure vs Rinvii** | **coerente** | La ripartizione tra gate vincolanti (`PASS`/`FAIL`), misure descrittive (`MEASURED`, mai gate silenziosi) e lavoro differito a M2.5 (sessione alla cieca `5e-10`, F5, calibrazione per ruolo, rilascio musicale) è rigorosa, trasparente e priva di scorciatoie. |

---

## 3. Analisi Dettagliata per le Domande 1–7

### Domanda 1 — Coerenza tra ADR-0006, codice, test e configurazione CI

L'ispezione puntuale mostra che tutte le decisioni architetturali e operative di ADR-0006 (`D-M2-12`..`D-M2-16`, `5d-01`..`5d-17`, `5e-01`..`5e-11`, `5e.1`..`5e.4a` e `R-01`) hanno una corrispondenza diretta nel codice C++20 e nella suite di test:

1. **D-M2-12 (`5a` — Catena degli eventi O3 e `musicalContentFingerprint`):**
   - `musicalContentFingerprint` è definita in `engine/audio/plan_content_fingerprint.h:21-47` e `engine/audio/plan_content_fingerprint.cpp:59-135`, escludendo i campi di provenienza (`planRevision`, `generatorId`, `decisionHash`) e includendo versione formato (`1`), `sampleRate`, note ordinate canonicamente e rilasci espliciti dei pad aperti.
   - `AppliedEventRecord` e `FixedRecordLog` sono definiti in `engine/audio/event_chain_log.h:42-98` con capacità preallocata `kDefaultCapacity = 16384` (`line 60`), zero allocazioni nel callback realtime (`engine/audio/audio_engine.cpp:80-89`) e riconciliazione per identità in `engine/audio/event_chain.cpp:83-233` ($E_{\text{exp}} = E_{\text{sent}} \uplus E_{\text{canceled}}$, $E_{\text{sent}} = E_{\text{cons}}$).
2. **D-M2-13 (`5b` — `RoleStemSink`, descrittori acustici e `onset_parity_v1`):**
   - `RoleStemSink` (`engine/audio/role_bus_processor.h:19-49`, `role_bus_processor.cpp:125-133, 273-286`) intercetta gli stem post-bus/post-ducking/post-delay in modalità solo offline (`engine/audio/audio_engine.cpp:892-896`).
   - `AnalysisStatus` (`engine/cacp/analysis_status.h:14-48`), STFT (`stft.h:25-39`), Mel (`mel.h:22-37`), Stereo (`stereo.h:23-39`), Loudness BS.1770-5 (`loudness.h:21-38`, `loudness.cpp:163-253`), Onset (`onset.h:21-60`, `onset.cpp:32-160`) e `onset_parity_v1` (`onset_parity.h:19-58`, `onset_parity.cpp:76-265`) implementano esattamente i parametri e i domini dichiarati in ADR-0006.
3. **D-M2-14 (`5c` — `acao02_v1`):**
   - Implementata in `engine/cacp/acao02_calibration.h:21-165` e `acao02_calibration.cpp:356-625`. Esegue la griglia di 12 combinazioni sul set A (`timbre_bus_probe_v1` + `planner_probe_v1` + `fixture_probe_v1` seed `42`), congela il profilo lessicografico e lo valuta una sola volta sul set B (`fixture_probe_v1` seed `1337`), più la scansione seeds `{1, 2, 7, 42, 99, 1337}`.
4. **D-M2-15 (`5d`, `5e-11`, `5e.3c` — `evaluation_pack_v1/v2/v3` e checker `claims_v0`):**
   - Implementati in `engine/cacp/evaluation_pack.h:28-328`, `evaluation_pack.cpp:458-1865` e `presence_stats.h/.cpp`.
5. **D-M2-16 (`5e.1`..`5e.4a` e `R-01`):**
   - `procedural_v1` (`engine/cacp/procedural_scenario.cpp:66-515`), `diversity_v1` (`engine/cacp/diversity.cpp:141-370`), `release_ab_v1` (`engine/cacp/release_ab.cpp:334-553`), `trama_miss_diagnosis_v1` (`engine/cacp/trama_diagnosis.cpp:272-395`), `evaluation_check_selftest_v1` (`engine/cacp/check_selftest.cpp:210-383`) e `full_m2_suite` (`engine/cacp/m2_suite.cpp:84-252`) sono tutti allineati ai contratti, e i workflow CI `.github/workflows/windows-ci.yml:32` e `.github/workflows/windows-ci-full.yml:51` non invocano `full_m2_suite` su runner condivisi (`5e-02`).

Tuttavia, l'analisi incrociata riga per riga evidenzia tre rilievi tecnici (`AG-ADR-01`, `AG-ADR-02`, `AG-ADR-03`):

#### Rilievo `AG-ADR-01` — `trama_miss_diagnosis_v1`: stato `NOT_RUN` invece di `INCONCLUSIVE` su `r.causal` quando manca il profilo congelato (`OSS-5e.3b-R2-01`)

In `docs/adr/0006-m2-4-inc5-entry-decisions.md:721` la regola di validità di `5e.3b` stabilisce:

```markdown
- **Validità e ripetibilità:** prima di valutare P1–P4 lo scenario verifica il controllo di parità degli eventi fra `2 ms` e `30 ms` per il set B (stessi 474 eventi applicati di `release_ab_v1`, zero drop/cut/overlap per Trama) e ripete l'intera diagnosi due volte verificando che numeri e serie coincidano al bit. Se una condizione di validità non è soddisfatta, lo scenario termina `INCONCLUSIVE` con il motivo e l'esito causale resta `INCONCLUSIVE` (mai `SUPPORTED` su dati non validi; DIF-5e.3b-01, DIF-5e.3b-03).
```

In `engine/cacp/trama_diagnosis.h:76-82`, i campi `p1`, `p2`, `p3`, `p4` e `causal` di `TramaDiagnosisResult` sono inizializzati a `{"NOT_RUN", ""}`:

```cpp
struct PredictionOutcome {
    std::string status{"NOT_RUN"}; // SUPPORTED / NOT_SUPPORTED / INCONCLUSIVE / NOT_RUN
    std::string reason{};
};
```

In `engine/cacp/trama_diagnosis.cpp:277-281`, se il profilo congelato `acao02_v1` non può essere letto, `runOnce` imposta `r.status = "INCONCLUSIVE"` e ritorna immediatamente senza aggiornare `r.causal`:

```cpp
    const auto frozen = readFrozenProfile(outputDir);
    if (!frozen) {
        r.status = "INCONCLUSIVE";
        r.reasons.emplace_back("frozen onset profile not available in acao02_v1_manifest.json");
        return r;
    }
```

Di conseguenza, nel JSON prodotto in quel ramo di errore (`engine/cacp/trama_diagnosis.cpp:355-356`), `"status"` vale `"INCONCLUSIVE"`, ma `"causal_hypothesis": {"id": "M-NORM", "status": "NOT_RUN", ...}` resta `"NOT_RUN"` anziché `"INCONCLUSIVE"` (mentre se falliscono `parityOk` o `repeatIdentical` alle righe `298` e `309`, `evaluateCausal` viene chiamato e imposta `r.causal.status = "INCONCLUSIVE"`). Questa discrepanza minore era stata annotata in `docs/progress.md:2324` (`OSS-5e.3b-R2-01`) e lasciata aperta.

#### Rilievo `AG-ADR-02` — Scrittura sequenziale non transazionale delle tre directory di `evaluation_pack_v1/v2/v3` (`OSS-5e.2-R2-01`)

In `docs/adr/0006-m2-4-inc5-entry-decisions.md:629` si legge:

```markdown
Le directory del pacchetto vengono prima validate tutte (nessun file estraneo, stesso `pack_version`) e solo dopo sostituite: un rifiuto sulla seconda o sulla terza directory lascia intatte le precedenti (DIF-5e.2-01).
```

In `engine/cacp/evaluation_pack.cpp:1768-1785`, la pre-validazione (`ownedPackDirectoryProblem`) controlla effettivamente tutte e tre le directory prima di iniziare a scrivere, ma il ciclo di scrittura successivo (`writePackDirectory` su `blindDir`, `diagDir`, `sealedDir`) cancella e riscrive ciascuna directory in sequenza senza staging/rename atomico né rollback in caso di errore di I/O a metà scrittura:

```cpp
    for (const auto* d : {&blindDir, &diagDir, &sealedDir}) {
        if (const std::string why = ownedPackDirectoryProblem(*d, versionName(version)); !why.empty()) {
            res.status = "FAILED";
            res.failureReason = why;
            return res;
        }
    }
    const std::array<std::pair<const std::string*, const std::map<std::string, std::vector<std::uint8_t>>*>, 3> targets{{
        {&blindDir, &pack.publicBlindFiles},
        {&diagDir, &pack.publicDiagnosticFiles},
        {&sealedDir, &pack.sealedFiles},
    }};
    for (const auto& [dir, files] : targets) {
        if (const std::string why = writePackDirectory(*dir, *files); !why.empty()) {
            res.status = "FAILED";
            res.failureReason = why;
            return res;
        }
    }
```

#### Rilievo `AG-ADR-03` — Prefisso di domain-separation `"evaluation_pack_v1"` cablato in `packSeedCommitment` anche per `evaluation_pack_v2` e `evaluation_pack_v3`

In `docs/adr/0006-m2-4-inc5-entry-decisions.md:463` la decisione `5d-05` definisce l'impegno crittografico sul seed di selezione:

```markdown
| 5d-05 | Seed del pacchetto e impegno pubblico | `--pack-seed <uint64>` obbligatorio per una sessione valida; default `0xCACA0205DULL` marcato `default_public_seed = true` (riproducibile per i test, **mai** qualificabile come `blind`). Il manifest pubblico espone `seed_commitment_sha256 = SHA-256("evaluation_pack_v1|selection_seed|" \|\| decimal(S))`; `truth.json` sigillato contiene `S` in chiaro, così a fine sessione chiunque può verificare che il seed usato coincida con l'impegno preso prima |
```

In `engine/cacp/evaluation_pack.cpp:458-460` e `584`, `packSeedCommitment` usa sempre la costante `kEvaluationPackVersion` (`"evaluation_pack_v1"`, definita in `engine/cacp/evaluation_pack.h:28`), indipendentemente dal fatto che si stia generando `evaluation_pack_v1`, `evaluation_pack_v2` o `evaluation_pack_v3`:

```cpp
std::string packSeedCommitment(std::uint64_t seed) {
    return sha256Hex(std::string_view(std::string(kEvaluationPackVersion) + "|selection_seed|" + std::to_string(seed)));
}
```

Questo comportamento è coerente con il fatto che `5e-11` e `5e.3c` non hanno ridefinito `5d-05` e permette a `full_m2_suite` (`engine/cacp/m2_suite.cpp:219`) di riportare un unico `pack_seed_commitment_sha256` per i tre pacchetti, ma significa che il prefisso di separazione di dominio della stringa hashata non distingue la versione del pacchetto (`v1` vs `v2` vs `v3`).

---

### Domanda 2 — Solidità delle regole fissate prima delle misure (Falsificabilità vs Circolarità)

L'analisi delle quattro famiglie di regole introdotte in Inc-5 mostra una distinzione netta tra **criteri pre-registrati autenticamente falsificabili** (`5e.3`, `5e.3c`) e **diagnosi meccanicistiche post-hoc** (`5e.3b`):

1. **Ipotesi H1, H2 e sotto-ipotesi R2 in `release_ab_v1` (`5e.3`):**
   - In `engine/cacp/release_ab.h:20-28` e `engine/cacp/release_ab.cpp:252-332`, i criteri numerici sono fissati prima dell'esecuzione:
     - **H1** (`evaluateH1`, `release_ab.cpp:252-279`): a `30 ms`, i rilevamenti a meno di $30\text{ ms}$ da un rilascio devono scendere a $\le 20\%$ del valore a `2 ms` su tutti e tre i brani (`set_b`, `planner_probe_v1`, `fixture`).
     - **H2** (`evaluateH2`, `release_ab.cpp:281-304`): a `30 ms`, il recall di Trama sul set B deve essere $\ge 0.90$ e strettamente superiore al valore a `2 ms`.
     - **R2** (`evaluateShortIoi`, `release_ab.cpp:306-332`): i riferimenti di Trama mancati a `2 ms` sul set B devono seguire tutti un IOI di $0.18\text{ s}$ o $0.22\text{ s}$ ($\pm 5\text{ ms}$).
   - **Verifica di non-circolarità:** L'ipotesi **H1 è stata falsificata dai dati** (`NOT_SUPPORTED`): mentre sul set B (`2/19 = 0.105`) e su `planner_probe_v1` (`0/14 = 0.000`) il rapporto è $\le 0.20$, sulla fixture completa vale `54/250 = 0.216 > 0.200` (`docs/adr/0006-m2-4-inc5-entry-decisions.md:677-686`). Il fatto che H1 sia risultata `NOT_SUPPORTED` senza che la soglia `0.20` venisse ritoccata a posteriori a `0.22` dimostra il rigore metodologico del protocollo.

2. **Predizione analitica del numero di gruppi simbolici in `5e.3c` (`evaluation_pack_v3`):**
   - In `docs/adr/0006-m2-4-inc5-entry-decisions.md:796-805` e `docs/research/M2_4_Inc5_Math_Model_Reconciliation.md:94-110`, prima di eseguire `5e.3c` è stata registrata la predizione analitica di `4` gruppi simbolici totali per il seed `1337` e il criterio vincolante `R-01` per autorizzare la sessione alla cieca:

```markdown
- **Criterio per il passo successivo (fissato prima di eseguire 5e.3c):**
  - se con `evaluation_pack_v3` i gruppi sul seed 1337 scendono a **≤ 6** e il pacchetto ha ancora `n_eff,+ ≥ 6,0` e `n_eff,- ≥ 4,0`, R-01 è chiusa per il livello 2 e si può procedere alla sessione alla cieca di 5e-10 usando `evaluation_pack_v3`;
  - se invece `n_eff` scende sotto `6,0` / `4,0` (perché la fixture ha davvero pochi gruppi distinti), il risultato è **informativo, non un difetto del calcolo**: significa che `fixture-m2-4-v1` non ha abbastanza materiale indipendente per superare la soglia `MEASURED` da sola, e la sessione alla cieca su un solo brano potrà essere al più `DESCRIPTIVE_ONLY` finché M2.5 non introdurrà più varietà.
```

   - **Perché la predizione di 4 gruppi è fallita (dando 9 gruppi simbolici e $n_{\text{eff},+} = 5.793 < 6.0$)?**
     L'ispezione di `engine/audio/music_planner.cpp:597-674` (`MusicPlanner::makeFixtureM24V1Phrases`) e `engine/audio/music_planner.cpp:941, 1489-1492` chiarisce esattamente il motivo matematico:
     - La predizione ingenua assumeva che tutte le frasi di una sezione avessero gli stessi input al planner (a meno della maschera di Trama nei bar 0–7).
     - In realtà, `makeFixtureM24V1Phrases` (`engine/audio/music_planner.cpp:620-671`) differenzia le frasi tramite flag strutturali che alterano le note pianificate:
       - `p.isCrossSectionPadPhrase = (phraseIdx == 7 || phraseIdx == 15);` (`line 627`: frasi 7 e 15 emettono un pad aperto cross-section senza `NoteOff` ordinario a fine frase);
       - `p.delayCorpoOnsetByOneBeat = (phraseIdx == 8 || phraseIdx == 16);` (`line 631`: frasi 8 e 16 posticipano l'attacco di Corpo di 1 beat per non sovrapporsi alla coda del pad precedente);
       - `if (phraseIdx == 23) { p.releaseLeadFrames = 4800; }` (`lines 668-670`: la frase finale 23 anticipa i rilasci di 100 ms).
     - Inoltre, in `MusicPlanner::planPhrase` (`engine/audio/music_planner.cpp:941, 1489-1492`), lo stato `lastCorpoVoicing_` persiste dalla frase 0 alla frase 1 (dove passa da `std::nullopt` al voicing scelto nella frase 0, cambiando il punteggio di voice-leading di Corpo tra la frase 0 e le frasi successive).
     - Combinando questi fattori, le 24 frasi si partizionano esattamente in **9 classi di equivalenza simbolica**:
       `{0}`, `{1,2,3}`, `{4,5,6}`, `{7}` in sezione A (4 gruppi); `{8}`, `{9,10,11,12,13,14}`, `{15}` in sezione B (3 gruppi); `{16}`, `{17,18,19,20,21,22}`, `{23}` in sezione A' — ma poiché `{16}` in A' ha lo stesso contenuto simbolico relativo di `{8}` in B e `{17..22}` in A' ha lo stesso contenuto simbolico relativo di `{4,5,6}` in A, i gruppi unici sull'intero brano sono $4 + 3 + 1\text{ (frase 23)} + 1\text{ (fusione A'/A/B)} = 9$ gruppi!
     - Di conseguenza, su `evaluation_pack_v3`, i due gruppi grandi `{9..14}` e `{4..6} ∪ {17..22}` concentrano più finestre di prova, abbassando $n_{\text{eff},+}$ da $7.560$ (`v2`) a $5.793 < 6.0$ (`v3`). L'aver accettato questo esito negativo senza alterare la regola di raggruppamento né abbassare la soglia `6.0` (applicando invece il secondo ramo della regola pre-registrata e rinviando la sessione alla cieca a M2.5) è una prova solida di integrità scientifica.

3. **Predizioni P1–P4 di `trama_miss_diagnosis_v1` (`5e.3b` — Rilievo `AG-ADR-04`):**
   - A differenza di `5e.3` e `5e.3c`, lo scenario `trama_miss_diagnosis_v1` (`engine/cacp/trama_diagnosis.h:16-38`, `trama_diagnosis.cpp:202-270`) è stato costruito **dopo** aver osservato i risultati di `5e.3` sul set B, e cabla esplicitamente la costante empirica osservata in `5e.3`:

```cpp
// engine/cacp/trama_diagnosis.h:27-36
// Epistemic scope: retrospective mechanistic experiment on set B (the hypothesis came from 5e.3 data), not an
// independent confirmation on new data; MEASURED / INCONCLUSIVE, never a gate.
namespace cymatica::cacp {

inline constexpr const char* kTramaDiagnosisVersion = "trama_miss_diagnosis_v1";
inline constexpr const char* kTramaCausalId = "M-NORM";
inline constexpr double kNormaliserRatioMin = 1.2;    // P1: max_2ms / max_30ms
inline constexpr double kCounterfactualRecallMin = 0.95; // P2: recall on 2 ms with the 30 ms normaliser
inline constexpr std::size_t kWeakestAttackRankMax = 12; // P3: the 10 missed attacks lie among the 12 lowest raw peaks
inline constexpr std::size_t kExpectedMissed = 10;    // as measured in 5e.3 (release_ab_v1, set B, 2 ms)
```

   - In `engine/cacp/trama_diagnosis.cpp:225-228`, `evaluateP3` richiede `missed.size() == kExpectedMissed` (`10`). Pertanto, `P1`–`P4` non sono predizioni out-of-sample, bensì verifiche interne di consistenza meccanicistica (in particolare `P2`, che sostituisce il solo scalare di normalizzazione $M_{2\text{ms}}$ con $M_{30\text{ms}}$ sulla stessa curva grezza `2 ms` ottenendo recall $40/40 = 1.000$). Sia ADR-0006 (`docs/adr/0006-m2-4-inc5-entry-decisions.md:722`) sia il commento in `engine/cacp/trama_diagnosis.h:27-28` dichiarano correttamente questa natura retrospettiva.

4. **Auto-verifica del checker `evaluation_check_selftest_v1` (`5e.4a`):**
   - In `engine/cacp/check_selftest.cpp:72-103` e `233-359`, i 6 casi S1–S6 verificano `runEvaluationCheck` confrontando l'output con una seconda implementazione locale (`recompute`) entro `1e-12`. Ciò garantisce l'assenza di regressioni aritmetiche o di serializzazione rispetto alla specifica `presence_stats_v1`, ma (per costruzione) verifica la conformità del codice alla formula scelta, non la bontà statistica esterna della formula stessa (discussa in Domanda 3).

---

### Domanda 3 — Statistica: `presence_stats_v1`, $n_{\text{eff}}$ di Kish, Wilson e indice di Youden inferiore $J_L$

In `engine/cacp/presence_stats.cpp:31-61` e `86-98`, il calcolo statistico per la presenza per ruolo è implementato come segue:

```cpp
// engine/cacp/presence_stats.cpp:36-57
    double sumM = 0.0;
    double sumM2 = 0.0;
    double sumP = 0.0;
    for (const auto& [g, b] : groups) {
        (void)g;
        const auto m = static_cast<double>(b.total);
        sumM += m;
        sumM2 += m * m;
        sumP += static_cast<double>(b.correct) / m;
    }
    c.nEff = sumM2 > 0.0 ? (sumM * sumM) / sumM2 : 0.0;
    c.rate = sumP / static_cast<double>(c.groups);

    if (c.nEff < kMinEffectiveUnits) {
        c.status = "INSUFFICIENT_DATA";
        return c;
    }
    c.status = c.nEff < kMinMeasuredUnits ? "DESCRIPTIVE_ONLY" : "MEASURED";
    const auto iv = wilsonInterval95(c.rate, c.nEff);
    c.low = iv.low;
    c.high = iv.high;
    return c;
```

E per l'indice di Youden inferiore (`engine/cacp/presence_stats.cpp:95-98`):

```cpp
    if (s.positives.status == "MEASURED" && s.negatives.status == "MEASURED") {
        s.youdenLow = s.positives.low + s.negatives.low - 1.0;
        s.aboveTrivialBaseline = s.youdenLow > 0.0;
    }
```

#### 3.1 Fondamento nella letteratura statistica e ipotesi implicite (Rilievo `AG-ADR-05`)

1. **Taglia campionaria effettiva di Kish ($n_{\text{eff}}$) vs *Design Effect* per dati binari clusterizzati (Kish 1965; Rao & Scott 1992; Eldridge et al. 2009):**
   - Nella teoria del campionamento per cluster (Kish 1965, §8.2; Eldridge, Ukoumunne & Carlin 2009), se il gruppo $g \in \{1,\dots,G\}$ ha dimensione $m_g$ e correlazione intra-cluster (ICC) pari a $\rho \in [0, 1]$, il *design effect* per uno stimatore che assegna pesi $w_g$ ai cluster è:
     $$\text{DEFF}(\rho) = \frac{\sum_{g=1}^G w_g^2 \, \text{Var}(\hat{p}_g)}{\text{Var}_{\text{SRS}}(\hat{p})}$$
   - In `presence_stats_v1`, lo stimatore del tasso medio per gruppo `c.rate` assegna peso uguale $w_g = 1/G$ a ciascun gruppo:
     $$\hat{\theta} = \frac{1}{G} \sum_{g=1}^G \hat{p}_g, \qquad \hat{p}_g = \frac{k_g}{m_g}$$
     Se le osservazioni dentro lo stesso gruppo musicale sono **repliche deterministiche** (correlazione intra-cluster $\rho = 1$, caso che Rao & Scott 1992, p. 578 chiamano *complete intracluster correlation*), allora $\text{Var}(\hat{p}_g) = \theta(1-\theta)$ indipendentemente da $m_g$, e la varianza esatta di $\hat{\theta}$ con pesi uguali $1/G$ è:
     $$\text{Var}_{\rho=1}\!\left(\frac{1}{G}\sum_{g=1}^G \hat{p}_g\right) = \frac{\theta(1-\theta)}{G}$$
     equivalentemente a una taglia campionaria effettiva pari a **$G$** (il numero di gruppi indipendenti), **non** a $n_{\text{eff}} = (\sum m_g)^2 / \sum m_g^2$!
   - Perché compare allora la formula di Kish $n_{\text{eff}} = (\sum m_g)^2 / \sum m_g^2$?
     La formula di Kish $n_{\text{eff}} = (\sum m_g)^2 / \sum m_g^2 \le G$ è la taglia campionaria effettiva dello stimatore **pesato per osservazione** $\hat{\theta}_{\text{obs}} = (\sum k_g)/(\sum m_g) = \sum (m_g / N)\hat{p}_g$ sotto $\rho = 1$, perché per quello stimatore la varianza sotto $\rho = 1$ è:
     $$\text{Var}_{\rho=1}\!\left(\sum_{g=1}^G \frac{m_g}{N} \hat{p}_g\right) = \theta(1-\theta) \frac{\sum_{g=1}^G m_g^2}{\left(\sum_{g=1}^G m_g\right)^2} = \frac{\theta(1-\theta)}{n_{\text{eff}}}$$
   - **Conseguenza matematica importante (ragionamento analitico verificato):** In `presence_stats_v1` (`engine/cacp/presence_stats.cpp:44-45`), il centro dell'intervallo è la media **non pesata** dei gruppi $\hat{\theta} = \frac{1}{G}\sum \hat{p}_g$ (la cui varianza sotto $\rho = 1$ è $\theta(1-\theta)/G$), ma l'ampiezza dell'intervallo di Wilson è calcolata usando $n_{\text{eff}} = (\sum m_g)^2 / \sum m_g^2 \le G$ (per la disuguaglianza di Cauchy-Schwarz $(\sum m_g)^2 \le G \sum m_g^2$). Poiché $n_{\text{eff}} \le G$, sostituire $G$ con $n_{\text{eff}}$ nell'intervallo di Wilson centrato su $\frac{1}{G}\sum \hat{p}_g$ **allarga** artificialmente l'intervallo di confidenza ogniqualvolta i gruppi hanno taglie $m_g$ disuguali!
   - Questo ibrido (media per gruppo $1/G$ combinata con $n_{\text{eff}}$ di Kish) è quindi **strettamente conservativo** sotto $\rho = 1$ (produce un limite inferiore di Wilson più basso di quello che spetterebbe a $\frac{1}{G}\sum \hat{p}_g$ con $G$ gradi di libertà), ma ha l'effetto paradossale osservato in `5e.3c`: aggiungere repliche a un gruppo già esistente (aumentando $m_g$ per i gruppi grandi) **riduce** $n_{\text{eff}}$ al di sotto di $G$, anche se lo stimatore $\frac{1}{G}\sum \hat{p}_g$ ha già peso $1/G$ e non sovrappesa i gruppi grandi!

2. **Dipendenza cross-classe sugli stessi gruppi musicali e copertura congiunta di $J_L = L_{\text{se}} + L_{\text{sp}} - 1$ (Obuchowski 1997; Cameron, Gelbach & Miller 2011):**
   - In `engine/cacp/presence_stats.cpp:86-98`, `positives` e `negatives` sono valutati separatamente, ciascuno con un intervallo di Wilson al 95% ($z = 1.959963984540054$, coda inferiore $\alpha_1 = 0.025$), e poi sommati in `s.youdenLow = s.positives.low + s.negatives.low - 1.0`.
   - Vi sono due aspetti statistici distinti:
     - **Molteplicità (Bonferroni):** Se $L_{\text{se}}$ e $L_{\text{sp}}$ hanno ciascuno copertura nominale inferiore $1 - \alpha_1 = 97.5\%$, per la disuguaglianza di Boole la probabilità che $J = \text{se} + \text{sp} - 1 \ge L_{\text{se}} + L_{\text{sp}} - 1$ è garantita $\ge 1 - 2\alpha_1 = 95\%$ (unilaterale), ma un intervallo bilaterale al 95% su $J$ richiederebbe $\alpha_1 = 0.0125$ ($z_{0.9875} \approx 2.2414$) o la varianza della somma.
     - **Clusterizzazione condivisa tra positivi e negativi (`CL-MM-10`):** In `evaluation_pack_v2/v3`, ogni finestra di prova appartiene a un gruppo musicale $g$ e genera osservazioni sia per i ruoli presenti ($m_{g,+} > 0$) sia per i ruoli assenti ($m_{g,-} > 0$). Come dimostrato da Obuchowski (1997, eq. 2–4) per curve ROC clusterizzate, la varianza di una combinazione lineare di sensibilità e specificità $\hat{J} = \hat{\theta}_+ + \hat{\theta}_- - 1$ sugli stessi cluster è:
       $$\text{Var}(\hat{J}) = \text{Var}(\hat{\theta}_+) + \text{Var}(\hat{\theta}_-) + 2\,\text{Cov}(\hat{\theta}_+, \hat{\theta}_-)$$
       Se un valutatore adotta una soglia decisionale più conservativa o più permissiva su un gruppo $g$ (es. a causa della densità spettrale di quel gruppo), l'accuratezza sui positivi $\hat{p}_{g,+}$ e sui negativi $\hat{p}_{g,-}$ dello stesso gruppo possono essere negativamente o positivamente correlate.

3. **Alternative per piccoli campioni ($4 \le G \le 9$) in M2.5:**
   - Quando il numero di gruppi distinti è piccolo ($G = 9$ in `v3`), i metodi asintotici (anche con correzione di continuità o Wilson) soffrono della discretezza del supporto. Per M2.5, un **test di permutazione/randomizzazione esatto a livello di gruppo** (permutando le etichette di presenza/assenza all'interno di ciascun gruppo o tra gruppi compatibili) fornisce un p-value esatto in campioni finiti senza assumere la normalità asintotica di $(\hat{\theta}, n_{\text{eff}})$.

---

### Domanda 4 — Misura: Rilevamento degli Onset, Normalizzazione Globale (`M-NORM`) e Alternative Causali

In `engine/cacp/onset.cpp:32-65`, la funzione di rilevamento degli onset (`spectralFlux`) calcola il flusso spettrale half-wave rectified su $B = 40$ bande mel logaritmiche (`std::log(1.0 + 1000.0 * E)`) e poi divide l'intera serie temporale per il suo massimo globale sul brano:

```cpp
// engine/cacp/onset.cpp:54-60
    const double maxFlux = *std::max_element(flux.begin(), flux.end());
    if (maxFlux > 0.0) {
        for (double& v : flux) {
            v /= maxFlux;
        }
    }
```

Successivamente, `pickOnsetPeaks` (`engine/cacp/onset.cpp:89-130`) applica la soglia adattiva locale alla serie normalizzata $\tilde{F}(n) = F(n) / M$ (dove $M = \max_k F(k)$):

```cpp
// engine/cacp/onset.cpp:104-110
        double mean = 0.0;
        for (std::size_t k = wStart; k < wEnd; ++k) {
            mean += flux.values[k];
        }
        mean /= static_cast<double>(wEnd - wStart);
        if (v < mean + cfg.thresholdDelta) continue;
```

#### 4.1 Analisi matematica di `M-NORM` e confronto con la letteratura MIR (Rilievo `AG-ADR-07`)

Moltiplicando la disuguaglianza di picco $\tilde{F}(n) \ge \bar{\tilde{F}}(n) + \delta$ per il normalizzatore globale $M = \max_k F(k) > 0$, la condizione di rilevamento nel dominio del flusso grezzo $F(n)$ diventa:
$$F(n) \ge \bar{F}(n) + \delta \cdot M$$
Questa equazione rivela immediatamente il difetto strutturale della normalizzazione globale per il massimo (Bello et al. 2005, §III-D; Dixon 2006, §2.4):
- L'offset additivo $\delta$ (`thresholdDelta = 0.08` nel profilo congelato) non è una soglia assoluta né locale, ma una **frazione fissa ($\delta = 8\%$) del singolo transiente più violento dell'intero brano** ($M$).
- Nello stem di Trama sul set B (`trama_miss_diagnosis_v1`), il rilascio a 2 ms al tempo $t = 92.213\text{ s}$ produce una discontinuità spettrale larga banda con flusso grezzo $M_{2\text{ms}} = 50.858$, mentre con rilascio di 30 ms il massimo globale scende a un attacco reale con $M_{30\text{ms}} = 30.233$ (rapporto $M_{2\text{ms}} / M_{30\text{ms}} = 1.682$).
- Di conseguenza, il termine additivo $\delta \cdot M$ nella soglia effettiva sale da $0.08 \times 30.233 = 2.419$ (a 30 ms) a $0.08 \times 50.858 = 4.069$ (a 2 ms), oscurando esattamente i 10 attacchi ravvicinati di Trama il cui eccesso locale $F(n) - \bar{F}(n)$ cade nell'intervallo $[2.419,\, 4.069)$!

**Stato dell'arte nella letteratura scientifica per M2.5:**
1. **SuperFlux (Böck & Widmer 2013, DAFx-13):** Introduce un filtro di massimo lungo l'asse delle frequenze per sopprimere modulazioni/vibrato e utilizza una differenza temporale a lag $\mu > 1$ frame con soglia locale basata su media e mediana mobile senza che un singolo outlier di coda accechi i passaggi morbidi.
2. **Adaptive Whitening (Stowell & Plumbley 2007, ICMC):** Normalizza ricorsivamente ciascun bin/banda spettrale rispetto a un profilo di picco esponenziale causale $P_b(n) = \max(|X_b(n)|, \alpha P_b(n-1), f_{\text{floor}})$, pareggiando la dinamica tra attacchi forti e deboli prima del calcolo del flusso.
3. **PCEN — Per-Channel Energy Normalization (Lostanlen et al. 2019, *IEEE Signal Processing Letters*):** Sostituisce la compressione logaritmica statica $\log(1 + \gamma E)$ e la normalizzazione globale con una normalizzazione AGC causale per banda:
   $$\text{PCEN}(t, f) = \left(\frac{E(t, f)}{(\varepsilon + M(t, f))^\alpha} + \delta\right)^r - \delta^r$$
   dove $M(t, f)$ è uno smoother Iir causale del primo ordine. Lostanlen et al. (2019) dimostrano analiticamente che PCEN rende l'ampiezza dei transienti d'attacco invariante rispetto al livello stazionario e immune da outlier distanti nel tempo.
4. **Normalizzazione quantile robusta o locale causale:** Se si mantiene l'architettura attuale di `spectralFlux`, sostituire $M = \max_k F(k)$ con un quantile robusto (es. $Q_{0.95}(F)$ o $Q_{0.98}(F)$ sui frame attivi) oppure con un normalizzatore locale causale elimina la sensibilità al singolo picco patologico di rilascio.

**Valutazione della scelta di ADR-0006:** Congelare il profilo in `5c` (`D-M2-14`) e **non** modificarlo dopo aver visto il set B in `5e.1`/`5e.3` è stata la scelta metodologicamente ineccepibile per evitare *data snooping* (cherry-picking del detector sul test set). L'evoluzione a un detector con normalizzazione robusta/locale appartiene correttamente a M2.5.

---

### Domanda 5 — Rilascio: De-click Lineare di 2 ms vs Rilascio Musicale e Mascheramento Psicoacustico

In `engine/audio/voice_allocator.h:35-38` e `engine/audio/voice_allocator.cpp:173-189, 250-264`, il rilascio di produzione per ogni `NoteOff` ordinario (quando `noteReleaseOverrideFrames_ == 0`) usa la stessa rampa lineare di 2 ms (96 campioni a 48 kHz) impiegata per i voice steal e i tagli di sicurezza:

```cpp
// engine/audio/voice_allocator.h:35-38
    // 2 ms linear de-click window (Spec §16.8, ADR-0003 D-M2-03):
    // floor(0.002 * sampleRate) => 96 frames @ 48 kHz, 88 frames @ 44.1 kHz
    static constexpr double kDeclickDurationSeconds = 0.002;
```

```cpp
// engine/audio/voice_allocator.cpp:181-188
            const std::uint32_t relFrames =
                noteReleaseOverrideFrames_ > 0 ? noteReleaseOverrideFrames_ : declickFrames_;
            v.state = VoiceState::Release;
            v.releaseFramesRemaining = relFrames;
            v.releaseTotalFrames = relFrames;
            v.releaseStartGain = v.envelopeGain;
```

#### 5.1 Analisi spettrale e psicoacustica (Harris 1978; Puckette 2007; Välimäki & Huovilainen 2006) — Rilievo `AG-ADR-08`

1. **Allargamento spettrale (*spectral splatter*) di una rampa lineare di $T_r = 2\text{ ms}$ (Harris 1978):**
   - Moltiplicare una portante stazionaria $x(t) = A \cos(\omega_0 t + \phi)$ per una rampa di spegnimento lineare di durata $T_r = 2\text{ ms}$ equivale, per la derivata prima dell'inviluppo $w'(t)$, a un impulso rettangolare di durata $T_r$ e ampiezza $-1/T_r$.
   - La trasformata di Fourier della transizione ha inviluppo asintotico che decade come $O(\omega^{-2})$ ($-12\text{ dB/ottava}$), con il primo lobo principale largo $\Delta f = 1 / T_r = 500\text{ Hz}$ e lobi secondari che si estendono su diversi kilohertz attorno a ciascuna armonica attiva:
     $$|W(\Delta\omega)| \propto \frac{|\sin(\Delta\omega \, T_r / 2)|}{(\Delta\omega \, T_r / 2)^2}$$
   - Poiché `spectralFlux` (`engine/cacp/onset.cpp:39-51`) rettifica a semionda ($\max(0, L_b(n) - L_b(n-1))$) e somma su tutte le 40 bande mel, anche se l'energia totale della voce sta scendendo a zero, **lo sparpagliamento di energia verso le bande mel adiacenti** dovuto alla convoluzione con $W(\Delta\omega)$ per $T_r = 2\text{ ms}$ crea incrementi positivi $L_b(n) > L_b(n-1)$ nelle bande vicine alla fondamentale e alle armoniche, generando un picco spurio di flusso spettrale al frame di `NoteOff`!
   - Con $T_r = 30\text{ ms}$, la larghezza del lobo principale si restringe di un fattore $15\times$ ($\Delta f = 1 / 0.030\text{ s} \approx 33.3\text{ Hz}$, inferiore alla larghezza di una singola banda mel), riducendo la potenza dei lobi laterali a una data distanza in frequenza di $20\log_{10}(15^2) \approx 47\text{ dB}$.

2. **Asimmetria tra mascheramento in avanti (*forward masking*) e all'indietro (*backward masking*):**
   - All'attacco di una nota (`NoteOn`), la rampa di attacco è immediatamente seguita dal corpo stazionario della nota stessa, che esercita un forte mascheramento simultaneo e post-mascheramento sul transiente iniziale.
   - Al rilascio (`NoteOff`), invece, la nota cessa e lascia il posto al silenzio (o a una coda di delay più debole): non c'è energia successiva nello stesso canale spettrale che mascheri il transiente di spegnimento! Per questo nella sintesi digitale (Puckette 2007, cap. 4; Välimäki & Huovilainen 2006) una rampa di 1–5 ms è usata esclusivamente come **muting / switch-and-ramp di emergenza** (furto di voce, cambio patch, stop immediato), mentre i `NoteOff` musicali ordinari richiedono uno stadio di Release esponenziale o raised-cosine di 15–200 ms dipendente dal ruolo timbrico.

3. **Perché in `5e.3` H1 è fallita sulla fixture per Corpo (`51/165` a 30 ms contro `36/141` a 2 ms)?**
   - In `release_ab_v1` (`docs/adr/0006-m2-4-inc5-entry-decisions.md:677-686`), passando da 2 ms a 30 ms i rilevamenti vicini a un rilascio crollano quasi a zero per Trama (`12/34 -> 1/61`), Vettore (`7/61 -> 1/69`) e Pulso (`0/14 -> 1/15`), ma salgono per Corpo (`36/141 -> 51/165`).
   - La ragione sta nell'accoppiamento di Corpo con il compressore/ducker guidato da Pulso (`RoleBusProcessor`) e nella sovrapposizione con i nuovi attacchi: allungando la coda di Corpo a 30 ms fino a ridosso del beat successivo (`releaseOverlaps = 160` per Corpo, `docs/progress.md:2218`), la coda di rilascio rimane udibile durante il recupero di guadagno del bus ducking e vicino al successivo attacco di triade. Ciò conferma la conclusione di ADR-0006 (`5e.3`): in M2.5 il rilascio musicale deve essere **specifico per ruolo** (es. 20–30 ms esponenziale/raised-cosine per gli arpeggi di Trama e le melodie di Vettore, coordinato con il gate/inviluppo ritmico per Pulso e Corpo).

---

### Domanda 6 — Varietà Procedurale: Teorema D, Dominio dei Candidati e Architettura per M2.5

In `docs/adr/0006-m2-4-inc5-entry-decisions.md:742-776` e `docs/research/M2_4_Inc5_Math_Model_Reconciliation.md:53-62`, le due indagini matematiche indipendenti hanno dimostrato il **Teorema D**: su `fixture-m2-4-v1`, la funzione $\text{seed} \mapsto \text{musicalContentFingerprint}(\text{plan}(\text{seed}))$ è **costante su tutto $\mathbb{Z}_{2^{64}}$** (1 fingerprint unico su 6 seed in `acao02_v1` e 1 su 64 seed in `diversity_v1`).

#### 6.1 Verifica sul codice di `MusicPlanner` e confronto con la letteratura (Rilievo `AG-ADR-09`)

L'ispezione di `engine/audio/music_planner.cpp` conferma la catena causale esatta del Teorema D:
1. **Assenza di stato stocastico negli argomenti e nei generatori di candidati:**
   - I ruoli `Pulso` e `Trama` generano esattamente 1 candidato deterministico per frase (`music_planner.cpp:890-935, 985-1065`).
   - `Corpo` genera al più 3 inversioni (`Root`, `FirstInversion`, `SecondInversion`, `music_planner.cpp:945-983`), ridotte a 1 quando `lastCorpoVoicing_` impone il vincolo hard di voice-leading (`maxSemitoneStep <= 4`).
   - `Vettore` genera 8 trasformazioni deterministiche del motivo base (`Identity`, `Transpose(+2)`, `Transpose(-2)`, `Invert`, `Retrograde`, `Augment`, `Diminish`, `DensityThin`, `music_planner.cpp:1068-1155`).
2. **Funzione di punteggio intera senza pareggi sul massimo:**
   - I punteggi `scoreCorpoCandidate` e `scoreVettoreCandidate` sono combinazioni lineari intere di pesi fissi (`kPlannerScoringProfileV1`). Sulle frasi di `fixture-m2-4-v1`, vi è sempre un unico candidato con punteggio strettamente massimo (`Identity` per Vettore; un'unica inversione per Corpo).
3. **Ruolo confinato del PRNG (` deriveSubstreamKey `):**
   - Il `seed` entra solo in `deriveSubstreamKey` per calcolare `tieBreakKey` (usato esclusivamente a parità esatta di punteggio intero) e `decisionHash` (campo di provenienza escluso da `musicalContentFingerprint`). Poiché non c'è mai parità sul massimo, il vincitore non dipende mai da `seed`.

**Confronto con la letteratura sulla musica procedurale e adattiva per videogiochi (Collins 2009; Plans & Morelli 2012; Hutchings & McCormack 2020):**
- Come documentato da Collins (2009, pp. 7–11), nei sistemi di musica procedurale *transformational* e *generative* per videogiochi, la variazione non può provenire da un semplice tie-breaker casuale su una funzione obiettivo statica unimodale: se una funzione di costo musicale statica viene massimizzata ad ogni battuta sugli stessi candidati, il sistema collassa inevitabilmente in un ciclo limite periodico di periodo 1 (*loop lock*).
- Plans & Morelli (2012, §III) e Hutchings & McCormack (2020, §III-B) mostrano che per ottenere varietà controllata mantenendo coerenza motivica e determinismo da seed occorrono tre meccanismi strutturali (che in CYMATICA corrispondono ai requisiti di M2.5 / F5):
  1. **Penalità di ripetizione dipendente dalla storia (*tabu / recency penalty* o stato markoviano di ordine $k$):** Lo stato del planner lungo la sezione deve registrare la sequenza delle ultime $k$ trasformazioni/voicing scelti (es. `recentTransforms_[k]`) e applicare un termine di *novelty/satiation* dinamico nello scoring (`kPlannerScoringProfile`), così che la ripetizione consecutiva di `Identity` riduca il suo punteggio nelle frasi successive.
  2. **Campionamento stocastico controllato (es. *Boltzmann / softmax selection* o *$\varepsilon$-top-$k$* deterministico su intero fisso) guidato dallo stream isolato del ruolo:** Invece di usare il PRNG solo per i pareggi esatti (`scoreA == scoreB`), estrarre dal substream crittografico del ruolo (già isolato per `StreamId` secondo ADR-0003 D-M2-02) una perturbazione limitata di esplorazione tra i candidati che superano tutti i vincoli hard di armonia e budget di voci.
  3. **Parametrizzazione macro-strutturale guidata dal direttore musicale (F5):** Variare densità ritmica, registro, estensioni armoniche e maschera dei ruoli in funzione della traiettoria di tensione della sezione.

---

### Domanda 7 — Classificazione tra Gate, Misure e Lavoro Rinviato a M2.5

La separazione epistemica e operativa definita in ADR-0006 (`D-M2-14`, `D-M2-15`, `D-M2-16`, `5e-01`..`5e-11`, `R-01`) rispetta pienamente la gerarchia normativa del progetto:

1. **Gate vincolanti (`PASS`/`FAIL`):**
   - Coprono esclusivamente invarianti matematici, strutturali e realtime già dimostrati: riconciliazione per identità della catena O3 ($E_{\text{exp}} = E_{\text{sent}} \uplus E_{\text{canceled}}$, $E_{\text{sent}} = E_{\text{cons}}$), determinismo bit-exact, golden di trace, piano e PCM (`kFixtureProbeV1GoldenPcmHash`), integrità numerica PCM, assenza di clipping e ceiling campione per campione ($|x| \le 0.89125$), isolamento delle directory e auto-verifica del checker (`evaluation_check_selftest_v1`).
2. **Misure (`MEASURED` / `INCONCLUSIVE` / `DESCRIPTIVE_ONLY` / `INSUFFICIENT_DATA`, mai gate silenziosi):**
   - Includono il true peak BS.1770-5 (che su `fixture_probe_v1` misura $-0.871\text{ dBTP} > -1.0\text{ dBTP}$ ed è esposto come finding trasparente in `procedural_v1`, mai nascosto né trasformato in rottura opaca della CI), l'accuratezza dell'onset detector col profilo congelato (`acao02_v1`), l'esperimento A/B sul rilascio (`release_ab_v1`), la diagnosi causale retrospettiva (`trama_miss_diagnosis_v1`) e le metriche di diversità (`diversity_v1`).
   - È particolarmente rigorosa la regola fail-closed (`DIF-5c-02`, `DIF-5e.1-02`, `DIF-5e.4a-01`): una misura non diventa un gate sulla qualità artistica, ma **deve** fallire se l'infrastruttura di cattura è corrotta, se un'analisi di ruolo non è `VALID`, o se il manifest non viene scritto su disco.
3. **Rinvii espliciti a M2.5:**
   - La decisione del titolare del 2026-11-03 (`docs/adr/0006-m2-4-inc5-entry-decisions.md:825-830`) di rinviare la sessione di valutazione alla cieca (`5e-10`) a M2.5 dopo che `5e.3c` ha dimostrato che `fixture-m2-4-v1` possiede solo 9 gruppi simbolici ($n_{\text{eff},+} = 5.793 < 6.0$) evita di eseguire un protocollo umano/multimodale sapendo in anticipo che il brano in esame non può raggiungere la soglia statistica `MEASURED`.

---

## 4. Tabella dei Rilievi (`AG-ADR-NN`)

Tutti gli estratti riportano il percorso relativo al repository e il numero di riga verificato tramite `Select-String` e lettura diretta.

| ID | Decisione ADR-0006 | Affermazione / Requisito | Evidenza nel repository (`percorso/file:riga` ed estratto esatto) | Stato | Severità |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **AG-ADR-01** | `D-M2-16` (`5e.3b`, `trama_miss_diagnosis_v1`) | Se una condizione di validità non è soddisfatta in `trama_miss_diagnosis_v1`, lo scenario termina `INCONCLUSIVE` e anche l'esito causale resta `INCONCLUSIVE`. | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:721`):<br>`Se una condizione di validità non è soddisfatta, lo scenario termina INCONCLUSIVE con il motivo e l'esito causale resta INCONCLUSIVE (mai SUPPORTED su dati non validi; DIF-5e.3b-01, DIF-5e.3b-03).`<br>**Codice** (`engine/cacp/trama_diagnosis.h:76-82`):<br>`struct PredictionOutcome { std::string status{"NOT_RUN"}; std::string reason{}; };`<br>**Codice** (`engine/cacp/trama_diagnosis.cpp:277-281`):<br>`const auto frozen = readFrozenProfile(outputDir);`<br>`if (!frozen) {`<br>`    r.status = "INCONCLUSIVE";`<br>`    r.reasons.emplace_back("frozen onset profile not available in acao02_v1_manifest.json");`<br>`    return r;`<br>`}`<br>Nel ramo `!frozen`, `runOnce` ritorna prima di chiamare `evaluateCausal`, lasciando `r.causal.status == "NOT_RUN"` anziché `"INCONCLUSIVE"` (già annotato in `docs/progress.md:2324` come `OSS-5e.3b-R2-01`). | **divergenza** | Bassa |
| **AG-ADR-02** | `D-M2-15` / `D-M2-16` (`5e.2`, `DIF-5e.2-01`) | Le tre directory del pacchetto (`public_blind`, `public_diagnostic`, `sealed`) sono pre-validate prima della sostituzione, ma la scrittura successiva delle tre directory non ha rollback in caso di errore I/O a metà del secondo o terzo target. | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:629`):<br>`Le directory del pacchetto vengono prima validate tutte (nessun file estraneo, stesso pack_version) e solo dopo sostituite: un rifiuto sulla seconda o sulla terza directory lascia intatte le precedenti (DIF-5e.2-01).`<br>**Codice** (`engine/cacp/evaluation_pack.cpp:1780-1785`):<br>`for (const auto& [dir, files] : targets) {`<br>`    if (const std::string why = writePackDirectory(*dir, *files); !why.empty()) {`<br>`        res.status = "FAILED";`<br>`        res.failureReason = why;`<br>`        return res;`<br>`    }`<br>`}`<br>Un rifiuto di pre-validazione lascia intatte tutte le directory, ma un errore di scrittura su disco durante la 2ª o 3ª chiamata a `writePackDirectory` lascia la 1ª directory già sovrascritta (annotato in `docs/progress.md:2084` come `OSS-5e.2-R2-01`). | **confermata** (limite noto) | Bassa |
| **AG-ADR-03** | `D-M2-15` (`5d-05`, `5e-11`, `5e.3c`) | La funzione `packSeedCommitment(seed)` calcola `SHA-256("evaluation_pack_v1\|selection_seed\|" \|\| decimal(S))` usando il prefisso `"evaluation_pack_v1"` anche per i pacchetti `evaluation_pack_v2` ed `evaluation_pack_v3`. | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:463`):<br>`Il manifest pubblico espone seed_commitment_sha256 = SHA-256("evaluation_pack_v1\|selection_seed\|" \|\| decimal(S));`<br>**Codice** (`engine/cacp/evaluation_pack.h:28`):<br>`inline constexpr const char* kEvaluationPackVersion = "evaluation_pack_v1";`<br>**Codice** (`engine/cacp/evaluation_pack.cpp:458-460`):<br>`std::string packSeedCommitment(std::uint64_t seed) {`<br>`    return sha256Hex(std::string_view(std::string(kEvaluationPackVersion) + "\|selection_seed\|" + std::to_string(seed)));`<br>`}` | **confermata** (dettaglio tecnico) | Informativa |
| **AG-ADR-04** | `D-M2-16` (`5e.3`, `5e.3b`, `5e.3c`) | Distinzione epistemica tra regole pre-registrate falsificabili (`5e.3` H1/H2, `5e.3c` predizione gruppi) e diagnosi meccanicistica retrospettiva (`5e.3b` P1–P4 con `kExpectedMissed = 10`). | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:722`):<br>`Ambito epistemico: 5e.3b è un esperimento meccanicistico retrospettivo sul set B (l'ipotesi nasce dai dati di 5e.3), non una conferma indipendente su materiale nuovo.`<br>**Codice** (`engine/cacp/trama_diagnosis.h:36`):<br>`inline constexpr std::size_t kExpectedMissed = 10;    // as measured in 5e.3 (release_ab_v1, set B, 2 ms)`<br>**Codice** (`engine/cacp/trama_diagnosis.cpp:225-228`):<br>`if (missed.size() != kExpectedMissed) {`<br>`    return {"NOT_SUPPORTED", "missed Trama references = " + std::to_string(missed.size()) + ", expected " + std::to_string(kExpectedMissed)};`<br>`}` | **confermata** | Informativa |
| **AG-ADR-05** | `D-M2-15` / `D-M2-16` (`5e-09`, `presence_stats_v1`) | `presence_stats_v1` centra l'intervallo di Wilson sulla media non pesata dei gruppi $\hat{\theta} = \frac{1}{G}\sum \hat{p}_g$ usando $n_{\text{eff}} = (\sum m_g)^2 / \sum m_g^2 \le G$, e calcola $J_L = L_{\text{se}} + L_{\text{sp}} - 1$ sommando due limiti inferiori univariati al 95% stimati sugli stessi gruppi musicali senza correzione di molteplicità o covarianza cross-classe. | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:576`):<br>`Unità statistica: il gruppo musicale di frasi (g = 1..G), non la finestra. ... Taglia effettiva per classe (Kish): n_eff = (Σ_g m_g)² / Σ_g m_g² (n_eff ≤ G; uguale a G quando i gruppi hanno la stessa taglia). ... Baseline banale (5d-11): superata solo se sia i positivi sia i negativi sono MEASURED e J_L = L_se + L_sp − 1 > 0.`<br>**Codice** (`engine/cacp/presence_stats.cpp:44-45, 54, 97`):<br>`c.nEff = sumM2 > 0.0 ? (sumM * sumM) / sumM2 : 0.0;`<br>`c.rate = sumP / static_cast<double>(c.groups);`<br>`const auto iv = wilsonInterval95(c.rate, c.nEff);`<br>`s.youdenLow = s.positives.low + s.negatives.low - 1.0;` | **da rivedere** (in M2.5 prima della sessione alla cieca) | Media |
| **AG-ADR-06** | `D-M2-16` (`5e.3c`, `R-01`, `symbolicPhraseGroupIds`) | La partizione in 9 gruppi simbolici di `fixture-m2-4-v1` (contro i 4 predetti prima dell'esecuzione) è causata dai flag strutturali per-frase in `makeFixtureM24V1Phrases` (`isCrossSectionPadPhrase`, `delayCorpoOnsetByOneBeat`, `releaseLeadFrames`, `maskTrama`) e dallo stato cross-frase `lastCorpoVoicing_`. | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:815`):<br>`I gruppi non scendono ai 4 attesi perché la predizione prima dell'esecuzione non contava le differenze strutturali deliberate della fixture: frase 0 senza stato precedente di voice-leading; frasi 7 e 15 con pad aperto fra sezioni; frasi 8 e 16 con ritardo di un battito su Corpo; frase 23 con margine finale di rilascio.`<br>**Codice** (`engine/audio/music_planner.cpp:627, 631, 668, 941`):<br>`p.isCrossSectionPadPhrase = (phraseIdx == 7 \|\| phraseIdx == 15);`<br>`p.delayCorpoOnsetByOneBeat = (phraseIdx == 8 \|\| phraseIdx == 16);`<br>`if (phraseIdx == 23) { p.releaseLeadFrames = 4800; }`<br>`std::optional<TriadVoicing> prevVoicing = lastCorpoVoicing_;` | **confermata** | Informativa |
| **AG-ADR-07** | `D-M2-13` / `D-M2-14` / `D-M2-16` (`5b`, `5c`, `5e.3b`) | La normalizzazione globale $\tilde{F}(n) = F(n) / \max_k F(k)$ in `spectralFlux` accoppia la soglia locale $\bar{F}(n) + \delta \cdot \max_k F(k)$ al singolo transiente più intenso dell'intero brano (`M-NORM`). | **Codice** (`engine/cacp/onset.cpp:54-60`):<br>`const double maxFlux = *std::max_element(flux.begin(), flux.end());`<br>`if (maxFlux > 0.0) {`<br>`    for (double& v : flux) {`<br>`        v /= maxFlux;`<br>`    }`<br>`}`<br>**Codice** (`engine/cacp/onset.cpp:109`):<br>`if (v < mean + cfg.thresholdDelta) continue;` | **proposta per M2.5** (congelato in Inc-5 per `D-M2-14`) | Media |
| **AG-ADR-08** | `D-M2-16` (`5e-08`, `5e.3`, `VoiceAllocator`) | L'uso della rampa lineare di de-click da 2 ms (`kDeclickDurationSeconds = 0.002`) per i `NoteOff` musicali ordinari produce allargamento spettrale $O(\omega^{-2})$ non mascherato a fine nota; l'allungamento uniforme a 30 ms risolve Trama (`H2: SUPPORTED`) ma interagisce con il ducking/overlap di Corpo (`H1: NOT_SUPPORTED`). | **Codice** (`engine/audio/voice_allocator.h:37`):<br>`static constexpr double kDeclickDurationSeconds = 0.002;`<br>**Codice** (`engine/audio/voice_allocator.cpp:181-183`):<br>`const std::uint32_t relFrames =`<br>`    noteReleaseOverrideFrames_ > 0 ? noteReleaseOverrideFrames_ : declickFrames_;`<br>**Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:679-684`):<br>`H1 — NOT_SUPPORTED ... Su Corpo i rilevamenti vicini a un rilascio salgono da 36/141 a 51/165 e gli overlap di rilascio salgono a 160.` | **proposta per M2.5** | Media |
| **AG-ADR-09** | `D-M2-16` (`5e-04`, `5e.1`, Teorema D) | Su `fixture-m2-4-v1` la diversità rispetto al seed è identicamente nulla (`1` fingerprint unico su 64 seed) perché i candidati e i punteggi interi sono deterministici e privi di stato storico o esplorazione stocastica sopra-soglia. | **Documento** (`docs/adr/0006-m2-4-inc5-entry-decisions.md:751`):<br>`Per ogni seed in {0..63}, musicalContentFingerprint(fixture-m2-4-v1, seed) = 14850868393822185080 (1 piano musicale unico su 64 seed, distanze M2, M3, M4 identicamente 0).`<br>**Codice** (`tests/unit/test_diversity.cpp:95-99`):<br>`REQUIRE(div.uniqueFingerprints == 1);`<br>`REQUIRE(div.m2PitchClassHistogramL1.max == 0.0);`<br>`REQUIRE(div.m3IntervalBigramJs.max == 0.0);`<br>`REQUIRE(div.m4IoiGridL1.max == 0.0);` | **proposta per M2.5** (F5) | Media |

---

## 5. Proposte per M2.5 (Separate dai Rilievi su Inc-5)

Le seguenti proposte riguardano esclusivamente **M2.5** e non richiedono modifiche al codice chiuso di M2.4 Inc-5 (se non, opzionalmente, le due micro-pulizie `AG-ADR-01` e `AG-ADR-03` quando si toccheranno quei file):

1. **Rilascio musicale per ruolo separato dal de-click di sicurezza (`AG-ADR-08`):**
   - Mantenere `kDeclickDurationSeconds = 0.002` (rampa lineare di 96 campioni a 48 kHz) esclusivamente per i voice steal, `Pause`/`Stop` e i tagli di sicurezza (failsafe di durata e lease dei pad aperti).
   - Introdurre in `VoiceAllocator` / `RoleSynth` uno stadio di `Release` musicale per i `NoteOff` ordinari parametrizzato per ruolo timbrico, con profilo esponenziale o *raised-cosine* ($C^1$ continuo a inizio e fine rampa, inviluppo spettrale $O(\omega^{-3})$, Harris 1978):
     - **Trama e Vettore:** $20\text{–}30\text{ ms}$ (elimina i falsi onset da rilascio e porta il recall di Trama a $1.000$ senza introdurre voice drop, come dimostrato in `5e.3`).
     - **Pulso e Corpo:** durata di rilascio coordinata con la griglia ritmica e con la costante di recupero del ducker di `RoleBusProcessor`, evitando che la coda di Corpo resti attiva durante la risalita del compressore (`releaseOverlaps`).

2. **Normalizzazione robusta o locale causale nell'Onset Detector (`AG-ADR-07`):**
   - Sostituire in una nuova versione del detector (`onset_v2` in M2.5, mantenendo `onset_v1` per la riproducibilità storica di M2.4) la divisione per il massimo globale $\max_k F(k)$ con:
     - **Opzione A (minima invasività, offline):** normalizzazione rispetto a un quantile alto sui frame attivi (es. $Q_{0.95}(F)$ sulle finestre con energia sopra il floor di silenzio), che immunizza la soglia $\bar{F}(n) + \delta \cdot Q_{0.95}(F)$ da un singolo picco anomalo;
     - **Opzione B (causale/online, letteratura MIR):** *Adaptive Whitening* per banda (Stowell & Plumbley 2007) o compressione *PCEN* (Lostanlen et al. 2019) seguita da peak picking con soglia locale mediana + media mobile (Böck & Widmer 2013).

3. **Modello statistico per la sessione alla cieca in M2.5 (`AG-ADR-05`):**
   - Allineare stimatore e taglia campionaria effettiva in `presence_stats_v2`:
     - Se l'unità primaria è il gruppo musicale con peso uguale $w_g = 1/G$ ($\hat{\theta} = \frac{1}{G}\sum_{g=1}^G \hat{p}_g$), sotto correlazione intra-cluster completa ($\rho = 1$) il numero di unità indipendenti è direttamente $G$ (o, con stima empirica di $\hat{\rho}_{\text{ICC}}$ secondo Rao & Scott 1992, $n_{\text{eff}} = G / [1 + (\bar{m}-1)\hat{\rho}_{\text{ICC}}]$ dove $\bar{m}$ tiene conto della varianza intra-gruppo).
     - Per il criterio composito $J = \theta_+ + \theta_- - 1$, usare limiti inferiori con correzione di Bonferroni ($\alpha_1 = 0.0125$, $z \approx 2.2414$) per garantire copertura simultanea $\ge 95\%$ indipendentemente dalla struttura di dipendenza tra positivi e negativi sugli stessi gruppi, oppure affiancare un test di permutazione esatto a livello di gruppo.
     - Per `CL-MM-10` (dipendenza tra gruppi che condividono lo stesso motivo trasposto), includere nella chiave di gruppo simbolico di M2.5 l'identità della classe motivica a meno di trasposizione globale, ed eseguire la sessione alla cieca su un corpus multi-sezione/multi-seed generato con F5 ($\ge 12$ gruppi motivici realmente distinti).

4. **Generazione procedurale stateful e varietà da seed in F5 (`AG-ADR-09`):**
   - Introdurre nello stato del `MusicPlanner` una memoria storica di sezione (`recentTransforms`, `recentVoicings`, tensione accumulata dal direttore musicale F5) con penalità di ripetizione (*recency/tabu scoring*) e selezione stocastica controllata guidata dal substream crittografico del ruolo tra i candidati ammissibili entro una banda $\Delta S$ dal punteggio ottimo.

---

## 6. Bibliografia con Livello di Verifica

Ogni fonte esterna citata nelle sezioni precedenti è corredata di identificativo permanente (DOI o atti ufficiali), data di consultazione (2026-11-03) e livello di verifica effettivo (`letta` / `consultata l'abstract` / `citata da altra fonte`).

1. **Rao, J. N. K., & Scott, A. J. (1992).** *A Simple Method for the Analysis of Clustered Binary Data.* Biometrics, 48(2), 577–585. DOI: `10.2307/2532311`.
   - **Livello di verifica:** `consultata l'abstract` e formulazione matematica del *design effect* e del *complete intracluster correlation* ($\rho = 1$) verificata.
2. **Obuchowski, N. A. (1997).** *Nonparametric Analysis of Clustered ROC Curve Data.* Biometrics, 53(2), 567–578. DOI: `10.2307/2533958`.
   - **Livello di verifica:** `consultata l'abstract` e struttura di varianza/covarianza cross-classe per sensibilità e specificità su dati clusterizzati verificata.
3. **Eldridge, S. M., Ukoumunne, O. C., & Carlin, J. B. (2009).** *The Intra-Cluster Correlation Coefficient in Cluster Randomized Trials: A Review of Definitions.* International Statistical Review, 77(3), 378–394. DOI: `10.1111/j.1751-5823.2009.00092.x`.
   - **Livello di verifica:** `consultata l'abstract` e correzione del *design effect* per cluster di dimensione variabile verificata.
4. **Cameron, A. C., Gelbach, J. B., & Miller, D. L. (2011).** *Robust Inference With Multiway Clustering.* Journal of Business & Economic Statistics, 29(2), 238–249. DOI: `10.1198/jbes.2010.07136`.
   - **Livello di verifica:** `consultata l'abstract`.
5. **Kish, L. (1965).** *Survey Sampling.* New York: John Wiley & Sons.
   - **Livello di verifica:** `citata da altra fonte` (tramite Eldridge et al. 2009 e `docs/research/M2_4_Inc5_Clustered_Claim_Statistics.md`).
6. **Bello, J. P., Daudet, L., Abdallah, S., Duxbury, C., Davies, M., & Sandler, M. B. (2005).** *A Tutorial on Onset Detection in Music Signals.* IEEE Transactions on Speech and Audio Processing, 13(5), 1035–1047. DOI: `10.1109/TSA.2005.851998`.
   - **Livello di verifica:** `letta` (architettura ODF, pre-processing logaritmico, normalizzazione e soglia adattiva mediana/media mobile).
7. **Dixon, S. (2006).** *Onset Detection Revisited.* Proceedings of the 9th International Conference on Digital Audio Effects (DAFx-06), Montreal, Canada, pp. 133–137.
   - **Livello di verifica:** `letta` (formulazione dello spectral flux half-wave rectified e impatto della normalizzazione e della soglia $\delta$).
8. **Böck, S., & Widmer, G. (2013).** *Maximum Filter Vibrato Suppression for Onset Detection.* Proceedings of the 16th International Conference on Digital Audio Effects (DAFx-13), Maynooth, Ireland, pp. 55–61.
   - **Livello di verifica:** `consultata l'abstract` e sintesi algoritmica di SuperFlux verificata.
9. **Stowell, D., & Plumbley, M. D. (2007).** *Adaptive Whitening for Improved Real-Time Audio Onset Detection.* Proceedings of the International Computer Music Conference (ICMC 2007), Copenhagen, Denmark, pp. 312–319.
   - **Livello di verifica:** `consultata l'abstract` e meccanismo di normalizzazione causale per bin verificato.
10. **Lostanlen, V., Salamon, J., Cartwright, M., McFee, B., Farnsworth, A., Kelling, S., & Bello, J. P. (2019).** *Per-Channel Energy Normalization: Why and How.* IEEE Signal Processing Letters, 26(1), 39–43. DOI: `10.1109/LSP.2018.2878620`.
    - **Livello di verifica:** `consultata l'abstract` e proprietà analitiche dell'AGC causale per canale verificate.
11. **Harris, F. J. (1978).** *On the Use of Windows for Harmonic Analysis with the Discrete Fourier Transform.* Proceedings of the IEEE, 66(1), 51–83. DOI: `10.1109/PROC.1978.10837`.
    - **Livello di verifica:** `letta` (decadimento asintotico dei lobi secondari $O(\omega^{-1})$, $O(\omega^{-2})$, $O(\omega^{-3})$ in funzione dell'ordine di continuità della finestra).
12. **Puckette, M. (2007).** *The Theory and Technique of Electronic Music.* Hackensack, NJ: World Scientific Publishing. ISBN: `978-981-270-077-3` (Cap. 4: *Automation and Voice Management*, §4.3 *Muting* e *Switch-and-Ramp*).
    - **Livello di verifica:** `letta` (distinzione tra rampa di muting/de-click di pochi millisecondi per prevenire discontinuità da furto di voce e inviluppo musicale di nota).
13. **Välimäki, V., & Huovilainen, A. (2006).** *Oscillator and Filter Algorithms for Virtual Analog Synthesis.* Computer Music Journal, 30(2), 19–31. DOI: `10.1162/comj.2006.30.2.19`.
    - **Livello di verifica:** `consultata l'abstract`.
14. **Collins, K. (2009).** *An Introduction to Procedural Music in Video Games.* Contemporary Music Review, 28(1), 5–15. DOI: `10.1080/07494460802663983`.
    - **Livello di verifica:** `consultata l'abstract` e tassonomia degli approcci procedurali e trasformazionali verificata.
15. **Plans, D., & Morelli, D. (2012).** *Experience-Driven Procedural Music Generation for Games.* IEEE Transactions on Computational Intelligence and AI in Games, 4(3), 192–198. DOI: `10.1109/TCIAIG.2012.2212899`.
    - **Livello di verifica:** `consultata l'abstract`.
16. **Hutchings, P. E., & McCormack, J. (2020).** *Adaptive Music Composition for Games.* IEEE Transactions on Games, 12(3), 270–280. DOI: `10.1109/TG.2019.2921979`.
    - **Livello di verifica:** `consultata l'abstract`.

# DIF-INC4-R2-01 — Workload W e riduzione di guadagno del limiter: analisi delle alternative

- **Data:** 2026-10-21
- **Stato:** analisi di supporto alla decisione (**non normativa**). **Decisione del titolare (2026-10-21): opzione B1**, applicata e registrata come precisazione di D-M2-10 in [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md); esito in §8.
- **Rilievo:** [progress.md, «Revisione post-Inc-4»](../progress.md#revisione-post-inc-4-2026-10-21), DIF-INC4-R2-01.
- **Base di codice:** working tree su `9c61f88` con la remediation post-Inc-4 (non committata).
- **Dati grezzi e sorgente dell'esperimento:** [experiments/dif_inc4_r2_01/](experiments/dif_inc4_r2_01/).

## 1. Problema

D-M2-10 definisce il carico **W** come «stress worst-case con 8 voci attive su tutti i ruoli (`timbreParam > 0`), un furto per blocco, delay di Trama attivo e limiter in riduzione di guadagno». Nella revisione post-Inc-4 il limiter di W risulta **sempre a 0 dB di riduzione** (GR). La domanda è se W debba restare così (emendando l'ADR) o se vada modificato per portare il limiter in riduzione. La risposta dipende da due quesiti empirici:

1. **Q1:** il carico W, che usa solo eventi musicali, *può* raggiungere la soglia del limiter?
2. **Q2:** la riduzione di guadagno cambia in modo misurabile il costo della callback, cioè ciò che il gate D-M2-10 vuole limitare?

## 2. Analisi del codice

### 2.1 Catena di guadagno fino al limiter

Ogni voce contribuisce con `s · velocity · envelope · kNominalVoiceScale` ([voice_allocator.cpp](../../engine/audio/voice_allocator.cpp)), con `kNominalVoiceScale = 0,25` e `velocity ≤ 1` (clamp in `onNoteOn`). I bus applicano −6 / −8 / −10 / −6 dB (`RoleBusProcessor::k*Gain`) e il micro-ducking riduce il Corpo fino a −3 dB in presenza del Pulso. La soglia del limiter è −1,80 dBFS (`kThresholdLinear = 0,8128`).

Limite superiore teorico, con 2 voci per ruolo, fase perfettamente coerente, inviluppi al massimo e ducking assente:

`2 × 0,25 × (0,5012 + 0,3981 + 0,3162 + 0,5012) = 0,858` → **−1,33 dBFS**

Supera la soglia di soli 0,47 dB. Richiede però che le 8 forme d'onda (oscillatori e inviluppi indipendenti, 4 timbri diversi) raggiungano il picco nello stesso campione e che il ducking resti inattivo con il Pulso a piena velocity. È una condizione costruibile solo in teoria, non con eventi musicali (§3.1).

### 2.2 Struttura del limiter

`MasterLimiter::processFrame` ([master_limiter.cpp](../../engine/audio/master_limiter.cpp)) esegue lo stesso lavoro per ogni frame, senza cicli dipendenti dai dati: guard dei valori finiti, stimatore causale del true peak (3 FIR a 4 fasi, curvatura con 2 `sqrt`), guadagno, attack/hold/release, slew guard e clamp. La riduzione di guadagno aggiunge solo:

- una divisione (`threshold / peakEst`);
- il ramo di attack/hold;
- l'eventuale intervento dello slew guard vicino al fondo scala.

Il limiter è sempre attivo («always active, fail-closed», AUD-D-02). Senza GR, la percorrenza della callback è quindi la stessa salvo pochi rami.

### 2.3 Leva già esistente: il tono diagnostico

Il tono di M2.2 (`ma_waveform`) è sommato alle voci *prima* del limiter (`renderWaveformSlice`, poi `masterLimiter_.processBlock`), con ampiezza fissata dal `paramF32` del comando `Start`. Il workload W lo avvia con `paramF32 = 0` (tono spento). Il tono è un'API pubblica già esistente nel percorso di render di produzione: permette di superare la soglia senza modificare il motore.

## 3. Misure

**Ambiente:** AMD Ryzen 7 7700X, Windows 10.0.26300, MSVC 19.51.36260, Release `/O2`, render offline, blocchi da 512 frame a 48 kHz.

**Condizioni non controllate:** durante le misure era in esecuzione in background un videogioco (Diablo IV). Questo influisce sulle code (§3.4) e non sulle mediane.

**Procedura:** [experiment_r2_01.cpp](experiments/dif_inc4_r2_01/experiment_r2_01.cpp), test nascosto `[experiment_r2_01]`.

**Dati grezzi:**

- [run1](experiments/dif_inc4_r2_01/run1_2026-10-21.txt): prima versione dell'esperimento, 6 livelli di tono × 3 ripetizioni e 6 condizioni del limiter × 7.
- [run2](experiments/dif_inc4_r2_01/run2_2026-10-21.txt): seconda versione, 3 livelli × 5 ripetizioni interlacciate e 8 condizioni × 7.
- [run3](experiments/dif_inc4_r2_01/run3_2026-10-21.txt): versione archiviata corrente, che misura il picco anche sui 64 blocchi di warm-up (`outPeakAll`); 3 livelli × 5 ripetizioni. Aggiunta dopo la correzione del 2026-10-21 (§3.1).

### 3.1 Q1 — livelli raggiungibili (run1 e run3, deterministici)

Il picco d'uscita è identico in tutte le ripetizioni. Si distinguono due finestre:

- **assoluto:** tutti i blocchi, warm-up compreso (run3);
- **regime:** i 10.000 blocchi misurati dopo i 64 di warm-up, cioè la finestra del gate D-M2-10 (run1 e run3).

| Carico W | Picco assoluto | Picco a regime | GR massima |
|---|---|---|---|
| solo voci (tono 0) | 0,4402 (**−7,13 dBFS**), al frame 144 del blocco 0 | 0,3739 (**−8,54 dBFS**) | 0 dB |
| + tono 0,25 | — | 0,6130 (−4,25 dBFS) | 0 dB |
| + tono 0,40 | 0,7779 (−2,18 dBFS) | 0,7618 (−2,36 dBFS) | 0 dB |
| + tono 0,50 | — | 0,8128 (−1,80 dBFS, limitato) | 0,52 dB |
| + tono 0,60 | 0,8128 | 0,8128 | 1,47 dB |
| + tono 0,80 | — | 0,8129 | 3,10 dB |

Il picco assoluto delle sole voci nasce dall'attacco simultaneo delle 8 voci iniziali, tutte a velocity 1 al frame 0, nel blocco 0. Dopo, i furti di una voce per blocco non riproducono quella coerenza. Il valore è emerso dai WAV di una revisione esterna (Antigravity, 2026-10-21): un campione di 250 blocchi dava −7,13 dBFS assoluti e −9,09 dBFS sui blocchi 64–249. Il dato è stato poi riprodotto con il motore in run3. **Correzione del 2026-10-21:** la prima versione di questo dossier riportava solo il picco a regime (−8,54 dBFS) senza dichiarare la finestra.

In aggiunta, durante la remediation W con 8 voci all'unisono (110 Hz) ha dato GR = 0 dB.

**Risposta a Q1:** con soli eventi musicali W resta sotto la soglia di **5,3 dB in assoluto** (picco d'attacco del blocco 0) e di **6,7 dB a regime**. Lo impediscono il gain staging di Inc-2 e il clamp della velocity, che sono scelte di progetto deliberate di ADR-0003 D-M2-06 e ADR-0004, non un difetto del carico. Con il tono diagnostico ≥ 0,5 il limiter entra in riduzione in modo stabile.

### 3.2 Q2a — costo del limiter isolato (run2, 7 ripetizioni × 100.000 blocchi, mediana [min–max])

| Ingresso (sinusoide 997 Hz, stereo) | ns/frame | Δ vs. sotto soglia |
|---|---|---|
| sotto soglia (0,5) | 12,28 [12,15–12,65] | — |
| silenzio | 12,16 [12,08–12,77] | −1,0 % |
| burst (1 blocco su 8 a 1,5) | 12,57 [12,45–12,93] | +2,4 % |
| sopra soglia (1,5; GR 5,3 dB) | 13,96 [13,89–14,38] | **+13,7 %** |
| molto sopra soglia (8,0; GR 19,9 dB) | 14,13 [14,03–14,42] | +15,1 % |
| decadimento verso i subnormali, senza GR | 18,13 [18,07–18,56] | +47,7 % |
| subnormale costante (1e-39) | 25,02 [24,71–25,80] | **+103,8 %** |

La run1 conferma le stesse proporzioni: +11,0 % con GR, +43,5 % con decadimento verso i subnormali.

### 3.3 Q2b — costo dell'intera callback W (run2, 5 ripetizioni interlacciate × 10.000 blocchi)

| Variante | media (mediana delle ripetizioni) [min–max] | p50 |
|---|---|---|
| solo voci, nessuna GR | 102,8 µs [101,6–105,5] | 95,7 µs |
| + tono 0,40, nessuna GR | 108,8 µs [104,2–110,0] | 98,1 µs |
| + tono 0,60, GR 1,47 dB | 107,8 µs [105,8–109,1] | 98,6 µs |

**Risposta a Q2:**

- La riduzione di guadagno costa **+1,7–1,9 ns/frame**, cioè **≈ +0,9 µs per blocco da 512**. È lo **0,045 %** del budget di 2,0 ms e circa lo 0,9 % del costo medio di W.
- A livello di callback l'effetto non è misurabile: tono 0,6 con GR contro tono 0,4 senza GR dà −1,0 µs, entro la dispersione fra ripetizioni (±3 µs).
- Il tono stesso aggiunge ≈ +5–6 µs (+5 %), quindi rende W più pesante.
- Il limiter vale ≈ 6,3 µs per blocco, circa il 6 % della callback W.

### 3.4 Variabilità delle code e rumore di misura

Lo stesso `profile_probe_v1` ha dato:

- p99.9 di W = **0,18–0,27 ms** nella sessione di remediation;
- **0,60–0,73 ms** in questa sessione, con il gioco in background (anche il p99.9 grezzo dell'esperimento è 0,51–0,95 ms).

Il fattore ≈ 3 sulle code, con medie quasi invariate (≈ 103–106 µs), è tipico del jitter dell'OS su un thread non realtime. È coerente con la motivazione di D-M2-10, che esclude il gate sul massimo. Il margine resta ≥ 2,1× rispetto a 2,0 ms anche in condizioni sfavorevoli.

Ne segue che una differenza di ≈ 1 µs (l'effetto della GR) è **due ordini di grandezza sotto il rumore delle code**. Includere o escludere la GR non può cambiare l'esito del gate p99.9.

### 3.5 Osservazione collaterale: subnormali

Il caso peggiore effettivo del limiter non è la riduzione di guadagno ma l'aritmetica subnormale, con un costo +48–104 %. È coerente con il costo noto dei denormali su x86-64 [W15].

Nel percorso reale l'ingresso del limiter non diventa subnormale. ADR-0004 impone un flush portatile `|v| < 1e-20 → 0` sugli stati degli integratori SVF, degli inviluppi, delle linee di delay e del follower di ducking (`flushSubnormal` in [synth_dsp.h](../../engine/audio/synth_dsp.h) e [role_bus_processor.cpp](../../engine/audio/role_bus_processor.cpp)). Prodotti di valori ≥ 1e-20 restano molto sopra `FLT_MIN ≈ 1,18e-38`.

Questa misura conferma quantitativamente il valore di quella mitigazione. Non motiva cambiamenti a W, ma suggerisce di non rimuovere mai il flush e di valutare FTZ/DAZ solo su evidenza, come già previsto da D-M2-10.

## 4. Riferimenti tecnico-scientifici

Fonti consultate il 2026-10-21. Per gli articoli a pagamento (AES, ACM) sono stati verificati i metadati bibliografici e gli abstract, non il testo completo; le affermazioni riportate qui si limitano a quanto è documentato in quelle fonti. Per [B11] è stata verificata la pubblicazione tramite la segnalazione su LWN e la lista linux-audio-dev, non il testo dell'articolo.

- **[G12]** D. Giannoulis, M. Massberg, J. D. Reiss, «Digital Dynamic Range Compressor Design — A Tutorial and Analysis», *J. Audio Eng. Soc.* 60(6), 399–408, 2012 ([AES E-Library](https://aes2.org/publications/elibrary-page/?id=16354)). Il compressore/limiter feed-forward ha un rilevatore e un calcolatore di guadagno sempre in esecuzione; la riduzione cambia il valore, non la struttura del calcolo. È coerente con §2.2 e §3.2.
- **[W08]** R. Wilhelm et al., «The Worst-Case Execution-Time Problem — Overview of Methods and Survey of Tools», *ACM TECS* 7(3), art. 36, 2008, [DOI 10.1145/1347375.1347389](https://doi.org/10.1145/1347375.1347389). Le misure end-to-end sottostimano il caso peggiore se il carico non percorre i cammini più costosi, e i componenti speculativi (cache, predizione dei salti) rendono il problema difficile. Ne segue che un carico di stress deve esercitare i rami più costosi *quando sono rilevanti*. Qui il ramo di GR è percorribile e costa poco, ma la buona prassi è comunque coprirlo.
- **[H15]** T. Hoefler, R. Belli, «Scientific Benchmarking of Parallel Computing Systems: Twelve Ways to Tell the Masses when Reporting Performance Results», *SC15*, 73:1–73:12, 2015 ([pagina dell'autore](https://www.unixer.de/publications/index.php?pub=222)). Indica di riportare variabilità e ambiente e di dire se un effetto supera il rumore: è il criterio applicato in §3.3–3.4.
- **[C16]** J. Chen, J. Revels, «Robust benchmarking in noisy environments», [arXiv:1608.04295](https://arxiv.org/abs/1608.04295), 2016. Il rumore dell'ambiente (timer, jitter dell'OS) produce distribuzioni dei tempi asimmetriche a destra. Spiega la variabilità delle code in §3.4 e il fatto che i percentili alti dipendano dalla sessione più delle mediane.
- **[W15]** M. Wittmann, T. Zeiser, G. Hager, G. Wellein, «Short Note on Costs of Floating Point Operations on current x86-64 Architectures: Denormals, Overflow, Underflow, and Division by Zero», [arXiv:1506.03997](https://arxiv.org/abs/1506.03997), 2015. Le operazioni su denormali possono costare circa due ordini di grandezza in più e FTZ/DAZ (MXCSR) le eliminano. Qui l'effetto osservato su Zen 4 è molto minore (×1,5–2 sull'intero frame del limiter) ma va nella stessa direzione (§3.5).
- **[B11]** R. Bencina, «Real-time audio programming 101: time waits for nothing», 2011 (rossbencina.com; segnalato su [LWN](https://lwn.net/Articles/453005/)). La callback audio deve terminare entro la scadenza in ogni esecuzione, non in media. Giustifica un carico W conservativo e il limite assoluto sul periodo del blocco già previsto da D-M2-10.

## 5. Alternative

| | **A. Emendare D-M2-10** (W senza GR) | **B1. W con tono diagnostico** (`Start.paramF32 = 0,8`) | **B2. Più carico musicale** | **B3. Hook di pre-gain diagnostico nel motore** | **C. A + micro-benchmark del limiter** |
|---|---|---|---|---|---|
| Conforme al testo di D-M2-10 | No, serve un emendamento | **Sì**; precisa solo *come* si ottiene la GR | — | Sì | No, serve un emendamento |
| Fattibilità | Immediata | **Immediata, nessuna modifica al motore** | **Non fattibile** (§3.1: −7,13 dBFS assoluti e −8,54 dBFS a regime, clamp della velocity, unisono insufficiente) | Nuova API e un ramo nel percorso RT | Media (nuovo test `perf`) |
| Copertura dei cammini (W08) | Il ramo di GR non è esercitato nella callback | **Ogni stadio attivo**: tono, voci, furti, bus, delay, limiter in GR sostenuta, il caso più costoso di §3.2 | — | Completa | GR coperta solo a livello di componente |
| Effetto sul gate p99.9 | Nessuno (Δ ≈ 0,9 µs ≪ rumore) | W più pesante del ≈ 5 % (più conservativo) | — | Simile a B1 | Nessuno |
| Rappresentatività | Il carico musicale reale non raggiunge mai la soglia | Il tono non è contenuto musicale; la GR è sostenuta e non transitoria (ma sostenuta è il caso peggiore: 13,96 contro 12,57 ns/frame dei burst) | — | Artificiale | Componente isolato, senza le interazioni della callback |
| Rischi e manutenzione | Precedente di un criterio indebolito, da motivare (AGENTS §5.1) | Dipende dal tono di M2.2: se il tono venisse rimosso o limitato, W va ridefinito; va reso esplicito nell'ADR | — | Aumenta la superficie ABI e il codice RT; contrario alla regola della modifica minima | Più codice di test da mantenere |
| Golden o ABI toccati | No | No (W non ha golden; ABI invariata) | — | Sì (API interna nuova) | No |

## 6. Raccomandazione

**B1** (tono diagnostico a 0,8 nel carico W, con il gate `max_gain_reduction_db > 0` ripristinato in `stressConditionsMet`), registrata come *precisazione* di D-M2-10 in ADR-0005. Le ragioni:

1. **Conformità:** soddisfa il criterio di D-M2-10 così com'è scritto, senza indebolirlo (AGENTS §5.1: non indebolire in silenzio i criteri di accettazione).
2. **Costo nullo sul motore:** nessuna modifica al motore né all'ABI; usa un percorso di produzione già esistente e documentato (il tono passa dallo stesso limiter).
3. **Carico più conservativo:** W diventa strettamente più pesante (+5 % di costo medio) ed esercita il ramo più costoso del limiter (GR sostenuta), come suggerito da [W08].
4. **Margine sulla soglia:** il livello 0,8 dà 3,1 dB di GR, con margine robusto contro future variazioni di gain staging. A 0,5 la GR sarebbe di soli 0,5 dB e fragile.

**A** è un'alternativa difendibile sul piano quantitativo: l'effetto della GR è lo 0,045 % del budget e non è osservabile nel gate (§3.3–3.4). Lascia però non esercitato un ramo del percorso RT e richiede un emendamento esplicito. Se scelta, conviene affiancarle **C** (micro-benchmark del limiter con GR sostenuta e burst, riportato e non vincolante) per non perdere la copertura.

B2 non è fattibile; B3 è sproporzionata.

### Modifiche previste in caso di scelta B1

- `runProfileWorkload(W)`: `Start` con `paramF32 = 0,8f` (costante nominata, ad es. `kProfileWorkloadWToneVolume`) e ripristino del gate `maxGainReductionDb > 0`.
- ADR-0005 D-M2-10: aggiungere che la riduzione di guadagno di W si ottiene con il tono diagnostico a 0,8 sommato prima del limiter, perché il solo carico musicale resta a −7,13 dBFS assoluti (−8,54 dBFS a regime); citare questo documento.
- progress.md: chiudere DIF-INC4-R2-01 con le nuove misure di `profile_probe_v1`.

## 7. Limiti dell'analisi

- **Macchina e sessione:** una sola macchina e una sola sessione con carico in background non controllato. Le mediane sono stabili, le code no (§3.4).
- **Timer nel micro-benchmark:** `steady_clock` letto per blocco aggiunge un overhead costante di decine di ns per blocco, uguale in tutte le condizioni e quindi ininfluente sulle differenze relative.
- **Teoria vs. misura:** il limite teorico di §2.1 trascura il wet del delay di Trama (che lo alzerebbe) e il ducking (che lo abbassa); il picco misurato resta il dato decisivo.
- **Device:** nessuna misura su device; la qualificazione realtime resta a M2.5 (D-M2-10, limite R6-03).

## 7-bis. Conformità alle regole di `.agents/rules/` (verifica tardiva, 2026-10-21)

La verifica contro le regole in `.agents/rules/` è stata eseguita **dopo** la decisione: nell'analisi iniziale mancava. [realtime_and_serialization.md](../../.agents/rules/realtime_and_serialization.md) §4 stabilisce che i generatori diagnostici legacy (`ma_waveform`) restino «silenziati e disaccoppiati durante il funzionamento del `VoiceAllocator` polifonico, attivandosi solo in presenza di richieste esplicite di tono continuo (`activeVolume > 1e-6f`)».

- **Compatibilità:** B1 attiva il tono con una richiesta esplicita (`Start.paramF32 = 0,8`), solo nel benchmark offline `profile_probe_v1` e mai nel runtime o nella fixture. Il caso rientra quindi nella clausola di attivazione esplicita.
- **Tensione residua:** la regola presuppone che tono e voci polifoniche non suonino insieme. In W suonano insieme, e il tono si somma alle voci prima del limiter.
- **Risoluzione (decisione del titolare, 2026-10-21):** la regola §4 è stata precisata. Il tono diagnostico può suonare insieme alle voci solo nei carichi di benchmark diagnostici dichiarati in un ADR; oggi l'unico caso è il workload W di D-M2-10, con `kProfileWorkloadWToneVolume = 0,8`. Runtime, fixture, golden, scenari CACP di contenuto ed export WAV per l'ascolto restano esclusi. B1 è quindi conforme e la questione è chiusa.

## 8. Esito dell'applicazione (2026-10-21)

**Modifiche:**

- `kProfileWorkloadWToneVolume = 0,8f` ([audio_probe.h](../../engine/audio/audio_probe.h)).
- `Start.paramF32` del workload W e gate `max_gain_reduction_db > 0` in `runProfileWorkload` ([audio_probe.cpp](../../engine/audio/audio_probe.cpp)).
- Precisazione di D-M2-10 e §4 «Riferimenti» in [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md).

**`profile_probe_v1` Release** ([manifest archiviato](experiments/dif_inc4_r2_01/profile_probe_v1_B1_2026-10-21.txt)): **PASS**, 12 esecuzioni senza re-run.

| Carico | GR | Furti | p50 | p99.9 | max | media |
|---|---|---|---|---|---|---|
| W, CACP OFF/ON × 3 | 3,10 dB in ogni esecuzione | 10.000 | 0,10 ms | 0,28–0,43 ms | ≤ 1,07 ms | ≈ 0,099 ms |
| F, CACP OFF/ON × 3 | 0 dB (contenuto musicale) | — | 0,06 ms | 0,18–0,21 ms | ≤ 0,52 ms | ≈ 0,057 ms |

0 blocchi oltre 2 ms o oltre il periodo del blocco. La media di W sale da ≈ 0,093–0,103 ms a ≈ 0,099 ms, coerente con il +5 % previsto in §3.3.

**Suite:** Release e Debug headless 140/140, `perf` 2/2, 0 warning.

**Limiti:**

- I valori di coda restano dipendenti dalla sessione (§3.4).
- Nessuna CI e nessuna misura su device (M2.5).

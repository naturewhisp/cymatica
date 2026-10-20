# ADR-0004 — Renderer nativo, protezione terminale e loudness (ratifica M2.4 e chiusura Gate pre-Inc-2)

- **Data di ratifica iniziale:** 2026-10-15; **Aggiornamento e chiusura Gate pre-Inc-2:** 2026-10-19.
- **Stato:** **accepted for implementation** — Inc-1, Inc-2 e Inc-3 implementati; emendamento del 2026-10-20 (§4.3) che allinea topologia, soglia e detector del limiter al codice dopo la revisione post-Inc-3. Lo stato operativo corrente è registrato solo in [progress.md](../progress.md).
- **Owner decisionale:** titolare CYMATICA; approvazione esplicita dopo Grill-me, review indipendente del Decision Record del 2026-10-15 e revisione tecnico-scientifica post-Inc-1 del 2026-10-19.
- **Baseline documentale verificata:** `32f93250f8d18c080ab165ddb12d2b88499eab42` (`main`, commit *Document M2.4 audio research and safety review*); **Remediation Inc-1 e Gate pre-Inc-2:** 2026-10-19 (91/91 test CTest Release/Debug, 90/90 Headless).
- **Percorso:** il suffisso storico `-proposal.md` è conservato per non invalidare riferimenti già committati; lo **stato nel documento**, non il filename, è autoritativo.
- **Gerarchia:** Specifica > DESIGN > AGENTS > ADR > piano operativo > ricerca. Questo ADR integra [ADR-0003](0003-m2-decisions-contracts.md) nei punti esplicitati al §6; non altera M1, M2.3, ABI pubbliche o roadmap.
- **Evidenze:** [review renderer](../research/M2_4_Audio_Renderer_Research_Review.md), [studio safety](../research/M2_4_Audio_Safety_Design.md), [protocollo sperimentale](../research/M2_4_Evaluation_Protocol.md), [piano M2.4](../research/M2_4_Implementation_Plan.md), [evidenza distorsione/aliasing Inc-1](../research/M2_4_Voice_Distortion_Aliasing_Evidence.md) e Decision Record Grill-me approvato dal titolare. Le evidenze PCM/CACP di Inc-1 (`voice_probe_v1`) sono verificate; l'implementazione di Inc-2..Inc-5 seguirà i contratti qui chiusi.

## 1. Contesto e perimetro della ratifica

La ricerca comparativa raccomanda un renderer nativo C++20 circoscritto, con eventuale riuso di singole primitive DSP a licenza compatibile e qualificate; **non** un host CLAP/VST3 obbligatorio in M2.4. Il Grill-me ha accettato questa scelta e il principio di protezione terminale sul mix controllato dal gioco. Con la revisione tecnico-scientifica del 2026-10-19 (a **Inc-1 concluso**), il **Gate tecnico pre-Inc-2** viene formalmente chiuso specificando la qualifica DSP nativa pura per Inc-2 (§2) e l'algoritmo/topologia del limiter terminale zero-latency (§4). Il monitoring LUFS/true peak resta approvato come **architettura progressiva a tre domini**, non come garanzia di conformità acustica.

Il presente ADR **ratifica AUD-D-01, AUD-D-04, AUD-D-02 (inclusa la specifica tecnica pre-Inc-2) e AUD-D-03 a livello architetturale**.

## 2. AUD-D-01 — Renderer nativo e qualifica DSP per Inc-2 (Gate pre-Inc-2 CHIUSO)

**Decisione approvata.** In M2.4, generazione audio e voice lifecycle rimangono **nativi C++20**, basati su `VoiceSlot[8]`, due slot per Pulso/Corpo/Trama/Vettore, quattro bus stereo preallocati, `MusicIntentEvent`, clock logico a 48 kHz, SPSC e vincoli di determinismo/realtime di ADR-0003. La pipeline v1 non richiede host plugin, librerie DSP monolitiche, sample bank esterne o file audio da caricare a runtime.

**Inc-1 è completato e verificato** (`VoiceAllocator`, 8 slot statici, voice stealing deterministico Rules 0/1/2, steal tails statiche lineari di $2\text{ ms} = \lfloor 0.002 f_{s,\text{device}} \rfloor$, `voice_probe_v1`).

**Qualifica tecnica DSP per Inc-2 (CHIUSO — 2026-10-19):** Per **Inc-2 ("Sintesi 4 Ruoli & 4 Bus")** è deliberata l'**implementazione C++20 nativa pura autocontenuta** in `engine/audio/` con **zero dipendenze esterne** (`docs/dependencies.md` invariato). Nessun file sorgente esterno da DaisySP o Airwindows viene importato in Inc-2, garantendo determinismo floating-point bit-exact su MSVC `/W4 /fp:precise`, zero overhead di licenza e controllo diretto delle guardie numeriche:

1. **Pulso (Sub-kick percussivo + click transiente):**
   - Oscillatore sinusoidale con sweep esponenziale di frequenza $f_{\text{inst}}[k] = f_{\text{base}} + (f_{\text{start}} - f_{\text{base}}) e^{-k / (\tau_p f_s)}$ ($f_{\text{start}} = \min(3.0 f_{\text{base}}, 180\text{ Hz})$, $\tau_p = 8\text{ ms}$).
   - Click transiente deterministico ($15\text{ ms}$) generato tramite hash puro stateless `core::staffordMix13(eventId ^ (k * 0x9E3779B97F4A7C15ULL) ^ slotIndex)` filtrato con TPT SVF passa-banda/passa-alto ($f_c \in [1200, 3500]\text{ Hz}$). Nessun accesso a `SeedBank` o stato RNG globale.
2. **Corpo (Pad / Basso armonico sottrattivo):**
   - Oscillatore sawtooth/pulse band-limited con **2-point PolyBLEP** analitico alle discontinuità di fase ($dt = f_0 / f_s < 0.45$, attenuazione dell'aliasing $> 40\text{ dB}$).
   - Filtro **Zavalishin/Simper Trapezoidal Topology-Preserving Transform (TPT) SVF** a 2 poli: pre-warping $g = \tan(\pi f_c / f_s)$ con clamp obbligatorio $f_c \in [20\text{ Hz}, 0.45 f_s]$ ($g \in [0.0013, 6.314]$) e smorzamento $R = 1/(2Q) \in [0.05, 1.0]$ ($Q \in [0.5, 10.0]$), garantendo poli discreti strettamente interni al cerchio unitario ($|z_p| < 1$, incondizionatamente BIBO stabile anche sotto modulazione rapida di cutoff).
3. **Trama (Arpeggio / Texture FM a 2 operatori + Micro-delay stereo):**
   - Sintesi FM a 2 operatori con **bound anti-aliasing di Carson**: indice di modulazione $I$ limitato dinamicamente a $I_{\max}(f_c, f_m) = \max\!\left(0.0\text{f}, \frac{0.42 f_s - f_c}{f_m} - 1.0\text{f}\right)$ con cap assoluto $I \le 4.0\text{f}$, prevenendo il ripiegamento (foldback aliasing) oltre Nyquist.
   - Micro-delay stereo preallocato in `RoleBusProcessor` (`std::array<float, 24000>` per canale, $500\text{ ms}$ max @ 48 kHz; taps nominali $L = 11\text{ ms}$, $R = 17\text{ ms}$; feedback $|g_{\text{fb}}| \le 0.45\text{f}$ per decadimento $RT_{60}$ finito e incondizionatamente stabile).
4. **Vettore (Lead melodico espressivo):**
   - Oscillatore 2-point PolyBLEP con portamento esponenziale one-pole ($\tau_{\text{glide}} = 15\text{ ms}$), LFO di vibrato sinusoidale ($5.5\text{ Hz}$, profondità $\pm 0.5\%$ di $f_0$ scalata da `timbreParam`) e filtro TPT SVF low-pass.
5. **Smoothing e guardie numeriche:**
   - Smoothing esponenziale one-pole ($\tau_{\text{smooth}} = 5\text{ ms}$) sui parametri continui (`timbreParam`, guadagni di bus) per prevenire zipper noise.
   - Flush portatile dei subnormali (`std::abs(v) < 1e-20f ? 0.0f : v`) sugli stati integratori del filtro TPT SVF, dell'inviluppo e delle linee di delay per azzerare i picchi di CPU da denormali IEEE 754.

## 3. AUD-D-04 — Plugin hosting differito e unica autorità temporale

**Decisione approvata.** Nessun host CLAP/VST3, backend plugin obbligatorio o astrazione generica plugin/nativo fa parte degli incrementi **Inc-1..Inc-5** di M2.4. Non si afferma che ospitare plugin in realtime sia impossibile: la complessità non è giustificata dalle evidenze disponibili per questa milestone.

`MusicClock` (48 kHz logici), `SetTempoMap`, `MusicIntentEvent`, committed scheduling, semantica cross-epoch e regole `Pause`/`Resume`/`Stop` restano autoritativi e invariati. Nessun clock MIDI/software concorrente. CACP è **Control & Observation Plane** diagnostico e non partecipa alla generazione del transport realtime.

Se in futuro verrà autorizzato uno spike CLAP o VST3, sarà **separato dalla roadmap vincolante M2.4**, con host headless e adapter bounded che *proietta* il transport canonico, gestione esplicita di identità note 64→32 bit, versionamento stato/preset, latenza, lifecycle e test di affidabilità. Il confronto timbrico offline con Surge XT resta un'opzione, senza milestone futura assegnata automaticamente.

## 4. AUD-D-02 — Protezione digitale terminale comune (Gate pre-Inc-2 CHIUSO)

**Principio architetturale e specifica tecnica pre-Inc-2 approvati e vincolanti.** Tutte le sorgenti sonore **sotto il controllo di CYMATICA** confluiscono in uno stadio terminale comune di protezione digitale **sempre attivo, zero-latency ($L = 0$) e fail-closed** prima della consegna del PCM al backend audio. In M2.4 il requisito riguarda le sorgenti musicali realmente presenti; futuri SFX/UI dovranno essere aggiunti allo stesso percorso quando saranno implementati. La protezione non dipende da CACP, AudioProbe o disponibilità dei loro worker.

### 4.1 Topologia esatta della catena master (a `impl->sampleRate`, zero SRC post-limiter)

Poiché `AudioEngine::init` inizializza `ma_device` e `VoiceAllocator` direttamente a `impl->sampleRate = impl->device.sampleRate` (es. 48 kHz o 44.1 kHz), **non esiste alcuno stadio di sample-rate conversion (SRC) software a valle di `processBlock`**. L'ordine dei blocchi per ciascun frame nella callback `processBlock` è fissato come segue:

1. **8 Voice Slots** (`VoiceSlot[0..7]`, scala nominale $k_{\text{scale}} = 0.25\text{f}$ per voce, `stealTail` lineari di $\lfloor 0.002 f_s \rfloor$ campioni).
2. **4 Stereo Role Buses** (`Pulso` $-6.0\text{ dB}$, `Corpo` $-8.0\text{ dB}$ con micro-ducking $-3.0\text{ dB}$ pilotato dall'inviluppo di `Pulso` con $\tau_a = 2\text{ ms}, \tau_r = 120\text{ ms}$, `Trama` $-10.0\text{ dB}$ + micro-delay stereo, `Vettore` $-6.0\text{ dB}$) + estrazione telemetria per-bus (`ChannelFrame`: `energy`, `rms`, `transient`, `pitchHz`, `spectralCentroid`).
3. **Somma Music Bus.** Un soft-clipper opzionale è previsto solo come stadio timbrico futuro: non è implementato in M2.4 e non fa parte della protezione.
4. **Master Finite Guard:** verifica `std::isfinite(L)` e `std::isfinite(R)`; in caso di `NaN`/`Inf`, azzera immediatamente il campione (`0.0f`) e incrementa il contatore anomalie del limiter, sommato in `dspAnomalyCount` della telemetria (remediation 2026-10-20).
5. **Terminal `MasterLimiter` Zero-Latency ($L = 0$) con rilevamento ibrido True-Peak:**
   - **Motivazione matematica di $L = 0$ (Zero Latency):** Qualsiasi linea di ritardo lookahead ($L > 0$ campioni) sposterebbe tutti gli onset `NoteOn` sample-accurate di $+L$ campioni (violando i test di precisione al singolo campione di `test_scheduler.cpp`) e farebbe uscire campioni non nulli dal buffer di lookahead dopo il confine tassativo di silenzio $n_{\text{pause}} + N_{\text{tail}}$ (violando ADR-0003 D-M2-04.7 e il requisito che il limiter non riemetta code sonore oltre i boundary di silenzio). Pertanto il percorso diretto del segnale ha latenza **esattamente zero** ($y_{\text{lim}}[n] = g[n] \cdot x[n]$).
   - **Stima causale di picco intersample (Sidechain):** combina il picco campione, tre interpolatori frazionari causali a 3 coefficienti (posizioni −0,25, −0,50, −0,75 campioni, su $x[n], x[n-1], x[n-2]$) e uno stimatore di curvatura del primo e secondo ordine (pesi emendati il 2026-10-20 ai valori del codice):
     $$\widehat{p}_{\text{curv}}[n] = \sqrt{x[n]^2 + 0.65\,(x[n] - x[n-1])^2 + 0.35\,(x[n] - 2x[n-1] + x[n-2])^2}$$
     Il detector è un'euristica conservativa, non un interpolatore BS.1770: la sua efficacia è dimostrata solo empiricamente sui vettori di prova (§4.2).
     $$p_{\text{est}}[n] = \max\!\left(|x_L[n]|, |x_R[n]|, p_{\text{FIR},L}[n], p_{\text{FIR},R}[n], \widehat{p}_{\text{curv},L}[n], \widehat{p}_{\text{curv},R}[n]\right)$$
     Questo garantisce che anche sul primo campione di uno step improvviso o di una sinusoide a $f_s / 4$ sfasata di $\pi/4$ (che presenta $+3.01\text{ dB}$ di picco intersample e $\widehat{p}_{\text{curv}} \approx 1.304\,|x|$), la riduzione di guadagno scatti istantaneamente al campione $0$ senza lookahead.
   - **Soglie e balistica:** soglia interna $C_{\text{lim}} = -1{,}80\text{ dBFS}$ ($0{,}8128305\text{f}$; emendata da −1,50 dBFS il 2026-10-19 nella remediation Inc-2, quando i coefficienti del meter offline sono stati corretti), attacco istantaneo ($g_{\text{target}}[n] = \min(1, C_{\text{lim}} / \max(p_{\text{est}}[n], 10^{-12}))$), hold di 2 ms ($N_{\text{hold}} = \lfloor 0{,}002 f_s \rfloor$), rilascio one-pole di 50 ms ($g \leftarrow g + \alpha(1-g)$ con $\alpha = 1 - e^{-1/(0{,}050 f_s)}$), guardia anti-slew e hard ceiling $C_{\text{max}} = -1{,}0\text{ dBFS}$ ($0{,}89125\text{f}$). Il margine di 0,80 dB è **tarato empiricamente** sui vettori di §4.2: il clamp finale limita i picchi campione, non i picchi intersample, e nessun limiter a latenza zero garantisce $\le -1\text{ dBTP}$ per segnali arbitrari.
6. **Rampa fisica di de-click di trasporto e gate di silenzio (`Pause`/`Resume`/`Stop`):** nel codice è applicata dal `VoiceAllocator` **a monte** del limiter terminale (emendamento §4.3), su $\lfloor 0{,}002 f_s \rfloor$ frame (96 a 48 kHz, 88 a 44,1 kHz). Il silenzio numerico esatto da $n_{\text{device, pause}} + N_{\text{tail}}$ resta garantito perché il limiter non ha ritardo ($y = g\,x$ con $L = 0$) e il clamp mappa 0 in 0: nessuna coda può essere riemessa.
7. **Clamp finale $[-0{,}89125, +0{,}89125]$** (parte del limiter) e consegna a `pOutput` e al tap `AudioProbe`, che osserva così l'esatto PCM consegnato al device. Un volume utente di sola attenuazione non è ancora implementato; quando lo sarà, si applicherà prima del clamp.

### 4.2 Verifica indipendente offline del True Peak (dBTP) e segnali avversari

In `engine/audio/audio_probe.h/.cpp` la funzione indipendente `analysis::estimateTruePeakDbtp` usa il filtro FIR polifase di sovracampionamento 4× a 48 tap (4 fasi × 12) riportato nell'Annex 2 di ITU-R BS.1770-4/-5 (coefficienti verificati, simmetria $h[i] = h[47-i]$). È distinta dallo stimatore causale del limiter. Fuori dalla cattura il segnale è trattato come silenzio digitale (**zero padding**, emendamento 2026-10-20): l'estensione per replica dei bordi usata in precedenza nascondeva l'overshoot di ricostruzione di un gradino verso il silenzio.

La suite `T-ADV-01..06` è definita **solo** in `tests/unit/test_synth_dsp.cpp`: tono $f_s/4$ a fase $\pi/4$ sovrapilotato di +3,5 dB; DC a 2,0 (+6 dB); gradino alternato ±1,5; ingressi non finiti; burst a 22 kHz ×1,8; onset a latenza zero. Ogni vettore termina con la rampa di 2 ms che il motore applica a ogni fine flusso. I risultati sono **stime** del picco ricostruito sui vettori testati, non una garanzia per segnali arbitrari; l'oversampling 4× è esso stesso una stima (§5).

### 4.3 Emendamento post-Inc-3 (2026-10-20)

La revisione indipendente post-Inc-3 ha rilevato divergenze fra questo ADR e il codice. Decisioni registrate:

| Elemento | Prima | Ora (autoritativo) | Motivo |
|---|---|---|---|
| Soglia interna | −1,50 dBFS | −1,80 dBFS | Valore effettivo dalla remediation Inc-2, tarato sui vettori §4.2 dopo la correzione dei coefficienti del meter |
| Pesi di curvatura | 0,55 / 0,30 | 0,65 / 0,35 | Valori effettivi nel codice |
| Detector | «FIR polifase 4×12» | picco campione + 3 interpolatori a 3 coefficienti + curvatura | Descrizione dell'implementazione reale |
| Istanze di limiter | una nel `VoiceAllocator` e una in `AudioEngine` | **una sola**, terminale in `AudioEngine` | Doppia protezione non prevista, costo doppio, anomalie non contate |
| Rampa di trasporto | a valle del limiter | a monte (nel `VoiceAllocator`) | Equivalente per il silenzio grazie a $L = 0$; evita di spostare la gestione di trasporto nel motore |
| Meter offline | replica dei bordi | zero padding | La replica sopprimeva l'overshoot reale di un gradino verso il silenzio |
| Formulazione | «garantisce ≤ −1 dBTP» | «stima ≤ −1 dBTP sui vettori testati» | Il clamp limita solo i picchi campione |
| Steal tail | sommate dopo lo stadio di bus | iniettate nell'ingresso del bus di ruolo (guadagno, ducking, dry/wet di Trama); le code di Stop non alimentano il delay | Nessun salto di livello al furto |
| Guadagni di bus e wet di Trama | commutazione istantanea | smoothing one-pole 5 ms (§2.5), mantenuti quando il ruolo è inattivo | Click a ruolo inattivo con delay ancora attivo |
| Guadagno unitario per `timbreParam == 0` | implicito | mantenuto come profilo legacy Inc-1 (sonde e test storici), con smoothing | Compatibilità delle evidenze Inc-1 |
| Normalizzazione del ducking | piena profondità a picco 0,25 | piena profondità al picco nominale del bus Pulso a velocità piena (≈ 0,125) | Con il valore precedente il ducking massimo era ≈ 1,2 dB |
| Pausa | DSP completo su voci congelate | dopo la rampa e senza code attive l'uscita è silenzio senza aggiornare lo stato DSP; durante la rampa le fasi degli oscillatori avanzano | Stato DSP corrotto e gradino DC alla ripresa |
| Pulso | sweep da 2,5 f0, τ 15 ms, click di 16 campioni non filtrato | conforme a §2.1, con $f_{\text{start}} = \max(f_0, \min(3 f_0, 180\text{ Hz}))$ | Divergenza normativa; il `max` evita uno sweep verso l'alto per $f_0 > 180$ Hz |
| Reset del limiter | anche su `Stop` | solo su `Start` da fermo | Le code di Stop di 2 ms passano ancora nel limiter |
| Hook diagnostico | — | `RoleBusProcessor::setDelayBypassed` solo per le sonde CACP | Verifica differenziale del delay |

Gli invarianti di AUD-D-02 (protezione terminale comune, sempre attiva, fail-closed, $L = 0$) sono invariati.

**Limite della garanzia:** CYMATICA controlla il PCM nel proprio dominio; il guadagno OS, l'amplificatore, la sensibilità dei trasduttori e la dose acustica reale non sono controllabili tramite il solo limiter. È vietato dichiarare garantita l'assenza di danno uditivo/hardware.

## 5. AUD-D-03 — Monitoring progressivo e conformità da dimostrare

**Architettura approvata:** tre domini separati, con comportamento misurabile e senza dipendenze critiche dalla diagnostica.

1. **Callback audio:** finite guard e contatori/peak detection a costo costante **per campione**, trasporto di dati/campioni tramite ring SPSC preallocata e non bloccante quando la diagnostica è attiva. Il lavoro complessivo è bounded rispetto al massimo di frame per callback: non eseguire lì K-weighting, gating LUFS esteso, file export o aggregazioni non bounded.
2. **Worker diagnostico/CACP:** quando attivato, monitoraggio incrementale Momentary LUFS (400 ms) e Short-term LUFS (3 s), con canali e profili dichiarati. Se `AudioProbe` perde campioni o la cattura è incompleta, le metriche dipendenti da quei dati sono **INCONCLUSIVE**, mai dichiarate `PASS`.
3. **Analisi offline:** Integrated LUFS **gated** sull'intervallo della run e stima **indipendente** del true peak (dBTP, tramite FIR 48-tap ITU-R BS.1770-5 Annex 2); LRA, storia della gain reduction e rolling Integrated gated su finestre effettive di 30 minuti quando richieste da una prova di durata adeguata. Un oversampling 4× fornisce una **stima**, non una misura matematicamente esatta dei picchi ricostruiti.

**Profili iniziali sperimentali (non gate normativi):** valutare `−23 LUFS ±2 LU` sull'intera fixture A–B–A′ (~183 s) e `≤−1 dBTP` come punti di partenza, con risultati `MEASURED/INCONCLUSIVE`; **non** certificano il riferimento di ITU-T H.872, definito su **30 minuti**, né l'esposizione acustica in dBA. `−14 LUFS` non è un target di safe listening approvato. Il rapporto fra loudness e output del gioco sarà qualificato quando saranno disponibili mix completo e sessioni idonee, **senza assegnare ora una milestone numerica arbitraria**.

**Gate di evidenza:** i controlli tecnici di M2/M2.5 e `AC-AO-02` già obbligatori restano invariati; introdurre nuovi gate obbligatori LUFS/true-peak o criteri di conformità H.872 richiede successiva ratifica esplicita nella milestone che implementerà le capacità necessarie. Il monitoring è diagnostico/progressivo; non è ancora una feature runtime continua e obbligatoria per il giocatore.

## 6. Emendamenti e rapporto con ADR-0003

| Elemento | Effetto della ratifica | Stato |
|---|---|---|
| D-M2-02/03, `MusicClock`, scheduler e ABI | Nessun cambiamento | **Preservati** |
| D-M2-04, otto slot, steal tails, cross-epoch e matrice Pause/Stop | Nessun cambiamento; anche il limiter zero-latency li rispetta | **Preservati e implementati in Inc-1** |
| D-M2-06, punto 2 | DSP C++20 nativo puro autocontenuto qualificato per Inc-2 (zero dipendenze esterne; moduli terzi non necessari in Inc-2) | **Emendato e qualificato pre-Inc-2** |
| D-M2-06, punto 4 | Protezione terminale comune `MasterLimiter` zero-latency ($L=0$), soglia interna $-1{,}80\text{ dBFS}$ (§4.3), hard ceiling $-1{,}0\text{ dBFS}$, rampa di de-click a monte | **Emendato e qualificato pre-Inc-2** |
| D-M2-07, punto 1 e analisi master | Benchmark della **catena reale intera** con protezione e acquisizione bounded; stima true peak indipendente ITU-R BS.1770-5 Annex 2 | **Precisato**, target `≤2,0 ms` invariato |
| Inc-1 | Voice Allocator nativo, 8 slot statici, steal tails 2 ms, `voice_probe_v1` | **COMPLETATO (2026-10-19)** |
| Gate pre-Inc-2 | Qualifica DSP nativa pura (`AUD-D-01`) e topologia/algoritmo `MasterLimiter` zero-latency (`AUD-D-02`) | **CHIUSO (2026-10-19)** |
| Inc-2 | Implementazione sintesi 4 ruoli, 4 bus, ducking, `MasterLimiter` e `timbre_bus_probe_v1` | **Implementato** (remediation 2026-10-20; stato in progress.md) |
| Inc-4/Inc-5 | Evidence dossier e analisi esplorativa conformi al protocollo | **Nessun nuovo gate mandatory M2.5** |
| `docs/dependencies.md` | Nessuna nuova dipendenza esterna adottata (DSP Inc-2 100% nativo C++20) | **Invariato** |

Il documento ADR-0003 incorpora gli emendamenti ai punti 2 e 4 di D-M2-06 e la precisazione a D-M2-07 in modo che **non rimangano due versioni concorrenti del contratto**.

## 7. Alternative considerate e respinte per M2.4

- **Host plugin CLAP/VST3 primario:** tecnicamente possibile con componenti qualificati, ma non supportato da evidenze sufficienti a giustificarne ora lifecycle, distribuzione, gestione note, crash e portabilità. Nessuna pretesa di impossibilità tecnica universale.
- **Limiter con linea di ritardo lookahead ($L > 0$):** respinto per il master realtime di CYMATICA poiché introdurrebbe uno shift di $+L$ campioni sugli onset sample-accurate e violerebbe il boundary di silenzio $n_{\text{pause}} + N_{\text{tail}}$ svuotando il buffer di lookahead durante la pausa; adottato invece `MasterLimiter` zero-latency ($L = 0$) con rilevatore ibrido FIR polifase causale + curvatura di Nyquist e rampa di de-click fisica a valle.
- **Integrare intere librerie senza audit:** rifiutato per superficie di dipendenza/manutenzione non giustificata; per Inc-2 il DSP C++20 nativo puro autocontenuto soddisfa tutti i requisiti con zero dipendenze esterne.
- **Usare il solo soft-clipper come limiter terminale qualificato:** non sufficiente a dimostrare ceiling true-peak; mantenuto solo come stadio timbrico opzionale pre-limiter (in bypass di default).
- **Rendere `−14 LUFS`, `−4` o l'audit rolling di 30 minuti gate M2.4:** non giustificato né verificabile con la sola run di circa tre minuti.

## 8. Stato operativo e tracciabilità

- **Ratificati dal titolare (2026-10-15) e qualificati pre-Inc-2 (2026-10-19):** AUD-D-01 (DSP C++20 nativo puro per Inc-2), AUD-D-04, AUD-D-02 (topologia e algoritmo `MasterLimiter` zero-latency $L=0$ + verifica true-peak ITU-R BS.1770-5 Annex 2 offline), AUD-D-03 (architettura a tre domini e piano di evidenza).
- **Completati e verificati in codice (2026-10-19):** **Inc-1** (`VoiceAllocator`, 8 slot statici, voice stealing deterministico, steal tails 2 ms adattive al sample rate, `voice_probe_v1`, 91/91 test Release/Debug, 90/90 Headless).
- **Implementati:** Inc-2 (sintesi 4 ruoli, 4 bus, micro-ducking, `MasterLimiter`, `timbre_bus_probe_v1`) e Inc-3 (planner). Decisioni d'ingresso di Inc-4 in [ADR-0005](0005-m2-4-inc4-entry-decisions.md); stato corrente ed evidenze in [progress.md](../progress.md).
- **Differiti a milestone successive (M5+):** durata/tolleranze di gate rolling di 30 minuti e certificazione H.872.

## 9. Fonti tecniche e normative

- [ITU-T H.872 (2024), Safe listening for video gameplay and esports, §9](https://www.itu.int/rec/T-REC-H.872-202410-I/en).
- [ITU-R BS.1770-5 (2023)](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I); [EBU Tech 3341](https://tech.ebu.ch/publications/tech3341).
- [WHO/ITU, Global standard for safe listening video gameplay and esports](https://www.who.int/publications/i/item/9789240108028).
- [Zavalishin, V. (2018), The Art of VA Filter Design (rev. 2.1.0)](https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf); [Välimäki, V. & Huovilainen, A. (2007), Antialiasing Oscillators in Subtractive Synthesis, IEEE SPM](https://doi.org/10.1109/MSP.2007.323276).
- [CLAP `events.h`](https://github.com/free-audio/clap/blob/main/include/clap/events.h); [VST3 `ProcessContext`](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1ProcessContext.html).
- [Airwindows](https://github.com/airwindows/airwindows); [DaisySP MIT](https://github.com/electro-smith/DaisySP); [DaisySP-LGPL](https://github.com/daisyaudio/DaisySP-LGPL); [Surge XT](https://github.com/surge-synthesizer/surge).

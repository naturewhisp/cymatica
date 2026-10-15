# ADR-0004 — Renderer nativo, protezione terminale e loudness (ratifica M2.4)

- **Data di ratifica:** 2026-10-15.
- **Stato:** **accepted for implementation — decisioni architetturali approvate; implementazione NON avviata né validata**.
- **Owner decisionale:** titolare CYMATICA; approvazione esplicita dopo Grill-me e review indipendente del Decision Record del 2026-10-15.
- **Baseline documentale verificata:** `32f93250f8d18c080ab165ddb12d2b88499eab42` (`main`, commit *Document M2.4 audio research and safety review*).
- **Percorso:** il suffisso storico `-proposal.md` è conservato per non invalidare riferimenti già committati; lo **stato nel documento**, non il filename, è autoritativo.
- **Gerarchia:** Specifica > DESIGN > AGENTS > ADR > piano operativo > ricerca. Questo ADR integra [ADR-0003](0003-m2-decisions-contracts.md) nei punti esplicitati al §6; non altera M1, M2.3, ABI pubbliche o roadmap.
- **Evidenze:** [review renderer](../research/M2_4_Audio_Renderer_Research_Review.md), [studio safety](../research/M2_4_Audio_Safety_Design.md), [protocollo sperimentale](../research/M2_4_Evaluation_Protocol.md), [piano M2.4](../research/M2_4_Implementation_Plan.md) e Decision Record Grill-me approvato dal titolare. Sono evidenze **documentali**, non benchmark o misure PCM già effettuate.

## 1. Contesto e perimetro della ratifica

La ricerca comparativa raccomanda un renderer nativo C++20 circoscritto, con eventuale riuso di singole primitive DSP a licenza compatibile e qualificate; **non** un host CLAP/VST3 obbligatorio in M2.4. Il Grill-me ha accettato questa scelta e il principio di protezione terminale sul mix controllato dal gioco, mantenendo differita la scelta dell'algoritmo limiter e dei target numerici. Il monitoring LUFS/true peak è approvato come **architettura progressiva a tre domini**, non come garanzia di conformità acustica.

Il presente ADR **ratifica AUD-D-01, AUD-D-04, AUD-D-02 a livello di principio e AUD-D-03 a livello architetturale**. Le decisioni dichiarate differite ai §§3–5 sono gate locali del lavoro che ne dipende; non bloccano Inc-1 e non possono essere risolte implicitamente dall'implementatore.

## 2. AUD-D-01 — Renderer nativo e ammissione selettiva di DSP esterno

**Decisione approvata.** In M2.4, generazione audio e voice lifecycle rimangono **nativi C++20**, basati su `VoiceSlot[8]`, due slot per Pulso/Corpo/Trama/Vettore, quattro bus stereo preallocati, `MusicIntentEvent`, clock logico a 48 kHz, SPSC e vincoli di determinismo/realtime di ADR-0003. La pipeline v1 non richiede host plugin, librerie DSP monolitiche, sample bank esterne o file audio da caricare a runtime.

**Inc-1 è limitato al voice allocator/lifecycle nativo, senza codice DSP terzo.** In Inc-2 è **ammesso, ma non richiesto**, importare *singole primitive* open source compatibili (ad esempio oscillatori, filtri o effetti) soltanto dopo una qualifica esplicita, registrata **prima dell'uso**:

1. Provenienza, revisione/tag/SHA, licenza effettiva del modulo e dei suoi asset, notices e modalità di incorporazione; registrazione in `docs/dependencies.md` soltanto al momento dell'adozione.
2. Allocazioni, lock, I/O, logging, inizializzazione e lifecycle: nessuno di questi deve operare in modo non bounded o bloccante nella callback; memoria e stato preallocati.
3. Assenza di RNG globale implicito; stato casuale locale riproducibile quando necessario; controlli su NaN/Inf, denormali, stabilità filtri e parametri limite.
4. Build/toolchain Windows e fattibilità Android, benchmark **dell'intera callback** nel profilo D-M2-07 e prova PCM/ascolto a Music Intent costante.

Il progetto principale **DaisySP** è generalmente distribuito con licenza MIT; il repository distinto **DaisySP-LGPL** presenta condizioni proprie. Non attribuire automaticamente obblighi LGPL alla libreria MIT né ritenere qualsiasi modulo esente da audit. Airwindows è un possibile candidato MIT: nessun suo effetto viene approvato o adottato con questo ADR. Un DSP scritto internamente resta una scelta valida quando il riuso non offre un beneficio verificabile.

## 3. AUD-D-04 — Plugin hosting differito e unica autorità temporale

**Decisione approvata.** Nessun host CLAP/VST3, backend plugin obbligatorio o astrazione generica plugin/nativo fa parte degli incrementi **Inc-1..Inc-5** di M2.4. Non si afferma che ospitare plugin in realtime sia impossibile: la complessità non è giustificata dalle evidenze disponibili per questa milestone.

`MusicClock` (48 kHz logici), `SetTempoMap`, `MusicIntentEvent`, committed scheduling, semantica cross-epoch e regole `Pause`/`Resume`/`Stop` restano autoritativi e invariati. Nessun clock MIDI/software concorrente. CACP è **Control & Observation Plane** diagnostico e non partecipa alla generazione del transport realtime.

Se in futuro verrà autorizzato uno spike CLAP o VST3, sarà **separato dalla roadmap vincolante M2.4**, con host headless e adapter bounded che *proietta* il transport canonico, gestione esplicita di identità note 64→32 bit, versionamento stato/preset, latenza, lifecycle e test di affidabilità. Il confronto timbrico offline con Surge XT resta un'opzione, senza milestone futura assegnata automaticamente.

## 4. AUD-D-02 — Protezione digitale terminale comune

**Principio architetturale approvato e vincolante.** Tutte le sorgenti sonore **sotto il controllo di CYMATICA** confluiscono in uno stadio terminale comune di protezione digitale **sempre attivo e fail-closed** prima della consegna del PCM al backend audio. In M2.4 il requisito riguarda le sorgenti musicali realmente presenti; futuri SFX/UI dovranno essere aggiunti allo stesso percorso quando saranno implementati. La protezione non dipende da CACP, AudioProbe o disponibilità dei loro worker.

Invarianti approvati:

- Finite-value guard contro `NaN`/`Inf`, input/stati invalidi e guadagni fuori specifica, con bonifica o silenziamento deterministico e telemetria bounded.
- Gain staging conservativo a monte; il limiter **non** sostituisce la gestione della somma di bus e transienti.
- Processamento bounded per campione e per blocco: nessun heap, lock bloccante, log o I/O nella callback; strutture/lookahead, se approvati, con memoria statica/preallocata e latency dichiarata.
- Nessuno stadio di CYMATICA a valle della protezione può amplificare o trasformare il segnale **invalidando il ceiling effettivamente qualificato**. La collocazione rispetto a resampling, volume dell'utente, AudioProbe e formato consegnato al device deve essere specificata e verificata.
- `Pause` conserva il **freeze logico immediato** di D-M2-04 e la de-click tail fisica separata; `Stop` ed `Emergency Mute` mantengono le loro garanzie. Il limiter non può riemettere code sonore oltre i boundary di silenzio prescritti.
- La protezione, incluso ogni suo eventuale oversampling, deve rientrare nel **budget della callback completa** di D-M2-07 (target Release `≤2,0 ms` per 512 frame a 48 kHz): nessun budget aggiuntivo per il limiter.

**Gate pre-Inc-2 (decisione tecnica differita, non delega tacita):** prima di implementare il master occorre scegliere e registrare algoritmo (sample-peak, true-peak/lookahead o combinazione), ordine dei processori/pre-post SRC, latenza, comportamento su segnali avversari, ceiling di test e procedura di verifica indipendente. La scelta richiede almeno analisi tecnica, benchmark/prove mirate e approvazione del titolare quando introduce tradeoff normativi; non è necessario un ulteriore Grill-me generale.

Il **soft-clipper** previsto originariamente da D-M2-06 può essere conservato come trattamento timbrico/intermedio **se** qualificato, ma **non** costituisce da solo la protezione terminale nuova. Un clamp sample-peak `[-1,+1]` non dimostra la limitazione dei picchi intersample. Il valore **`−1 dBTP` è un obiettivo di prova iniziale**, non un ceiling normativo già approvato. Non incorporare l'indicazione ambigua `−4`.

**Limite della garanzia:** CYMATICA controlla il PCM nel proprio dominio; il guadagno OS, l'amplificatore, la sensibilità dei trasduttori e la dose acustica reale non sono controllabili tramite il solo limiter. È vietato dichiarare garantita l'assenza di danno uditivo/hardware.

## 5. AUD-D-03 — Monitoring progressivo e conformità da dimostrare

**Architettura approvata:** tre domini separati, con comportamento misurabile e senza dipendenze critiche dalla diagnostica.

1. **Callback audio:** finite guard e contatori/peak detection a costo costante **per campione**, trasporto di dati/campioni tramite ring SPSC preallocata e non bloccante quando la diagnostica è attiva. Il lavoro complessivo è bounded rispetto al massimo di frame per callback: non eseguire lì K-weighting, gating LUFS esteso, file export o aggregazioni non bounded.
2. **Worker diagnostico/CACP:** quando attivato, monitoraggio incrementale Momentary LUFS (400 ms) e Short-term LUFS (3 s), con canali e profili dichiarati. Se `AudioProbe` perde campioni o la cattura è incompleta, le metriche dipendenti da quei dati sono **INCONCLUSIVE**, mai dichiarate `PASS`.
3. **Analisi offline:** Integrated LUFS **gated** sull'intervallo della run e stima **indipendente** del true peak (dBTP, con algoritmo/metodo e accuratezza documentati); LRA, storia della gain reduction e rolling Integrated gated su finestre effettive di 30 minuti quando richieste da una prova di durata adeguata. Un oversampling 4× fornisce una **stima**, non una misura matematicamente esatta dei picchi ricostruiti.

**Profili iniziali sperimentali (non gate normativi):** valutare `−23 LUFS ±2 LU` sull'intera fixture A–B–A′ (~183 s) e `≤−1 dBTP` come punti di partenza, con risultati `MEASURED/INCONCLUSIVE`; **non** certificano il riferimento di ITU-T H.872, definito su **30 minuti**, né l'esposizione acustica in dBA. `−14 LUFS` non è un target di safe listening approvato. Il rapporto fra loudness e output del gioco sarà qualificato quando saranno disponibili mix completo e sessioni idonee, **senza assegnare ora una milestone numerica arbitraria**.

**Gate di evidenza:** i controlli tecnici di M2/M2.5 e `AC-AO-02` già obbligatori restano invariati; introdurre nuovi gate obbligatori LUFS/true-peak o criteri di conformità H.872 richiede successiva ratifica esplicita nella milestone che implementerà le capacità necessarie. Il monitoring è diagnostico/progressivo; non è ancora una feature runtime continua e obbligatoria per il giocatore.

## 6. Emendamenti e rapporto con ADR-0003

| Elemento | Effetto della ratifica | Stato |
|---|---|---|
| D-M2-02/03, `MusicClock`, scheduler e ABI | Nessun cambiamento | **Preservati** |
| D-M2-04, otto slot, steal tails, cross-epoch e matrice Pause/Stop | Nessun cambiamento; anche il limiter deve rispettarli | **Preservati** |
| D-M2-06, punto 2 | Consentire **moduli DSP permissivi selettivi e auditati** incorporati nel renderer C++20; nessun sample asset esterno richiesto | **Emendato esplicitamente** |
| D-M2-06, punto 4 | Sostituire il requisito che il solo soft-clipper sia la protezione terminale con **master protection comune, bounded e fail-closed**; tecnologia/ceiling prima di Inc-2 | **Emendato esplicitamente** |
| D-M2-07, punto 1 e analisi master | Benchmark della **catena reale intera** con protezione e acquisizione bounded; stima true peak indipendente | **Precisato**, target `≤2,0 ms` invariato |
| Inc-1 | Voice Allocator nativo, indipendente da modulo DSP terzo, limiter e LUFS | **Sbloccabile su autorizzazione** |
| Inc-2 | Qualifica locale di DSP opzionali e **scelta tecnica del limiter prima del codice master** | **Gate tecnico futuro** |
| Inc-4/Inc-5 | Evidence dossier e analisi esplorativa conformi al protocollo | **Nessun nuovo gate mandatory M2.5** |
| `docs/dependencies.md` | Nessuna nuova dipendenza effettivamente adottata | **Invariato** |

Il documento ADR-0003 deve incorporare gli emendamenti ai punti 2 e 4 di D-M2-06 e la precisazione a D-M2-07 in modo che **non rimangano due versioni concorrenti del contratto**. Se una decisione differita cambia ABI, tempi di silenzio, voice budget, profilo numerico o gate mandatory, avviare una revisione normativa circoscritta prima del codice interessato.

## 7. Alternative considerate e respinte per M2.4

- **Host plugin CLAP/VST3 primario:** tecnicamente possibile con componenti qualificati, ma non supportato da evidenze sufficienti a giustificarne ora lifecycle, distribuzione, gestione note, crash e portabilità. Nessuna pretesa di impossibilità tecnica universale.
- **Riscrivere obbligatoriamente ogni oscillatore/filtro/effetto:** non dimostra vantaggio artistico; accettato riuso selettivo qualificato come opzione.
- **Integrare intere librerie senza audit:** rifiutato per superficie di dipendenza/manutenzione non giustificata; non attribuire automaticamente GPL/LGPL a moduli MIT.
- **Usare il solo soft-clipper come limiter terminale qualificato:** non sufficiente a dimostrare ceiling true-peak; possibile funzione artistica ma non prova di protezione digitale equivalente.
- **Rendere `−14 LUFS`, `−4` o l'audit rolling di 30 minuti gate M2.4:** non giustificato né verificabile con la sola run di circa tre minuti.

## 8. Stato operativo e tracciabilità

- **Ratificati dal titolare (2026-10-15):** AUD-D-01, AUD-D-04, AUD-D-02 **come principio**, AUD-D-03 **come architettura e piano di evidenza**.
- **Non ancora scelti/approvati:** identità di librerie/effetti terzi, algoritmo limiter e posizione nel grafo rispetto al SRC, latency/lookahead, ceiling numerico obbligatorio, durata/tolleranze di gate rolling, conformità H.872.
- **Non implementati:** limiter nuovo, metriche LUFS, host/plugin e integrazione di DSP terzi. Questo ADR non certifica alcuna misura realtime o PCM.
- **Sviluppo:** la ratifica **non costituisce autorizzazione all'avvio di Inc-1**, da richiedere separatamente.

## 9. Fonti tecniche e normative

- [ITU-T H.872 (2024), Safe listening for video gameplay and esports, §9](https://www.itu.int/rec/T-REC-H.872-202410-I/en).
- [ITU-R BS.1770-5 (2023)](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I); [EBU Tech 3341](https://tech.ebu.ch/publications/tech3341).
- [WHO/ITU, Global standard for safe listening video gameplay and esports](https://www.who.int/publications/i/item/9789240108028).
- [CLAP `events.h`](https://github.com/free-audio/clap/blob/main/include/clap/events.h); [VST3 `ProcessContext`](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1ProcessContext.html).
- [Airwindows](https://github.com/airwindows/airwindows); [DaisySP MIT](https://github.com/electro-smith/DaisySP); [DaisySP-LGPL](https://github.com/daisyaudio/DaisySP-LGPL); [Surge XT](https://github.com/surge-synthesizer/surge).

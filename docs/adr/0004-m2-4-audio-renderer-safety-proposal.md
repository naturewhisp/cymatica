# ADR-0004 (PROPOSTA) — Renderer DSP, protezione master e loudness per M2.4

- **Data:** 2026-10-15.
- **Stato:** **PROPOSED — NON APPROVATO, NON NORMATIVO, NON IMPLEMENTATO**.
- **Owner decisionale:** titolare CYMATICA; review indipendente richiesta per ogni emendamento a contratti ratificati.
- **Baseline:** `090dc2a45cf3785a2055c492415f9151321ea18a`.
- **Riferimenti:** [ADR-0003 D-M2-04/06/07](0003-m2-decisions-contracts.md); [ricerca revisionata](../research/M2_4_Audio_Renderer_Research_Review.md); [safe-listening design](../research/M2_4_Audio_Safety_Design.md); [piano](../research/M2_4_Implementation_Plan.md); [progress](../progress.md).

> **Vincolo sulla precedenza:** questo documento espone scelte da approvare. Non crea un secondo contratto operativo concorrente con ADR-0003. Fino a un'approvazione esplicita, D-M2-06 e D-M2-07 e i gate della specifica rimangono vigenti come scritti. Una ratifica deve indicare precisamente quali punti dell'ADR-0003 siano confermati/emendati e adeguare piano/protocollo di verifica prima del codice dipendente.

## 1. Contesto

La Deep Research completa, integrata con le verifiche ITU/EBU e delle licenze, preferisce per M2.4 un piccolo renderer nativo C++20, con possibile riuso di algoritmi DSP open source auditati. Non trova elementi sufficienti per imporre un host CLAP o VST3, che aumenterebbe le responsabilità di lifecycle, distribuzione, latency, note identity e compatibilità senza eliminare mixer, telemetry e safe output.

Il titolare ha inoltre espresso una esigenza distinta: limiter terminale digitale su **tutte** le sorgenti del gioco e monitoraggio continuo LUFS/peak. ADR-0003 D-M2-06 al momento prescrive un **soft-clipper bounded** con protezione dei valori campionati e analisi *offline* del true peak, ma non un limiter true-peak obbligatorio. È quindi necessario un atto deliberativo, evitando di spacciare la sicurezza proposta per una feature già accettata o realizzata.

## 2. Decisione proposta AUD-D-01 — Renderer nativo con DSP selettivo

**Proposta:** confermare il renderer nativo C++20 a 8 slot e 4 bus di D-M2-04/D-M2-06, consentendo il riuso di *singole primitive* da sorgenti permissive dopo qualifica documentata. Nessun framework, plugin, sample bank o effetto esterno è dipendenza obbligatoria di M2.4.

**Prerequisiti per ammettere un modulo:** sorgente/revisione pin, licenza e notices, numerica finita e denormali, bound di memoria/CPU, nessun lock o heap in callback, stato RNG locale deterministico, smoothing, portabilità Windows/Android, profilo di test e benchmark su catena completa. Registrare in `docs/dependencies.md` all'adozione effettiva.

**Non implica:** scegliere DaisySP o Airwindows oggi, né integrare un'intera libreria per ragioni reputazionali. Surge XT/CLAP resta *strumento potenziale* di ascolto/A-B fuori dal runtime obbligatorio.

**Conseguenze:** non cambia l'ABI `MusicIntentEvent` né il significato di MusicClock, seed/epoch, committed horizon e SPSC. L'implementazione di Inc-1 resta basata su voice allocator nativo.

## 3. Decisione proposta AUD-D-02 — Master protection terminale

**Proposta:** tutte le sorgenti sonore sotto controllo CYMATICA (musica e successivamente SFX/UI) devono attraversare, prima dell'uscita device, una protezione terminale digitale **sempre attiva e fail-closed**. Il limiter non deve dipendere dal consumo di CACP/AudioProbe per continuare a funzionare. Deve essere implementabile in maniera bounded e senza operazioni bloccanti nella callback.

### Requisiti da valutare prima della ratifica

1. Sede esatta nel grafo e rapporto con il resampling: nessuna amplificazione a valle che invalidi il ceiling; misurare l'uscita effettiva del dispositivo.
2. Gestione di NaN/Inf, segnale fuori range, burst di voci, cambi rapidi di gain, pause/stop, underrun e device changes.
3. Differenza fra simple sample-peak clamp e **limiter true-peak** (ricostruzione intersample, oversampling e possibile lookahead); il contratto finale non deve confonderli.
4. Latency introdotta nota e contabilizzata nel rendering/compensazione, non correggendo `MusicClock`.
5. Obbligo del target D-M2-07: **<=2.0 ms sulla callback intera** nel profilo Release di 512 frame/48 kHz; il limiter non ottiene budget aggiuntivo.
6. Verifica tramite meter *indipendente*, PCM registrato dopo gli stadi finali, casistica patologica e ascolto umano per effetti collaterali (pumping/distorsione).

**Numeri da ratificare:** `−1 dBTP` è proposto come riferimento coerente con H.872 §9.3.1, ma non è ancora una costante normativa CYMATICA. Il limiter specifico (sample-peak oppure true-peak lookahead) e la sua latenza non sono stati selezionati né misurati. L'indicazione ambigua `−4` non viene recepita.

**Relazione con D-M2-06:** se AUD-D-02 viene approvata, *emendare formalmente il punto 4* (protezione master) e il relativo profilo di test; non aggiungere un secondo limiter incoerente in parallelo al soft-clipper esistente. L'ordine di priorità fra stadi resta un unico grafo audio.

## 4. Decisione proposta AUD-D-03 — Loudness e audit safe-listening

**Proposta:** monitoraggio diagnostico e validazione su **Momentary LUFS (400 ms), Short-term LUFS (3 s), Integrated LUFS gated dell'intervallo, rolling 30 min gated, sample peak dBFS, true peak dBTP e gain reduction** secondo gli standard pertinenti; distinguere misure durante la run da controlli offline e da eventuali future impostazioni accessibili al giocatore.

**Aspetti invarianti:** calcoli complessi di gating, export e aggregazioni estese NON nella callback audio. Gli acquisitori devono essere bounded; perdita di campioni o intervalli rende la metrica **inconclusiva**, non PASS. Limitazione digitale non equivale a misurazione della dose acustica.

**Riferimento non automaticamente ratificato:** ITU-T H.872 §9.3.1 per output scaling videogame, 30 min circa `−23 LUFS ±2 LU` e `≤−1 dBTP`, con eccezioni e funzioni di sicurezza ulteriori. Nessuna equivalenza con target di distribuzione musicale come `−14 LUFS`, né con SPL o dBA non misurati.

**Fasi proposte:** Inc-2 mette a punto protezione e campioni avversari; Inc-4/Inc-5 acquisiscono evidenze di run di ~183 s e misure EBU; test reali su 30 minuti, mix SFX/UI e profili di device quando tali feature vengono effettivamente sviluppate, prima di dichiarare conformità H.872 per la piattaforma target. Il gate M2.5 resta invariato finché la modifica normativa non sia approvata.

## 5. Decisione proposta AUD-D-04 — Plugin hosting eventuale e clock

Non introdurre oggi un host CLAP/VST3 come deliverable di M2.4. Un eventuale spike futuro deve essere separato e approvato, con MIDI note identity bounded, plugin/preset versioning, lifecycle e audio graph state.

Qualora venga autorizzato, il transport del plugin è una **proiezione** del `MusicClock` autoritativo 48 kHz: CLAP ammette eventi transport sample-offset, VST3 gestisce il tempo tramite `ProcessContext` per processing block (sub-block ai boundary se richiesto). Nessun nuovo MIDI Clock interno né modifica di `MusicIntentEvent`/`SetTempoMap` per ragioni di mera compatibilità plugin. Sidechain audio e onset simbolici restano canali distinti; CACP è Control & Observation Plane, non time master.

## 6. Conseguenze sugli incrementi e sulle decisioni già ratificate

| Ambito | Conseguenza proposta | Richiede ratifica? |
|---|---|---|
| M1/M2.3, `MusicClock`, `MusicIntentEvent` | Nessuna modifica | No |
| D-M2-04, VoiceSlot[8] e lifecycle | Confermati | No |
| D-M2-06, sintesi C++20 | Resta nativa; ammissione selettiva di algoritmi terzi con licenze idonee | **Sì**, se si emenda il perimetro della sintesi pura vigente |
| D-M2-06, master | Protezione terminale oltre il solo soft-clipper; ceiling/profilo da fissare | **Sì** |
| D-M2-07, test/profiling | Includere limiter/monitoring quando introdotti e qualificare il nuovo budget | **Sì** per nuovi gate mandatory |
| `docs/dependencies.md` | Nessuna dipendenza attiva aggiunta da questo ADR | Solo con nuova adozione concreta |
| Piano Inc-1..5 | Sequenza invariata; dipendenza decisionale su protezione prima di Inc-2 | **Sì** solo se modifica i criteri vincolanti |
| CACP / AC-AO-02 | Verifica tecnica e percettiva separata; dati di safety in analisi | Per cambi di schema pubblico/gate sì |
| SFX/UI e sessioni 30 min | Prove future al momento della feature reale | Decisione futura |

## 7. Alternative respinte per questa fase

- **Host Surge XT/CLAP/VST3 obbligatorio:** costi e incertezza sul vincolo realtime, voice budget effettivo, portabilità e distribuzione; nessun benchmark di vantaggio sufficiente.
- **Intero DSP scritto necessariamente da zero:** costo di manutenzione aggiuntivo senza beneficio automatico; l'audit di moduli aperti può essere preferibile.
- **Assenza di master limiter oltre al clipper:** economica ma non dimostra ceiling true-peak sul mix complessivo; la scelta è accettabile solo se il titolare decide esplicitamente di non ampliare il requisito, senza promessa di safe listening.
- **`−14 LUFS` o `−4` imposti a priori:** definizioni e contesto inadeguati; non sono limiti di sicurezza derivati da H.872.

## 8. Stato, follow-up e approvazioni

- **AUD-D-01:** ☐ Approvare renderer nativo con DSP esterno modulare selettivo e auditato (senza scegliere alcuna libreria ora).
- **AUD-D-02:** ☐ Approvare il principio di limiter terminale obbligatorio, e incaricare Inc-2 di qualificare il tipo/ceiling prima di costruire il master definitivo.
- **AUD-D-03:** ☐ Approvare monitoring LUFS/true peak progressivo, con vincoli mandatory e durata definiti in una successiva revisione normativa; registrare H.872 come riferimento di progettazione.
- **AUD-D-04:** ☐ Confermare host plugin solo opzionale/sperimentale per M2.4 e preservare `MusicClock`/CACP.

**Nessuna casella è spuntata da questa review.** In seguito all'approvazione del titolare, produrre una versione ratificata che indichi data, motivazione, owner, contratti ADR-0003 emendati, nuovi criteri di accettazione e review indipendente coerente. Finché ciò non accade, **questo ADR-0004 non autorizza implementazione nuova né altera l'ambito di Inc-1**.

## 9. Fonti normative e tecniche

- [ITU-T H.872 (2024), §9](https://www.itu.int/rec/T-REC-H.872-202410-I/en).
- [ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I).
- [EBU Tech 3341](https://tech.ebu.ch/publications/tech3341).
- [CLAP event/transport API](https://github.com/free-audio/clap/blob/main/include/clap/events.h).
- [VST3 ProcessContext](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1ProcessContext.html).
- [Airwindows MIT](https://github.com/airwindows/airwindows/blob/master/LICENSE); [DaisySP MIT/LGPL](https://github.com/daisyaudio/DaisySP/releases); [Surge XT GPL](https://github.com/surge-synthesizer/surge).

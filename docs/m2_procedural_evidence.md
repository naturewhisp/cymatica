# M2 — dossier delle evidenze procedurali e handoff M2.5

- **Data:** 2026-11-03
- **Fase:** M2.4 Inc-5, sottofase 5e.4b (ADR-0006 5e-10)
- **Riferimenti di build:**
  - codice valutato a `18366dc` (5e.4a chiusa, [CI run 41655772732](https://github.com/naturewhisp/cymatica/actions/runs/41655772732) verde);
  - suite locale eseguita su `31b6c43` più il working tree di 5e.4a (manifest archiviati), con verdetti identici dopo la remediation.
- **Status:** documento di evidenza, **non normativo**. Non dichiara M2 completa e non introduce nuove affermazioni di validazione percettiva o statistica; la sessione alla cieca è rinviata a M2.5 (ADR-0006 «5e.3c»). L'autorità sui criteri resta la specifica §32, «Sottofasi operative M2», insieme ad ADR-0003…0006.

## 1. Come leggere le evidenze

| Classe | Significato | Esempi |
|---|---|---|
| **Vincolante** | controllo PASS/FAIL ripetibile; un FAIL blocca | catena O3, golden di trace, piano e PCM, determinismo, `procedural_v1`, `full_m2_suite` |
| **Misura** | valore registrato su dati validi (`MEASURED`), mai un gate | onset per ruolo, LUFS, true peak, presenza, diversità |
| **Esplorativo** | esito di un'ipotesi o limite dichiarato, mai un gate | H1/H2/R2 di 5e.3, M-NORM di 5e.3b, `NULL_DIVERSITY`, specificità `INSUFFICIENT_DATA` |
| **Percettivo** | giudizio umano o d'agente sull'ascolto | ascolto dell'esperto (2026-10-28 e fine Inc-5), sessione alla cieca (rinviata a M2.5) |

Le misure provano che un evento previsto è arrivato al segnale, non che sia udibile o musicalmente riuscito: l'intent non è percezione (dossier sui metodi CACP §8).

## 2. Evidenze di Inc-5

| Scenario | Classe | Esito | Manifest archiviato |
|---|---|---|---|
| `full_m2_suite` (15 scenari, gate locale Release, mai in CI) | vincolante | **PASS**, 202 s | [inc5_full_m2_suite](research/experiments/inc5_full_m2_suite/README.md) |
| `evaluation_check_selftest_v1` (14 casi su v1, v2 e v3) | vincolante | **PASS** | idem |
| `procedural_v1` (render della fixture e tutte le misure) | vincolante + misure | **PASS** | rigenerabile con `--cacp-run procedural_v1` |
| `acao02_v1` (calibrazione e verifica degli onset per ruolo) | misura | `MEASURED` | [inc5_calibration](research/experiments/inc5_calibration/README.md) |
| `release_ab_v1` (release 2–80 ms) | misura + esplorativo | `MEASURED`; H1 `NOT_SUPPORTED`, H2 `SUPPORTED`, R2 short-IOI `NOT_SUPPORTED` | [inc5_release_ab](research/experiments/inc5_release_ab/README.md) |
| `trama_miss_diagnosis_v1` | misura + esplorativo | `MEASURED`; M-NORM `SUPPORTED` (retrospettivo sul set B) | [inc5_trama_diagnosis](research/experiments/inc5_trama_diagnosis/README.md) |
| `evaluation_pack_v1`, `v2` e `v3` | pacchetti | `GENERATED`, ripetizione identica | [v2](research/experiments/inc5_evaluation_pack_v2/README.md), [v3](research/experiments/inc5_evaluation_pack_v3/README.md) |
| Modello matematico del motore (indagini A e B, riconciliazione) | analisi | rilievi CL-MM, AG-MM e R-01..07 | [riconciliazione](research/M2_4_Inc5_Math_Model_Reconciliation.md) |

Alle evidenze di Inc-5 si aggiungono quelle di Inc-1..Inc-4, registrate in [progress.md](progress.md) e negli ADR-0003…0005: sonde del tono, voci, timbro, planner, fixture e profilo.

### 2.1 Misure principali della fixture `fixture-m2-4-v1` (seed 1337, `procedural_v1`)

**Durata e struttura:**
- 8.777.142 frame di contenuto, cioè 182,857 s; 8.825.200 frame resi;
- forma A (Re dorico, 120 BPM) – B (Sol eolio, 140 BPM) – A' (Re dorico, 120 BPM), 4/4, 96 battute.

**Catena O3:** 2.784 eventi attesi, inviati, consumati e applicati; 0 scartati, 0 ignorati, 0 discrepanze.

**Golden:** trace, piano e PCM PASS (hash del PCM 15126969907236845529); determinismo PASS; JIT = offline.

**Loudness esplorativa** (BS.1770-5 / EBU Tech 3341, mai un gate):

| Porzione | Integrated | Momentary max | True peak |
|---|---|---|---|
| Brano | −21,25 LUFS | −17,90 LUFS | −10,45 dBTP |
| A | −21,15 LUFS | −17,90 LUFS | −10,45 dBTP |
| B | −21,27 LUFS | −20,10 LUFS | −11,68 dBTP |
| A' | −21,35 LUFS | −20,12 LUFS | −11,07 dBTP |

Riduzione del limiter 0 dB; prova −23 ± 2 LU: −21,25 − (−23) = **+1,75 LU**, dentro la tolleranza di prova (rettifica OSS-5e.4b-01: la prima versione indicava per errore +2,25 LU).

**Onset per ruolo** (±50 ms, profilo congelato `stem_onset_profile_v1`):

| Ruolo | Riferimenti | Precisione | Richiamo | Precisione senza rilasci | Rilevazioni sui rilasci |
|---|---|---|---|---|---|
| Pulso | 416 | 1,000 | 1,000 | 1,000 | 0 |
| Corpo | 96 | 0,990 | 0,990 | 1,000 | 1 |
| Trama | 400 | **0,500** | 1,000 | 1,000 | **400** |
| Vettore | 384 | 0,997 | 0,997 | 1,000 | 1 |

**Ricostruzione degli stem:** bit-exact (0 campioni discordanti).

**Diversità fra seed** (1..64): `NULL_DIVERSITY`. È strutturale: il seed agisce solo sui pareggi e se ne misurano 0 (Teorema D, condizione sufficiente).

**Gruppi musicali:** 14 con la regola a frame (v2), 8 con quella simbolica (v3).

## 3. Mappatura sui criteri di M2.5 (specifica §32)

| Criterio di M2.5 | Evidenza disponibile | Stato | Cosa resta per M2.5 |
|---|---|---|---|
| **AC-AO-01A** (mandatory, specifica §31.5 e §32): controllo del tono via runner e via input fisico, tap PCM/WAV, verifica quantitativa 220 → 440 → 220 Hz, manifest, gap/overflow e RT safety, prova SPACE dell'OS distinta dallo smoke sintetico | Evidenza storica di M2.2 in [m2_tone_probe_evidence.md](m2_tone_probe_evidence.md) §1.1, esito **PASS**, prova fisica SPACE distinta dallo smoke (DIF-M2-05); `tone_test` PASS in ogni CI ordinaria e in `full_m2_suite` | **PASS storico** (M2.2), con regressione automatica per la parte offline | Riverifica integrata al gate M2.5 sulla build finale, compresa la prova fisica SPACE con tastiera: la CI copre solo la parte offline |
| **AC-AO-01B** (supplementare, dipendente dalle capacità dell'agente; **non** un gate mandatory): audio consegnato al client e giudizio d'agente registrato | Evidenza storica di M2.2 in [m2_tone_probe_evidence.md](m2_tone_probe_evidence.md) §1.2, esito PASS con l'artefatto tracciato per hash | **PASS storico**, supplementare | Facoltativo; l'agente di Inc-5 non ha un canale audio |
| Tre minuti di musica continua senza crackle | Render continuo di 182,857 s (`procedural_v1`); 0 frame persi, 0 clipping, guardia sui non finiti, silenzio esatto dopo lo Stop, golden PCM | **evidenza tecnica disponibile** | «senza crackle» è anche percettivo: l'[ascolto del 2026-11-03](m2_4_human_listening_2026-11-03.md) sul WAV offline non rileva click né artefatti, ma resta la verifica sul percorso realtime (device) e sulla build finale |
| Due sezioni distinguibili con transizione tempo/metrica | Sezioni A, B e A' con cambio di tempo 120 → 140 → 120 BPM, modo e motivo diversi; il cambio di metro è supportato e testato in `MusicClock` (`test_music_clock.cpp`) | **parziale** | La fixture non contiene un cambio di **metro** (tutto 4/4). La «distinguibilità» è percettiva: l'esperto (2026-11-03) giudica le sezioni tutte distinguibili, con A→B «molto brusca» e B→A′ «più fluida»; va riconfermata sulla build finale. Le transizioni sono istantanee (F3) |
| Sincronismo intent/audio | Catena O3 esatta (2.784/2.784); onset per ruolo `MEASURED` su stem con profilo congelato | **evidenza tecnica disponibile** | AC-AO-02: ratificare le soglie proposte e la scelta fra precisione grezza e precisione senza rilasci (§4) |
| Trace deterministico | Golden di trace, piano e PCM; JIT = offline; determinismo su due esecuzioni; riproducibilità dei pacchetti al byte | **evidenza tecnica disponibile** | Valida per la toolchain fissata (ADR-0001) |
| Callback che esegue soltanto | `profile_test` PASS (Release, gate dei tempi); test di zero allocazioni nella callback; planner e analisi fuori dalla callback; sink e log offline installati solo in quiescenza | **evidenza tecnica disponibile** | Verifica sul device reale con la build Release del gate finale |
| AC-AO-02 (definizione normativa in [agent_control_observability](agent_control_observability.md), tabella delle estensioni): elementi sotto | | | |
| — cattura bounded | `AudioProbe` e registro degli eventi applicati preallocati; `droppedFrames == 0` e catena O3 esatta in `fixture_test` | **evidenza tecnica disponibile** | Ripetere sul percorso realtime (device) |
| — overflow e lifecycle testati | Overflow vincolante del registro O3 (overflow > 0 dà FAIL) e installazione solo in quiescenza (`test_event_chain.cpp`, «Applied-event log: dispositions, drop reasons, overflow and quiescence»); overflow e lifecycle della probe in AC-AO-01A (M2.2) | **evidenza tecnica disponibile** | Nessuna oltre al device |
| — onset correlati all'intent | O3 PASS; precisione e richiamo per ruolo `MEASURED` sugli stem; soglie **proposte** dalla calibrazione 5c | **proposto, non ratificato** | Ratifica delle soglie al gate (§4, H-01/H-02) |
| — l'adapter dichiara capacità e limiti | `evaluation_result_v0`: il checker rende inammissibile un risultato senza valutatore, canale (`audio_channels`, `tokens_per_s`) e accessi dichiarati (D-M2-15, 5d) | **contratto e checker disponibili**; nessuna sessione reale (rinviata) | Sessione alla cieca di M2.5 |
| Build, configurazioni e review indipendente (AGENTS §5.1) | Release, Debug e Headless con 0 warning; CI ordinaria e completa; review indipendente per ogni sottofase di Inc-5 | **in corso** | Il gate finale M2.5 con la sua review iterativa |
| Prova d'ascolto e confronto (specifica §32, raccordo con la ricerca) | Ascolto dell'esperto del 2026-10-28 (F1–F4); [ascolto di fine Inc-5 del 2026-11-03](m2_4_human_listening_2026-11-03.md), sul [pacchetto](m2_expert_listening_package.md) | **eseguita e registrata** per M2.4 | Non è un gate mandatory senza decisione esplicita. L'ascolto formale resta al gate M2.5, sulla build finale |

## 4. Handoff M2.5

### 4.1 Decisioni da prendere al gate

| ID | Decisione | Fonte |
|---|---|---|
| H-01 | Ratifica delle soglie AC-AO-02 proposte (richiamo / precisione grezza): Pulso 0,90/0,95, Corpo 0,75/0,50, Trama 0,70/0,40, Vettore 0,90/0,45 | [calibrazione](research/M2_4_Inc5_AC-AO-02_Calibration.md) §3.5 |
| H-02 | Precisione grezza contro precisione senza rilevazioni sui rilasci (1,000 in tutti gli insiemi; soglia proposta 0,95 per ruolo) | idem; 5e.3, 5e.3b |
| H-03 | Release dei NoteOff per ruolo (F6): sulla base di 5e.3 (pendenze, rilevazioni) e dell'ascolto A/B dell'esperto, che il 2026-11-03 non ha rilevato differenze significative fra 2 e 80 ms né espresso preferenze; nessun valore è adottato in Inc-5 | [release A/B](research/experiments/inc5_release_ab/README.md), dossier A §6.4 |
| H-04 | Rivelatore: normalizzazione globale (M-NORM) contro normalizzazione locale o robusta in un `descriptors_v2`; ricalibrazione di δ dopo H-03 | 5e.3b, riconciliazione R-03 |
| H-05 | Statistica di presenza: dipendenza fra gruppi con materiale imparentato (CL-MM-10), test con gruppi condivisi fra sezioni | riconciliazione R-01, review di `794b642` |

### 4.2 Questioni musicali F1–F6 (dall'ascolto esperto e dalle misure)

| ID | Questione | Collegamento al modello |
|---|---|---|
| F1 | Pulso e metro: il kick sul tempo forte solo in 16 battute su 96 | rotazione euclidea per indice di candidato (dossier A §3.3) |
| F2 | Trama lateralizzata a sinistra (precedenza, delay 11/17 ms) | slot 4 sempre preferito, guadagni 1 : 0,5 e tap sinistro in anticipo di 6 ms (AG-MM-12, confermato analiticamente) |
| F3 | Ripetitività e transizioni brusche | periodo di 1 battuta del materiale, punteggi uniformi nella sezione (CL-MM-03/05), cambi di tempo istantanei |
| F4 | Mix e timbro (kick basso, FM «dissonante») | costanti di sintesi e di bus (dossier A §6) |
| F5 | «Direttore musicale»: stato sopra la frase | assenza di stato lungo la sezione, varietà nulla dal seed (Teorema D) |
| F6 | Rilascio di 2 ms | ρ = 6,5 per la Trama (dossier A §6.4), 5e.3, 5e.3b |

**Dopo l'[ascolto del 2026-11-03](m2_4_human_listening_2026-11-03.md)** (stesso PCM del 2026-10-28, proposte e non decisioni):
- F2, F3 ed F5 sono confermati;
- F4 è rafforzato e misurato: Pulso −10,6…−12,4 dB rispetto al mix, Corpo e Vettore circa −3 dB;
- F1 e F6 sono ridimensionati: il metro si percepisce, e fra i rilasci non emergono differenze significative né click;
- F1 e F4 vanno valutati insieme, perché alzare il kick rende più evidente il suo spostamento.

Priorità aggiornate: [proposta per l'ingresso di M2.5](research/M2_5_Entry_Music_Quality_Proposal.md) §8.

### 4.3 Profili non validati (accettati per l'implementazione, da valutare)

- `kPlannerScoringProfileV1` («experimental, NOT validated»): pesi del punteggio del planner.
- `stem_onset_profile_v1`: δ e offset per ruolo. Il δ della Trama (0,20) è calibrato in presenza dei transitori di 2 ms.
- `presence_profile_v1` e `presence_stats_v1`: soglie di presenza (5d-03); modello con ρ = 1 dentro il gruppo e indipendenza fra gruppi.
- `PlayerProfile_v1`: tuning per M5/M7 (non M2).

### 4.4 Lavoro rinviato a M2.5

- **Sessione alla cieca di Antigravity**, sensibilità e specificità: richiede materiale più vario e almeno 4 gruppi negativi indipendenti. `evaluation_pack_v3` è pronto, da rigenerare con un seed nuovo e non registrato.
- **Ablazione** (5e-07) e **direttore musicale** (F5).
- **Staging transazionale** della scrittura dei pacchetti su più directory (OSS-5e.2-R2-01).
- **Qualificazione realtime su device** (ADR-0005 D-M2-10, fuori da Inc-4): corsa WASAPI ≥ 180 s con periodo effettivo, MMCSS e conteggio degli underrun. È un gate distinto dal profiling offline di `profile_test`, che non la sostituisce (precisazione R6-03).
- **DIF-INC4-R3-05**, prima dei test su device: `callbackProfileHistogram()` e `resetCallbackProfileHistogram()` vanno resi sicuri, con uno snapshot pubblicato in uno slot dedicato oppure con un'asserzione di quiescenza, senza lock nella callback.

### 4.5 Ricerche aperte e punti del modello

- **R1** (gerarchia dei metodi) e **R3** (risultato strutturato v0) del dossier sui metodi CACP: già attuati nel perimetro di Inc-5; da rivedere con la sessione alla cieca.
- **Punti aperti R-01..R-07** della [riconciliazione](research/M2_4_Inc5_Math_Model_Reconciliation.md) §6. R-01 è risolto in 5e.3c per l'artefatto numerico; resta la dipendenza fra gruppi.
- **Guasti latenti da coprire con test di caratterizzazione:**
  - CL-MM-01/AG-MM-01, `barsPerPhrase`;
  - CL-MM-02/AG-MM-04, trasposizione al bordo;
  - CL-MM-04/AG-MM-05, densità dei pad incrociati;
  - AG-MM-02, `phrasePhase`;
  - AG-MM-15, catena di fusione;
  - CL-MM-07, tracker contro motore;
  - vettore f_s/4 nel test del limiter.
- **Osservazioni aperte:**
  - OSS-5e.3b-R2-01: campo causale `NOT_RUN` senza profilo congelato;
  - nel manifest di `procedural_v1` il campo `measures.loudness.note` mostra «Â§» al posto di «§», per una doppia codifica del letterale. È cosmetico, rilevato nella preparazione di questo dossier;
  - AG-ADR-03 (informativa): l'impegno del seed (`packSeedCommitment`) usa il prefisso `evaluation_pack_v1` anche per v2 e v3. Resta verificabile, perché generazione e checker usano la stessa funzione; manca solo la separazione di dominio per versione.
- **Proposte della review di ADR-0006** ([verifica](research/M2_4_Inc5_ADR0006_Review_Verification.md) §4), da istruire con fonti lette:
  - normalizzazione robusta o locale dell'onset, insieme a H-04;
  - statistica per gruppo con test di permutazione, insieme a H-05;
  - varietà per stato e selezione deterministica fra candidati quasi ottimi, per F5.

## 5. Limiti di queste evidenze

- **Una sola fixture** e un solo contenuto: le misure non generalizzano ad altro materiale. La diversità fra seed è nulla per costruzione.
- **Offline:** tutte le misure di Inc-5 provengono dal rendering offline, che ha lo stesso codice del percorso realtime ma non la callback del device.
- **L'agente non ascolta:** nessuna affermazione percettiva è dell'agente. Le misure di onset riguardano «intent → segnale», non l'udibilità.
- **Toolchain:** golden e hash valgono per la toolchain fissata (ADR-0001).
- **Retrospettività:** M-NORM è un esperimento meccanicistico retrospettivo sul set B, non una conferma su materiale nuovo.

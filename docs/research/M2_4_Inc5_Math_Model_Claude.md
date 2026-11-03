# Modello matematico del motore di generazione musicale — indagine A (Claude)

- **Data:** 2026-11-03
- **Commit esaminato:** `df0bcbe` (fine di M2.4 Inc-5 fase 5e.3b, CI verde, [run 41621675815](https://github.com/naturewhisp/cymatica/actions/runs/41621675815))
- **Autore:** agente Claude (indagine A), in parallelo e in modo indipendente dall'indagine B di Antigravity (`M2_4_Inc5_Math_Model_Antigravity.md`), che non è stata letta. Protocollo in [progress.md](../progress.md), «Consolidamento del modello matematico».
- **Status:** **descrittivo, non normativo.** La specifica e gli ADR restano l'autorità sulle intenzioni; il codice al commit indicato è la fonte di verità sul comportamento. Le divergenze sono rilievi, non correzioni. Nessun tuning, nessuna scelta algoritmica nuova.

> **Erratum (2026-11-03, dopo la riconciliazione).** Il §4.5 (CL-MM-05) attribuisce allo stato di voice leading e ai flag di frase i 14 gruppi musicali su 24 frasi. Nella sezione B (140 BPM) la distinzione fra le frasi 9–14 è invece un artefatto di arrotondamento di ±1 campione nei frame relativi alla frase (AG-MM-07 dell'indagine B, verificato dall'agente A). Con una griglia simbolica i gruppi sarebbero 9. Dettagli e conseguenze per `evaluation_pack_v2` in [M2_4_Inc5_Math_Model_Reconciliation.md](M2_4_Inc5_Math_Model_Reconciliation.md) §4.1. Il resto del testo è lasciato come consegnato.

## 0. Sommario

Il motore è una catena **deterministica a stati finiti**, con un solo punto di contatto con il seed:

- tempo razionale esatto;
- piano simbolico ottenuto come argmax di un punteggio intero;
- eventi con identità e disposizione contabilizzate;
- sintesi a voci fisse con mix per ruolo e limiter.

**Risultati principali:**

1. **Tempo** (blocco 1). Tutte le conversioni musica → frame sono razionali, del tipo f = f_j + ⌊Δ·N/D⌋, ancorate al segmento di tempo; l'errore per beat è < 1 frame e non si accumula. L'inversione del confine di beat ha forma chiusa esatta, verificata contro la forza bruta su 2·10⁵ casi.
2. **Il seed tocca solo i pareggi** (blocchi 2 e 4). `runSeed` entra nel planner soltanto in `tieBreakHash`, mentre candidati, vincoli e punteggio sono funzioni deterministiche del piano di frase e dello stato.
   - Misura: **0 pareggi** in 24 decisioni della fixture, scarto minimo fra primo e secondo 400 punti; 0 pareggi in `planner_probe_v1`, scarto minimo 300.
   - Ne segue, come teorema e non come statistica, che **la diversità fra seed è nulla per costruzione**: la `NULL_DIVERSITY` misurata in 5e.2 è una conseguenza strutturale, non un caso.
3. **Ripetizione strutturale** (blocchi 3 e 4).
   - Dentro una frase, Pulso, Trama e Vettore ripetono lo stesso pattern a ogni battuta (periodo 1 battuta); solo il Corpo segue la progressione di 4 accordi.
   - Dentro una sezione il vettore dei punteggi è identico in tutte le frasi, quindi vince sempre la stessa ricetta: l'unica variazione fra frasi viene dallo stato di voice leading e dai flag di frase.
   - È la forma matematica dell'impressione di «loop giustapposti» registrata nell'ascolto esperto (F1–F5).
4. **Misura** (blocco 7). La soglia degli onset in unità grezze dipende dal massimo dell'intero stem (M-NORM, 5e.3b), e le statistiche di presenza dipendono dall'ipotesi ρ = 1 dentro il gruppo musicale. Sono limiti del modello di misura, non del motore.

La tabella dei rilievi è al §8, i modi di guasto trasversali al §9, le indicazioni per M2.5 al §10.

## 0.1 Metodo

- **Lettura del codice** a `df0bcbe`, con rimandi `file:riga` dove servono. Sono stati letti `engine/core`, `engine/audio`, `engine/cacp` ed `engine/replay` nelle parti del motore.
- **Confronto con le intenzioni:** specifica §7.5, §15–§16, §20, ADR-0002…0006.
- **Verifiche numeriche** fuori dal repository:
  - script Python che replicano le formule razionali e le confrontano con la forza bruta;
  - un programma C++ di analisi (`tie_probe`) collegato alle librerie compilate del commit, che pianifica la fixture e `planner_probe_v1` con i seed 1337 e 1..64 e riporta i punteggi di tutti i candidati.

  Il codice del progetto non è stato modificato.
- **Stato di ogni affermazione:**
  - *verificata*, con rimando a test, evidenza o verifica numerica;
  - *lacuna di verifica*, quando nessun test la copre;
  - *divergenza*, fra codice e specifica o ADR;
  - *modo di guasto*, quando sono note le condizioni che rompono un'ipotesi.

---

## 1. Tempo e trasporto

### 1.1 Modello

**Unità.**
- *Frame:* f ∈ ℕ (u64), un frame multicanale alla frequenza interna f_s = 48.000 Hz.
- *Beat:* è la figura 1/`timeSignatureDen`. Il BPM è espresso in semiminime al minuto (specifica §7.5), come razionale `num/den`.

**Costante del segmento.** Per una mappa di tempo:

  N = 60 · f_s · bpm_den · 4,  D = bpm_num · ts_den,  K = N / D  (frame per beat, razionale)

**Segmenti.** La timeline è una lista di al più 16 segmenti S_j = (f_j, β_j, B_j, o_j, map_j): frame d'inizio, beat assoluto, battuta assoluta, offset del beat nella battuta, mappa (`music_clock.h`). Si scelgono così:
- per una conversione beat → frame, l'ultimo segmento con β_j ≤ β;
- per frame → posizione, l'ultimo con f_j ≤ f.

**Conversioni** (`music_clock.cpp`):

- beat → frame: f(β) = f_j + ⌊(β − β_j) · N_j / D_j⌋.
- sotto-beat (passo r di s per beat): f(β, r, s) = f_j + ⌊((β − β_j)·s + r) · N_j / (D_j · s)⌋. Coincide con f(β) quando r ≡ 0 (mod s).
- battuta → beat: β(B) = β_j + [B > B_j] · ((b_pb − o_j) mod b_pb + (B − B_j − [o_j ≠ 0]) · b_pb), con correzione per un segmento iniziato a metà battuta.
- frame → beat trascorsi (inversione esatta del floor):

  e(Δf) = ⌊(Δf · D + D − 1) / N⌋ = q + [r ≥ N − (D − 1)],  con (q, r) = divmod(Δf · D, N).

  È il massimo b con ⌊b·N/D⌋ ≤ Δf. Richiede D ≤ N, garantito da `isSupportedTempoMap`.
- tick di simulazione (f_s = 48.000, S = 120 Hz):
  - confine ⌊k · f_s / S⌋ = 400·k;
  - frame → tick: ⌈f · S / f_s⌉ (primo tick il cui confine raggiunge il frame, specifica §7.5);
  - tick attesi dopo T ns: ⌊(T·S + S − 1)/10⁹⌋.
- device ↔ interno:
  - device → interno con floor;
  - interno → device con ceiling, perché un evento non scatti mai prima del suo tempo logico.

**Aritmetica.** `core::multiplyDivide(a, b, d)` (`unsigned_math.h`) calcola ⌊a·b/d⌋ e il resto senza prodotti a 128 bit, con una ricorrenza bit a bit, e satura solo se il quoziente non è rappresentabile. Restituisce (0, 0) per d = 0; i chiamanti escludono d = 0 a monte.

### 1.2 Invarianti

| Invariante | Verifica |
|---|---|
| Nessuna deriva: f(β) è calcolato dall'ancora del segmento, mai per somma; l'errore per beat è < 1 frame e non cresce con β | test «MusicClock handles non-integer BPM over 1 virtual hour with zero drift», «…127.5 BPM beat boundaries are exact…» (`test_music_clock.cpp`) |
| Durata di ogni beat ∈ {⌊K⌋, ⌈K⌉} | verificata numericamente: a 140 BPM i beat durano 20.571 o 20.572 frame |
| Inversione esatta: e(Δf) è il massimo b con ⌊bN/D⌋ ≤ Δf | verificata contro la forza bruta su 2·10⁵ casi casuali (BPM 30–300 razionale, ts_den ∈ {2, 4, 8, 16}): 0 discrepanze |
| Cambio di tempo solo su un confine di beat; cambio di metro solo su un confine di battuta; segmenti monotoni, al più 16 | `setTempoMap` (`music_clock.cpp`); test DIF-M2-02, DIF-M2-10, DIF-M2-review |
| Confini della fixture: inizio di B 3.072.000, di A' 5.705.142, fine 8.777.142 (182,857 s); NoteOff dei pad incrociati 3.092.571 e 5.729.142 | verificati con lo script, coincidono con i commenti del codice e con il test «25 phrase boundaries match an independent rational oracle» |
| Fixed step: niente tick scartati, al più 4 tick per aggiornamento, sospensione tecnica da 16 in su, α ∈ [0, 1) | `test_fixed_step.cpp` (deriva zero su 60 ore virtuali, equivalenza 60/120 FPS, catch-up, overload) |

### 1.3 Ipotesi e limiti

- Il dominio supportato è chiuso:
  - ts_den ∈ {2, 4, 8, 16}, numeratore ≤ 16, `beatsPerBar` = numeratore;
  - suddivisioni e battute per frase ≤ 65.535;
  - D ≤ N (almeno un frame per beat).

  Fuori dal dominio la creazione fallisce in modo esplicito.
- `beatPhase`, `subdivision` e `phrasePhase` sono viste derivate in float (specifica §7.5): non governano mai le decisioni.
- La posizione musicale di un frame è definita solo dentro la timeline pianificata. Il clock non conosce epoch né pause: le discontinuità sono del trasporto (blocco 5).

### 1.4 Modi di guasto

- **CL-MM-01, frasi e cambio di `barsPerPhrase`.** `frameAtPhrase` usa `barsPerPhrase` del solo segmento 0; `positionAtFrame` applica `barsPerPhrase` del segmento corrente al numero globale di battute.
  - Se una mappa successiva cambia `barsPerPhrase`, i due calcoli danno indici di frase diversi e incoerenti con i confini effettivi.
  - Nella fixture `barsPerPhrase` = 4 in tutte le sezioni, quindi il guasto è **latente**. Nessun test lo copre (lacuna di verifica).
- **Saturazione.** Le conversioni saturano a UINT64_MAX invece di avvolgersi: un frame saturo è un tempo «infinito», non un errore segnalato. È accettabile ai limiti di u64 (≈ 12 milioni di anni a 48 kHz).

---

## 2. Determinismo e identità

### 2.1 Modello

**Mixer.** È Stafford Mix 13, cioè il finalizzatore di SplitMix64 (Steele, Lea e Flood 2014), una biiezione su 64 bit:

  m(z) = z₃, con z₁ = (z ⊕ z≫30)·c₁, z₂ = (z₁ ⊕ z₁≫27)·c₂, z₃ = z₂ ⊕ z₂≫31.

Le funzioni a 3, 4 e 5 argomenti sono catene h ← m(h + a_i · k_i), con costanti di dominio diverse per stream, campione e decisione (`stable_random.h`).

**Generatore senza stato** (specifica §15.3–§15.4):

- seme di stream: s = mix₃(runSeed, streamId, policy);
- valore: u = mix₄(s, decisionId, sampleIndex, retryTag);
- float: ⌊u / 2⁴⁰⌋ · 2⁻²⁴ ∈ [0, 1);
- interi in [a, b): metodo di Lemire (2019), con soglia t = (2³² − r) mod r e ripetizioni su `retryTag`, mai su `sampleIndex`.

**Identità di decisione:** decisionId = mix₅(section, phrase, bar, slot, policy). Non dipende dal seed; il seed entra solo nel valore.

**Hash canonico di provenienza** (`plan_canonical_hash.cpp`): FNV-1a 64 di una serializzazione little-endian di intent, NoteOff del planner e tracce di decisione. Include seed, id, punteggi, hash di spareggio e float per pattern di bit.

**Fingerprint di contenuto** (`plan_content_fingerprint.h`): FNV-1a 64 delle sole note complete, cioè ruolo, frame, cents, velocity e timbro ×1000, durata, rilasci espliciti. Esclude seed, id, tick, epoch e decisioni; un'origine opzionale lo rende relativo all'inizio della frase.

### 2.2 Invarianti

| Invariante | Verifica |
|---|---|
| Vettori di riferimento del mixer e dei mix | `test_random.cpp`, «…expected golden vectors» |
| Isolamento degli stream: il consumo di uno stream non altera gli altri | «Stream isolation: VFX stream consumption…» |
| Lemire non distorto, nessun overflow su intervalli ampi, i rifiuti non correlano `sampleIndex` | tre test in `test_random.cpp` |
| Determinismo del piano: due esecuzioni danno intent, tracce e NoteOff identici | `fixture_probe_v1` (`determinismPassed`) e test «MusicPlanner replay determinism…» |
| Isolamento metamorfico: un altro seed può cambiare solo decisioni in pareggio | `fixture_probe_v1` (`streamIsolationPassed`), test «Seed isolation is metamorphic…» |
| Hash canonico == golden; hash offline == hash JIT | `fixture_test` in CI (golden di trace, piano e PCM) |

### 2.3 Ipotesi e limiti

- **Toolchain.** Le altezze sono calcolate con `std::pow` in float, quindi hash canonico e golden PCM valgono per la toolchain e il CRT fissati (ADR-0001), non per piattaforme arbitrarie. Il codice lo dichiara (`plan_canonical_hash.h`). È coerente con la CI, che usa la stessa toolchain.
- **`PolicyVersionId::toU32`** impacchetta quattro campi in 8 bit ciascuno senza maschera: valori ≥ 256 in `major`, `minor` o `patch` collidono. È latente, perché i valori attuali sono piccoli.

---

## 3. Composizione

### 3.1 Altezze

- Scala modale con radice MIDI R e modo M (tabella di 7 intervalli per modo).
- Grado d → MIDI: n(d, c) = R + 12·⌊d/7⌋ + I_M[d mod 7] + c, con divisione e modulo per difetto anche per gradi negativi.
- MIDI → Hz: 440 · 2^((n − 69)/12), con n saturato in [0, 127] e frequenza saturata in [20, 16.000] Hz (`music_planner.cpp`, `midiNoteToPitchHz`).
- **Registri per ruolo:**

  | Ruolo | Grado | Esempio |
  |---|---|---|
  | Pulso | −14 (due ottave sotto la tonica) | D2 ≈ 73,4 Hz in Re dorico, G1 ≈ 49,0 Hz in Sol eolio |
  | Corpo | basso = radice − 7, armonia = terza o quinta | — |
  | Trama | colore + 7 | — |
  | Vettore | gradi del motivo + 7·offset di ottava | — |

### 3.2 Armonia

**Accordi per grado.** Un accordo è una terna di gradi (radice, terza, quinta) in classe di altezza, non sovrapposta. Per esempio il v dorico è (4, 6, 1): la «quinta» è il grado 1, quindi sotto la radice nella stessa ottava.

**Progressioni della fixture** (`makeFixtureM24V1Phrases`): una battuta per accordo, con funzioni Stabilità, Preparazione, Tensione, Risoluzione.
- A e A': i–IV–v–i.
- B: i–VI–VII–i.

**Voicing del Corpo** (`voiceTriad`):
- basso = radice − 7;
- l'armonia è la terza o la quinta, scelta per minima distanza in gradi dall'armonia precedente (voice leading); a parità vince la quinta, e senza riferimento si usa la quinta;
- il grado non usato va al colore della Trama.

### 3.3 Ritmo

- **Pattern euclidei** E(k, n) per partizione di Bjorklund (Toussaint 2005), con il primo onset sull'indice 0 e rotazione circolare (`euclidean_rhythm.h`).
- **Griglia:** 16 passi per battuta in 4/4.
- **Parametri dei candidati** (indice k = 0…7):
  - Pulso: E(4 + k mod 3, 16) ruotato di k mod 4;
  - Trama: E(6 + (k + 1) mod 3, 16) ruotato di (k + 2) mod 4, solo sui passi non occupati dal Pulso;
  - durata della Trama: 1, 2 o 8 passi.

### 3.4 Motivo e trasformazioni

**Motivo.** Una lista di note (grado, alterazione ∈ {−1, 0, +1}, onset e durata in passi, velocity, flag d'ancora).

**Motivi di base:**
- dorico: gradi (0, 2, 4, 3), ancore sul primo e sul terzo;
- eolio: gradi (0, 4, 5, 4), ancore sui primi due.

Ogni nota dura 4 passi, quindi il motivo occupa una battuta.

**Trasformazioni pure** (`music_planner.cpp`, sezione 2), con le invarianti verificate da `verifyMotifTransformInvariants`:

| Trasformazione | Definizione | Invariante |
|---|---|---|
| Trasposizione modale (δ) | d ↦ d + δ, ripiegata di un'ottava se esce da [−7, 14] | contorno (differenze successive) preservato |
| Inversione (pivot p) | d ↦ 2p − d; le ancore si spostano sul membro più vicino della triade di tonica | simmetria sulle non-ancore; ancore su {0, 2, 4} mod 7 |
| Variazione euclidea (r) | onset ← E(numero di note, 16) ruotato di r; durate fino all'onset successivo | gradi invariati, onset strettamente crescenti |
| Alterazione cromatica | ±1 semitono sulla prima nota non ancora | al più una nota alterata, mai un'ancora |
| Ottava o riorchestrazione | offset di registro ± ottave | note invariate |

**Ricette per sezione** (`kCandidateRecipesA/B/APrime`):
- A: tabella standard di 8 ricette;
- B: solo trasposizioni e inversioni;
- A': solo ripresa con ancore preservate. Contiene doppioni: l'identità alle posizioni 0 e 7, l'inversione con pivot 2 alle posizioni 1 e 5.

### 3.5 Invarianti e verifiche

| Invariante | Verifica |
|---|---|
| Bjorklund e rotazione | «Bjorklund canonical partitioning and circular rotation» |
| Invarianti delle trasformazioni | «Motif pure transformations preserve semantic invariants» |
| Voicing, accordi per battuta, pad aperti | «Triad voicing, per-bar chord realization…» |
| Altezze contro un oracolo | «Onset grid is derived from MusicClock (zero drift at 140 BPM) with oracle pitches» |
| Maschere di ruolo, ricette, lease del pad e rilascio finale della fixture | «Fixture M2.4 V1: planned content honours role masks…» |

### 3.6 Modi di guasto

- **CL-MM-02, trasposizione al bordo del registro.** Il ripiegamento si applica nota per nota: se solo alcune note escono da [−7, 14], il contorno si rompe. Esempio: gradi (13, 12) con δ = +2 danno (8, 14); le differenze passano da −1 a +6.
  - `verifyMotifTransformInvariants` lo rileverebbe come violazione.
  - Il generatore però **non chiama** l'oracolo sui candidati: la ricetta passa ai vincoli hard, che non controllano il contorno.
  - Con i motivi e le ricette attuali i gradi restano in [−2, 7], quindi è latente. Nessun test copre il caso (lacuna di verifica).
- **CL-MM-03, periodicità di una battuta.** Dentro una frase, Vettore, Pulso e Trama ripetono lo stesso contenuto in ogni battuta (`generateCandidates`: il motivo e i pattern euclidei sono generati identici per ogni b). Il periodo del materiale melodico e ritmico è quindi una battuta; il Corpo ha periodo 4 battute (la progressione).
  - Non è un difetto rispetto agli ADR: è il modello d'ingresso di M2.3/M2.4.
  - È però la radice formale delle osservazioni F1–F4 (ripetizione, mancanza di sviluppo) e del principio del titolare «ricordi, sviluppi, sorprenda e ritorni». Un motivo di una battuta ripetuto 4 volte per frase e 4–8 frasi per sezione, senza varianti, **ricorda e ritorna ma non sviluppa**.

---

## 4. Decisione del planner

### 4.1 Modello

Per ogni frase φ, con stato σ (voice leading precedente, tracker delle voci simulato, prossimo `eventId`), il planner:

1. Costruisce la griglia dalla sola mappa di tempo e ne verifica la chiusura:
   - inizio e fine della frase coincidono con `frameAtBar`;
   - il passo finale di ogni battuta coincide con l'inizio della successiva (`buildGrid`).
2. Genera C ∈ [4, 8] candidati deterministici c_k = G(φ, σ, k), uno per ricetta (§3).
3. Filtra con i vincoli hard V (`validateHardConstraints`):
   - parametri finiti e nei domini: MIDI 0–127, 20–16.000 Hz, velocity e timbro in [0, 1];
   - eventi dentro [max(inizio, orizzonte impegnato), fine);
   - al più 2 NoteOn simultanei per ruolo;
   - al più 128 NoteOn per frase;
   - al più 128 eventi SPSC in ogni finestra di lookahead.
4. Simula l'allocazione delle voci su una copia del tracker (regole 0–2) e conta i furti di voci tenute.
5. Assegna un punteggio intero S(c) ∈ [0, 100.000]:

   S = 25·s_VL + 25·s_T + 25·s_M + 25·s_R − 2.500·furti

   - s_VL = clamp(1000 − 40·d_VL): d_VL è la somma dei movimenti in gradi delle voci del Corpo fra onset d'accordo;
   - s_T = clamp(1000 − |τ̂ − τ*|): τ̂ è la tensione stimata, cioè tensione della trasformazione + densità + registro + funzione armonica media; τ* è il target della sezione in millesimi;
   - s_M = fedeltà delle ancore in millesimi;
   - s_R = clamp(1000 − 0,1·|ρ − 13.000|): ρ sono gli onset melodici e ritmici per battuta, in millesimi.

   I pesi sono nel profilo versionato `kPlannerScoringProfileV1`, «experimental, NOT validated».
6. Sceglie il vincitore con un ordine totale stretto:

   c* = argmax (S, H(seed, decisione, c), −id)

   H è l'hash di spareggio sullo stream `MusicForm`.
7. Se nessun candidato è valido, scende la scala di fallback L1 → L2 → L3. L1 sono le ancore più i downbeat, L2 il pad, L3 il silenzio, sempre ammissibile. Ogni livello passa gli stessi vincoli.
8. Impegna il vincitore: ordine canonico (frame, ruolo, MIDI, durata), `eventId` sequenziali, tick = ⌈f·S/f_s⌉, NoteOff espliciti per i pad aperti. Il tracker avanza solo con il vincitore.

### 4.2 Proprietà derivate

**Teorema D (diversità fra seed).** Per un piano di forma fissato, il contenuto musicale prodotto è indipendente dal seed se e solo se ogni decisione ha un unico candidato di punteggio massimo, oppure tutti i candidati in pareggio producono lo stesso contenuto.

*Dimostrazione:*
- G, V e S non leggono il seed (`runSeed` compare solo in `tieBreakHash`, `music_planner.cpp`), e lo stato σ dopo la decisione dipende solo dal vincitore.
- Per induzione sulle frasi: se nessuna decisione dipende dal seed, nemmeno σ ne dipende.
- Il seed entra quindi solo nel secondo criterio dell'ordine, che conta solo a parità del primo. ∎

**Misura** (programma `tie_probe`, §0.1):

| Piano | Decisioni | In pareggio | Scarto minimo primo−secondo | Seed 1..64 con contenuto diverso dal 1337 |
|---|---|---|---|---|
| fixture-m2-4-v1 (8 candidati) | 24 | **0** | 400 | **0** |
| planner_probe_v1 (6 candidati) | 2 | 0 | 300 | 0 |
| planner_probe_v1 (8 candidati) | 2 | 0 | 2.150 | 0 |

È la spiegazione strutturale della `NULL_DIVERSITY` misurata in 5e.2 sui seed 1..64 (ADR-0006 5e-04), e coincide con il «limite già noto» registrato in 5c («il seed agisce solo sui pareggi»), qui dimostrato e misurato.

**Uniformità dei punteggi nella sezione.** Nella fixture il vettore degli 8 punteggi è identico in tutte le frasi di una stessa sezione, salvo la variazione di maschera fra le frasi 0–3 e 4–7 di A:

| Frasi | Vincitore | Punteggio vincente |
|---|---|---|
| A, 0–3 | inversione con pivot 2 | 75.175 |
| A, 4–7 | identità | 87.500 |
| B, 8–15 | inversione con pivot 4 | 81.600 |
| A', 16–23 | ottava −1 | 85.150 |

Le feature dipendono solo da grandezze costanti nella sezione:
- target di tensione e accordi;
- maschera della Trama;
- ricetta e d_VL, che non cambia perché ogni frase ricomincia dallo stesso accordo.

Il planner **non ha uno stato che vari lungo la sezione**: niente arco di tensione, nessun contatore di ripetizioni, nessuna memoria delle scelte precedenti oltre al voice leading.

**Robustezza degli spareggi.** Con pesi interi e feature intere i pareggi sono possibili ma, sul materiale attuale, assenti con margine (≥ 300 punti). Lo spareggio è quindi una garanzia di ordine totale, non una fonte di varietà.

### 4.3 Invarianti e verifiche

| Invariante | Verifica |
|---|---|
| Rifiuto, mai troncamento, oltre la capacità | «Hard constraints reject over-capacity candidates instead of truncating» |
| Scala di fallback validata | «Fail-closed fallback ladder L1 -> L2 -> L3…» |
| Spareggio: invarianza alla permutazione, id canonici | «Candidate tie-break permutation invariance…» |
| Punteggio intero, versionato, penalità per i furti | «Integer score uses the versioned profile…» |
| Checkpoint e rollback bit-exact | «MusicPlanner: checkpoint and rollback restores bit-exact state» |
| Golden del piano | «Golden regression: canonical plan of fixture-m2-4-v1» |

### 4.4 Ipotesi e limiti

- **Tracker contro motore.** Il tracker simulato modella un rilascio di 96 frame (`kReleaseTailFrames`), cioè quello di produzione.
  - Con l'override sperimentale di 5e.3 il motore usa rilasci più lunghi, ma il planner continua a simulare 96 frame: furti e sovrapposizioni simulati possono divergere da quelli reali. Per costruzione l'esperimento di 5e.3 varia solo il rendering.
  - La regola 1 del tracker sceglie la voce in rilascio più vecchia (le ampiezze non sono note in simulazione); quella del motore va confrontata (§6.4).
- **Densità SPSC dei pad incrociati.** Il vincolo conta il NoteOff di un pad aperto alla fine della frase, mentre quello di un pad incrociato cade un beat dopo, nella sezione successiva (`validateHardConstraints` contro `planPhrase`). La finestra di densità sottostima di un beat il NoteOff dei pad incrociati.
  - Con 128 eventi per finestra e le densità attuali l'effetto è trascurabile. Resta un'imprecisione del modello (CL-MM-04).
- **Pesi non validati.** I pesi del punteggio sono un profilo iniziale dichiarato, da valutare in M5/M7 secondo AGENTS §1.1.

### 4.5 Modi di guasto

- **CL-MM-05, assenza di variazione nella sezione.** Conseguenza diretta del §4.2. Per una sezione di n frasi il numero di contenuti distinti è limitato dalle varianti di stato, voice leading e flag di frase, non dal planner. In 5e.2 la fixture conta 14 gruppi musicali su 24 frasi.
  - Effetto musicale: ripetizione percepita, F1–F4.
  - Effetto sulle misure: pochi gruppi indipendenti, quindi `n_eff` basso (ADR-0006 5e-11).
- **CL-MM-06, il seed non è una sorgente di varietà.** Ogni affermazione del tipo «seed diversi → musica diversa» è falsa per il modello attuale (Teorema D). Il RunRecord e la diversità fra seed vanno letti di conseguenza: il seed serve all'identità e alla riproducibilità, non alla varietà.

---

## 5. Catena degli eventi

### 5.1 Modello

**Da intent a eventi** (`audio_scheduler.cpp`). Un intent (eventId, f, ruolo, f₀, v, durata Δ, timbro, epoch) diventa:
- un NoteOn a f;
- se Δ > 0, un NoteOff a f + Δ (saturato);
- per un pad aperto (Δ = 0), un NoteOff esplicito del planner al frame dichiarato.

Nella fixture ogni NoteOn ha quindi esattamente un NoteOff.

**Staging.** È una sequenza ordinata dall'ordine totale stabile (f, priorità, eventId), con priorità NoteOff < ParameterChange < NoteOn, come prescrive la specifica §7.5.

**Dispatch.** Al cursore di rendering c si inviano alla coda SPSC del motore, in ordine, tutti gli eventi con f ≤ c + L, dove L = 7.200 frame (150 ms). Se la coda è piena l'invio si ferma, preservando l'ordine cronologico, e conta un'unità di backpressure: l'evento resta in staging, non è perso.

**Revoca** (`revokeFromFrame`, checkpoint di ADR-0005):
- rimuove i NoteOn con f ≥ b, i loro NoteOff e gli eventi non-nota con f ≥ b;
- conserva i NoteOff di NoteOn già inviati.

**Applicazione nel motore** (`audio_engine.cpp`). Il blocco si divide in fette agli offset esatti degli eventi e dei comandi; ogni evento estratto riceve una disposizione:
- **Dropped:**
  - con ragione `StaleFrameOrEpoch` se f < inizio del blocco o l'epoch è sotto il minimo valido;
  - con ragione `FutureEpochUnmatchable` se l'epoch è futura e nessun comando la attiverà prima di f.
- **Applied o Ignored:** secondo lo slot restituito dall'allocatore (≥ 0 oppure −1). Per esempio, un NoteOff di una voce già tornata Idle è Ignored.
- **Limite per blocco:** al più `kMaxScheduledEventsPerBlock` eventi esaminati per blocco; gli altri sono differiti, non scartati.

**Conti O3** (ADR-0006 D-M2-12):

  intent = inviati = consumati = Applied + Ignored attesi + Dropped attesi

Ogni NoteOn dell'intent è Applied esattamente una volta, allo stesso frame, ruolo ed `eventId`.

### 5.2 Invarianti e verifiche

| Invariante | Verifica |
|---|---|
| Riconciliazione O3 esatta, anche con guasto iniettato; rifiuto delle perdite anche quando i contatori osservati tornano | `test_event_chain.cpp` (O3, quattro casi) e `fixture_test` in CI (`event_chain.passed`, discrepanze 0) |
| Registro degli eventi applicati: disposizioni, ragioni, overflow vincolante, installazione solo in quiescenza | «Applied-event log: dispositions, drop reasons, overflow and quiescence» |
| Revoca: un NoteOff conservato e cancellato allo Stop è cancellato, non inviato | «Scheduler log: a NoteOff preserved by the revocation…» |
| Densità SPSC: ≤ 128 eventi per finestra di lookahead a monte, verificata dal planner | §4.1, vincoli hard |

### 5.3 Ipotesi e limiti

- **Ritardo dentro il blocco.** Un evento con f dentro il blocco ma prima dell'offset corrente, possibile dopo un comando che ha consumato la fetta, è applicato all'offset corrente, cioè leggermente in ritardo, non scartato. Lo scarto avviene solo per f < inizio del blocco.
- **Presentazione.** Il cursore di presentazione è stimato come cursore di rendering meno un blocco (`presentationCursor`): è una stima, con qualità pubblicata, coerente con la specifica §7.5. Non si equiparano frame generati e frame uditi.
- **NoteOff prima del NoteOn.** Allo stesso frame, per la priorità di §7.5 il NoteOff precede il NoteOn. Ne segue che, a ogni cambio di nota legato, la voce precedente è già in Release quando arriva il NoteOn successivo. È l'origine delle «sovrapposizioni» e dei «release tagliati allo stesso frame» misurati come confondenti in 5e.3: nella fixture sono costanti fra le varianti di release, 190 tagli del Corpo e 95 + 383 sovrapposizioni di Corpo e Vettore (§6.4).

---

## 6. Sintesi e mix

### 6.1 Modello della voce

**Allocatore** (`voice_allocator.cpp`): 8 slot statici, 2 per ruolo. Selezione per NoteOn:
- regola 0: uno slot libero, prima lo slot 0 della coppia;
- regola 1: uno slot in Release, quello di inviluppo minore e a parità il più vecchio;
- regola 2: il furto della voce attiva più vecchia, che conta come «voice drop».

Il furto copia l'uscita corrente in una coda lineare di 2 ms (96 frame), sommata nel bus del ruolo.

**Inviluppo ADSR lineare**, campione per campione:
- attacco: e = k/A;
- decadimento: e = 1 − (k/D)(1 − s);
- sostegno: e = s;
- rilascio: e = e₀ (1 − k/R), con R = 96 frame (declick di 2 ms) in produzione.

Le costanti sono per ruolo, per `timbreParam` > 0 (`onNoteOn`):

| Ruolo | A | D | s | Sintesi |
|---|---|---|---|---|
| Pulso | 1 ms | 50 + 70·timbro ms | 0 | seno con glissando esponenziale f(t) = f₀ + (f_s − f₀)·e^{−t/8 ms}, f_s = min(3f₀, 180 Hz), più un click di rumore deterministico (Stafford Mix 13) di 15 ms, filtrato passa-banda SVF a 1,2–3,5 kHz |
| Corpo | 15 ms | 100 ms | 0,75 | dente di sega PolyBLEP più SVF passa-basso, f_c = clamp(min(3f₀, 1,5 kHz)(1 + 1,5·timbro) + 1.200·e, 60 Hz, 0,42 f_s), Q = 0,8 + 0,4·timbro |
| Trama | 20 ms | 120 ms | 0,65 | FM a 2 operatori, rapporto 1 + 1,76·timbro, indice I = I_max·clamp(timbro + 0,25)·e, con I_max di Carson = clamp((0,42 f_s − f_c)/f_m − 1, 0, 4) |
| Vettore | 5 ms | 50 ms | 0,85 | dente di sega PolyBLEP con portamento a un polo (τ = 15 ms dalla nota Vettore precedente), vibrato a 5,5 Hz di ±0,5 %·timbro, SVF passa-basso f_c = clamp(2,5 f + 1.500·e, 100 Hz, 0,42 f_s), Q = 1 |

**Ampiezza della voce:** y = sintesi · velocity · e · 0,25 (`kNominalVoiceScale`). Lo spread stereo è un guadagno fisso per slot.

### 6.2 Primitive DSP (`synth_dsp.h`)

- **PolyBLEP a 2 punti** sul dente di sega (Välimäki e Huovilainen 2007):
  - residuo 2x − x² − 1 per φ < Δ (x = φ/Δ);
  - residuo x² + 2x + 1 per φ > 1 − Δ (x = (φ − 1)/Δ);
  - Δ = f/f_s ≤ 0,45.

  Coincide con la forma standard.
- **SVF trapezoidale TPT** (Zavalishin 2018; Simper): g = tan(π f_c/f_s), R = 1/(2Q), a₁ = 1/(1 + g(g + 2R)), a₂ = g·a₁, a₃ = g·a₂, aggiornamento degli stati s ← 2v − s.
  - Q è limitato a [0,5, 10] e f_c a [20 Hz, 0,45 f_s]; stabile sotto modulazione.
  - Coincide con la forma standard della letteratura.
- **Smoother a un polo:** α = 1 − e^{−1/(τ f_s)}.
- **Flush dei subnormali:** |v| < 10⁻²⁰ → 0.

### 6.3 Bus e master

**Bus di ruolo** (`role_bus_processor.cpp`), dopo le voci:
- guadagni di bus −6, −8, −10 e −6 dB (Pulso, Corpo, Trama, Vettore), lisciati con τ = 5 ms e mantenuti quando il ruolo tace;
- **ducking** del Corpo pilotato dal Pulso:
  - inseguitore d'inviluppo con τ_a = 2 ms e τ_r = 120 ms;
  - guadagno 1 − (1 − 10^{−3/20})·clamp(env / (0,25 · g_Pulso));
- **micro-delay stereo della Trama:** tap fissi L = 11 ms e R = 17 ms, feedback ≤ 0,45, wet = clamp(0,20 + 0,25·timbro, 0, 0,45).

**Master:**
- rampa di trasporto di 2 ms su Pausa, Ripresa e Stop;
- tono diagnostico opzionale;
- limiter terminale:
  - stima causale del true peak;
  - guadagno stereo-collegato verso la soglia di −1,8 dBFS;
  - attacco istantaneo, hold di 2 ms, rilascio esponenziale di 50 ms;
  - limitatore di slew vicino al fondo scala;
  - clamp duro a −1,0 dBFS.

### 6.4 Proprietà derivate: transitori di rilascio

**Modello approssimato dei transitori.** In un ADSR lineare la pendenza dell'inviluppo vale:
- 1/A all'attacco;
- e₀/R al rilascio, con e₀ ≈ s per una nota rilasciata in sostegno.

Il rapporto ρ = (e₀/R)/(1/A) misura quanto il rilascio sia «più brusco» dell'attacco.

**Calcolo** (script fuori dal repository, f_s = 48 kHz):

| Ruolo | ρ a 2 ms | 15 ms | 30 ms | 50 ms | 80 ms |
|---|---|---|---|---|---|
| Corpo | 5,62 | 0,75 | 0,38 | 0,23 | 0,14 |
| Trama | **6,50** | 0,87 | 0,43 | 0,26 | 0,16 |
| Vettore | 2,12 | 0,28 | 0,14 | 0,08 | 0,05 |
| Pulso | 0,11 (120 BPM), 0,16 (140 BPM) | — | — | — | — |

Per il Pulso il decadimento ha già portato e₀ ≈ 0,21 o 0,33 al NoteOff, posto a metà passo.

**Coerenza con le misure di 5e.3 e 5e.3b** (verifica qualitativa, non un fit):
- il Pulso non ha mai rilevazioni sui rilasci (ρ < 0,2 già a 2 ms);
- il Vettore le perde da 15 ms in su (ρ ≤ 0,28);
- la Trama, con il ρ più alto, le conserva in parte a 30 ms nella fixture (243 su 400, ρ = 0,43) e le perde a 50 ms (ρ = 0,26);
- a 2 ms il transitorio di rilascio della Trama supera l'attacco, coerente con il massimo globale del flusso su un rilascio (P1 di 5e.3b).

Il modello ignora il contenuto spettrale e la compressione logaritmica del flusso: spiega l'ordinamento dei ruoli e la soglia approssimativa, non i valori.

### 6.5 Invarianti e verifiche

| Invariante | Verifica |
|---|---|
| Pool statico, selezione deterministica (regole 0–2), code di furto, nessuna allocazione nella callback | `test_voice_allocator.cpp` (pool, regole, furti rapidi, zero allocazioni) |
| Primitive stabili, aliasing limitato | «Synth DSP primitive components satisfy stability and alias bounds»; dossier [aliasing](M2_4_Voice_Distortion_Aliasing_Evidence.md) |
| Glissando del Pulso, portamento e vibrato del Vettore come da ADR-0004 | tre test dedicati in `test_synth_dsp.cpp` |
| Limiter: true peak sotto il tetto sui vettori avversari, silenzio esatto dopo la pausa, guardia sui non finiti | «MasterLimiter keeps the estimated true peak under the ceiling…», «Terminal limiter keeps exact digital silence…» |
| Override del release solo offline; rilasci di sicurezza a 2 ms | `test_release_ab.cpp` (5e.3) |
| Golden PCM della fixture | `fixture_test` in CI (15126969907236845529) |

### 6.6 Ipotesi e limiti

- **Tracker contro motore, regola 1.** Con entrambe le voci in rilascio, il motore sceglie l'inviluppo minore e il tracker del planner la voce più vecchia.
  - Le due scelte possono divergere quando le voci sono entrate in rilascio in un ordine diverso da quello d'inizio, o da livelli diversi.
  - In produzione la finestra in cui entrambe le voci di un ruolo sono in rilascio dura al più 2 ms (R = 96 frame), quindi la divergenza è rara ma non esclusa. Con l'override di 5e.3 la finestra si allunga (CL-MM-07).
  - Il tracker modella sempre R = 96 frame.
  - Nessun test confronta direttamente le scelte del tracker con quelle del motore sullo stesso flusso (lacuna di verifica).
- **Legacy `timbreParam` = 0:** per tutti i ruoli resta il modo di test sinusoidale Inc-1 (A = 1 ms, D = 10 ms, s = 0,8) con guadagno di bus unitario. Il planner non emette mai timbro 0 (valori 0,30–0,60).
- **Commento di `RoleBusProcessor`** (`role_bus_processor.h`): parla di micro-delay «tempo-synced», ma i tap sono fissi a 11 e 17 ms, come prescrive ADR-0004 §2.3. La divergenza è solo nel commento (CL-MM-08, cosmetica).

### 6.7 Modi di guasto

- **F6, rilascio di 2 ms.** È un transitorio a banda larga a ogni NoteOff, quantificato al §6.4. Per la Trama è più ripido dell'attacco di un fattore 6,5. L'udibilità non è verificata ed è affidata all'esperto con le clip A/B di 5e.3.
- **Saturazione del limiter.** Con carichi estremi la riduzione di guadagno cresce: 3,1 dB nel carico W di `profile_test`, con tono diagnostico a 0,8. Il materiale della fixture resta invece a 0 dB di riduzione (ADR-0005, dossier DIF-INC4-R2-01). Il limiter è una protezione, non un compressore musicale.

---

## 7. Misura e statistica (CACP)

### 7.1 Onset (`descriptors_v1`, D-M2-14)

**Spettrogramma:**
- downmix mono (L + R)/2;
- STFT con finestra di Hann simmetrica, N = 2.048, hop H = 256;
- tempo del frame k: t_k = (k·H + N/2)/f_s, cioè il centro del frame (`stft.cpp`, `frameCentreSeconds`).

**Flusso spettrale:** F[k] = Σ_b max(0, log(1 + γ|X_k(b)|) − log(1 + γ|X_{k−1}(b)|)), con γ = 100 e F[0] = 0.

**Normalizzazione e soglia:**
- n[k] = F[k] / max_j F[j];
- θ[k] = MA(n)[k] + δ, dove MA è la media mobile centrata su 2w + 1 frame con zero padding e w = round(0,1·f_s/H) = 19.

**Picchi:** n[k] > n[k−1], n[k] ≥ n[k+1] e n[k] > θ[k]. Le rilevazioni a meno di round(0,03·f_s/H) = 6 frame si fondono tenendo il picco maggiore.

**Riferimenti e abbinamento:**
- riferimenti: i NoteOn Applied del ruolo, fusi se distano meno di 30 ms;
- le rilevazioni sono corrette per l'offset del profilo congelato (per ruolo: δ e offset; Trama δ = 0,20, offset +9,96 ms);
- abbinamento uno a uno di cardinalità massima con tolleranza inclusiva ±50 ms, convenzione di mir_eval (Raffel et al. 2014), da cui P, R e F.

**Diagnostica dei rilasci:** le rilevazioni non abbinate entro ±50 ms da un NoteOff del ruolo.

**Proprietà derivate:**
- **Soglia in unità grezze (M-NORM, 5e.3b):** un picco è rilevato se e solo se F[k] > MA(F)[k] + δ · max_j F[j]. La soglia effettiva dipende dal massimo dell'**intero stem**, quindi richiamo e precisione di un evento dipendono da eventi lontani. Su set B, Trama, 2 ms la dipendenza è dimostrata in modo meccanicistico ma retrospettivo: 10 attacchi recuperati su 10 normalizzando senza i rilasci.
- **Invarianza al guadagno:** n è invariante a un guadagno costante dello stem, a meno della compressione log1p.
- **Lunghezza dello stem:** allungare uno stem aggiungendo un transitorio più forte può soltanto alzare la soglia.

### 7.2 Loudness e true peak

- **Loudness** (BS.1770-5, EBU Tech 3341, `loudness.h`):
  - filtro K a 48 kHz, L = −0,691 + 10 log₁₀ Σ_c G_c z_c;
  - Momentary su 0,4 s, Short-term su 3 s;
  - Integrated su blocchi di 400 ms con sovrapposizione del 75 %, gate assoluto −70 LUFS e relativo −10 LU.

  Solo `MEASURED`; i valori non definiti sono `null`.
- **True peak:** sovracampionamento polifase con i coefficienti dell'Allegato 2 di BS.1770-5 (`test_synth_dsp.cpp`, «True-peak meter uses exact normative ITU-R BS.1770-5 Annex 2 table coefficients…»).

### 7.3 Statistica delle affermazioni di presenza (`presence_stats_v1`)

- **Unità:** il gruppo musicale c (stessa frase sorgente, varianti per rimozione, duplicato, oppure stesso fingerprint di frase). Per classe:

  n_eff = (Σ_c m_c)² / Σ_c m_c²  (Kish 1965),  x_eff = p · n_eff,

  con intervallo di Wilson al 95 % (z = 1,95996) su (x_eff, n_eff).
- **Stati:** `DESCRIPTIVE_ONLY` con meno di 4 gruppi, `INSUFFICIENT_DATA` con n_eff < 4, altrimenti `MEASURED`.
- **Baseline:** J_L = L_se + L_sp − 1, solo con entrambe le classi `MEASURED`.

**Ipotesi:**
- **ρ = 1 dentro il gruppo:** un gruppo vale un'osservazione pesata. Se la correlazione vera è minore, n_eff è sottostimato e l'intervallo è conservativo.
- **Indipendenza fra gruppi:** i gruppi sono definiti per identità di contenuto. Due frasi con materiale trasposto o variato, cioè lo stesso motivo in un altro grado, appartengono a gruppi diversi ma non sono indipendenti. In questo caso n_eff è sovrastimato e l'intervallo **anti-conservativo** (CL-MM-10).
- **Popolazione:** con un planner deterministico (§4.2) le finestre sono un insieme finito fissato, non un campione di musica generabile. L'intervallo descrive l'affidabilità del valutatore su questo pacchetto, non una generalizzazione. È coerente con l'etichetta `model_based_unverified`.

### 7.4 Diversità (5e-04)

Definizioni (`diversity.h`):
- **M1:** quota di coppie di seed con fingerprint identico.
- **M2:** distanza fra vettori di feature (istogramma delle classi di altezza, istogramma degli IOI, numero di note, ambito, entropia).
- **M3:** 1 − max_t |Q ∩ (R + t)| / max(|Q|, |R|) su punti (onset, MIDI), con intersezione di multinsiemi. È la misura di corrispondenza parziale sotto traslazione della famiglia di Ukkonen, Lemström e Mäkinen (2003) e Meredith et al. (2002).
- **M4:** distanza di Levenshtein normalizzata su token (Δonset, intervallo, classe di durata).

**Proprietà derivata:** per il Teorema D, sulla scansione dei seed M1 vale 1 e M2–M4 non sono definite. Lo strumento misura correttamente una diversità che il modello non può produrre: per misurare la varietà servono assi che il planner legge davvero, come parametri di forma e stato, non il seed (CL-MM-11).

### 7.5 Invarianti e verifiche

| Invariante | Verifica |
|---|---|
| FFT contro la DFT diretta, STFT e convenzione del centro del frame | `test_cacp_analysis.cpp` |
| Onset su attacchi sintetici, abbinamento a cardinalità massima | idem, più la parità E1 (`onset_parity_v1`) |
| Loudness sui segnali minimi di EBU 3341 | «Loudness: EBU Tech 3341 minimum-requirement signals…» |
| Stem: ricostruzione bit-exact del mix dei bus | `test_stem_analysis.cpp` |
| `n_eff`, stati e intervalli sui casi della review | `test_presence_stats.cpp` |
| M2–M4 su insiemi noti, M3 a multinsiemi | `test_diversity.cpp` |
| M-NORM su una curva giocattolo; regole P1–P4 fail-closed | `test_trama_diagnosis.cpp` |

---

## 8. Rilievi

| ID | Blocco | Affermazione | Evidenza | Stato | Sev. |
|---|---|---|---|---|---|
| CL-MM-01 | 1 | `frameAtPhrase` usa `barsPerPhrase` del segmento 0; `positionAtFrame` usa quello del segmento corrente sul conteggio globale: incoerenti se una mappa cambia `barsPerPhrase` | `music_clock.cpp` (`frameAtPhrase`, `positionAtFrame`) | modo di guasto latente, lacuna di verifica | LOW |
| CL-MM-02 | 3 | La trasposizione modale ripiega le note una per una: al bordo di [−7, 14] il contorno si rompe; l'oracolo delle invarianti non è applicato ai candidati | `applyModalTranspose`; `generateCandidates` non chiama `verifyMotifTransformInvariants` | modo di guasto latente, lacuna di verifica | LOW |
| CL-MM-03 | 3 | Periodicità di una battuta per Vettore, Pulso e Trama dentro la frase | `generateCandidates` (ciclo sulle battute) | verificata dal codice; collegata a F1–F4 | — (musicale) |
| CL-MM-04 | 4 | La densità SPSC conta il NoteOff dei pad incrociati alla fine della frase invece che al frame effettivo, un beat dopo | `validateHardConstraints` contro `planPhrase` | divergenza interna | LOW |
| CL-MM-05 | 4 | Vettore dei punteggi identico in tutte le frasi di una sezione, nessuno stato lungo la sezione | `tie_probe` sulla fixture | verificata | — (musicale) |
| CL-MM-06 | 2, 4 | Il seed agisce solo sui pareggi; 0 pareggi su 24 + 2 decisioni; contenuto identico per i seed 1..64 (Teorema D) | `music_planner.cpp`, `tie_probe` | verificata | — (strutturale) |
| CL-MM-07 | 4, 6 | La regola 1 del tracker (voce più vecchia) diverge da quella del motore (inviluppo minore); il tracker modella R = 96 frame anche con l'override | `SimulatedVoiceTracker::simulateNoteOn` contro `selectSlotForRole` | divergenza documentata nel codice, lacuna di verifica | LOW |
| CL-MM-08 | 6 | Il commento di `RoleBusProcessor` parla di delay «tempo-synced»; i tap sono fissi a 11 e 17 ms come da ADR-0004 | `role_bus_processor.h/.cpp` | divergenza nel commento | INFO |
| CL-MM-09 | 7 | Soglia degli onset dipendente dal massimo dell'intero stem (M-NORM) | 5e.3b, `trama_miss_diagnosis_v1` | verificata (retrospettiva) | MED (misura) |
| CL-MM-10 | 7 | `presence_stats_v1` assume l'indipendenza fra gruppi: materiale trasposto o variato in gruppi diversi la viola e rende l'intervallo anti-conservativo | `presence_stats.h`, definizione dei gruppi | ipotesi non verificata | MED (inferenza) |
| CL-MM-11 | 7 | La diversità per seed è degenere per costruzione: M1 = 1, M2–M4 non definite | Teorema D, `diversity.h` | verificata | — (strutturale) |
| CL-MM-12 | 2 | `PolicyVersionId::toU32` impacchetta campi a 8 bit senza maschera: collisioni per valori ≥ 256 | `version_ids.h` | modo di guasto latente | INFO |
| CL-MM-13 | 6 | Pendenza relativa del rilascio a 2 ms: ρ = 6,5 per la Trama, 5,6 per il Corpo, 2,1 per il Vettore, < 0,2 per il Pulso; ordinamento coerente con le rilevazioni sui rilasci di 5e.3 | §6.4, script | verificata in modo qualitativo | — (F6) |

**Verifiche positive principali**, senza rilievo: aritmetica razionale del clock e inversione dei confini; fixed step; mixer e Lemire; ordine totale degli eventi; conti O3; ADSR e primitive DSP conformi alla letteratura; limiter terminale; misure di onset, loudness, `n_eff` e Wilson come dichiarati.

**Lacune di verifica** (nessun test le copre):
- cambio di `barsPerPhrase` fra segmenti (CL-MM-01);
- trasposizione al bordo del registro (CL-MM-02);
- confronto diretto tracker contro motore (CL-MM-07);
- indipendenza fra gruppi musicali (CL-MM-10).

---

## 9. Modi di guasto trasversali

1. **Sintesi → misura → calibrazione.**
   - Il rilascio di 2 ms (§6.4) crea transitori più ripidi dell'attacco nella Trama e nel Corpo.
   - Il massimo globale del flusso cade su quei transitori (P1 di 5e.3b), la soglia sale per tutto lo stem (M-NORM) e gli attacchi deboli si perdono.
   - Il δ della Trama (0,20) è stato calibrato sulla fixture in presenza degli stessi transitori.

   Un cambio del rilascio sposta quindi insieme il suono e la scala di misura: un confronto A/B con lo stesso δ congelato mescola i due effetti.
2. **Determinismo → ripetizione → statistica.**
   - Il planner deterministico, con stato quasi costante nella sezione, produce poche frasi distinte: 14 gruppi su 24 frasi.
   - Ne seguono pochi gruppi indipendenti, quindi n_eff basso e intervalli larghi; con materiale correlato fra gruppi, anche anti-conservativi (CL-MM-10).

   La debolezza statistica della valutazione percettiva non è un problema di campionamento: deriva dalla struttura del generatore.
3. **Priorità degli eventi → confondenti.** La priorità NoteOff < NoteOn allo stesso frame (specifica §7.5) mette la voce uscente in rilascio proprio quando arriva la nota successiva. Sovrapposizioni e tagli «allo stesso frame» sono quindi un tratto costante del materiale legato, non un effetto del rilascio: vanno separati da quelli dovuti a code più lunghe (5e.3).
4. **Toolchain → identità.** Golden di piano e PCM dipendono da `std::pow` e da `sin`/`tan` in float. Il determinismo è garantito per la toolchain fissata (ADR-0001), non in generale. Un porting richiede una nuova versione dei golden, non una correzione.

---

## 10. Indicazioni per M2.5

Sono separate dai rilievi: non sono decisioni, ma punti per l'ingresso di M2.5.

- **Sorgenti di varietà reali.** Per il Teorema D la varietà deve venire da grandezze che il planner legge, non dal seed:
  - stato lungo la sezione: arco di tensione, contatori di ripetizione, memoria delle scelte;
  - varianti di motivo per battuta;
  - parametri di forma.

  È il dominio del «direttore musicale» proposto dal titolare (F5). Va misurata con assi diversi dal seed (CL-MM-11).
- **Rivelatore.** Valutare una normalizzazione locale o robusta del flusso in un `descriptors_v2` versionato, e ricalibrare δ per ruolo dopo la scelta del rilascio, non prima (§9.1).
- **Rilascio per ruolo.** Il modello delle pendenze (§6.4) suggerisce di scegliere R per ruolo in base al rapporto ρ con l'attacco, insieme all'ascolto dell'esperto (5e-08); la decisione resta di M2.5.
- **Statistica.** Dichiarare o stimare la dipendenza fra gruppi con materiale imparentato, per esempio raggruppando per motivo e sezione oltre che per contenuto identico, prima di usare gli intervalli come evidenza (CL-MM-10).
- **Lacune di test.** Sono quelle del §8: piccoli test di caratterizzazione per CL-MM-01, -02 e -07.

---

## 11. Bibliografia e livello di verifica

| Fonte | Uso | Verifica |
|---|---|---|
| Steele G. L., Lea D., Flood C. H., «Fast splittable pseudorandom number generators», OOPSLA 2014, pp. 453–472, [doi:10.1145/2660193.2660195](https://doi.org/10.1145/2660193.2660195) | mixer SplitMix64 (§2) | riferimento verificato via web il 2026-11-03 ([SPLASH 2014](https://2014.splashcon.org/details/oopsla2014/48/Fast-Splittable-Pseudorandom-Number-Generators)); testo non riletto |
| Lemire D., «Fast Random Integer Generation in an Interval», ACM TOMACS 29(1), 2019, [doi:10.1145/3230636](https://doi.org/10.1145/3230636), [arXiv:1805.10941](https://arxiv.org/abs/1805.10941) | interi senza distorsione (§2) | riferimento verificato via web il 2026-11-03; l'algoritmo del codice coincide con quello descritto nell'abstract |
| Toussaint G., «The Euclidean Algorithm Generates Traditional Musical Rhythms», Bridges 2005, pp. 47–56 | ritmi euclidei (§3) | verificato in [M2_4_Deep_Research_Reviewed.md](M2_4_Deep_Research_Reviewed.md) ([archivio Bridges](https://www.archive.bridgesmathart.org/2005/bridges2005-47.html)) |
| Välimäki V., Huovilainen A., «Antialiasing oscillators in subtractive synthesis», IEEE Signal Processing Magazine 24(2), 2007 | PolyBLEP (§6) | citato in ADR-0004 e nella ricerca rivista; forma del residuo controllata sul codice |
| Zavalishin V., *The Art of VA Filter Design*, rev. 2.1.0, Native Instruments, 2018 | SVF TPT (§6) | citato in [M2_4_Voice_Distortion_Aliasing_Evidence.md](M2_4_Voice_Distortion_Aliasing_Evidence.md); coefficienti controllati sul codice |
| Carson J. R., «Notes on the theory of modulation», Proc. IRE 10(1), 1922 | banda FM, limite dell'indice (§6) | regola citata in ADR-0003; la formula del codice ne è la forma con f_max = 0,42 f_s |
| ITU-R BS.1770-5 (2023); EBU Tech 3341 v4.0 | loudness e true peak (§7) | citati e testati in Inc-4/5 (dossier sul renderer audio, ADR-0006) |
| Bello J. P. et al., «A tutorial on onset detection in music signals», IEEE TSAP 13(5), 2005; Dixon S., «Onset detection revisited», DAFx 2006 | flusso spettrale e picking (§7) | verificati in [M2_4_CACP_Evaluation_Methods.md](M2_4_CACP_Evaluation_Methods.md) e [M2_4_Inc5_Entry_Research.md](M2_4_Inc5_Entry_Research.md) |
| Raffel C. et al., «mir_eval», ISMIR 2014 | abbinamento degli onset (§7) | verificato negli stessi dossier |
| Kish L., *Survey Sampling*, Wiley, 1965; Wilson E. B., JASA 22, 1927 | n_eff e intervallo (§7) | verificati in [M2_4_Inc5_Clustered_Claim_Statistics.md](M2_4_Inc5_Clustered_Claim_Statistics.md) |
| Meredith D., Lemström K., Wiggins G. A., «Algorithms for discovering repeated patterns in multidimensional representations of polyphonic music», JNMR 31(4), 2002, pp. 321–345 | contesto di M3 (§7.4) | riferimento verificato via web il 2026-11-03 ([Goldsmiths](https://research.gold.ac.uk/id/eprint/1008)); testo non riletto |
| Ukkonen E., Lemström K., Mäkinen V., «Geometric algorithms for transposition invariant content-based music retrieval», ISMIR 2003, pp. 193–199 | corrispondenza parziale sotto traslazione (M3) | riferimento verificato via web il 2026-11-03 ([abstract](https://cs.helsinki.fi/u/vmakinen/papers/geometric_ismir_abstract.html)); la corrispondenza fra M3 e il loro problema di matching parziale è un'interpretazione dell'agente |

---

## 12. Riproducibilità

- **Commit:** `df0bcbe`; Windows 11, MSVC della toolchain fissata (ADR-0001), build `scripts/build.ps1 -Config Release`.
- **Strumenti fuori dal repository** (scratchpad della sessione, non versionati):
  - `tie_probe.cpp`: compilato con `cl /std:c++20 /O2 /EHsc /MD` contro `cymatica_audio.lib` e `cymatica_core.lib` di `build/x64-release`. Pianifica la fixture (8 candidati) e `planner_probe_v1` (6 e 8 candidati) con i seed 1337 e 1..64, stampa per ogni decisione punteggi, pareggi e vincitore, e confronta il contenuto degli eventi fra seed. I risultati sono al §4.2.
  - Script Python per le formule del clock (inversione su 2·10⁵ casi, confini della fixture) e per le pendenze degli inviluppi (§6.4).
- **Evidenze del repository riusate:**
  - manifest di 5e.3 (`release_ab_v1`) e 5e.3b (`trama_miss_diagnosis_v1`);
  - diversità di 5e.2;
  - CI dei commit `95b58c0`, `c4857b2` e `df0bcbe`.
- **Limiti dell'indagine:**
  - nessun ascolto: l'agente non ha un canale audio;
  - gli stati «verificata» si basano su lettura del codice, test esistenti o calcoli dell'agente, non su una prova formale meccanizzata;
  - le parti di `audio_probe.cpp` non legate al motore (scenari diagnostici) sono state lette solo dove servivano.

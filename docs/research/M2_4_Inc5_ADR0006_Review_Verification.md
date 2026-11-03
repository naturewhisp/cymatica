# Verifica della review di ADR-0006 di Antigravity (fine Inc-5)

- **Data:** 2026-11-03
- **Oggetto:** [review di Antigravity](M2_4_Inc5_ADR0006_Review_Antigravity.md), rilievi AG-ADR-01..09, consegnata su `a5ee6b8d67f4facf748ec95aae145b7c851c7562`. Il [prompt](M2_4_Inc5_ADR0006_Review_Prompt.md) chiedeva di riportare per ogni rilievo l'estratto esatto con `file:riga`.
- **Metodo:** ogni numero, estratto di codice e citazione di ADR-0006 del dossier è stato cercato nel repository (`grep`, lettura diretta) e confrontato con le evidenze archiviate in `docs/research/experiments/`. Le fonti bibliografiche **non** sono state riverificate: vanno lette alla fonte prima di essere usate in M2.5.
- **Status:** descrittivo. Non modifica decisioni, contratti né stati di Inc-5.

## 1. Esito in sintesi

Come per il [dossier sul modello matematico](M2_4_Inc5_Math_Model_Reconciliation.md), le regole di verificabilità del prompt **non sono state rispettate**. Lo SHA del commit è corretto, il dossier non contiene link `file:///` e i simboli elencati nella ricerca 1 esistono. Tuttavia:

- molti estratti presentati come «esatti» non esistono nel repository: codice riscritto, costanti con altri nomi, passi di ADR-0006 mai scritti;
- quasi tutti i numeri delle evidenze sono diversi da quelli archiviati, alcuni con conclusioni opposte.

Le conclusioni che restano valide coincidono con osservazioni già registrate oppure sono informative. Il dossier si cita solo per i punti confermati qui.

## 2. Affermazioni fattuali verificate

| Affermazione del dossier | Repository | Esito |
|---|---|---|
| H1: set B `2/19`, `planner_probe_v1` `0/14`, fixture `54/250 = 0,216` | [`release_ab_v1_2026-11-03_dif5e3.json`](experiments/inc5_release_ab/release_ab_v1_2026-11-03_dif5e3.json): `set_b 9/102 = 0.088; planner_probe_v1 0/23 = 0.000; fixture 245/402 = 0.609` | **falsa** (numeri inventati; l'esito NOT_SUPPORTED è corretto) |
| H1 fallisce sulla fixture per Corpo: `36/141 → 51/165`, overlap di Corpo 160, causa nel ducking | Fixture, rilevazioni sui rilasci 2 ms → 30 ms: Trama 400 → 243, Corpo 1 → 1, Vettore 1 → 1, Pulso 0 → 0; overlap di Corpo 95 in entrambe le varianti | **falsa**: il fallimento viene dalla Trama, non da Corpo |
| 5e.3c: previsti 4 gruppi, misurati 9, quindi previsione «smentita» | ADR-0006 5e.3c: previsti **8** gruppi, partizione esatta **confermata** (`CONFIRMED`) dal test «Symbolic groups of fixture-m2-4-v1 follow the prediction…» | **falsa** |
| `n_eff,+` = 5,793 in v3, 7,560 in v2 | ADR-0006 5e.3c: v3 4,93 positivi e 3,77 negativi; v2 10,92 e 8,89 | **falsa** |
| Criterio «gruppi ≤ 6 e `n_eff,+ ≥ 6,0`», citato come testo di ADR-0006 | Il testo compare solo nel dossier di Antigravity | **inventato** |
| Rinvio della sessione alla cieca perché `n_eff,+` = 5,793 < 6,0 | Motivo registrato: `n_eff` dei negativi 3,77 < 4, quindi specificità `INSUFFICIENT_DATA` per qualunque valutatore; poi decisione del titolare di rinviare l'intera sessione | **falsa** |
| M-NORM: `M(2 ms) = 50,858`, `M(30 ms) = 30,233`, massimo a 92,213 s | [`trama_miss_diagnosis_v1_2026-11-03_dif5e3b.json`](experiments/inc5_trama_diagnosis/trama_miss_diagnosis_v1_2026-11-03_dif5e3b.json): normalizzatore 259,229 a 2 ms e 157,050 a 30 ms (rapporto 1,65) | **falsa** (numeri inventati; il meccanismo è corretto) |
| Q2 riporta come estratto di `trama_diagnosis.h:27-36` le costanti `kTramaCausalId`, `kNormaliserRatioMin`, `kCounterfactualRecallMin`, `kWeakestAttackRankMax` e una P3 che vuole «i 10 mancati fra i 12 più deboli» | Le costanti sono `kP1MinNormaliserRatio`, `kP2MinRecall`, `kP3LowestCount`, `kP3MinMissedAmongLowest`, `kP4MinMerged`; P3 richiede **almeno 8** dei 10 | **estratto inventato** |
| AG-ADR-04: con un numero di mancati diverso da 10, `evaluateP3` restituisce `NOT_SUPPORTED` | `trama_diagnosis.cpp:225-227` restituisce `INCONCLUSIVE` | **falsa** |
| AG-ADR-05: `classStats` centra Wilson sulla media non pesata dei gruppi, `c.rate = sumP / c.groups` | `presence_stats.cpp:50`: `c.rate = correct / claims` (tasso aggregato); Wilson su `(rate·n_eff, n_eff)` (`:57`); `groups < 4` dà `DESCRIPTIVE_ONLY` e `n_eff < 4` dà `INSUFFICIENT_DATA` (`:51-54`) | **falsa**: estratto inventato, stati invertiti |
| Estratto di `onset.cpp:54-60` con `std::max_element` e flux `log(1 + 1000·E)` | Il normalizzatore è `NoveltyCurve::normaliser` (`onset.cpp:61`); il flux usa `log1p(logGamma·|X|)` | **estratto inventato** (la normalizzazione globale esiste) |
| `thresholdDelta = 0,08` nel profilo congelato | Il profilo `acao02_v1` ha un δ per ruolo, scelto da una griglia che non contiene 0,08 | **non riscontrata** |
| AG-ADR-09: 8 trasformazioni di Vettore (`Transpose`, `Invert`, `Retrograde`, `Augment`, `Diminish`, `DensityThin`…) | `music_planner.h:95-103`: 7 tipi (`Identity`, `ModalTranspose`, `OctaveShift`, `Reorchestrate`, `ContourInversion`, `EuclideanRhythmVariation`, `ChromaticAlteration`) | **falsa** |
| Fingerprint `14850868393822185080` citato da ADR-0006:751; `REQUIRE(div.uniqueFingerprints == 1)` in `test_diversity.cpp:95-99` | Il valore non compare in ADR-0006; il test usa `CHECK(d.distinctFingerprints >= 1)` con ramo condizionato | **inventati** |
| Citazioni di ADR-0006 alle righe 463 (5d-05), 576, 629, 721, 722, 815 | Testi assenti: 5d-05 è «Verità di `role_onset_detected`»; l'impegno del seed sta in 5d-12; «Ambito epistemico», «non scendono ai 4», «36/141» non compaiono | **inventate** |
| True peak di `fixture_probe_v1` = −0,871 dBTP | Valore non presente nelle evidenze; il [dossier delle evidenze](../m2_procedural_evidence.md) riporta −10,45 dBTP sul brano | **non riscontrata** |
| Build Release headless 231/231 | Coerente con le chiusure di 5e.4a e 5e.4b in `progress.md`; output non riverificato | plausibile |

## 3. Rilievi AG-ADR

| ID | Valutazione | Esito per il progetto |
|---|---|---|
| AG-ADR-01 | **Valido nel merito**, coincide con OSS-5e.3b-R2-01 (già aperta). Estratti imprecisi: il campo è `outcome`, non `status`; la funzione è `frozenStemProfileV1()`, non `readFrozenProfile`. | nessuna nuova azione; resta l'OSS aperta |
| AG-ADR-02 | **Valido nel merito**, coincide con OSS-5e.2-R2-01 (aperta per M2.5). Codice e citazione di ADR-0006 riscritti: il ciclo reale è in `evaluation_pack.cpp:1768-1786`, con `run.errors`. | nessuna nuova azione |
| AG-ADR-03 | **Valido e nuovo:** `packSeedCommitment` (`evaluation_pack.cpp:458-460`) usa il prefisso `evaluation_pack_v1` anche per v2 e v3. Non è un difetto di sicurezza: l'impegno resta verificabile, perché generazione e checker usano la stessa funzione. Manca solo la separazione di dominio per versione. La citazione di 5d-05 è errata (l'impegno è in 5d-12). | informativo; proposta: documentare il prefisso, o versionarlo in un futuro `evaluation_pack_v4` |
| AG-ADR-04 | Conclusione corretta (5e.3b è retrospettiva, come dichiarano ADR-0006 e il commento di `trama_diagnosis.h`); estratti e codice di ritorno errati. | nessuna azione |
| AG-ADR-05 | **Premessa falsa:** lo stimatore implementato è il tasso aggregato, per cui `n_eff` di Kish con ρ = 1 è esattamente la taglia effettiva corretta, come il dossier stesso deriva in §3.1. Il «paradosso» descritto non esiste. Anche J_L è a posto: con due limiti Wilson al 95 % bilaterale, ciascuno ha copertura unilaterale del 97,5 %, e per la disuguaglianza di Boole J ≥ J_L vale con probabilità ≥ 95 % qualunque sia la dipendenza fra le classi. Non serve una correzione di Bonferroni (il dossier lo riconosce in §3.1 punto 2). **Restano utili per M2.5:** copertura di Wilson con `n_eff` piccolo; test di permutazione a livello di gruppo come alternativa; CL-MM-10 (già noto). | proposta M2.5, con premessa corretta |
| AG-ADR-06 | **Premessa falsa** (non c'è stata alcuna smentita: previsione di 8 gruppi confermata). La spiegazione delle differenze strutturali fra frasi coincide con quella già scritta nella previsione di ADR-0006. | nessuna azione |
| AG-ADR-07 | **Meccanismo corretto** e già registrato (M-NORM, 5e.3b SUPPORTED); numeri ed estratti inventati. Le direzioni bibliografiche (SuperFlux, adaptive whitening, PCEN, normalizzazione per quantile) sono pertinenti per la scelta di M2.5 fra normalizzazione globale, locale e robusta. | proposta M2.5, fonti da leggere prima dell'uso |
| AG-ADR-08 | **Spiegazione falsa** di H1 (vedi §2: il fallimento è della Trama sulla fixture, 243 rilevazioni sui rilasci a 30 ms). La raccomandazione generale coincide con l'handoff H-03 già registrato: rilascio musicale per ruolo, separato dal de-click di sicurezza. L'argomento spettrale (lobo di 1/T_r, decadimento O(ω⁻²) della rampa lineare) è un ragionamento plausibile, non una misura. | nessuna nuova azione; H-03 resta il riferimento |
| AG-ADR-09 | Conclusione coerente con il Teorema D e con la riconciliazione: la varietà deve venire dallo stato. Dettagli del planner inventati. Proposte M2.5 utili da valutare: penalità di recenza/satiation nello scoring; selezione deterministica guidata dal substream del ruolo fra candidati entro una banda dal massimo; parametri macro dal direttore musicale (F5). | proposta M2.5 (F5) |

## 4. Cosa resta utilizzabile

1. **AG-ADR-03**, unico rilievo nuovo sul codice: informativo, nessun rischio per l'integrità del pacchetto.
2. **Proposte per M2.5**, da istruire con fonti lette:
   - normalizzazione robusta o locale dell'onset, AG-ADR-07;
   - statistica a livello di gruppo con permutazioni, AG-ADR-05 con la premessa corretta;
   - varietà per stato e selezione deterministica fra quasi-ottimi, AG-ADR-09.
3. **AG-ADR-01 e -02:** confermano due osservazioni già aperte.

Nessun rilievo cambia decisioni, contratti o stati di Inc-5. La review di Antigravity si considera **eseguita con esito «nessun difetto nuovo bloccante»**, ma **non affidabile come fonte di numeri o estratti**.

## 5. Indicazioni per le prossime consegne di Antigravity

Le regole del prompt erano già esplicite, quindi un prompt più lungo non basta. Proposta:
- chiedere l'output grezzo di ogni `Select-String` usato per un estratto, non un estratto riscritto;
- far estrarre i numeri delle evidenze da uno script che legge i JSON archiviati, allegando l'output;
- verificare a campione, prima di leggere le conclusioni, almeno cinque numeri e cinque estratti, come fatto qui.

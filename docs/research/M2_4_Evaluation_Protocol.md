# CYMATICA — Protocollo sperimentale della qualità musicale e audiovisiva

- **Stato:** protocollo di ricerca **proposto**, non gate normativo M2 né studio già eseguito.
- **Riferimenti:** [rapporto revisionato](M2_4_Deep_Research_Reviewed.md), [review](M2_4_Research_Review.md), [piano M2.4](M2_4_Implementation_Plan.md).
- **Principio:** confrontare opzioni con materiale comparabile, senza confondere qualità tecnica, correttezza compositiva, preferenza e originalità. Il giudizio umano non si sostituisce con un criterio FFT o una preferenza dichiarata da un agente.

## 1. Ipotesi da testare

| ID | Ipotesi | Contrasto controllato | Collocazione |
|---|---|---|---|
| H-COMP-1 | Piano gerarchico + motif memory migliora riconoscibilità e coerenza | A (scala/Euclidean) vs B (A+forma/motivo) | M2.4 |
| H-COMP-2 | Funzione armonica e voicing bounded aggiungono intenzionalità | B vs C (B+harmonic function/voice-leading) | M2.4 o post, se pronto |
| H-COMP-3 | Variation engine probabilistico fornisce ulteriore qualità rispetto a C | C vs D (MVS/factor oracle subordinato) | ricerca post-M2, **non** milestone obbligatoria |
| H-SYN-1 | Renderer role-specific supera un timbro uniforme sul materiale identico | S1 vs S2/S3/S4 | M2.4 pilot |
| H-VOICE-1 | Voci condivise/12–16 migliorano l'espressione più di un'orchestrazione sparsa a 8 | 8 fixed vs 8 shared vs 12–16 | offline post-M2, ADR solo se necessario |
| H-GAME-1 | Identità/trasformazione motivo→geometria aumenta riconoscimento e anticipazione | G1 beat-only vs G2 intent-symbolic vs G3 structural | M3–M7 |
| H-SURP-1 | Coordinare sorpresa musicale e ludica evita sovraccarico cognitivo | budget indipendente vs budget condiviso | M6–M7 |
| H-TIM-1 | Profili di microritmo deterministici migliorano un particolare stile rispetto a griglia esatta | esatto vs pushed/laid-back vs jitter | post baseline, non a priori |

## 2. Fattori di controllo e materiali

Per il **confronto compositivo** mantenere costanti: seed high-level e forma bersaglio, BPM/metro/durata per condizione quando confrontabili, renderer/sound bank/mix, livello di uscita, numero massimo di voci, scenari di ascolto e labelizzazione dei clip. Le varianti modificano soltanto il sottosistema oggetto dell'ipotesi; salvare tutto ciò che differisce intenzionalmente.

Per il **confronto di sintesi**, congelare rigorosamente il piano di `MusicIntentEvent` e cambiare solo renderer/palette. L'impressione di "miglioramento compositivo" attribuita a un synth più brillante è un confondente da evitare.

Per il **confronto cross-modale**, mantenere allineati difficulty target, hazard budget, telegraph, seed gameplay e fairness validator; non utilizzare livelli non comparabili per attribuire differenze all'informazione musicale. Randomizzazione e ordine dei test non devono poter modificare la policy generativa della run.

## 3. Disegno del pilot

- Creare un **set di seed congelato**, include casi lenti, densi, cadenzali, con pad sustain e transizioni. Dimensionare seed e durate dopo un primo spike; preferire **decine di seed** per screening offline, con campione di clip 60–90 s e alcune run di più minuti per fatica/monotonia.
- Per ogni coppia A/B, stessi seed e lunghezza utile: ordine di ascolto randomizzato/controbilanciato, sigle anonime, file loudness-matched entro un range predefinito, nessun clipping, stesso monitor/output.
- Form **within-subject** nel pilot: ciascun partecipante ascolta più condizioni ma non sempre la stessa sequenza. Registrare se musicista, compositore/sound designer o giocatore non esperto senza informazioni non necessarie.
- Distinguere ascolto dell'agente/CACP come **supporto qualitativo supplementare**: non deve essere contato come partecipante umano né come verifica di generalizzazione estetica.
- Il pilot ha finalità esplorativa: non usare p-value per dichiarare eccellenza artistica. Uno studio confermativo richiederebbe ipotesi primarie/preregistrazione, power analysis e campione adeguato; per dati ripetuti considerare modelli mixed-effects/analisi appropriate.

## 4. Valutazioni separate

| Dimensione | Misura/strumento suggerito | Interpretazione vietata |
|---|---|---|
| Tecnica | underrun, missed deadlines, CPU/memoria, clip/gap, allocazioni RT | 0 errori => bella musica |
| Forma | contrasto tra sezioni, ritorno del motivo, risoluzione di frase, distanza trasformazionale | alto contrasto => buona forma |
| Ritmo | onsets per misura, syncopation/regularity, intervallo percepito dopo inviluppo | jitter => automaticamente groove migliore |
| Armonia | distribuzione registro, motion score, candidati scartati, cadenze | meno motion => sempre migliore |
| Diversità | distanza tra seed, novelty, near-duplicates, coverage | entropia maggiore => originalità artistica |
| Percezione | scelta pairwise e rating coerenza, espressività, qualità timbrica, interesse, monotonia, transizioni | preferenza del pilota => validità universale |
| Gameplay | riconoscimento pattern, anticipazione, performance, fairness percepita, affaticamento | più collisioni evitate => prova sola della superiorità musicale |

### Mini-questionario suggerito (1–7, più risposta aperta)

1. La musica presenta un'idea riconoscibile che si sviluppa nel tempo?
2. Le transizioni sembrano motivate/organiche piuttosto che arbitrarie?
3. Armonia e frase danno una sensazione di direzione, non solo una serie di note?
4. Il timbro è caratteristico e appropriato ai ruoli?
5. La densità rimane leggibile e non affaticante nel contesto di gioco?
6. A parità di volume, quale clip preferisci e perché? (forced choice, consentire indifferenza)
7. Dopo l'ascolto, il motivo A' è riconosciuto come trasformazione del motivo A? (test separato con randomizzazione)

Evitare di condensare automaticamente i rating in un singolo `artistic_score`; usare giudizi e note come evidenze con incertezza.

## 5. Protocollo realtime DSP

### Setup

Identificare: hash commit, build Release/Debug, versione compiler/OS, CPU, core count, device audio, sample rate device/logico, numero canali, framesPerCallback effettivi, driver/backend, load profile, ottimizzazioni CPU, volume e stati CACP. Non confrontare valori di piattaforme/quantum diversi come se fossero equivalenti.

### Carichi

- 8 voci contemporanee, inviluppi indipendenti, release/steal e nuovo `NoteOn` sullo stesso slot.
- Scene ad alta densità Pulso/Trama con filtri/modulazioni in intervalli massimi validi.
- Cambio di metro/BPM con note sostenute e cross-epoch `NoteOff`, tempo in 44.1/48 kHz.
- Scheduler backpressure e note off scartate/duplicate/out-of-order, pausa/stop/resume, startup/shutdown.
- CACP capture OFF e ON con consumatore lento, senza I/O callback.
- 3 minuti di ascolto minimo per M2.5 e soak esteso secondo profilo/requisito della milestone (M7 prevede sessione lunga).

### Metriche

Registrare per callback tempo CPU o wall time con clock monotono appropriato, p50/p95/p99/p99.9, massimo osservato, budget ratio (`time_callback / period_callback`), max voice occupancy, voice steal/drop e note orfane, underrun device effettivi se il backend li espone, sample gap e lag di presentation cursor. Il budget **≤2 ms per 512 frame a 48 kHz** viene dall'addendum di ADR-0003: è un target dichiarato, da misurare e riportare come PASS/FAIL del profilo testato; non è garanzia matematica su tutti gli hardware. Se fallisce, correggere DSP o registrare una revisione autorizzata del profilo; non allargare silenziosamente la soglia.

Il tempo limite 50 ms del test M2.3 di flooding protegge contro hanging, non dà margine su callback da 10,667 ms. La registrazione non deve utilizzare file I/O, log o allocazioni nella callback.

## 6. Evidenza / manifest consigliato

Directory locale `artifacts/research/m2_4/<experiment-id>/` (non committare grandi WAV per default). Ogni esperienza esporta un manifest **versionato** con:

```json
{
  "schema_version": 1,
  "experiment_id": "H-COMP-1-pilot-v1",
  "source_commit": "<actual full commit sha>",
  "profile_id": "<versioned build/audio/policy profile>",
  "seed_set_id": "<frozen set identifier>",
  "condition_id": "A-or-B",
  "generator_version": "<actual version>",
  "synth_version": "<actual version>",
  "sound_bank_hash": "<if present>",
  "duration_frames": 0,
  "output_sha256": "<calculated hash>",
  "technical_verdict": "NOT_RUN",
  "human_evaluation": "NOT_RUN"
}
```

I valori sono **segnaposto**, non uno schema normativo né una falsa prova già eseguita. Eventuali campi aggiunti al CACP runtime devono seguire il processo decisionale CACP; i log di ricerca offline possono avere un formato separato.

## 7. Stop/go e transizione fra milestone

**M2.4:** confronto minimale A vs B e prova timbrica come *decision-support*; non bloccare il completamento normativo se manca una preferenza soggettiva significativa. Tuttavia, un risultato negativo va documentato e non può essere pubblicizzato come eccellenza artistica.

**M2.5:** obbligatori i gate tecnici della Spec; pilot di ascolto con evidenza esplicita e conclusioni limitate. Se emergono crackle, clipping, note orfane o deadline failure, il gate tecnico è NON OK anche se l'ascolto soggettivo appare gradevole.

**M3–M7:** valutare mappature G1/G2/G3 soltanto su candidati *fair* validati; alla vertical slice M7 puntare a playtest umano audiovisivo comparativo oltre ai criteri già previsti, con obiettivi fissati prima della misura.

**Post-M2:** D o S4 o >8 voci richiedono esito A/B favorevole e un ADR appropriato, non vengono imposti dalla ricerca.

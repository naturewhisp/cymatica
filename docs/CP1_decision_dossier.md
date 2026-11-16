# CP-1 — Dossier delle decisioni

- **Stato:** preparato il 2026-11-16; **tutte le proposte approvate dal titolare il 2026-11-16**, con un'annotazione su D7 (varietà della sessione d'ascolto). Registro delle decisioni: ADR-0007 §10, D-M2-21..29. Per ogni domanda: evidenze, opzioni e una proposta motivata. Gli esiti possibili di ogni voce sono quelli di ADR-0007 §6: procedi, approfondisci, rinvia.
- **Fonte delle domande:** [ADR-0007](adr/0007-m2-5-entry-decisions.md) §4 (domande di CP-1), §5 (calendario), §7 (ricerche) e §8.1 (suggerimenti dell'esperto in scadenza).
- **Preparazione completata:** M2.5.1 chiusa (a, b, c, d); prova del volume USB (R-M25-12); parte anticipata di R-M25-3; bozza del protocollo d'ascolto; monitor `IAudioClock` con controllo positivo e ascolto controllato. Resta facoltativo il confronto controbilanciato di MMCSS (R-M25-10).

## Riepilogo

| # | Decisione | Proposta |
|---|---|---|
| D1 | Cambio di metro (P-M25-01) | Opzione **B + C**: fixture in 4/4 con emendamento motivato della specifica §32, più uno scenario tecnico che dimostri il cambio di metro |
| D2 | Criterio PASS del device | Procedura R-M25-8 A–C come criterio, con le aggiunte delle prove di CP-1 |
| D3 | Canale ETW `/GlitchDetection` | **Non attivarlo ora**; riconsiderarlo se un artefatto viene sentito e non rilevato |
| D4 | Default di MMCSS nel runtime | **Rinviare** a R-M25-10; nessun cambio ora |
| D5 | Margine DSP per gli effetti | Prendere atto del margine misurato; ogni effetto con la sua misura prima dell'integrazione |
| D6 | Perimetro di M2.5.2 | F4 e F2 come previsto, **più il rapporto FM**; LFO, saturazione e supersaw come A/B successivi, uno per pacchetto; la riassegnazione delle voci fuori da ADR-0007 |
| D7 | Protocollo d'ascolto (P-M25-03) | Adottare la bozza, con eventuali modifiche del titolare |
| D8 | R-M25-10..12 in ADR-0007 | R-M25-12 eseguita e chiusa; R-M25-10 aggiunta prima di M2.5.6; R-M25-11 solo se D1 sceglie A |
| D9 | Voci di §8.1 in scadenza a CP-1 | Esiti come in D6 e D7 |

## D1 — Cambio di metro (P-M25-01)

**Domanda.** La fixture deve contenere un cambio di time signature, o basta il cambio di tempo?

**Evidenze.**
- [Fattibilità](research/M2_5_1b_Meter_Feasibility.md): orologio, scheduler e motore reggono 4/4 → 3/4 → 4/4 fino al PCM, senza modifiche. Il motivo di 16 step non entra in 3/4 e 6/8 (fallback silenzioso); un motivo di 12 step basta.
- Manca una gerarchia di accenti per metro: un 3/4 si genera ma nulla dimostra che si senta. Nessun ascolto.
- La specifica §32 chiede «transizione di tempo/metrica via `SetTempoMap`»; il dossier delle evidenze segna il criterio come parziale. La review (OSS-M2.5.1b-01) chiede una riconciliazione normativa esplicita prima di rinunciare al metro.

**Opzioni** (dal dossier di fattibilità):

| Opzione | Costo | Rischio |
|---|---|---|
| A. Metro diverso nella fixture | Motivi per metro, gerarchia degli accenti, due formule della fixture e una sonda, golden rigenerati, ascolto; ricerca R-M25-11 | Un metro senza accenti propri chiude la voce senza differenza udibile |
| B. Fixture in 4/4 con emendamento della specifica | Una modifica di §32, motivata e registrata | Senza emendamento è un indebolimento silenzioso |
| C. Scenario tecnico dedicato | Promuovere la prova a scenario con manifest | Rafforza la capacità tecnica, non la presenza nel contenuto: da sola non basta |

**Proposta: B + C.** Il cambio di metro è già dimostrato tecnicamente; renderlo udibile richiede lavoro musicale (accenti, motivi) che oggi non ha una motivazione artistica dichiarata. Si emenda §32 dicendo che in M2 la transizione obbligatoria nel contenuto è quella di tempo e che il cambio di time signature è qualificato da uno scenario tecnico ripetibile. Se il titolare vuole un metro diverso come scelta musicale, è A con R-M25-11, da pianificare in M2.5.3 o più avanti.

## D2 — Criterio PASS del device

**Domanda.** Quando una corsa sul device conta come superata?

**Evidenze.**
- [Dossier R-M25-8](research/M2_5_1_Windows_Audio_Glitch_Detection.md), procedura A–C: controllo positivo per uscita; PCM uguale al golden; nessun sospetto underrun; deficit `IAudioClock` senza salti oltre 5 ms; loopback senza slittamenti, silenzi o raffiche; overflow 0.
- [Monitor `IAudioClock` e controllo positivo USB](research/experiments/cp1_device_clock/README.md): superato (25 ms nessun buco, 60 ms buco di 30 ms visto ogni volta); corsa pulita su tutti i rilevatori; ascolto del titolare senza artefatti.
- [Prova del volume USB](research/experiments/cp1_volume_loopback/README.md): il cursore per app del mixer di Windows attenua il loopback come livello²; con il cursore a 1 il loopback è uguale al golden bit per bit.
- Una corsa sola non stima gli eventi rari.

**Proposta.** Criterio PASS per uscita, tutti i punti insieme:
1. controllo positivo `glitchcal` superato sull'uscita, dopo ogni cambio di driver;
2. binario compilato da un commit (OSS-CP1-CLOCK-03);
3. manifest: endpoint e formato attesi, rerouting disattivato e client invariato, cursore dell'app a 1 e volume di sistema registrati;
4. PCM della callback uguale al golden; `suspected_underruns` 0; nessun salto `IAudioClock` oltre 5 ms; loopback con 0 slittamenti, silenzi, raffiche e overflow;
5. ascolto umano della stessa corsa senza artefatti (Q1);
6. **tre corse** per uscita qualificata, non una, con il limite dichiarato sugli eventi rari.

Un rilevatore che non reagisce al controllo positivo rende la corsa `INVALID` per quell'uscita, non superata.

## D3 — Canale ETW `/GlitchDetection`

**Evidenze.** 24 eventi `op_EVT_GLITCH_*` esistono nel canale; attivarlo richiede privilegi di amministratore e cambia una configurazione di sistema, in modo reversibile. Non è stato verificato che segnali i buchi della modalità condivisa usata dal motore.

**Opzioni.** Attivarlo ora come quinto rilevatore; non attivarlo.

**Proposta: non attivarlo ora.** Sulle cuffie USB i quattro rilevatori verificati e l'ascolto concordano. Lo si riconsidera se un artefatto viene sentito e nessun rilevatore lo vede, o per qualificare un'uscita senza loopback affidabile.

## D4 — Default di MMCSS nel runtime

**Evidenze.**
- Prima serie, con un gioco aperto: senza MMCSS una callback ha superato il periodo (10,87 ms), con MMCSS massimo 0,49 ms. Una sola serie, condizioni non controllate.
- [Carico controllato](research/experiments/m25_device_baseline/README.md): nessun beneficio riprodotto; il consolidamento §6.1 spiega che quella prova non mette in difficoltà il thread audio.
- R-M25-10 propone uno stress con controllo positivo e un confronto controbilanciato.

**Proposta: rinviare a R-M25-10.** Le evidenze non bastano né per cambiare il default né per escluderlo. Il probe continua a usare `games`.

## D5 — Margine DSP per gli effetti

**Evidenze.** D-M2-10: p99.9 ≤ 2,0 ms per 512 frame; carico peggiore misurato 0,27–0,43 ms; sul device p99.9 0,20 ms per blocco di 480 frame. [R-M25-3 anticipata](research/CP1_RM25_3_Early_Results.md): supersaw a 7 oscillatori su 8 voci 0,15 µs per frame; saturazione ADAA1 ≈ 30 ns per campione, sovracampionamento 2× con FIR diretto ≈ 180 ns; LFO sull'SVF ≈ 18 ns per campione; senza azzeramento dei denormali una coda è 8 volte più lenta.

**Proposta.** Prendere atto di un margine indicativo di circa 1,6 ms per 512 frame. Ogni effetto entra solo con la sua misura nel profilo D-M2-10 e con l'azzeramento dei denormali nelle strutture a retroazione. Riverbero, chorus e delay restano a R-M25-3 dopo CP-1.

## D6 — Perimetro di M2.5.2

**Evidenze.**
- [Loudness per stem](m2_procedural_evidence.md): Pulso circa 10 LU sotto Corpo e Vettore, nella direzione dei 7–9 dB stimati dall'esperto (F4).
- Rapporto FM della Trama: con m/c = 1,88 il modello di Sethares dà un'autodissonanza alta; cambiare la costante 1,76 lascia invariati piano e trace e aggiorna solo il golden PCM ([dossier R-M25-7](research/M2_5_F5_Music_Director_Research.md)). Da confermare all'ascolto.
- Proposte timbriche dell'esperto (LFO, saturazione, supersaw): costi misurati in D5; la supersaw sta dentro una voce, ma le fasi casuali dell'originale vanno prese da uno stream deterministico (Szabo 2010).
- «Riassegnare le voci libere» modifica D-M2-03 (due voci per ruolo): richiede un ADR, fuori dal perimetro di ADR-0007.

**Proposta.**
1. M2.5.2 comprende F4 (kick), F2 (stereo della Trama) e **il rapporto FM della Trama**, ciascuno con A/B, null test e golden aggiornato con motivo.
2. LFO sul filtro, saturazione e supersaw entrano come **A/B successivi**, uno per pacchetto, dopo F4, con la misura di costo e un'ipotesi registrata prima dell'ascolto.
3. La riassegnazione delle voci libere si rinvia a un ADR dedicato, fuori da M2.5.

## D7 — Protocollo d'ascolto (P-M25-03)

**Evidenze.** [Bozza](m2_5_listening_protocol_P-M25-03.md): scale 1–7 del 2026-10-28, descrizione libera prima delle scale, istante del calo, costrutti separati, controlli B0, Q3 per sezione, domande semplici per il secondo ascoltatore, ipotesi con hash prima della sessione, volume fisso per giudicare il kick, cursore dell'app verificato a 1.

**Proposta: adottarla**, con le modifiche che il titolare ritiene utili, prima dell'ascolto di CP-2.

## D8 — Ricerche R-M25-10..12 in ADR-0007

| Ricerca | Stato | Proposta |
|---|---|---|
| R-M25-10, stress con controllo positivo e MMCSS | proposta | Aggiungere ad ADR-0007, prima di M2.5.6 (criterio «senza crackle») |
| R-M25-11, metro udibile | proposta | Solo se D1 sceglie A |
| R-M25-12, volume e loopback | **eseguita e chiusa** ([risultati](research/experiments/cp1_volume_loopback/README.md)) | Registrarla come conclusa |

## D9 — Voci di §8.1 in scadenza a CP-1

| Voce | Esito proposto |
|---|---|
| LFO sul filtro a quarti o ottavi | Adottata come A/B in M2.5.2 (D6.2) |
| Saturazione o distorsione | Adottata come A/B in M2.5.2 (D6.2); ADAA o sovracampionamento secondo la misura |
| Supersaw sul lead | Adottata come A/B in M2.5.2 (D6.2), con fasi deterministiche |
| Modulazioni di altri parametri | Rinviata: dopo l'esito dell'LFO |
| Volume per giudicare il kick | Adottata nel protocollo (D7) |

## Che cosa succede dopo le decisioni

Le decisioni prese vanno registrate in ADR-0007 (decisione, motivo, alternative, contratti toccati, verifica) e, per D1 se si sceglie B, nella specifica §32. Poi M2.5.2 parte secondo D6.

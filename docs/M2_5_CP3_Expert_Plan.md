# M2.5 CP-3 — Piano degli interventi dopo l'ascolto cumulativo

- **Stato:** proposto il 2026-11-20. Ogni intervento si avvia con una sua pre-registrazione.
- **Ordine delle decisioni (titolare, 2026-11-23):** prima tutte le indagini I1–I6, poi le adozioni insieme. I1–I2: indagine conclusa, candidato Do2 (MIDI 36) con click +3 dB, da rimisurare con il basso.
- **Decisione del titolare (D-M2-51):** «mi fido dell'esperto, mettiamo a piano gli interventi e le indagini necessarie ad avvalorare e confermare il giudizio dell'esperto».
  - Dove i due ascoltatori del 2026-11-19 non concordano, vale l'esperto, i cui controlli B0 sono superati.
  - Ogni giudizio dell'esperto va **avvalorato** da misure e fonti verificate prima di intervenire, e **confermato** da un ascolto mirato dopo.
- **Fonti:**
  - [ascolto cumulativo del 2026-11-19](m2_5_cumulative_listening_2026-11-19.md);
  - [analisi della produzione dell'esperto](research/M2_5_Expert_Production_Analysis_Results.md);
  - ADR-0007 §8.1;
  - ascolti del [2026-10-28](m2_4_human_listening_2026-10-28.md) e del [2026-11-03](m2_4_human_listening_2026-11-03.md).
- **Collocazione:** CP-3 di M2.5.3–4. Gli interventi precedono il congelamento del contenuto (CP-4 e M2.5.5), perché cambiano il PCM.

## 1. Che cosa resta deciso

L'esperto conferma alcune scelte già fatte. Restano come sono:

| Voce | Giudizio dell'esperto | Decisione |
|---|---|---|
| Livello della Trama (voce 1, D-M2-36) | «è giusto» | **−9 dB resta**; la voce si chiude |
| Timbro della Trama (voce 2) | non fastidioso | Resta FM 1:1 |
| Tempo forte e spostamento in B (voce 7, F1) | percepito «sempre» in tutte le parti; spostamento «voluto» | F1 resta |
| Entrate e uscite (voce 12, D-M2-44) | «diano forma al brano» | Tabella delle maschere invariata |
| Sensibile in B (voce 17, D-M2-49) | nessun accordo fuori posto; preferito il brano 2 per la parte centrale | L'alternativa 2 resta |
| Ritorno di A′ (voce 13) | sì | Ripresa invariata |
| Scatti e acuti (voci 4 e 6) | nessuno; equilibrati | Chiuse |
| Palette chiptune (voce 16) | «molto acido», preferita la produzione in entrambe le coppie | Resta uno spike: non entra nel gioco |

## 2. Interventi, in ordine

L'ordine segue due criteri:
- **la dipendenza:** il kick e il basso si dividono la stessa banda, quindi si fanno insieme e prima della forma, che ne userà gli strati;
- **l'urgenza dichiarata dall'esperto:** il basso è chiesto tre volte, dal 2026-11-03.

| # | Intervento | Giudizio dell'esperto | Indagine che lo avvalora, prima | Conferma, dopo |
|---|---|---|---|---|
| **I1** | **Kick ad altezza fissa** fra le sezioni | «non ho gradito il cambio di nota sul kick»; il kick si accorda per qualità sonora, non per tonalità | **Misura:** già verificato che Pulso segue la tonica (Re2 in A, Sol1 in B). Per 3–5 altezze fisse candidate si misurano energia sotto 100 Hz, durata del decadimento e mascheramento con Corpo. **Fonti:** letteratura sull'accordatura della batteria e sul kick elettronico, verificata | Una coppia breve sul confine A→B, altezza che segue la tonica contro altezza fissa |
| **I2** | **Kick più presente** | «gradirei un kick un po' più presente» (anche il 2026-11-03) | **Misura:** loudness di Pulso per sezione e rapporto kick/mix; transiente nella banda 2–5 kHz (il «click» che dà presenza) contro la coda sotto 100 Hz. Si cerca **dove** manca presenza prima di alzare il livello, che è già +8 dB (D-M2-31) | Insieme a I1 |
| **I3** | **Linea di basso** sotto 100 Hz | «manca un basso» (2026-11-03, 2026-11-19); voce urgente di §8.1 | **Misura:** la voce più grave di Corpo ha solo il 3–5 % d'energia sotto 100 Hz; i bassi dell'esperto il 90–99 %. **Principi** dal suo brano, senza copiarne le note: ottavi, una finestra di 4 battute mai ripetuta uguale, fondamentale prevalente, scala della sezione. **Indagine:** separazione fra kick e basso, con alternanza ritmica, banda divisa o ducking; il suo sidechain sullo stem esportato è solo +1,7 dB. **Fonti:** letteratura sul basso nella musica elettronica e adattiva, verificata. **Contratto:** voce nuova di Corpo oppure ruolo dedicato; decisione d'architettura con un ADR, perché tocca il budget di voci | Coppia breve con e senza basso; domanda: «il basso c'è? si confonde col kick?» |
| **I4** | **Più cambiamenti entro 8 battute** | ripetitività 6/7, calo al «terzo o quarto loop», «figura che si ripete» | **Misura:** CYMATICA cambia ogni 10–12 battute, l'esperto ogni battuta; finestre di 8 battute con un cambiamento al 50 % contro l'81 %. **Indagine:** quali leve senza varietà fittizia (memoria del titolare). Entrate e uscite più frequenti, variazioni ritmiche di Pulso e basso, riempimenti a fine frase, segni di forma come piatti o crash, varianti d'accordo. Ogni leva è misurata con S1 e con i pacchetti di sviluppo (M5, lignaggio). **Fonti:** ripetizione e aspettativa (Huron, *Sweet Anticipation*), forma nella musica per videogiochi, verificate | Ascolto del brano intero con le scale 1–7: previsione di ripetitività ≤ 4 |
| **I5** | **Raccordi fra le sezioni** (passaggio A→B) | «brusco» (2026-11-03 «molto brusca», 2026-11-19 per entrambi) | **Misura:** novità spettrale e simbolica al confine, confrontata con i confini interni e con le transizioni del brano dell'esperto, che usa una battuta di passaggio, crash e piatti al contrario. **Alternative:** battuta di raccordo, riempimento, crescendo filtrato, pad che anticipa il modo nuovo, rampa di tempo. La rampa richiede di estendere `MusicClock` con un ADR, ed è già una voce aperta di CP-3 | Coppia breve sul confine, con e senza raccordo |
| **I6** | **Melodia oltre la battuta** (domanda e risposta) | «una figura che si ripete»; prevedibilità 5/7 | **Misura:** contenuto informativo della melodia con IDyOM (approvato), sul riferimento e sul seed 34, e lignaggio M5r. Il limite era dichiarato nella pre-registrazione di 6b: la progressione ripete la stessa figura. **Contratto:** motivi di 2 battute, antecedente e conseguente. Tocca la struttura del motivo e i controlli d'invarianza; precede o accompagna I4 | Domanda sul brano: «frase con inizio e fine o figura che si ripete?» |
| **I7** | Click del chiptune filtrato | «molto acido» | Solo se la palette torna in gioco a M3 o M7 (A4 delle note su AURA) | — |

## 3. Metodo comune

1. **Prima dell'intervento:** l'indagine dà misure e fonti verificate, con le fonti nel dossier di ricerca e i dati grezzi nel repository. Poi si scrive la pre-registrazione, con criteri e previsioni sotto hash.
2. **Durante:** le variazioni piccole si verificano senza ascolto (D-M2-30). I golden si aggiornano con la loro storia.
3. **Dopo:** **un ascolto di conferma con l'esperto**, breve (15–20 minuti), con il protocollo P-M25-03 e i controlli B0:
   - solo sulle voci toccate;
   - coppie brevi da punti diversi;
   - brano intero una volta sola.

   Si fa su materiale maturo, cioè dopo I1–I5 insieme, non dopo ogni intervento.
4. **Ascoltatore A:** può fare da secondo ascoltatore, ma solo con le domande semplici, compilate da solo.

## 4. Domande per il titolare, quando servono

- **I3:** basso come seconda voce di Corpo o come ruolo nuovo. È una decisione d'architettura, da prendere dopo l'indagine.
- **I5:** se serve, la rampa di tempo, che estende `MusicClock`.
- **I1–I3:** ordine di lavoro proposto: I1 e I2 insieme, poi I3. Si può invertire se il basso ha la precedenza.

## 5. Stato delle indagini (2026-11-23)

Tutte concluse; nessuna adozione. Le decisioni si prendono insieme.

| # | Esito dell'indagine | Candidato | Sostegno |
|---|---|---|---|
| I1 | [risultati](research/M2_5_CP3_I1_I2_Kick_Results.md): al confine il kick perde anche 1,26 dB | Do2 fisso | Buono; deroga alla regola |
| I2 | stesso documento: presenza mancante nel transiente di A e A′ | click +3 dB | Discreto |
| I3 | [indagine](research/M2_5_CP3_I3_Bass_Investigation_Results.md) e [prototipo](research/M2_5_CP3_I3_Bass_Prototype_Results.md) | V2, quinto ruolo (ADR); V1 come confronto | Fattibile; superiorità da sentire |
| I4 | [risultati](research/M2_5_CP3_I4_Form_Investigation_Results.md): pattern di una battuta | pattern di 2 battute, [prototipo con I6](research/M2_5_CP3_I4_I6_Two_Bar_Prototype_Results.md) | Ripetitività 6 → 4 (solo ritmo) e 3 (con la melodia), un ascoltatore; [salto A→B](research/M2_5_CP3_I4_I6c_Boundary_Pairs_Results.md): candidato c, al confine resta brusco |
| I6 | [risultati](research/M2_5_CP3_I6_Melody_Investigation_Results.md): nessuna chiusura, stesso contorno 89 % | motivo di 2 battute, con I4 | Diagnosi forte; il seed preferito è il più ripetitivo |
| I5 | [risultati](research/M2_5_CP3_I5_Transition_Investigation_Results.md): misure negative; esperto e titolare indicano il cambio di tempo | [rampa di tempo](research/M2_5_CP3_I5_Tempo_Ramp_Prototype_Results.md): non brusca all'ascolto cieco del titolare; serve `MusicClock` e un ADR. **Coppie ripetute sopra c ([I5e](research/M2_5_CP3_I5e_Ramp_Over_C_Results.md)): rampa non dimostrata (4 su 6).** **B→A′:** [salto di 19 semitoni della melodia](research/M2_5_CP3_I5b_BAprime_Harmony_Results.md) al confine, in conflitto con D-M2-43; [I5c](research/M2_5_CP3_I5c_Melody_Approach_Results.md): ingresso basso più dolce, brusco 8 s dopo; salita graduale in B non basta; [I5d](research/M2_5_CP3_I5d_Register_Return_Results.md): candidato x, prime due battute di A′ un’ottava sotto | Buono, da confermare con l'esperto |

**Ascolto dell’esperto:** [disegno e ipotesi](research/M2_5_CP3_Expert_Listening_Hypotheses.md), eseguito, [esito](m2_5_cp3_expert_listening_2026-11-23.md): controllo identico fallito (indicazione debole); basso e kick graditi, V2m preferita; c, x e rampa non confermate; nuove richieste: accenti comuni batteria-basso, percussioni, motivo nuovo. **Dopo:** accenti non decisi (I8), basso a −3 LU ([misure](research/M2_5_CP3_I3b_Bass_Level_Results.md)), motivo N2 preferito dal titolare ([I9](research/M2_5_CP3_I9_New_Motif_Results.md)). Brano intero: B2, arco specchiato ([I9 bis](research/M2_5_CP3_I9b_Whole_Piece_Results.md)). **Kick e basso insieme:** [misurati](research/M2_5_CP3_I1_I3_Kick_Bass_Joint_Results.md); mascheramento nei criteri, ma V1 e V2m a 0,73 LU (criterio 0,5 fallito). **Da preparare:** prototipo di I4 e I6, ablazione di I5, poi un solo ascolto cieco con l'esperto, con V1 e V2 del basso pareggiate nelle basse.

**Secondo ascolto dell’esperto (2026-11-25):** [esito](m2_5_cp3_expert_listening_2026-11-25.md). Coerenza 6, interesse 6, ripetitività 5. H6 confermata (B2); H1, H2, H4, H5 mancate, H3 «non so»; basso resta −3 LU. Coppie deboli. N2 si rivede, con B2; primo punto il respiro della battuta 55. Nulla adottato.

**Revisione di N2:** [I9 ter](research/M2_5_CP3_I9c_N2_Revision_Results.md), casa ridotta non candidata; [I9 quater](research/M2_5_CP3_I9d_Breath_Confirmation_Results.md), **respiro pieno candidato**. Aperti: salto della frase V, ritmo uguale in sette frasi su otto.

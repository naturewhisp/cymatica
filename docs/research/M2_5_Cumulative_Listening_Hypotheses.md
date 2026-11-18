# M2.5.3–4 — Ascolto cumulativo: disegno della sessione e ipotesi

- **Stato:** scritto il 2026-11-18, prima di generare i materiali. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Protocollo:** [P-M25-03](../m2_5_listening_protocol_P-M25-03.md), con la varietà del §2bis.
- **Registro:** [domande per l'ascolto cumulativo](../m2_5_cumulative_listening_questions.md), voci 1–17.
- **Decisione del titolare (2026-11-18):** «trovare un compromesso per non tediare troppo l'ascoltatore»; valutare «cosa serve davvero come feedback umano e cosa invece è superfluo e valutabile diversamente»; poi «procedi», con «una pagina interattiva come l'altra volta, ma con grafica cymatica come il pdf». La decisione è registrata come D-M2-50.

## 1. Criterio di selezione

Una voce va all'ascolto umano solo se valgono entrambe le condizioni:
- una decisione ne dipende;
- nessuna misura può rispondere, come per gradimento, noia, identità del brano, riconoscimento di un ritorno o preferenza fra palette.

Si evita di mettere a confronto più varianti della stessa melodia, perché stanca e fa perdere attenzione (osservazione del titolare).

| Esito | Voci |
|---|---|
| Unica coppia A/B | 16 (chiptune) |
| Domanda sul brano completo | 1 e 2 (arpeggio), 5 (kick), 7 (tempo per sezione), 8 (passaggio A→B), 11 e 15 (melodia), 12 (entrate e uscite), 13 (sviluppo e ritorno), 14 e 17 (sul secondo brano) |
| Una riga di controllo | 4 (acuti), 6 (scatti) |
| Senza ascolto: valgono le misure | 3 (stereo della Trama: IACC 0,89, centrata), 9 (diade: 0 tritoni Corpo–Vettore), 10 (ablazione della lettura dell'accordo: CTnCTR misurato). La voce 10 torna con un A/B mirato solo se la domanda sulla prevedibilità della melodia dà un segnale |

## 2. Materiali

Tutti i materiali vengono dalla stessa build, con il commit e gli hash scritti nella chiave.

1. **Brano 1:** la fixture di riferimento, seed 1337 e percorso di riferimento (`cumulative_listening_v1`).
2. **Brano 2:** un altro percorso della tabella `fixture-variants-v1`, renderizzato offline (D-M2-50). Il seed è **il più piccolo fra 1 e 64** il cui percorso estratto ha tutte queste proprietà:
   - `bHarmony = 2`, cioè il giro con la sensibile;
   - cella di AURA in A e in B;
   - ingresso della Trama diverso da 4;
   - inizio dello spostamento diverso da 12.

   Così il secondo brano differisce dal primo in tutte le dimensioni della tabella, tranne la pausa del kick. Il planner usa quello stesso seed.
3. **Coppia chiptune:** le clip di `chiptune_ab_v1` della sezione B (frasi 3–4, 8 battute), produzione e chiptune allineate in loudness.
4. **Coppia identica (B0):** la clip di produzione di A′ (frasi 3–4) due volte. È un passaggio diverso da quello della coppia chiptune.
5. **Duplicato nascosto (B0):** la coppia chiptune ripetuta, con l'ordine invertito, nella terza posizione.

L'ordine dentro le coppie e l'ordine delle tre coppie si mescolano con un seed scritto solo nella chiave; il duplicato non è mai contiguo alla coppia originale. Le etichette dei file non rivelano la versione.

## 3. Sessione, circa 30 minuti

| Blocco | Compito | Durata |
|---|---|---|
| 1 | Brano 1: descrizione libera, poi scale 1–7 | 8 min |
| 2 | Domande brevi per parte su tempo forte, passaggio fra le parti, ritorno, melodia, arpeggio e kick | 5 min |
| 3 | Brano 2: istante del calo d'interesse; confronto col primo; qualcosa fuori posto nella seconda parte? | 7 min |
| 4 | Tre coppie di estratti brevi: differenza, preferenza, motivo | 6 min |
| 5 | Domande finali su scatti, acuti e osservazioni libere; affaticamento | 3 min |

Il secondo ascoltatore risponde alle domande semplici del protocollo (§6) sul brano 1 e alla coppia 1. La pagina è un unico file HTML interattivo nello stile visivo di CYMATICA. Le risposte si copiano o si scaricano come testo e non lasciano il computer dell'ascoltatore.

## 4. Ipotesi

Ogni ipotesi dice che cosa ci si aspetta e quale esito cambierebbe una decisione. Gli esiti si riportano come sono, anche se negativi.

| ID | Voce | Previsione | Esito che cambia una decisione |
|---|---|---|---|
| L1 | Scale 1–7, brano 1 | Rispetto al 2026-10-28 (6 / 4 / 7): coerenza ≥ 5, interesse ≥ 5, ripetitività ≤ 5 | Ripetitività ≥ 6: il contratto di sviluppo non basta, e la priorità va a più dimensioni della tabella e allo sviluppo in M2.5.5 |
| L2 | 7, tempo forte | Si percepisce in A, B e A′. Lo spostamento in B si nota e si giudica voluto, oppure non si nota | Giudicato un errore: lo spostamento di F1 si rivede a CP-3 |
| L3 | 8, passaggio A→B | Naturale o marcato, non brusco | «Brusco»: si rivedono la continuità di F3 e il pad ai confini |
| L4 | 13, ritorno | A′ si riconosce come ritorno di A | Non riconosciuto: si rivede il contratto della ripresa (D-M2-45) |
| L5 | 15 e 11, melodia | Si sente come una frase e si segue; gli acuti non sono stridenti | Acuti stridenti: tetto di registro sulla progressione di 6b. Melodia «prevedibile»: torna la voce 10 |
| L6 | 1 e 2, arpeggio | Né copre né si perde; non è fastidioso | «Copre»: −12 dB alla prova. «Si perde»: −5 dB (D-M2-36). «Fastidioso»: si rivede il timbro FM |
| L7 | 5, kick | Ha corpo e si sente il tempo forte | «Debole» o «rimbomba»: si riapre F4 |
| L8 | 12, entrate e uscite | Danno forma al brano | «Interrompono»: si rivede la tabella delle maschere |
| L9 | 14, secondo brano | Giudicato una **variazione dello stesso brano** | «Lo stesso brano»: la varietà fra seed non basta, e la priorità va a più dimensioni della tabella. «Un brano diverso» va bene |
| L10 | 17, seconda parte del brano 2 | Nessun accordo fuori posto | Fuori posto: l'alternativa 2 esce dalla tabella |
| L11 | 16, chiptune | Differenza rilevata; nessuna previsione sulla preferenza | Preferenza coerente con il duplicato: a CP-3 si valuta la palette per sezione (AURA A4). Rifiutata: resta uno spike |
| L12 | Controlli B0 | Coppia identica giudicata uguale; duplicato con la stessa preferenza della coppia originale | Un controllo fallito: i confronti A/B valgono come indicazione debole (§8 del protocollo) |
| L13 | 4 e 6 | Nessuno scatto; acuti equilibrati | Uno scatto con un istante preciso: si cerca nel log delle dissolvenze |

## 5. Limiti

- Il secondo ascolto completo è su un brano diverso dal primo. L'istante del calo e il confronto valgono per quel brano; non c'è un secondo ascolto dello stesso brano.
- Una sessione, uno o due ascoltatori: i risultati orientano, non validano.
- Chi conduce conosce la chiave.

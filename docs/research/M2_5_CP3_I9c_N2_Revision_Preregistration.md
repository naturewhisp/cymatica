# M2.5 CP-3 — I9 ter, revisione di N2: respiro e nota «casa». Pre-registrazione

- **Stato:** scritto il 2026-11-25, **prima** di codice, render e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Origine:** [secondo ascolto dell'esperto](../m2_5_cp3_expert_listening_2026-11-25.md): H1, H2 e H5 mancate, quindi N2 si rivede. Solo offline, seed 1337; stesso kick, basso e tempo di Y; nulla adottato.
- **Già noto prima di scrivere:** la battuta 4 della frase F tace (battute 23, 55, 87); cinque frasi su otto per sezione finiscono sulla tonica; le note sulla tonica sono il 19 / 18 / 18 % in A / B / A′; la forma fa cominciare ogni frase sul grado 0 (in B 7), prima dello spostamento dei conflitti. Quindi la «casa» sta anche negli **inizi**, non solo nelle chiusure.

## 1. Classificazione (D-M2-52)

- **Respiro:** difetto percepito, un istante per sessione in due sessioni; spiegazione post hoc.
- **Casa:** gusto secondo l'esperto. Si prova perché è compatibile con «figura che si ripete» e ripetitività 5, non per seguire il suo stile. Nessun suo brano è usato come obiettivo.

## 2. Versioni (fattoriale 2 × 2, tutte con B2 in B)

Formato delle note: (sedicesimo, durata in sedicesimi, grado). Le modifiche si applicano a tutte le sezioni.

| Versione | Respiro | Casa |
|---|---|---|
| **Y** | di oggi | di oggi |
| **R1** | pieno | di oggi |
| **R2** | di oggi | ridotta |
| **R12** | pieno | ridotta |

- **Respiro pieno:** nella frase F la battuta 4 ha una risposta di tre note, (48,6,4)(54,2,3)(56,8,2); in B il grado g diventa 7 − g come nel resto. Corpo, basso e Trama non cambiano.
- **Casa ridotta, inizi:** la prima nota (sedicesimo 0) di S′, V, C e Z passa al grado 4, anche in B (grado 4, non specchiato). S e F restano sulla tonica: 3 inizi su 8.
- **Casa ridotta, chiusure:** l'ultima nota (sedicesimo 56) diventa, per le otto frasi in ordine: 4, 2, 4, 0, 4, come F, 2, 0 (in B la tonica resta 7 come oggi, gli altri gradi sono quelli scritti). Chiude sulla tonica solo alla quarta e all'ottava frase. In Z la tonica tenuta resta.
- In C non si applica l'abbassamento d'ottava della prima nota (oggi non avviene mai).
- Note in conflitto di classe 1, 6 o 11 con Corpo: regola di I9, sull'intera melodia.

## 3. Criteri

| ID | Criterio | Previsione |
|---|---|---|
| X1 | Fuori dal Vettore tutto identico a oggi; Y uguale nelle note a `k3_B2` e nel render entro 0,001 | Sì |
| X2 | R1 e R12: almeno una nota di Vettore in ogni battuta (Y e R2: vuote solo 23, 55, 87) | Sì |
| X3 | Conflitti 0; note spostate ≤ 20 % in ogni sezione | Sì |
| X4 | D-M2-43: almeno metà delle note sopra Corpo in ogni frase | Sì |
| X5 | Salto massimo ≤ 12; confini A→B e B→A′ ≤ 9 | Sì |
| X6 | R2 e R12: inizi sulla tonica ≤ 4 su 8 e chiusure ≤ 3 su 8 in ogni sezione, dopo gli spostamenti; ultima nota del brano sulla tonica | Sì |
| X7 | R2 e R12: quota della durata di Vettore sulla tonica almeno un terzo più bassa di Y in ogni sezione | Sì |
| X8 | Ritmo: gli attacchi di S, S′, V e C sono identici in tutte e quattro le versioni | Sì, per costruzione |
| X9 | Render MEASURED, limiter 0 dB, nessuno scarto, picco ≤ −1 dBFS; estratti entro 0,1 LU | Sì |

X1 e X9 bloccano la consegna. Gli altri si riportano senza ritocchi. X2, X6 e X8 sono in gran parte garantiti dalla costruzione: il loro peso è basso.

## 4. Ascolto cieco del titolare

- Cinque estratti di 78 s (56–134 s, come I9 bis): Y, R1, R2, R12 e **un duplicato di uno dei quattro**, scelto a caso; ordine casuale, loudness alla più debole. Circa 7 minuti. La battuta 55 è a circa 0:47–0:49.
- Domande, in parole semplici, per ogni estratto: c'è un momento in cui la musica si svuota o perde interesse, e quando? La melodia sembra tornare sempre allo stesso punto? Ha un inizio e una fine? Quanto è ripetitiva, da 1 a 7? Voto da 1 a 10. Alla fine, il preferito.

**Previsioni:**
- (a) R1 e R12 non segnalate come vuote fra 0:44 e 0:53; Y e R2 sì, almeno una delle due.
- (b) R2 e R12 «torna sempre allo stesso punto» meno di Y e R1.
- (c) **Rischio:** R2 e R12 con un inizio e una fine meno chiari, perché tolgono chiusure. In quel caso la leva della casa va contro H2.

Nessuna previsione sul preferito.

**Regola:**
1. Il respiro pieno diventa candidato solo se (a) è confermata. Se nessun estratto è segnalato in quella finestra, il respiro di oggi **resta**: un solo ascoltatore non basta a cambiarlo.
2. La casa ridotta diventa candidata solo se (b) è confermata **e** (c) non si verifica.
3. Se nessuna leva passa, Y resta il riferimento e si riesamina la diagnosi, senza altri ritocchi a tentativi.
4. Se i due duplicati differiscono di più di 1 punto di voto, o cambiano una risposta sì/no, l'indicazione è debole: nessuna leva diventa candidata senza una conferma.

In ogni caso il risultato è un candidato offline, non un'adozione: il motivo di 4 battute richiede un ADR.

## 5. Limiti

- **Il ritmo non cambia (X8):** S, S′, V e C, cioè sette frasi su otto per sezione, hanno gli stessi attacchi. È una causa plausibile di «figura che si ripete» che questa prova **non tocca**. Anche con esito positivo, H1 e H2 possono restare mancate.
- Un ascoltatore che conosce il materiale e l'ipotesi del respiro, e sa dove guardare: (a) è poco cieca per natura.
- Un seed, motivi d'autore, senza planner né K4; timbri provvisori (M7).
- L'estratto contiene tutta B ma solo l'inizio di A′: le battute 23 e 87 non si ascoltano.

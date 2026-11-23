# M2.5 CP-3 — I5, raccordi: risultati dell'indagine

- **Pre-registrazione:** [documento](M2_5_CP3_I5_Transition_Investigation_Preregistration.md), SHA-256 (LF) `1b287b3e2c6a861c89e820fe4b832ab99922c3e2786bd0890ed35f90c112b42a`.
- **Esecuzione:** 2026-11-23. **Dati:** [esperimento](experiments/m25_cp3_transitions/README.md). Nessun cambiamento alla musica.
- **Esito in breve: l'indagine non spiega perché il passaggio suoni brusco.** Cinque previsioni su sette sono mancate.

## 1. Esiti

| ID | Misura | Esito |
|---|---|---|
| T1, croma di A→B fra le prime 3 | 5ª su 91 linee (riferimento); 1ª nel seed 34 | **Mancata** |
| T2, spettro entro il 95° percentile | 93° | Confermata |
| T3, confini non preparati | A→B risulta «preparato» (energia sopra 4 kHz −2,6 dB; −2,2 nel seed 34); B→A′ no | **Mancata** |
| T4, almeno 3 confini su 4 dell'esperto preparati | 3 su 4 | Confermata, ma vedi T5 |
| T5, linee «preparate» per caso, esperto, < 50 % | 50,8 % | **Mancata** |
| T6, gradino di livello ≥ 1 dB | −0,58 dB | **Mancata** |
| T7, scontro fra pad tenuto e attacchi di B | 0 coppie | **Mancata** |

(La pre-registrazione parlava di 92 linee interne; quelle con finestra completa sono 91.)

## 2. Lettura

1. **La mia misura di «preparazione» non vale.** È «vera» in metà delle linee dell'esperto e nel 40 % delle nostre: T3 e T4 non dicono nulla. Il calo di acuti prima di A→B è di 2,6 dB e non so se si senta.
2. **Il contenuto di A→B non è eccezionale:** armonia e timbro sono fra i passaggi più distanti, ma non i primi; il livello è quasi uguale; il pad non scontra.
3. **Regola scritta prima:** con T3 falsa il paragone **non sostiene** il raccordo. Non lo si adotta su questa base.
4. **Che cosa resta:** il cambio di **tempo** (120 → 140) è l'unica cosa che queste misure non vedono, e nel brano dell'esperto non c'è. È un'ipotesi per esclusione, **non una misura**. Anche B→A′ cambia tempo, e non è stato chiesto se suoni brusco.

## 3. Proposta: isolare la causa con l'ascolto

La misura non può decidere. Servono estratti del solo confine, da 8 + 8 battute, renderizzati offline e uno alla volta diversi:

| Estratto | Cosa isola |
|---|---|
| a | com'è oggi |
| b | B a 120 BPM: toglie il cambio di tempo |
| c | una battuta di raccordo prima di B, stesso tempo finale |
| d | rampa di tempo di 2 battute |

Se **b** non suona brusco, la causa è il tempo e si valuta **d**, che estende `MusicClock` con un ADR. Se b suona brusco lo stesso, conta la preparazione e si valuta **c**. Nell'ascolto di conferma si chiede anche di B→A′.

Rendere **b** richiede una forma con il tempo di B cambiato, e la rampa richiede un percorso offline: si fanno come prototipi, senza toccare il gioco.

## 4. Limiti

- Un brano d'esempio, quattro confini; le distanze non sono «bruschezza».
- I piatti e i crash al contrario dell'esperto non sono stati misurati singolarmente.

## 5. Correzione e aggiunta (2026-11-23), dopo il parere del titolare

**Il titolare**, dopo l'ascolto: «a me sembra proprio il cambio di tempo a essere brusco». Non è cieco, e l'ipotesi gli era stata appena esposta: è concorde, non indipendente.

**Evidenza precedente che questo documento aveva trascurato:**
- 2026-11-03, esperto, Q2: «trovo la **transizione di tempo** tra A e B molto brusca. Tra B e A' è più fluida». Nomina il tempo da solo, prima di ogni ipotesi. È la prova più forte.
- 2026-10-28: suggerisce «rampe di BPM o modulazioni ritmiche sulle percussioni» (ADR-0007 §8.1, F3).
- **Correzione al §2:** scrivevo che su B→A′ «non è stato chiesto». Non era stato chiesto il 2026-11-19, ma il 2026-11-03 l'esperto l'aveva giudicato «più fluido».

**Che cosa ne segue:**
1. Il cambio di tempo è ora il candidato **principale**, non solo per esclusione.
2. F3 era il tentativo alternativo: tenere i pattern uguali per una frase, senza rampe. L'ascolto del 2026-11-19 lo trova ancora brusco, quindi **non è bastato**. ADR-0007 §3 lega la rampa proprio a questo caso («Solo se la continuità dei pattern non basta»): la condizione è soddisfatta e la decisione è di CP-3.
3. **Non è tutto il tempo in sé:** B→A′ cambia della stessa quantità e suona più fluido. Restano possibili il verso (accelerare è più brusco che rallentare) e altre differenze. L'ablazione del §3 resta utile, e si aggiunge B→A′ come riferimento.
4. **Priorità degli estratti:** b (stesso tempo) e d (rampa) per primi; c (raccordo) come confronto.

La misura di questo documento resta negativa: la conclusione viene dagli ascolti, non dai numeri.

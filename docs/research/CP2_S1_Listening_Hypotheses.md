# CP-2 — Ascolto, sessioni 1 e 2 (F4a): ipotesi registrate prima

- **Stato:** scritto il 2026-11-16 prima di costruire il pacchetto e prima di ogni ascolto (P-M25-03 §2.2). L'hash SHA-256 (LF) di questo file è registrato in `progress.md`. Dopo l'hash il file non si modifica: esiti diversi si riportano così come sono.
- **Fonti:** [protocollo P-M25-03](../m2_5_listening_protocol_P-M25-03.md) (§2bis varietà, §4 costrutti e controlli); [pre-registrazione F4a](M2_5_2_F4_Kick_Level_Preregistration.md) (L1, L2 e regola di decisione); [risultati F4a](M2_5_2_F4_Kick_Level_Results.md); [ascolto del 2026-11-03](../m2_4_human_listening_2026-11-03.md) §3.2 (pattern del kick per sezione).
- **Perimetro:** CP-2 chiede «il mix è credibile? Con il kick più alto il metro regge? F1 va corretto, e come?» (ADR-0007 §4). Qui si ascolta solo F4a, il livello del kick. F2 e il rapporto FM della Trama arrivano dopo, con un loro pacchetto: la risposta completa a «il mix è credibile?» si dà a fine M2.5.2.

## 1. Disegno

Le prove superano 45 minuti, quindi le sessioni sono due (§2bis).

**Sessione 1, circa 40 minuti, con pausa a metà:**
1. **Brano completo**, kick a **+6 dB**: è il punto medio della scala, scelto prima dell'ascolto come materiale di lavoro e non come valore candidato. Primo ascolto: descrizione libera, poi le scale 1–7 (coerenza, interesse, ripetitività).
2. **Classifiche per sezione:** tre gruppi in ordine mescolato, uno per sezione (A, B, A', battute 9–16). Ogni gruppo ha le 5 varianti **allineate in loudness** in ordine mescolato. Nel gruppo di B c'è una sesta clip, **duplicato nascosto** di una delle cinque (controllo B0). Per ogni gruppo:
   - quali clip non si distinguono (rilevazione);
   - la classifica dalla preferita (preferenza);
   - il perché della prima e dell'ultima (giudizio motivato).
3. **Pausa.**
4. **Secondo ascolto del brano completo**, con l'istante del calo e **Q3 per sezione**: il tempo forte si percepisce? Dove cambia?
5. **Coppie**, in ordine mescolato, ciascuna con rilevazione, preferenza e giudizio motivato:
   - A, 0 contro +3 dB;
   - A', 0 contro +3 dB;
   - B, +6 contro +9 dB;
   - **coppia identica** (A', +6 contro +6), controllo B0.
6. **Domande brevi:** Q1 (artefatti), Q6 (timbro e livelli), Q8 (osservazioni libere).

**Sessione 2, circa 20 minuti, in un altro giorno, anche insieme al pacchetto di F2 e FM:** ABX su A e su A', 0 contro +3 dB allineati, 16 prove ciascuno. In ogni prova X è una delle due versioni, scelta a caso con il seed della chiave.

**Secondo ascoltatore:** il brano completo e una coppia (A, 0 contro +9 dB, allineate), con le domande 1–7 del protocollo §6.

## 2. Ipotesi

| ID | Ipotesi | Esito che cambia una decisione |
|---|---|---|
| **L1** (pre-registrazione F4a) | La variante al primo posto, sulle clip allineate, è +6 o +9 dB | Il valore si adotta con la regola del §3 |
| **L2** (pre-registrazione F4a) | +3 dB contro 0 si rileva su A e su A' | Se non si rileva, il passo minimo utile della scala è più largo di 3 dB |
| **S1** | La coppia B +6 contro +9 dB si rileva | Se non si rileva, +6 e +9 sono equivalenti per l'ascoltatore e si sceglie il più basso (più margine) |
| **S2, F1** | Nel secondo ascolto del brano a +6 dB, Q3 risponde «no» o «a tratti» per B e A', dove il kick cade fuori dal tempo forte (passi 2, 6, 10, 14), e «sì» almeno nella seconda metà di A (passi 0, 4, 8, 12) | Se il tempo forte regge anche in B e A', F1 resta minore come il 2026-11-03; altrimenti F1 va corretto in M2.5.3 |
| **S3, F1** | Nella descrizione libera o in Q3 l'esperto nota il kick fuori dal tempo forte in almeno una sezione | Come S2 |
| **B0** | Coppia identica giudicata uguale; duplicato nascosto riconosciuto o messo in posizioni adiacenti della classifica | Se un controllo fallisce, la sessione vale come indicazione debole (protocollo §8) |

Senza predizione: le scale 1–7, da confrontare con il 2026-10-28 (coerenza 6, interesse 4, ripetitività 7); l'istante del calo; Q1 e Q6.

## 3. Regole di lettura

- **Valore adottato (F4a):** la variante con la **mediana di posizione migliore** nelle tre classifiche. A parità vale la posizione in B, dove il kick è più basso. Se la migliore è +12 dB, il bordo, la scala si estende prima di adottare un valore. Se nessuna variante sopra 0 batte 0 dB in almeno due sezioni, il valore resta −6 dB (pre-registrazione F4a §5).
- **ABX (L2):** con 16 prove, almeno **12 risposte corrette** su ciascun estratto (binomiale unilaterale, p < 0,05). L2 è confermata se il criterio vale su A e su A'.
- **Secondo ascoltatore:** le risposte valgono per quella persona e quelle clip (protocollo §8). Non entrano nella regola del valore, ma si riportano.
- **Specificità e sensibilità** non si dichiarano: due ascoltatori, nessun gruppo indipendente (`INSUFFICIENT_DATA`).

# M2.5 CP-3 — I9 ter, revisione di N2: risultati

- **Pre-registrazione:** [documento](M2_5_CP3_I9c_N2_Revision_Preregistration.md), SHA-256 (LF) `ecf1267f62214da8415281ea5a08de569e31dc8660a051b5f83b4577cbf9f690`.
- **Dati:** `experiments/m25_cp3_motif/n2_revision.{py,json}`. Solo offline, seed 1337; nulla adottato.

| ID | Y | R1 | R2 | R12 | Esito |
|---|---|---|---|---|---|
| X1 | CSV identico a I9 bis, render diverso di 0 | | | | Confermata |
| X2, battute vuote | 23, 55, 87 | nessuna | 23, 55, 87 | nessuna | Confermata |
| X3, note spostate A / B / A′ | 2 / 12 / 1 | 2 / 13 / 1 | 2 / 12 / 1 | 2 / 13 / 1 | Confermata, conflitti 0 |
| X4, minimo sopra Corpo | 1,00 | 1,00 | 1,00 | 1,00 | Confermata |
| X5, salto massimo; confini | 9; 5 / 5 | 9; 5 / 5 | 9; 5 / 5 | 9; 5 / 5 | Confermata |
| X6, inizi / chiusure sulla tonica (per sezione) | 8 / 5 | 8 / 5 | 3 / 2 | 3 / 2 | Confermata |
| X7, durata sulla tonica A / B / A′ | 23 / 23 / 24 % | 22 / 22 / 23 % | 10 / 10 / 11 % | 10 / 9 / 11 % | Confermata |
| X8 | | | | | Confermata per costruzione |
| X9 | | | | | MEASURED, limiter 0, picco −5,1 dBFS |

**Peso:** quasi tutto è garantito dalla costruzione; le misure dicono che le versioni sono ciò che dovevano essere, non che siano migliori. In R2 e R12 il contorno uguale in B sale da 0 a 12,5 %, effetto non previsto e piccolo.


## Ascolto cieco del titolare (2026-11-25)

Risposte trascritte prima di aprire la chiave:

> estratto 1: mi sembra perfetto, ha tutte le caratteristiche richieste (nessun vuoto o perdita di interesse, la melodia varia sempre senza ripetersi, c'è una progressione e un inizio e fine chiari ma una chiusura molto brusca, quasi per niente ripetitiva nel senso che il pattern c'è ma ha sempre delle variazioni di scala o di note che lo rendono vario secondo me). I cambi di ritmo sono perfetti. Voto 10 per me.
>
> estratto 2: molto buono anche questo come il precedente, unico "buco" ma non come silenzio a 0:47 per il resto valgono le impressioni positive di 1. Voto 8
>
> estratto 3: rispetto a 1 è più ripetitivo e si avvertono più chiaramente dei pattern ripetuti, i cambi nota/scala sono più "brutti" e bruschi ad esempio a 0:20 1:02 1:09 . Voto 6
>
> estratto 4: giudizio analogo a 3. 0:45 e 0:47 i punti più brutti. Voto 5
>
> estratto 5: il più simile a 1 nei pregi soprattutto per la fluidità, difetto a 0:47. Voto 8
>
> dove non ho specificato diversamente valgono le considerazioni di estratto 1

**Chiave, aperta dopo:** 1 = R1, 2 = Y, 3 = R12, 4 = R2, 5 = R2 (duplicato; file identici). Le risposte sopra non sono state modificate.

| Versione | Voto | Vuoto fra 0:44 e 0:53 |
|---|---:|---|
| **R1**, respiro pieno | **10** | nessuno |
| Y, riferimento | 8 | 0:47 |
| R12, entrambe | 6 | nessuno (0:20, 1:02, 1:09) |
| R2, casa ridotta | 5 e 8 | 0:45 e 0:47; 0:47 |

| Previsione | Esito |
|---|---|
| (a) vuoto assente in R1 e R12, presente in Y o R2 | **Confermata:** cinque estratti su cinque, duplicato compreso |
| (b) R2 e R12 tornano «sempre allo stesso punto» meno di Y e R1 | **Non confermata:** nessuna risposta esplicita alla domanda; R12 e un R2 sono giudicati **più** ripetitivi |
| (c) rischio: inizio e fine meno chiari in R2 e R12 | Non segnalato («dove non specificato valgono le considerazioni di 1») |

**Regola, applicata alla lettera**
- **Duplicato: 5 e 8, scarto 3 (soglia 1).** Punto 4: indicazione debole, **nessuna leva diventa candidata senza una conferma**.
- **Respiro pieno:** (a) confermata, ma per il punto 4 serve una conferma. È l'indicazione più coerente della prova: il duplicato discorda sul voto, non sul punto di vuoto.
- **Casa ridotta:** (b) non confermata, quindi **non è candidata**. Lo scarto di voto di R2 (5–8) comprende Y (8): non prova che sia peggiore.
- Y resta il riferimento.

**Lettura, post hoc**
- **0:20 in R12:** a 0:20,9 la casa ridotta fa scendere la melodia di 7 semitoni (Fa5–Si♭4), dove Y e R1 salgono di 2. Compatibile con «brutto e brusco», ma R2 ha la stessa nota e lì non è segnalato.
- **1:02 e 1:09 in R12:** in quei secondi la melodia è **identica** in tutte e quattro le versioni. Non sono difetti della revisione, o dipendono dal contesto precedente.
- **«Chiusura molto brusca» in 1:** l'estratto è tagliato a 134 s, non alla fine del brano. È un effetto dell'estratto, uguale per tutti.
- **Ripetitività:** nessun numero da 1 a 7; solo giudizi relativi.

**Limiti:** un ascoltatore che conosceva l'ipotesi del respiro e, dall'annuncio della prova, l'istante: (a) è poco cieca. I voti hanno un rumore di almeno 3 punti, quindi 10 contro 8 non separa R1 da Y. Solo la battuta 55 è stata ascoltata.

**Esito:** nessun candidato nuovo. Nulla adottato.

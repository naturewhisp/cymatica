# M2.5 CP-3 — I3, prototipo del basso a tre versioni. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima del codice. Dopo l'hash, registrato in `progress.md`, il file non si modifica.
- **Decisione del titolare:** «Commit e prototipo», dopo l'[indagine](M2_5_CP3_I3_Bass_Investigation_Results.md), da cui vengono i vincoli.
- **Domanda:** il basso autonomo porta qualcosa che una voce di Corpo non può dare, senza danni? Se non supera i criteri, il quinto ruolo non si giustifica.

## 1. Versioni

Brano di riferimento, seed 1337, con il kick attuale e con Do2 e click +3 dB.

- **V0:** senza basso, com'è oggi.
- **V1:** la voce bassa di Corpo un'ottava sotto, con un'opzione del planner spenta di default. Scelte e punteggi non cambiano.
- **V2:** linea autonoma, scritta da uno script e suonata da un motore offline separato con la **stessa voce di Corpo**, così V1 e V2 differiscono per la linea e non per il timbro. Somma al mix in mono. Livelli: loudness K di sezione pari a Corpo (V0) −6, −3 e 0 LU; **−3 è il principale**.

**Linea di V2,** per ottavi, registro della fondamentale MIDI 28–39:
- sei cellule di battuta: `R--R--O-`, `R-R--F-O`, `R--F-O-N`, `-R--R-O-`, `R------N`, `-R-F--RN` (R fondamentale, O ottava, F quinta, N nota della scala appena sotto la fondamentale seguente, - pausa);
- ogni frase usa una coppia (a, b) come `a b a b*`, dove b* è la cellula dopo b. La coppia è quella la cui coincidenza col kick è più vicina al 36 % dell'esperto, diversa da quella della frase precedente; parità in ordine di indice;
- nelle frasi dispari la prima battuta scambia O e F;
- A′ richiama le coppie di A;
- **silenzio e pedale:** frase 0 tace; frasi 1, 7, 8, 15, 16 e 23 tengono la fondamentale per battuta;
- separazione S0 (nessuna); S1 come confronto (chi coincide col kick entra 60 ms dopo).

## 2. Misure e previsioni

| ID | Misura | Previsione |
|---|---|---|
| P1 | Golden PCM di V0 PASS; in V1 Pulso, Trama e Vettore identici; limiter fermo in V0 e V1; picco della somma V2 ≤ −1 dBFS | Vero |
| P2 | Quota non di kick in 30–120 Hz | V2 > V0 e V1 > V0 in ogni sezione |
| P3 | R (come nell'indagine) di V2 a −3 LU | ≥ 6 dB in ogni sezione, con S0 |
| P4 | Quota degli attacchi di basso a un semitono (classe d'intervallo 1) da una nota di Corpo (voce alta), Trama o Vettore che suona in quel momento | V2 al massimo 5 punti sopra V1 |
| P5 | Finestre di 4 battute in cui il basso differisce dalla precedente (altezza e ritmo, stessa sezione) | V1 ≤ 10 %; V2 ≥ 80 % |
| P6 | Finestre di 8 battute con almeno un'entrata o uscita di strato (S1 dell'analisi dell'esperto, strato basso compreso in V2) | V2 ≥ V0 |
| P7 | Coincidenza col kick di V2 nelle frasi con linea | 25–50 % |

## 3. Regola

- **V2 merita un ADR** se valgono P1–P7 con almeno un kick. **V1 basta** se V2 fallisce P3, P4 o P7.
- P5 e P7 sono in parte **veri per costruzione**: servono a scoprire un difetto, non a dimostrare che V2 è migliore. Che sia musicalmente migliore lo può dire solo l'ascolto di conferma, con V1 fra le alternative.
- Nessuna adozione: le decisioni vengono dopo tutte le indagini.

## 4. Limiti

- V2 è sommata dopo il render: niente ducking del kick sul basso e niente furti di voce; il master è assunto lineare, e lo si verifica sommando gli stem di V0.
- La linea è scritta a mano per un brano: non è il generatore.
- Una cellula per archetipo non è prevista: si suona nello stile attuale.

# M2.5.2 — F4c, decadimento del kick: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Richiesta:** il titolare il 2026-11-17 («facciamo anche f4c»); verifica autonoma secondo D-M2-30.
- **Giudizio umano già raccolto:** «kick: più peso nelle basse, durata oltre i 54–62 ms» (ADR-0007 §8.1).
- **Stato attuale:** con F4b il kick segue l'inviluppo progettato. Decadimento di 50 + 70 · `timbreParam` ms, cioè 71–82 ms con i `timbreParam` del planner (0,30–0,45). Durata mediana misurata 76,1 ms; la loudness di Pulso è cresciuta solo di 0,05 LU ([risultati F4b](M2_5_2_F4b_Kick_OneShot_Results.md)).

## 1. Variabile

Solo la **lunghezza del decadimento** di Pulso: (50 + 70 · `timbreParam`) ms · **k**, con k ∈ {1,00; 1,25; 1,50; 1,75; 2,00}. Override solo offline per la misura; produzione invariata finché non si adotta. Forma lineare, sweep, click, livello e timbro restano invariati.

## 2. Criterio di scelta, fissato prima

Per ogni k, sul render della fixture (seed 1337):

| Condizione | Definizione |
|---|---|
| **N, nessuna sovrapposizione** | Per ogni coppia di kick consecutivi, il valore assoluto massimo dello stem di Pulso nell'ultimo millisecondo prima dell'attacco successivo è ≤ 10⁻³ volte il picco del kick precedente (−60 dB). L'intervallo minimo fra kick nella fixture è 125 ms (uno solo); gli altri sono di 250 ms o più |
| **C, Corpo non coperto** | Perdita per mascheramento di Corpo (MoSQITo, come nei pacchetti 4 e 4b, sugli estratti di 4b: A frasi 5–6, B e A′ frasi 3–4) non oltre **+0,02** rispetto a k = 1, in ogni sezione. Con il kick più lungo dura di più anche il ducking di Corpo |
| **S, sicurezza** | Riduzione del limiter 0 dB; loudness BS.1770 del mix intero non oltre +0,5 LU rispetto a k = 1; nessun `steal` di Pulso nel registro delle dissolvenze |

**Scelta:** il **k più grande** che soddisfa N, C e S. Se nessun k > 1 li soddisfa, il decadimento resta com'è.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **D1** | N è soddisfatta per k ≤ 1,50 (decadimento massimo 122,3 ms + 1 ms di attacco < 125 ms) e violata per k ≥ 1,75 |
| **D2** | k scelto = **1,50** (decadimento 107–122 ms) |
| **D3** | A k = 1,50: durata mediana del kick fra 100 e 125 ms; loudness di Pulso fra +0,8 e +1,8 LU rispetto a k = 1; quota d'energia sotto 100 Hz in aumento |
| **D4** | A ogni k: piano, trace ed eventi invariati; Trama e Vettore identici |

## 4. Adozione

Se il criterio sceglie k > 1: nuova costante del decadimento di Pulso nel synth, con l'emendamento di ADR-0004 §2.1 (intervallo di decadimento) registrato in ADR-0007, golden PCM aggiornato con il motivo, e le stesse verifiche di regressione dei passi precedenti.

## 5. Limiti

- Il criterio dice qual è il kick più lungo che non si sovrappone, non copre Corpo e non alza il mix: non dice che sia il kick «giusto». Il giudizio resta all'ascolto cumulativo.
- La forma del decadimento resta lineare. Un decadimento esponenziale, più tipico di un kick, è un'altra variabile.

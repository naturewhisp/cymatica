# Pacchetto di misura 2b — Registro delle dissolvenze dall'allocatore (rilevatore v2): pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Origine:** il rilevatore A1 del pacchetto 2 non ha superato la validazione ([risultati](M2_5_Pkg2_Discontinuity_Detector_Results.md)), perché confonde i reset del dente di sega con i click. Il titolare il 2026-11-17 ha approvato di pre-registrare il v2 basato sull'inviluppo, la strada 1 del §3 di quei risultati. Perimetro: pacchetti 1–5 approvati.
- **Principio** (dossier delle lacune, «Sintesi»): CYMATICA conosce la verità. I punti in cui un suono si interrompe non si indovinano dal segnale: si registrano dove l'allocatore li produce.

## 1. Che cosa si misura

Un **registro delle dissolvenze**, offline e a capacità fissa, scritto dall'allocatore solo quando un'opzione di diagnostica lo installa (come lo stem sink: in quiescenza, mai con un device avviato; nessuna allocazione nel percorso di render). Ogni voce che smette di suonare lascia un record con:

| Campo | Definizione |
|---|---|
| frame, ruolo, slot | istante logico dell'evento |
| **causa** | `release` (NoteOff abbinato), `steal` (NoteOn su voce attiva), `release_cut` (NoteOn su voce in rilascio), `stop` (Stop o Pause), `safety` (scadenza di `checkLifetimes`) |
| **ampiezza d'inviluppo A** | `envelopeLevel · velocity · kNominalVoiceScale` all'istante dell'evento: l'ampiezza dell'oscillazione che si sta togliendo, prima del bus |
| **ampiezza in uscita** | A moltiplicata per il guadagno di bus corrente del ruolo, in dBFS. Il ducking di Corpo non è incluso, e lo si dichiara |
| **durata della dissolvenza** | frame in cui l'oscillazione va a zero: `releaseSamples` per `release`, `stop` e `safety`; **0** per `steal` e `release_cut`, dove l'oscillazione si interrompe subito e resta una coda lineare del solo ultimo campione per 96 frame |

**Uscita descrittiva**, senza soglie di udibilità: per ruolo e causa, numero di eventi, mediana e massimo dell'ampiezza in uscita, e conteggio delle dissolvenze di al più 2 ms con ampiezza in uscita sopra −40 dBFS. Il bin di −40 dBFS serve solo a raggruppare, non è un giudizio. Lo strumento dice dove, quanto e quanto in fretta un suono si interrompe; se si sente resta una domanda percettiva (pacchetto 4 per il mascheramento, poi l'ascolto).

## 2. Validazione dello strumento

| ID | Prova | Esito atteso |
|---|---|---|
| **E1, positivo** | Nota sintetica di Corpo arrivata al sustain, poi NoteOff, render offline | 1 record `release`; durata 96 frame; A = velocity · 0,75 · 0,25 esatta in float; con l'override di rilascio a 30 ms la durata è 1440 |
| **E2, positivo** | Tre NoteOn di Corpo sovrapposti (due slot), poi un NoteOn su una voce in rilascio | Un record `steal` e un `release_cut`, con durata 0 e A uguale all'inviluppo del momento |
| **E3, negativo** | Nota di Pulso già decaduta a zero (sustain 0) prima del suo NoteOff | Nessun record: la voce è già `Idle` e il NoteOff non trova corrispondenza |
| **E4, coerenza** | Fixture: conteggi del registro contro i contatori esistenti dell'allocatore | `steal` = `voiceDropCount` e `release_cut` = `releaseCutCount`, per ruolo, esattamente |
| **E5, invarianza** | Due esecuzioni dello scenario | Registri identici |
| **E6, nessun effetto** | Fixture con il registro installato | PCM uguale al golden `18256941985354732249`: il registro non cambia il suono |

## 3. Misura descrittiva sulla fixture

Tabella per ruolo e causa (seed 1337). Unica predizione oltre E4–E6: i `release_cut` di Corpo sono **190**, il numero già registrato per H-03. Tutto il resto è descrittivo: è la prima misura diretta delle interruzioni nel contenuto.

## 4. Limiti

- La durata 0 di `steal` e `release_cut` descrive il codice attuale: l'oscillazione si ferma e la coda lineare dell'ultimo campione è un'altra forma di dissolvenza, con un suo contenuto spettrale. Lo strumento la riporta, non la giudica.
- Il ducking di Corpo e il delay della Trama, che prolunga i suoni dopo la fine della voce, non entrano nell'ampiezza in uscita.
- Esito `MEASURED`, mai `PASS`, finché il titolare non ratifica una soglia.

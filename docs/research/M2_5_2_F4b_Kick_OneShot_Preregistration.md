# M2.5.2 — F4b, kick one-shot: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Perimetro:** F4b in M2.5.2 (ADR-0007 D-M2-26, D-M2-32: si chiude M2.5.2 prima della nuova fase); verifica autonoma secondo D-M2-30. In M2.5.2 piano e trace restano invariati: le durate pianificate non si toccano.
- **Giudizio umano già raccolto:** «kick: più peso nelle basse, durata oltre i 54–62 ms» (ADR-0007 §8.1).
- **Evidenza:** il [registro delle dissolvenze](M2_5_Pkg2b_Fade_Event_Log_Results.md) mostra che i 416 kick sono troncati dal NoteOff del planner, a 53,6 e 62,5 ms (mezzo step), con una dissolvenza di 2 ms a −26 dBFS di mediana. L'inviluppo progettato di Pulso è un decadimento con sustain 0 di 50 + 70 · `timbreParam` ms. Con i `timbreParam` del planner (0,30–0,45) fa **71–82 ms**, quindi il NoteOff lo interrompe prima della fine.

## 1. Variabile

**Pulso one-shot:** la voce di Pulso segue il proprio inviluppo (1 ms di attacco, decadimento fino a zero) e si spegne da sola. Il NoteOff di Pulso non la interrompe più, e la scadenza di durata di `checkLifetimes` non la tronca. La scadenza di sicurezza dei pad aperti resta, e Pause, Stop e furto di voce restano invariati.

Ruoli, piano, eventi pianificati, decadimento e timbro del kick restano invariati.

- **Escluso, dichiarato:** allungare il decadimento oltre il progetto (50–120 ms) è un'altra variabile, F4c, da decidere dopo questa misura.
- **Escluso:** «più peso nelle basse» come cambio di timbro. Qui lo si misura soltanto, perché un kick più lungo porta più energia grave.

## 2. Misure

Sullo stem di Pulso del brano intero (`stem_export_v2`, prima e dopo), con gli attacchi dall'event log:

- **durata del kick:** dall'attacco all'ultimo campione sopra il −40 dB del picco della nota, entro 200 ms; mediana e quantili;
- **energia grave:** quota di energia sotto 100 Hz dello stem di Pulso, e loudness BS.1770 dello stem;
- **registro delle dissolvenze** (`fade_events_v1`): eventi di Pulso per causa.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **B1** | Prima: durata mediana del kick fra 52 e 66 ms (troncatura a 53,6 / 62,5 ms più 2 ms di dissolvenza) |
| **B2** | Dopo: durata mediana fra 68 e 84 ms (decadimento lineare fino a −40 dB, cioè al 99 % dei 71–82 ms, più 1 ms di attacco) |
| **B3** | Dopo: 0 eventi `release` di Pulso nel registro delle dissolvenze (prima 416); nessun `steal` di Pulso, perché i colpi distano almeno 107 ms e i due slot bastano |
| **B4** | Piano e trace uguali ai golden; eventi pianificati invariati. Trama e Vettore identici; Corpo cambia, perché il ducking segue l'inviluppo di Pulso. Cambia il golden PCM |
| **B5** | Loudness dello stem di Pulso in aumento fra +0,3 e +2 LU; energia sotto 100 Hz di Pulso in aumento |
| **B6** | Limiter 0 dB; true peak entro ± 0,5 dB dall'attuale |

## 4. Limiti

- La durata ottenuta è quella del progetto (71–82 ms), non una scelta d'ascolto. Se basti, o se serva F4c, è una domanda per l'ascolto cumulativo, oppure per un criterio pre-registrato.
- Il ducking di Corpo si allunga con il kick: è voluto, ma cambia anche Corpo.

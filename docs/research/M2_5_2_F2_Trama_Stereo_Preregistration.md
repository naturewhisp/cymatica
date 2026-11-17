# M2.5.2 — F2, immagine stereo della Trama: pre-registrazione

- **Stato:** scritto il 2026-11-16, prima del codice di misura, del cambio e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Perimetro:** ADR-0007 D-M2-26 (F2 in M2.5.2), verifica autonoma secondo D-M2-30. Giudizio umano già raccolto: la percussione FM «a sinistra», «fastidiosa in cuffia» (2026-10-28 e 2026-11-03, due segnalazioni). L'esperto avverte inoltre che il Side/Mid può indicare larghezza mentre si sente un suono spostato: per questo la misura usa il bilanciamento fra i canali, non il Side/Mid.
- **Fonti:** [ascolto del 2026-11-03](../m2_4_human_listening_2026-11-03.md) (Q4, «Seguito sulla larghezza stereo»); [ascolto del 2026-10-28](../m2_4_human_listening_2026-10-28.md) (H5, F2); codice: `voice_allocator.cpp` (pan della voce Trama) e `role_bus_processor.cpp` (delay della Trama, wet 0,325, tap L 11 ms e R 17 ms).

## 1. Diagnosi dal codice

- La Trama usa sempre la voce dello slot 4 (AG-MM-12), con guadagni L 1,0 e R 0,5: la sorgente diretta è 6,02 dB più forte a sinistra.
- Il delay riceve lo stesso segnale sbilanciato, quindi anche i tap sono 6 dB più forti a sinistra, e il tap sinistro arriva 6 ms prima del destro.
- La lateralizzazione nasce quindi soprattutto dalla **differenza di livello fra i canali** del suono diretto, che arriva per primo.

## 2. Variabile

Solo il pan della voce Trama: **centro a potenza costante**, L = R = √((1² + 0,5²)/2) = 0,7906, per entrambi gli slot della Trama. Il delay (11/17 ms, wet, feedback) resta invariato e continua a dare spazio. Note, piano, trace e guadagni degli altri ruoli restano invariati.

## 3. Misure

Sullo stem della Trama della fixture (seed 1337), con gli attacchi dai NoteOn applicati della Trama:

| Misura | Definizione |
|---|---|
| **ILD del primo fronte** | 10·log10(E_L / E_R) nei primi 5 ms dopo ogni attacco, prima di qualunque tap (11 ms), sommata su tutte le note |
| **ILD dell'intero stem** | 10·log10(E_L / E_R) su tutto lo stem |
| Senza regola | Side/Mid dello stem; coefficiente di correlazione L/R a ritardo zero; ILD per sezione |

## 4. Predizioni

| ID | Predizione | Regola |
|---|---|---|
| **S1** | Prima del cambio: ILD del primo fronte +6,0 dB ± 0,5 | fuori → la diagnosi del §1 è incompleta, e il cambio si ferma |
| **S2** | Dopo: ILD del primo fronte 0,0 dB ± 0,5 | fuori → mancata |
| **S3** | Prima: ILD dell'intero stem +6,0 dB ± 0,5. Dopo: 0,0 dB ± 0,5 | fuori → mancata |
| **S4** | Loudness integrata dello stem della Trama invariata ± 0,2 LU (potenza costante; BS.1770 somma le potenze dei canali) | fuori → mancata; non si compensa senza una nuova pre-registrazione |
| **S5** | Piano, trace ed eventi invariati; stem di Pulso, Corpo e Vettore identici; mix entro ± 0,2 LU; limiter 0 dB | fuori → mancata |
| **S6** | Il coefficiente di correlazione L/R della Trama dopo il cambio resta sotto 1: il delay dà ancora differenze fra i canali. Senza soglia: è una misura di larghezza, non di lateralizzazione | — |

## 5. Limiti

- Lateralizzazione e larghezza percepite non coincidono con queste misure. Lo dice l'esperto stesso: se la Trama sembri ancora spostata o troppo stretta lo stabilisce l'ascolto cumulativo (D-M2-30).
- Un'alternanza deterministica del pan per nota (ping-pong) è un'altra variabile, e sarebbe anche una scelta musicale: è esclusa da questo passo.

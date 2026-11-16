# M2.5.2 — Rapporto FM della Trama: risultati

- **Pre-registrazione:** [M2_5_2_FM_Ratio_Preregistration.md](M2_5_2_FM_Ratio_Preregistration.md), SHA-256 (LF) `f55dcde4c7eb171be1182b276f87537a0ce2e8840caad610a119568f1d6938a8`, registrata prima dello script, del codice e delle misure.
- **Esecuzione, 2026-11-16:** script di contesto, poi Release e Debug, scenari `fixture_test`, `procedural_v1`, `timbre_test`, `kick_level_ab_v1` e `full_m2_suite --pack-seed 1618033988`. Dati: [esperimento](experiments/m25_2_fm_ratio/README.md).
- **Decisione:** m/c = **1:1** (`kTramaFmRatio`, prima `1 + 1,76·timbreParam` = 1,88) e bus della Trama da −10 a **−9,0 dB** per allineare la loudness. Verifica autonoma (ADR-0007 D-M2-30), registrata come D-M2-33.

## 1. Criteri di scelta

Su 400 note Trama, tutte con Corpo che suona all'attacco:

| m/c | C1, autodissonanza (≤ 0,028) | C2, P5 e P8 < tritono | C3, contesto Corpo | C3 con Vettore (senza regola) |
|---|---|---|---|---|
| 1,88 (precedente) | 0,828, no | 1,303 / 1,023 < 1,336, sì | 1,166 | 1,811 |
| **1,0** | 0,001, sì | 0,038 / 0,001 < 0,210, sì | **0,489** | 0,804 |
| 2,0 | 0,000, sì | 0,085 > 0,068, **no** | 0,280 | 0,480 |
| √2 | 0,040, no | 0,426 < 0,509 ma 0,609 > 0,509, no | 1,570 | 2,360 |

- **F1 confermata:** solo 1,0 supera C1 e C2, quindi la scelta è 1:1.
- **F2 confermata:** C3 scende da 1,166 a 0,489.
- **Il rapporto 2,0 avrebbe la dissonanza di contesto più bassa** (0,280), ma non supera C2: nel modello il tritono risulta più consonante della quinta, perché lo spettro ha solo armoniche dispari. Lo esclude la regola fissata prima, non una scelta a posteriori.

## 2. Adozione e misure

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| F3 | Piano e trace invariati; Pulso, Corpo e Vettore identici; cambiano Trama e PCM | Piano `5693144463743088024` e trace `10949526468471789087` uguali ai golden; eventi invariati. Pulso, Corpo e Vettore con loudness identica a tutte le cifre per brano e per sezione (identità al bit attesa per costruzione, non confrontata campione per campione) | Confermata |
| F4 | Limiter 0 dB; true peak entro ± 0,5 dB da −6,87 | 0 dB; −6,866 dBTP | Confermata |
| F5 | Mix entro ± 0,3 LU | −20,365 → −20,356 LUFS (+0,009 LU), dopo la compensazione | Confermata |

- **Predizione mancata, compensazione di loudness:** con 1:1 e il bus della Trama a −10 dB la Trama scendeva di 1,03 LU (−34,087 → −35,122). Il motivo è che con portante e modulante uguali le bande laterali si ripiegano sulle armoniche e in parte si cancellano: l'energia dipende dall'indice, −2,9 dB a I = 1,5 e +0,9 dB a I = 3 rispetto a una sinusoide.
  - Il calo faceva fallire il controllo di attività di `timbre_bus_probe_v1`: −40,12 dBFS contro la soglia di −40.
  - Secondo l'indicazione registrata alla review di CP-1 («negli A/B di timbro allineare la loudness»), il bus della Trama passa a −9,0 dB. La Trama torna a −34,122 LUFS, 0,035 LU dal valore di prima.
  - La pre-registrazione non prevedeva la compensazione: errore dichiarato. La soglia del controllo non è stata toccata.
- **Golden PCM della fixture:** da `365661633327826884` a **`7790666370008029299`**, con il motivo in `audio_probe.h`.
- **Build e test:** Release e Debug headless con `/WX`, 270/270 ciascuna.

## 3. Onset della Trama (`acao02_v1`)

- **Il doppio rilevamento non è nuovo:** con 1,88 (calibrazione del 2026-10-29) erano già 800 rilevazioni per 400 note, e la seconda era il rilascio del NoteOff a circa 100 ms (F6, H-03).
- **Profilo congelato** (δ 0,2): con 1:1 il richiamo degli attacchi della Trama passa da 1,0 a 0,9875, cioè 5 attacchi su 400 non rilevati. È coerente con un attacco più morbido, perché l'indice parte da 0 con l'inviluppo e la nota comincia quasi sinusoidale.
- **La calibrazione degenera a δ 0,3:** sceglie un offset di 95 ms con precisione e richiamo pari a 1, cioè si allinea ai rilasci invece che agli attacchi. Il profilo calibrato cambia, e `acao02_v1` resta `INCONCLUSIVE` come già dopo F4a. **Per M2.5.5:** la ricalibrazione deve limitare l'offset ammesso, oppure escludere le rilevazioni sui rilasci, altrimenti può scegliere l'evento sbagliato.
- `full_m2_suite`: tutte le voci PASS, `MEASURED` o `GENERATED`, tranne `acao02_v1` (limite noto, ricalibrazione a M2.5.5).

## 4. Limiti

- Il modello di Sethares conosce solo l'interferenza fra parziali. Con 1:1 lo spettro della Trama è armonico completo, come quello di Corpo. Due domande restano all'ascolto cumulativo (D-M2-30): se la Trama resti distinguibile da Corpo, e se l'evoluzione spettrale nella nota, simile a un ottone, sia gradita.
- Con 1:1 il rapporto non dipende più da `timbreParam`, che continua a muovere solo l'indice.

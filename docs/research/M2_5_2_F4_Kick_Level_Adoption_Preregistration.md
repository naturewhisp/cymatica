# M2.5.2 — F4a, adozione del kick a +8 dB: predizioni

- **Stato:** scritto il 2026-11-16, prima di cambiare `kPulsoGain` e prima di misurare. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione:** ADR-0007 D-M2-31 (titolare, 2026-11-16): Pulso da −6 a +2 dB, cioè +8 dB. Regola di verifica autonoma: D-M2-30.
- **Riferimenti misurati** ([risultati F4a](M2_5_2_F4_Kick_Level_Results.md), produzione a −6 dB): mix −21,255 LUFS; Pulso −34,487; Corpo −24,819; Trama −34,087; Vettore −24,212 LUFS; true peak −10,45 dBTP; con l'override: +6 dB → −8,00 dBTP, +9 dB → −6,10 dBTP, riduzione del limiter 0 dB.

## Predizioni

| ID | Predizione | Regola |
|---|---|---|
| **A1** | `fixture_test` resta PASS in ogni controllo tranne il golden PCM, che cambia. Golden del piano (`0x4f021cb9e8637598`) e del trace invariati, eventi applicati invariati | un controllo diverso dal PCM che fallisce → l'adozione si ferma |
| **A2** | Loudness integrata di Pulso −26,487 LUFS ± 0,05 (+8 LU) | fuori → mancata |
| **A3** | Corpo, Trama e Vettore invariati entro ± 0,01 LU (il ducking segue il guadagno) | fuori → mancata |
| **A4** | Mix +0,98 LU ± 0,3 (somma di potenze non correlate, p = 0,0475), cioè −20,28 LUFS. Con la sottostima sistematica osservata in F4a ci si aspetta un valore sotto la predizione, fra +0,52 (+6 dB) e +1,13 (+9 dB) | fuori da ± 0,3 → mancata |
| **A5** | Riduzione del limiter 0 dB; true peak fra −8,00 e −6,10 dBTP | fuori → mancata |
| **A6** | Le altre prove di regressione restano verdi, tranne i golden PCM che dipendono dal livello di Pulso. Ognuno di questi si aggiorna con questo motivo, e se ne registrano il vecchio e il nuovo valore | un fallimento senza legame con il livello di Pulso → si indaga prima di aggiornare |
| **A7** | `kick_level_ab_v1` resta come evidenza di F4a sulla base −6 dB: gli offset sono relativi alla produzione corrente, quindi P5 e P6 valevano per quella base e non si rivalutano. Rieseguito sulla nuova produzione, con il golden collegato a quello della fixture, P1–P4 e P7 restano confermate: meccanismo dell'override, null test, +g LU esatti, determinismo | P1–P4 o P7 che falliscono → difetto dello scenario |

Senza predizione: loudness per sezione e quota d'energia sotto i 150 Hz del brano.

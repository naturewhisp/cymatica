# CP-1 — Prova del volume sul loopback USB (R-M25-12): risultati

- **Pre-registrazione:** [CP1_Volume_Loopback_Preregistration.md](../../CP1_Volume_Loopback_Preregistration.md), SHA-256 (LF) `9ed5c11a…`, registrata prima del codice e delle corse.
- **Esecuzione, 2026-11-12:** cuffie USB come uscita predefinita, collegate ma **non indossate**; volume impostato a mano dal titolare; macchina silenziosa. `device_probe_v1 --device-usage games --device-capture-pcm --device-loopback`, build Release; il manifest legge in sola lettura il volume dell'endpoint all'inizio e alla fine della cattura. Analisi con `analyze_loopback.py`. Dati: `v10`, `v40`, `v100` (manifest e analisi).

## Risultati

| Cursore | Volume master | Volume hardware | Invariato nella corsa | Probe | Guadagno del loopback | Scarto | Silenzi, picchi |
|---|---:|---|---|---|---:|---:|---|
| 10 % | −33,98 dB | sì | sì | `MEASURED` | −59,547 dB | −148,89 dB | 0, 0 |
| 40 % | −13,75 dB | sì | sì | `MEASURED` | −59,547 dB | −148,89 dB | 0, 0 |
| 100 % | 0,00 dB | sì | sì | `MEASURED` | −59,547 dB | −148,89 dB | 0, 0 |

Intervallo dichiarato dall'endpoint: da −74 a 0 dB, passo 0,5 dB; volume e muto gestiti in hardware (`QueryHardwareSupport`).

## Esito rispetto alle ipotesi

- **H1 respinta:** a 10 % il guadagno differisce dal volume master di 25,6 dB, a 100 % di 59,5 dB; la tolleranza era 0,5 dB.
- **H0 accettata:** escursione del guadagno fra i livelli 0,000 dB su 34 dB di escursione del volume (tolleranza 0,5 dB). Lo scarto resta −148,89 dB, sotto il limite di −140 dB: un solo stadio di guadagno.
- **Lettura fissata prima:** il −59,5 dB non viene dal volume dell'endpoint, che è hardware e agisce dopo il punto di cattura. La causa è altrove e va cercata prima di usare il loopback USB per la qualifica. OSS-M2.5.1d-02 resta aperta, con il campo delle cause ristretto.

## Ipotesi successiva, non pre-registrata

Il mixer di Windows applica anche il **volume della sessione dell'applicazione** (il cursore per app del mixer del volume), memorizzato per ogni coppia app-dispositivo. È compatibile con i fatti: un solo stadio di guadagno, prima del punto di cattura, presente sull'uscita USB e assente sulla prima uscita, dove il loopback era identico al bit. **Non è verificato.** La prova successiva, da pre-registrare, legge in sola lettura il volume della sessione (`ISimpleAudioVolume`) nel manifest.

## H2, volume della sessione dell'app (2026-11-12)

Pre-registrazione: [aggiunta H2](../../CP1_Volume_Loopback_Preregistration_Addendum_Session.md), SHA-256 (LF) `06eff045…`, registrata prima del codice e della corsa. Dati: `session` (manifest e analisi). Volume dell'endpoint 0,00 dB (100 %), cuffie non indossate.

| Grandezza | Valore |
|---|---|
| Sessioni del processo sull'endpoint | 1, invariata fra inizio e fine |
| Livello di sessione (`ISimpleAudioVolume`) | 0,0324589 |
| Livelli dei canali (`IChannelAudioVolume`) | 1, 1 |
| 20·log10(livello) | −29,773 dB |
| Guadagno del loopback | −59,547 dB, scarto −148,89 dB |

**Esito secondo il criterio fissato: H2 parziale.** La sessione attenua (livello diverso da 1), ma con la legge lineare pre-registrata il valore è −29,77 dB, fuori dalla tolleranza di ±0,5 dB.

**Osservazione non pre-registrata.** Il guadagno coincide con il **quadrato** del livello: 40·log10(0,0324589) = −59,547 dB, cioè −29,773 × 2. È compatibile con una legge quadratica fra cursore dell'app e ampiezza, ma è stata trovata dopo aver visto i dati: va confermata con una seconda corsa pre-registrata a un livello di sessione diverso, impostato a mano dal titolare nel mixer del volume di Windows.

La sessione risulta «inattiva» in entrambe le letture, fatte prima che il flusso entri nel mixer e dopo l'arresto: lo stato non dice nulla sul volume, che Windows conserva per la coppia app-dispositivo.

**Conseguenza pratica, se confermata:** sull'uscita USB il cursore di `cymatica_game` nel mixer di Windows è quasi a zero (circa 3 %). Riportandolo al massimo, il loopback dovrebbe tornare uguale al golden senza guadagno, come sulla prima uscita.

## H2b, legge del volume di sessione (2026-11-12)

Pre-registrazione: [aggiunta H2b](../../CP1_Volume_Loopback_Preregistration_Addendum_SessionLaw.md), SHA-256 (LF) `e0d7180b…`, registrata prima che il titolare cambiasse il cursore e prima della corsa. Il gioco è stato avviato 120 s solo per far comparire il cursore; il titolare ha portato `cymatica_game` al 50 % nel mixer di Windows (volume di sistema circa 70 %, −5,37 dB). Dati: `law` (manifest e analisi).

| Grandezza | Valore |
|---|---|
| Livello di sessione | 0,5, canali 1 e 1, invariato fra inizio e fine |
| Predizione quadratica 40·log10(0,5) | −12,0412 dB |
| Predizione lineare 20·log10(0,5) | −6,0206 dB |
| Guadagno del loopback | **0,25 (−12,041 dB)** |
| Scarto | **zero esatto** (`exact_zero_residual` vero; −2967 dB è il pavimento numerico dello script) |

**Esito: H2b accettata, H2a respinta.** Il guadagno è esattamente il quadrato del livello di sessione; con 0,25, potenza di due, la moltiplicazione in float32 è esatta e il loopback è il golden scalato bit per bit. Il −148,89 dB delle corse precedenti era l'arrotondamento di un fattore non esatto (0,0324589² ≈ 0,001054).

**OSS-M2.5.1d-02 spiegata.** Sull'uscita USB Windows aveva memorizzato il cursore di `cymatica_game` a 0,0324589 (circa 3 %); il mixer applica il quadrato del livello di sessione prima del punto di cattura del loopback. Il volume dell'endpoint, hardware, agisce dopo e non entra nel loopback (H0).

**Per la qualifica sul device (CP-1):** il manifest registra ora volume dell'endpoint e livello di sessione; il confronto col golden può compensare il fattore livello², oppure il titolare riporta il cursore dell'app al massimo prima delle corse. La legge quadratica è misurata su due livelli (0,0325 e 0,5) su questo dispositivo e questa versione di Windows; non è stata verificata su documentazione Microsoft.

## Note della review (2026-11-12)

- **Provenienza del binario (OSS-CP1-RM25-12-01):** tutte le corse riportano `build_id = 92e47d3`. Il binario è stato compilato dal working tree, con la lettura dei volumi già presente ma non ancora committata; il codice delle corse è quello di `5a796d4`. Il campo `build_id` identifica solo l'ultimo commit, non le modifiche locali.
- **Identità dell'endpoint (OSS-CP1-RM25-12-02):** i volumi sono letti dall'endpoint predefinito `eConsole` con una enumerazione propria, non dal dispositivo aperto da miniaudio. In queste corse coincidono (unica uscita predefinita); per la qualifica generale va letto l'`IMMDevice` dell'endpoint acquisito, o verificato che gli identificativi coincidano. Rinviato alla qualifica sul device.
- **Portata della legge quadratica (OSS-CP1-RM25-12-03):** osservata su due livelli e un dispositivo. Per CP-1 la raccomandazione è una sessione a guadagno unitario, impostata a mano e verificata nel manifest, invece di una compensazione generale livello²; la compensazione andrebbe ammessa solo dopo una qualifica specifica dell'endpoint.

# CP-1 — Prova del volume sul loopback USB (R-M25-12): pre-registrazione

- **Stato:** pre-registrazione, scritta il 2026-11-12 **prima** di implementare la lettura del volume e di eseguire le corse. Non normativa. L'hash di questo file è registrato in `docs/progress.md` prima della prima corsa.
- **Origine:** [consolidamento §6.2](CYMATICA_Analysis_Consolidation_2026-11-09.md), OSS-M2.5.1d-02: sulle cuffie USB il loopback è uguale al golden scalato di −59,5 dB (scarto −148,89 dB), causa non identificata.
- **Sicurezza (OSS-CONS-R1-02, R2-01):** il limiter controlla il segnale digitale, non la pressione acustica. **Le cuffie non si indossano durante le corse.** I livelli li imposta il titolare a mano; l'agente non modifica impostazioni di sistema. Il volume massimo non diventa un requisito del gate.

## Misura

`device_probe_v1 --device-usage games --device-capture-pcm --device-loopback` sull'uscita USB predefinita, a macchina silenziosa. Il manifest registra, in sola lettura e all'inizio e alla fine della cattura, il volume master dell'endpoint predefinito (`IAudioEndpointVolume`: dB, valore scalare 0–1, muto, intervallo in dB) e se volume e muto sono hardware (`QueryHardwareSupport`). Il guadagno del loopback è il `gain_db` di `analyze_loopback.py`.

**Livelli:** almeno tre volumi scalari crescenti scelti dal titolare (proposta: 10 %, 40 %, 100 %), una corsa per livello. Una corsa vale solo se il volume letto all'inizio coincide con quello alla fine e il loopback è `MEASURED`.

## Ipotesi e predizioni

| Ipotesi | Predizione | Si accetta se |
|---|---|---|
| **H1** (consolidamento §6.2): il volume dell'endpoint è applicato in software prima del punto di cattura | `gain_db` uguale al volume master in dB a ogni livello | \|`gain_db` − volume master dB\| ≤ 0,5 dB a ogni livello valido |
| **H0**: il loopback è preso prima del volume (default documentato per `AUDCLNT_STREAMOPTIONS`) | `gain_db` costante fra i livelli | escursione di `gain_db` fra i livelli ≤ 0,5 dB |

La tolleranza di 0,5 dB è fissata ora; la differenza di 3 dB citata nel consolidamento non è spiegata e non la si recupera a posteriori. Se nessuna delle due regge, la causa è altrove e si riporta così. Lo scarto residuo deve restare sotto −140 dB a ogni livello: sopra quella soglia la catena non è più un solo stadio di guadagno e la lettura in dB perde significato.

## Lettura per CP-1

- **H1 regge:** la qualifica sul device registra il volume master nel manifest e il confronto col golden lo compensa; nessuna corsa a volume alto con le cuffie indossate. Chiude OSS-M2.5.1d-02.
- **H0 regge:** il −59,5 dB nasce altrove (driver, APO, guadagno del dispositivo); si indaga prima di usare il loopback USB per la qualifica.
- **Volume hardware:** se `QueryHardwareSupport` lo dichiara, la lettura del volume master può non descrivere ciò che accade nel mixer software; lo si riporta e la prova vale solo per i livelli osservati.

Nessun ascolto è richiesto da questa prova.

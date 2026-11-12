# CP-1 — Prova del volume sul loopback USB: aggiunta per il volume di sessione (H2)

- **Stato:** aggiunta scritta il 2026-11-12, dopo la prova del volume dell'endpoint ([risultati](experiments/cp1_volume_loopback/README.md): H1 respinta, H0 accettata) e **prima** di implementare la lettura del volume di sessione e di eseguire la corsa. Non modifica la [pre-registrazione](CP1_Volume_Loopback_Preregistration.md). L'hash è registrato in `docs/progress.md` prima della corsa.

## Ipotesi H2

Il −59,547 dB è il **volume della sessione audio di `cymatica_game`** sull'endpoint USB: il cursore per applicazione del mixer di Windows, memorizzato per ogni coppia app-dispositivo e applicato dal motore audio prima del punto di cattura del loopback.

## Misura

Il manifest legge in sola lettura, all'inizio e alla fine della cattura, il volume della sessione predefinita del processo sull'endpoint predefinito: `ISimpleAudioVolume` (livello 0–1, muto) e `IChannelAudioVolume` (livello di ogni canale). Una corsa sull'uscita USB, `device_probe_v1 --device-usage games --device-capture-pcm --device-loopback`; il guadagno è il `gain_db` di `analyze_loopback.py`. La corsa vale se la sessione è leggibile e invariata fra inizio e fine.

## Predizioni e criterio

Il livello della sessione è un fattore lineare di ampiezza: è l'ipotesi più semplice, e la si verifica qui.

| Esito | Condizione | Lettura |
|---|---|---|
| **H2 accettata** | 20·log10(livello di sessione × livello di canale) entro ±0,5 dB da `gain_db` | Il −59,5 dB è il volume della sessione dell'app: si chiude OSS-M2.5.1d-02 |
| **H2 parziale** | Livello di sessione o di canale diverso da 1, ma fuori dalla tolleranza | La sessione attenua, ma la legge livello→guadagno non è lineare o c'è un altro stadio: si riporta così |
| **H2 respinta** | Livelli di sessione e di canale uguali a 1 (entro 0,001) | Il −59,5 dB nasce altrove: driver, APO o dispositivo |

Il guadagno atteso nel caso H2 accettata è −59,547 dB, cioè un fattore di circa 0,00105.

## Conseguenza per CP-1

Se H2 è accettata, la qualifica sul device registra anche il volume di sessione e il confronto col golden lo compensa, oppure si chiede di riportare il cursore dell'app a 1 prima delle corse. In nessun caso l'agente modifica il volume della sessione o del sistema.

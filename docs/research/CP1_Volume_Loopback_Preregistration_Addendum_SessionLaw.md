# CP-1 — Prova del volume sul loopback USB: aggiunta per la legge del volume di sessione (H2b)

- **Stato:** aggiunta scritta il 2026-11-12, dopo l'esito parziale di H2 ([risultati](experiments/cp1_volume_loopback/README.md)) e **prima** che il titolare cambi il cursore e prima della corsa. Non modifica le aggiunte precedenti. L'hash è registrato in `docs/progress.md` prima della corsa.
- **Origine:** osservazione non pre-registrata di H2: con livello di sessione 0,0324589 il guadagno del loopback è −59,547 dB, cioè 40·log10(livello) e non 20·log10(livello).

## Procedura

1. Il gioco viene avviato per circa 90 secondi solo per far comparire `cymatica_game` nel mixer del volume di Windows; il titolare porta a mano il cursore dell'app sull'uscita USB a **circa 50 %**. Questa esecuzione non è una misura.
2. Corsa di misura `device_probe_v1 --device-usage games --device-capture-pcm --device-loopback`: il manifest legge il livello di sessione memorizzato da Windows; il guadagno è il `gain_db` di `analyze_loopback.py`. Vale se il livello di sessione è invariato fra inizio e fine e diverso da quello di H2.
3. Le cuffie non si indossano durante la corsa, o il volume di sistema resta a un livello sicuro.

## Ipotesi e criterio

| Ipotesi | Predizione | Si accetta se |
|---|---|---|
| **H2b, quadratica** | `gain_db` = 40·log10(livello × livello di canale) | entro ±0,5 dB |
| **H2a, lineare** | `gain_db` = 20·log10(livello × livello di canale) | entro ±0,5 dB |

Con un livello vicino a 0,5 le due predizioni distano circa 6 dB, quindi la corsa le distingue. Se nessuna regge, si riporta così.

## Conseguenza

Se H2b regge, OSS-M2.5.1d-02 è spiegata: il −59,5 dB era il cursore di `cymatica_game` sull'uscita USB, memorizzato da Windows a circa il 3 %, con una legge quadratica. Per la qualifica sul device il manifest registra il livello di sessione e il confronto col golden lo compensa, oppure si riporta il cursore al massimo prima delle corse. L'agente non modifica mai volumi di sessione o di sistema.

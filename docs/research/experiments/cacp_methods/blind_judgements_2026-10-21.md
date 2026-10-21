# Giudizi alla cieca sugli spettrogrammi (Claude, 2026-10-21)

Valutatore: Claude (Opus 5.5), lettura di immagini PNG. Domanda: «la finestra di 4 battute contiene la Trama?». Ogni giudizio è stato scritto nella conversazione prima di leggere la chiave.

## Turno 1 (nessun riferimento; chiave `blind_key_round1.json`, seed 20261021)

| Finestra | Giudizio | Chiave (battuta, onset Trama) | Esito |
|---|---|---|---|
| A | assente | 24, 20 | errato |
| B | assente | 16, 20 | errato |
| C | presente | 8, 0 | errato |
| D | presente | 40, 20 | corretto |
| E | presente | 72, 20 | corretto |
| F | presente | 0, 0 | errato |

Risultato: 2/6. L'errore è sistematico: ho preso le linee verticali larghe (attacchi di Pulso e Vettore) per la Trama. La Trama corrisponde invece ai brevi trattini orizzontali nel registro alto.

## Turno 2 (riferimenti: le immagini e la chiave del turno 1; chiave `blind_key_round2.json`, seed 777)

| Finestra | Giudizio | Chiave | Esito |
|---|---|---|---|
| G | assente | 4, 0 | corretto |
| H | presente | 84, 20 | corretto |
| J | assente | 12, 0 | corretto |
| K | presente | 52, 20 | corretto |
| L | presente | 28, 20 | corretto |
| M | presente | 20, 20 | corretto |

Risultato: 6/6. **Avvertenza:** frasi ripetute producono spettrogrammi quasi identici, quindi questo turno misura il confronto con riferimenti etichettati, non la capacità di interpretare uno spettrogramma nuovo.

# Baseline su device della fixture (M2.5.1a, 2026-11-04)

Prima esecuzione in tempo reale di `fixture-m2-4-v1` sul dispositivo predefinito, con lo scenario `device_probe_v1`. È una misura, non un gate: il criterio PASS si decide a CP-1 ([ADR-0007](../../../adr/0007-m2-5-entry-decisions.md)). La qualificazione finale resta in M2.5.6, sulla build finale.

- **Ambiente:** vedi `environment` nei manifest; WASAPI condiviso, 48 kHz, periodo 480 frame (10 ms) per 3 periodi (buffer 30 ms).
- **Comando:** `bash docs/research/experiments/m25_device_baseline/run.sh` dalla radice del repository: crea una nuova cartella `artifacts/m25_device_baseline/series_<data>` e ne stampa il nome. Riassunto con `python docs/research/experiments/m25_device_baseline/summarize.py <cartella>`, che rifiuta una serie incompleta o incoerente. La serie archiviata qui si riassume passando questa stessa cartella.
- **Controlli negativi degli script:** `test_series_tools.py`, con un gioco finto e senza device (14/14, `test_series_tools.output.txt`).
- **Disegno:** 3 ripetizioni per ciascuna configurazione MMCSS (nessuna, Games, Pro Audio), sempre nello stesso ordine: questo limita l'interpretazione causale del confronto (osservazione della review). `run.sh` ora ruota l'ordine a ogni ripetizione e si ferma alla prima corsa non valida. La macchina non era isolata: il titolare riferisce che Diablo 4, un gioco pesante, è rimasto aperto per tutta la serie. Il carico effettivo di CPU e GPU non è stato misurato.

## Risultati

Output completo in `summary.txt`. In tutte e 9 le esecuzioni: fixture completa, 8.777.142 frame, ack di Start, tempo e Stop esatti, 0 eventi scartati o rinviati, limiter a 0 dB, 0 sospetti underrun.

| Configurazione | MMCSS | Callback p99.9 | Callback max | Oltre il periodo | Intervallo max |
|---|---|---:|---:|---:|---:|
| Default (attuale) | no | 0,21–0,28 ms | 0,72–10,87 ms | 0, 0, 1 | 16,55–19,55 ms |
| Games | sì | 0,16–0,17 ms | 0,31–0,49 ms | 0 | 10,82–10,91 ms |
| Pro Audio | sì | 0,13–0,15 ms | 0,31–0,35 ms | 0 | 10,79–11,04 ms |

**Lettura.** Senza MMCSS compaiono picchi isolati: una callback ha superato il periodo di 10 ms, ma il buffer di 30 ms li ha assorbiti. Con MMCSS i picchi spariscono in entrambe le configurazioni. Fra Games e Pro Audio queste misure non mostrano una differenza utile. Tre ripetizioni non permettono di stimare la frequenza dei picchi rari. Il gioco aperto vale per tutte le corse, quindi il confronto fra configurazioni avviene in condizioni simili. Il suo carico non è misurato e può variare nel tempo: la serie indica una buona tenuta con un carico concorrente, ma non è una prova da stress controllata.

## Limiti

- WASAPI condiviso non espone un conteggio di underrun. `suspected_underruns` conta gli intervalli fra callback almeno pari al buffer: è una stima indiretta, e un valore zero non prova l'assenza di crackle. Nessuno ha ascoltato queste corse in modo controllato.
- Un solo dispositivo, una sola macchina, contenuto attuale.
- `frames_rendered` non è l'audio udito. CACP spento: nessuna cattura PCM, quindi nessun confronto col golden.
- Gli eventuali effetti di M2.5.2 cambieranno il costo della callback.

- Le 9 corse valide precedono DIF-M2.5.1a-02: lo stato `MEASURED` di allora non verificava frasi pianificate ed eventi consumati. Una corsa successiva con i controlli completi (Games) è in `verifica_games_dif_m251a.manifest.json.txt`, escluso da `summarize.py`: 24 frasi su 24, 1.392 note, 2.784 eventi inviati e consumati, 0 in staging allo Stop, callback massima 0,36 ms, intervallo massimo 15,42 ms.

## `invalid_timeout/`

Prima serie di 9 esecuzioni, `INVALID` per un errore dello scenario: dopo lo Stop il cursore si ferma e il ciclo attendeva un frame mai raggiunto. Conservata senza modifiche; le misure includono circa 16 s di callback dopo lo Stop.

# Baseline su device della fixture (M2.5.1a, 2026-11-04)

Prima esecuzione in tempo reale di `fixture-m2-4-v1` sul dispositivo predefinito, con lo scenario `device_probe_v1`. È una misura, non un gate: il criterio PASS si decide a CP-1 ([ADR-0007](../../../adr/0007-m2-5-entry-decisions.md)). La qualificazione finale resta in M2.5.6, sulla build finale.

- **Ambiente:** vedi `environment` nei manifest; WASAPI condiviso, 48 kHz, periodo 480 frame (10 ms) per 3 periodi (buffer 30 ms).
- **Comando:** `bash docs/research/experiments/m25_device_baseline/run.sh` dalla radice del repository: crea una nuova cartella `artifacts/m25_device_baseline/series_<data>` e ne stampa il nome. Riassunto con `python docs/research/experiments/m25_device_baseline/summarize.py <cartella>`, che rifiuta una serie incompleta o incoerente. La serie archiviata qui si riassume passando questa stessa cartella.
- **Controlli negativi degli script:** `test_series_tools.py`, con un gioco finto e senza device (15/15, `test_series_tools.output.txt`).
- **Disegno:** 3 ripetizioni per ciascuna configurazione MMCSS (nessuna, Games, Pro Audio), sempre nello stesso ordine: questo limita l'interpretazione causale del confronto (osservazione della review). `run.sh` ora ruota l'ordine a ogni ripetizione e si ferma alla prima corsa non valida. La macchina non era isolata: il titolare riferisce che Diablo 4, un gioco pesante, è rimasto aperto per tutta la serie. Il carico effettivo di CPU e GPU non è stato misurato.

## Risultati

Output completo in `summary.txt`. In tutte e 9 le esecuzioni: fixture completa, 8.777.142 frame, ack di Start, tempo e Stop esatti, 0 eventi scartati o rinviati, limiter a 0 dB, 0 sospetti underrun.

| Configurazione | MMCSS | Callback p99.9 | Callback max | Oltre il periodo | Intervallo max |
|---|---|---:|---:|---:|---:|
| Default (attuale) | no | 0,21–0,28 ms | 0,72–10,87 ms | 0, 0, 1 | 16,55–19,55 ms |
| Games | sì | 0,16–0,17 ms | 0,31–0,49 ms | 0 | 10,82–10,91 ms |
| Pro Audio | sì | 0,13–0,15 ms | 0,31–0,35 ms | 0 | 10,79–11,04 ms |

**Lettura.** Senza MMCSS compaiono picchi isolati: una callback ha superato il periodo di 10 ms, ma il buffer di 30 ms li ha assorbiti. Con MMCSS i picchi spariscono in entrambe le configurazioni. Fra Games e Pro Audio queste misure non mostrano una differenza utile. Tre ripetizioni non permettono di stimare la frequenza dei picchi rari. Il gioco aperto vale per tutte le corse, quindi il confronto fra configurazioni avviene in condizioni simili. Il suo carico non è misurato e può variare nel tempo: la serie indica una buona tenuta con un carico concorrente, ma non è una prova da stress controllata.

## Confronto del PCM (M2.5.1d)

`pcm_games_m251d.manifest.json.txt`: con `--device-capture-pcm`, 0 campioni diversi su 8.825.200 frame rispetto al render offline a 480 frame, hash uguale al golden. Il confronto è a monte del mixer di Windows.

## Loopback a valle del mixer (M2.5.1d)

`device_probe_v1 --device-capture-pcm --device-loopback` registra il mix di Windows sull'uscita predefinita. `analyze_loopback.py` allinea la cattura al golden e cerca slittamenti, silenzi e picchi di residuo; `compare_loopback_bits.py` confronta i bit. PC silenzioso, Games, 2026-11-05.

| Corsa | Campioni diversi | Guadagno | Slittamenti | Silenzi anomali |
|---|---:|---:|---:|---:|
| Pulita (`loopback_games_m251d`) | 0 su 17.553.674 | 1,0 | 0 | 0 |
| Controllo con `chimes.wav` a 60 s (`loopback_control_chimes`) | 115.617 | 0,99998 | 0 | 0 |

Il controllo positivo dimostra che la cattura è il vero mix: il suono compare a 60,26 s, 121 finestre da 10 ms oltre la soglia. In questa corsa il mixer ha lasciato passare il segnale identico al bit. Il segno di uno slittamento distingue i casi: ritardo in aumento è silenzio inserito, ritardo in calo è perdita in cattura ([dossier R-M25-8](../../M2_5_1_Windows_Audio_Glitch_Detection.md)).

`residual_db` di −179,18 dB nei file `loopback_*` è il pavimento numerico della prima versione dello script, non una misura: il residuo è zero. `analyze_loopback.py` ha controlli su segnali sintetici in `test_analyze_loopback.py` (15/15). I file `.f32` restano in `artifacts/`, non versionati.

### Seconda uscita: cuffie USB (2026-11-05)

Stessa procedura, uscita predefinita sulle cuffie USB del titolare, volume basso. Periodo e buffer concessi uguali: 10 ms per 3, mix a 48 kHz.

| Corsa | Guadagno | Scarto dal golden scalato | Slittamenti | Silenzi anomali | PCM della callback |
|---|---:|---:|---:|---:|---|
| Pulita (`usb_clean`) | −59,5 dB | sotto −120 dB | 0 | 0 | 0 campioni diversi su 8.825.200 |
| Controllo con `chimes.wav` a 60 s | −59,5 dB | −140,6 dB fuori dal suono | 0 | 0 | non catturato |

- **Attenuazione osservata:** il segnale è il golden per un guadagno costante di −59,5 dB, quindi il confronto al bit non si applica. La causa non è identificata: il volume era basso, ma la documentazione Microsoft colloca il loopback predefinito prima del volume dell'endpoint, quindi possono entrare volume di sessione o di stream, o elaborazioni dell'uscita. Sull'uscita precedente il guadagno era 1,0.
- **Controllo positivo:** il suono compare a 60,22 s e il residuo torna al livello di arrotondamento a 61,44 s.
- **Cambio nello script:** la prima versione misurava il residuo in valori assoluti, e a volume basso finiva sul pavimento numerico. Ora i livelli sono relativi al segnale e il guadagno è una mediana, così un altro suono non lo altera. La corsa pulita è stata analizzata con la prima versione e la sua cattura non è conservata: il limite «sotto −120 dB» è ricavato da quel pavimento. Il valore −140,6 dB viene dalla corsa di controllo. La nuova corsa pulita con la versione corretta è nel paragrafo sul carico controllato (OSS-M2.5.1d-03, chiusa).

### Carico controllato (2026-11-05)

`load_series.py`, dati in `load_series_m251d/`. Cuffie USB, una corsa per condizione. Il carico è un processo per ogni core logico, ciascuno su 64 MB di memoria; la CPU è campionata ogni secondo. Non carica la GPU.

| Corsa | CPU mediana | MMCSS | Callback p99.9 | Callback max | Intervallo max | Eventi persi | PCM uguale al golden | Scarto loopback |
|---|---:|---|---:|---:|---:|---:|---|---:|
| Riposo | 2,6 % | sì | 0,13 ms | 0,27 ms | 17,36 ms | 0 | sì | −148,89 dB |
| Carico | 100 % | sì | 0,12 ms | 0,23 ms | 17,34 ms | 0 | sì | −148,89 dB |
| Riposo | 2,7 % | no | 0,15 ms | 0,39 ms | 17,35 ms | 0 | sì | −148,89 dB |
| Carico | 100 % | no | 0,13 ms | 0,23 ms | 17,42 ms | 0 | sì | −148,89 dB |

In tutte e quattro: nessuno slittamento, picco di residuo o silenzio anomalo, 0 sospetti underrun, guadagno −59,5 dB. La corsa a riposo con MMCSS è la nuova corsa USB pulita (OSS-M2.5.1d-03).

**Lettura.**
- La CPU piena non ha degradato né i tempi né il segnale.
- **MMCSS non mostra qui un vantaggio.** Senza MMCSS e con la CPU piena la callback massima è 0,23 ms. I picchi fino a 10,87 ms della prima serie, con Diablo 4 aperto, non si riproducono con questo carico. Il beneficio attribuito a MMCSS resta quindi non spiegato: può dipendere dalla GPU, dalla priorità dei processi del gioco o dall'uscita HDMI. La scelta di MMCSS a CP-1 deve tenerne conto.
- Il coordinatore sul thread normale risente del carico (intervallo massimo da 17 a 42–52 ms), ma resta entro l'anticipo di 150 ms.
- L'intervallo massimo di circa 17 ms è comune a tutte le corse USB, carico o no, quindi sembra una proprietà dell'uscita.

Una corsa per condizione non stima gli eventi rari.

## Limiti

- WASAPI condiviso non espone un conteggio di underrun. `suspected_underruns` conta gli intervalli fra callback almeno pari al buffer: è una stima indiretta, e un valore zero non prova l'assenza di crackle. Nessuno ha ascoltato queste corse in modo controllato.
- Due uscite, una sola macchina, contenuto attuale. Una corsa loopback pulita non esclude eventi rari.
- Il loopback non vede driver, connessione, DAC e casse. Volume e stadi a valle non sono distinti da questa prova.
- `frames_rendered` non è l'audio udito. Le 9 corse hanno CACP spento: il confronto col golden è nella corsa separata qui sopra.
- Gli eventuali effetti di M2.5.2 cambieranno il costo della callback.

- Le 9 corse valide precedono DIF-M2.5.1a-02: lo stato `MEASURED` di allora non verificava frasi pianificate ed eventi consumati. Una corsa successiva con i controlli completi (Games) è in `verifica_games_dif_m251a.manifest.json.txt`, escluso da `summarize.py`: 24 frasi su 24, 1.392 note, 2.784 eventi inviati e consumati, 0 in staging allo Stop, callback massima 0,36 ms, intervallo massimo 15,42 ms.

## `invalid_timeout/`

Prima serie di 9 esecuzioni, `INVALID` per un errore dello scenario: dopo lo Stop il cursore si ferma e il ciclo attendeva un frame mai raggiunto. Conservata senza modifiche; le misure includono circa 16 s di callback dopo lo Stop.

# CP-1 — Monitor `IAudioClock` e ascolto controllato sul device: risultati

- **Pre-registrazione:** [CP1_DeviceClock_Preregistration.md](../../CP1_DeviceClock_Preregistration.md), SHA-256 (LF) `fc295232…`, registrata prima del codice e delle corse.
- **Esecuzione, 2026-11-12:** cuffie USB come uscita predefinita, macchina silenziosa. Binario compilato dal working tree con il monitor non ancora committato (il `build_id` del manifest indica l'ultimo commit).

## Controllo positivo (`glitchcal`, procedura B di R-M25-8)

`glitchcal` compilato dal [sorgente versionato](../m25_glitch_calibration/glitchcal.cpp) con MSVC (`/O2`, eseguibile non versionato), tono a −80 dBFS, stalli di 25 e 60 ms per tre cicli. Dati: `positive_control_usb_stalls.csv`.

| Stallo | Buco in `IAudioClock` | Silenzio nel loopback |
|---|---:|---:|
| 25 ms (3 volte) | 0,02 / 0,00 / 0,04 ms | 0 / 0 / 0 ms |
| 60 ms (3 volte) | 30,64 / 30,01 / 30,00 ms | 30,00 / 30,02 / 30,00 ms |

**Esito come atteso:** con un buffer di 30 ms lo stallo di 25 ms non apre buchi, quello di 60 ms apre un buco di 30 ms; il monitor lo vede ogni volta, in accordo col loopback entro 0,64 ms. Sull'uscita USB lo zero del monitor vale. Una corsa di silenzio di 379 ms cade dopo la fine dello stream (13.859 ms su 13.808), quindi è la chiusura, non un buco.

## Corsa del probe con monitor e ascolto

`device_probe_v1 --device-usage games --device-capture-pcm --device-loopback`; cursore di `cymatica_game` riportato al 100 % dal titolare e letto a 1,0 nel manifest; volume di sistema −5,37 dB (70 %). Dati: `probe_usb` (manifest e analisi).

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| K1 | Campioni validi ≥ 90 % di circa 3.700 | 2.955 campioni, 0 fallimenti (circa 80 %) | **Mancata** |
| K2 | Nessun salto oltre 5 ms dopo i primi 500 ms | salto massimo 0,69 ms (a 155,5 s), 0 oltre 5 ms | Confermata |
| K3 | Loopback uguale al golden con guadagno 1 | guadagno 1,0, scarto esattamente zero | Confermata |

- **K1:** il ciclo del coordinatore dorme 1 ms per volta e legge il monitor quando sono passati almeno 50 ms; con la granularità del timer di Windows l'intervallo medio diventa circa 62 ms (2.955 letture in circa 183 s). Causa probabile, non verificata. Non riduce la capacità di vedere un buco, che resta nello scarto cumulativo.
- **Scarto finale 24,2 ms**, cresciuto gradualmente senza salti: compatibile con una differenza di circa 130 ppm fra l'orologio del PC e quello del dispositivo USB. Non verificato.
- **Resto della corsa:** `MEASURED`, 0 eventi persi, PCM uguale al golden, callback massima 0,49 ms, p99.9 0,20 ms, intervallo massimo 17,36 ms, 0 sospetti underrun, nessuno slittamento, picco o silenzio nel loopback.
- **Ascolto controllato:** il titolare ha ascoltato la corsa in cuffia a volume sicuro: **nessun click, buco o artefatto**.

**Lettura:** su questa uscita i tre rilevatori (monitor `IAudioClock` verificato dal controllo positivo, loopback, intervalli della callback) e l'ascolto concordano: nessun buco. È una corsa sola su un dispositivo: non stima gli eventi rari. Il criterio PASS del device resta una decisione di CP-1.

## Note della review e correzione (2026-11-16)

- **DIF-CP1-CLOCK-01 (MED), client WASAPI sostituito dal rerouting:** il monitor conservava `IAudioClock` del primo client fino allo shutdown; se Windows cambia l'endpoint predefinito, miniaudio può ricreare il client e il monitor leggerebbe quello vecchio. **Corretto in due modi:** il probe apre il device con `noAutoStreamRouting = true` (opzione `AudioEngineConfig::diagnosticNoAutoStreamRouting`, spenta per default: il runtime ordinario conserva il rerouting); in ogni caso `sampleDeviceClock` confronta il client corrente con quello da cui ha preso il clock e, se è cambiato, non lo legge più e lo segnala (`client_changed`), e il blocco `device_clock` del manifest diventa `INCONCLUSIVE`. La corsa archiviata non è da ripetere: nulla indica un cambio di dispositivo.
- **OSS-CP1-CLOCK-01 (LOW), cadenza:** il manifest registra ora `max_read_interval_ms` e `coverage` (letture / letture attese a 20 Hz), così K1 diventa diagnosticabile.
- **OSS-CP1-CLOCK-02 (LOW), portata del controllo positivo:** `glitchcal` valida il principio di rilevazione sull'endpoint USB, non la sensibilità esatta del monitor del probe a circa 16 Hz; loopback e ascolto restano indispensabili.
- **OSS-CP1-CLOCK-03 (LOW), provenienza del binario:** il manifest riporta `build_id = 3099920` perché il binario era compilato dal working tree con il monitor non ancora committato; per le qualifiche future il binario va compilato da un commit.

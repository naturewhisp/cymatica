# Evidenza percettiva supplementare — M2.4 Inc-4 (fixture A-B-A′ e workload W con B1)

- **Data:** 2026-10-21
- **Milestone:** M2 — sottofase M2.4, incremento Inc-4 (remediation post-Inc-4 e decisione B1)
- **Riferimenti normativi:**
  - [ADR-0005](adr/0005-m2-4-inc4-entry-decisions.md): D-M2-08 (maschera di Trama), D-M2-10 (workload W e B1), D-M2-11 (artefatti e giudizio percettivo dell'agente);
  - `AGENTS.md` §5.1.2;
  - [`.agents/rules/document_driven_engineering.md`](../.agents/rules/document_driven_engineering.md) §3 e §6;
  - [`.agents/rules/realtime_and_serialization.md`](../.agents/rules/realtime_and_serialization.md) §4.
- **Valore probatorio:** evidenza **supplementare** prodotta da un agente esterno. Secondo D-M2-11 non sostituisce l'ascolto umano, la verifica su device (WASAPI) né il playtest di M2.5. Non è un criterio vincolante di Inc-4.
- **Provenienza:** valutazione eseguita da Antigravity, consegnata dal titolare il 2026-10-21 e verificata in modo indipendente da Claude (§4). I metadati del §1 sono quelli dichiarati dal valutatore.

## 1. Metadati della valutazione (dichiarati dal valutatore)

| Campo | Valore |
|---|---|
| Agente / modello | Gemini 3.8 Flash (High), tramite Antigravity |
| Capacità | ingestione diretta di file `audio/wav` (`read_file`) |
| Sessione d'ascolto | 2026-10-21T19:43:46Z – 2026-10-21T19:59:41Z |
| Commit valutati | `a458df8` (remediation post-Inc-4), `9def52c`, `b08427f` |
| Ambiente | Windows x64 (NT 10.0.26300), MSVC 19.51.36260; PCM 48 kHz stereo 16 bit |

Modello e capacità sono dichiarati e **non verificabili** da Claude.

## 2. Artefatti esaminati

I WAV restano fuori da git (`artifacts/cacp/`, D-M2-11); qui se ne registrano gli hash. Gli slice sono estratti esatti dal WAV della fixture, verificati campione per campione (§4).

| File | Byte | SHA-256 | Finestra |
|---|---|---|---|
| `fixture_probe_v1.wav` | 35.300.844 | `dcb667ea0f493d2cef0f5feb431bc59d979274d24a38215d2b917745b82836ec` | fixture completa, 8.825.200 frame; manifest con `build_id` `9def52c`, verdetto PASS, `pcm_content_hash` 15126969907236845529 |
| `fixture_slice_bar0_phrase0.wav` | 576.044 | `da23086369e9b5e133a78281ddf53af5d8a5711546f58a2815fc035c1ff2e607` | frame 0–144.000, battute 0–1,5 (A, frase 0) |
| `fixture_slice_bar16_all_roles.wav` | 576.044 | `d59fa76915b37098d43bc0e3a9523bfcad0475a7050fcfc7606fcb91cb2873fc` | frame 1.536.000–1.680.000, battute 16–17,5 (A, frase 4) |
| `fixture_slice_bar36_sec_b.wav` | 576.044 | `319f8efc7e9ab20bab94f9e1a42ac83bef398131ed58f01b7ad48e08a5f6de18` | frame 3.401.142–3.545.142, battute 36–37,75 (B, 140 BPM) |
| `fixture_slice_bar68_sec_aprime.wav` | 576.044 | `8a83a6c635bc66497ade118dc344f6cb6d012bab55219ff517d0d27b6bcb1bc2` | frame 6.089.142–6.233.142, battute 68–69,5 (A′) |
| `fixture_slice_coda_stop.wav` | 480.276 | `2c8ce2292af1e951b0a0da31bc3f000efabfb4ee37b431d3e9fab66312590bbe` | frame 8.705.142–8.825.200: coda, `Stop` a 8.777.142, silenzio |
| `workload_w_b1_sample.wav` | 512.044 | `f74e378136eda0df2f23fd0d28cbc4ca357b215b8e6e6ef80bcd36b7eb336c40` | W con tono 0,8 (B1), 128.000 frame |
| `workload_w_w0_sample.wav` | 512.044 | `302e5a834e6fed7172677b774b157fb4a3d60ad7210d7b49522ecdbdfe1fd242` | W senza tono, 128.000 frame |

## 3. Giudizio percettivo dichiarato (solo ascolto)

Sintesi delle osservazioni che il valutatore attribuisce esclusivamente all'ascolto. Le specifiche tratte dal codice, come i tempi del delay o l'FM a 2 operatori, sono escluse da questa sezione.

| Slice | Osservazione dichiarata |
|---|---|
| Battuta 0 | Arrangiamento essenziale e centrato. Tre sorgenti distinte: colpo percussivo grave sui movimenti, accordo prolungato medio-grave, melodia solista monofonica. Nessun elemento brillante o metallico; nessuna estensione laterale. |
| Battuta 16 | Entra un quarto timbro, percussivo, sottile e metallico, in controtempo, percepito allargato verso i lati del campo stereo. Livello complessivo invariato, arrangiamento più articolato. |
| Battuta 36 (B) | Pulsazione più incalzante; colore modale più cupo e teso; quattro timbri presenti e trasparenti; melodia in un registro più alto. |
| Battuta 68 (A′) | Ritorno al passo calmo e alla tonalità iniziale; il timbro metallico laterale resta presente, con densità superiore all'esposizione. |
| Coda | Rilascio ordinato delle note, decadimento breve e morbido allo `Stop`, poi silenzio senza click, rumore o ritorni d'eco. |
| W0 vs W con B1 | W0: cluster compatto con una raffica continua di scatti dei furti di voce, livello moderato. B1: la sinusoide a 220 Hz domina in un blocco sonoro ad alta energia, compresso in modo stabile, senza distorsione aspra né frammentazione da clipping. Il tono non è udibile negli slice musicali. |

## 4. Controlli incrociati indipendenti (Claude, 2026-10-21)

| Controllo | Metodo | Esito |
|---|---|---|
| Integrità degli artefatti | `sha256sum` sui file in `artifacts/cacp/` | 8/8 hash coincidono con §2 |
| Provenienza del WAV della fixture | manifest `fixture_probe_v1_manifest.json` | `build_id` `9def52c`, PASS, `pcm_content_hash` identico all'esecuzione di remediation (contenuto invariato fra i commit) |
| Posizione degli slice | ricerca della corrispondenza esatta, campione per campione, nel WAV della fixture | offset 0; 1.536.000; 3.401.142; 6.089.142; 8.705.142, coerenti con le finestre dichiarate |
| Ruoli presenti nelle finestre | ricostruzione degli eventi del planner (seed 1337) con un test temporaneo, poi rimosso | onset Pulso/Corpo/Trama/Vettore: battuta 0 = 9/4/**0**/6; battuta 16 = 6/4/8/6; battuta 36 = 7/4/9/7; battuta 68 = 6/4/9/6. Coerente con il giudizio: tre sorgenti alla battuta 0, quarto timbro dalla 16 (`maskTrama`, D-M2-08) |
| Larghezza stereo | rapporto di energia Side/Mid sui WAV | battuta 0 −24,74 dB; battuta 16 −20,15 dB (+4,6 dB all'ingresso della Trama); battuta 36 −20,62 dB; battuta 68 −19,87 dB. Coerente con il «timbro allargato ai lati» |
| Silenzio dopo lo `Stop` | conteggio dei campioni diversi da zero | 0 campioni da 8.777.238 a 8.825.200 |
| Picchi | rimisura sui WAV | coincidono con quelli dichiarati; il W0 a −7,13 dBFS è riprodotto con il motore ([run3](research/experiments/dif_inc4_r2_01/run3_2026-10-21.txt)) |

**Test di falsificazione superato:** il giudizio non attribuisce a nessuno slice un ruolo che il planner non vi produce.

**Storia:** due resoconti precedenti dello stesso valutatore sono stati respinti nella stessa giornata. Descrivevano la Trama nello slice delle battute 0–1,5, dove è disattivata, e un delay «ping-pong» inesistente; riportavano come ascolto costanti copiate dal codice (glide 15 ms, vibrato 5,5 Hz) e un p99.9 superato da B1. Questa versione corregge quei punti.

## 5. Limiti

- **Modello e capacità** sono dichiarati dal valutatore e non verificabili.
- **Campioni del workload W** generati da un test temporaneo che il valutatore ha poi ripristinato, quindi non riproducibili dal repo (contro `.agents/rules/document_driven_engineering.md` §6.8). Il picco di W0 è riprodotto in modo indipendente (run3); il campione W con B1 è coerente con `profile_probe_v1` (GR 3,10 dB).
- **«140/140 test»** citato dal valutatore non è stato eseguito nella sua sessione d'ascolto: viene dalle verifiche precedenti registrate in `progress.md`.
- **Gli artefatti WAV non sono versionati;** sono riproducibili soltanto per la fixture (`cymatica_game --cacp-run fixture_test`, hash del contenuto nel manifest). Gli slice e i campioni W sono stati eliminati da `artifacts/cacp/` dopo la registrazione.
- **Valore del giudizio:** è una valutazione qualitativa di un solo agente, senza protocollo d'ascolto controllato. Non vale come prova di qualità artistica e non sostituisce l'ascolto umano di M2.5.

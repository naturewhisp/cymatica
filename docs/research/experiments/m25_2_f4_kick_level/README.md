# M2.5.2 F4a — `kick_level_ab_v1`

Confronto controllato del livello del kick. Regole e predizioni sono in [pre-registrazione](../../M2_5_2_F4_Kick_Level_Preregistration.md) (SHA-256 LF `fc8a15ba…`); i risultati in [M2_5_2_F4_Kick_Level_Results.md](../../M2_5_2_F4_Kick_Level_Results.md).

- `kick_level_ab_v1_2026-11-16.json`: manifest dello scenario. Contiene, per variante (0, +3, +6, +9, +12 dB sul guadagno di bus di Pulso):
  - loudness integrata del mix, del bus e di ogni stem, e per sezione del mix e di Pulso;
  - true peak, riduzione del limiter, hash del PCM e degli eventi applicati;
  - null test contro l'offset 0 ed esiti di P1–P7.

  Si rigenera con `cymatica_game --cacp-run kick_level_ab_v1` (Release, circa 18 s). Binario compilato dal working tree su `ca625ef` con lo scenario non ancora committato.
- `clips_sha256_2026-11-16.txt`: SHA-256 delle 30 clip in `artifacts/cacp/kick_level_ab_v1/`: 3 sezioni × 5 varianti, ciascuna grezza e allineata. Le clip non sono versionate (circa 84 MB) e si rigenerano con lo scenario.
- `clip_check.py` e `clip_check_2026-11-16.txt`: controllo delle clip in Python puro. Riporta durata, picco, RMS, quota d'energia sotto i 150 Hz (passa-basso a un polo) e campioni a fondo scala.
- `envelope_A_0dB_vs_9dB_aligned.png`: inviluppo dei primi 4 s dell'estratto di A, allineato. In alto offset 0, in basso +9 dB.
- **Sorgente:** [kick_level_ab.cpp](../../../../engine/cacp/kick_level_ab.cpp); override in [role_bus_processor.cpp](../../../../engine/audio/role_bus_processor.cpp) e `AudioEngine::setPulsoBusGainScale`.

## Pacchetto d'ascolto di CP-2 (2026-11-16)

- `make_cp2_package.py`: costruisce dalle clip dello scenario i due pacchetti per gli ascoltatori e la chiave di chi conduce, secondo il disegno di [CP2_S1_Listening_Hypotheses.md](../../CP2_S1_Listening_Hypotheses.md). Il seed del mescolamento viene dal sistema operativo e sta solo nella chiave. Lo script rifiuta di sovrascrivere pacchetti esistenti. Uscite, non versionate, in `artifacts/expert_listening/`:
  - `cymatica_ascolto_CP2_sessione1_2026-11-16.zip` (circa 101 MB): brano completo a +6 dB, classifiche, coppie, domande per il secondo ascoltatore;
  - `cymatica_ascolto_CP2_sessione2_2026-11-16.zip` (circa 100 MB): ABX;
  - `CHIAVE_CP2_2026-11-16_NON_INVIARE.txt`: mappatura, seed, risposte corrette degli ABX e SHA-256 degli zip. **Non va inviata agli ascoltatori.**
- Il brano completo a +6 dB viene da `kick_level_ab_v1`, che ora scrive anche `f4a_full_+06dB_raw.wav` (`kFullPieceOffsetDb`). Ricorsa dopo la modifica: clip e misure identiche alla corsa archiviata.
- **Due correzioni del disegno prima dell'invio**, entrambe prima di ogni ascolto e senza legame con un esito:
  1. il duplicato nascosto di B era finito accanto al suo originale; ora la regola lo vieta;
  2. un `SHA256SUMS.txt` dentro i pacchetti avrebbe rivelato i file identici, cioè duplicato, coppia identica e risposte ABX; è stato tolto, e l'integrità si verifica con l'hash dello zip nella chiave.

  I pacchetti delle prime due generazioni sono stati cancellati senza essere inviati. Il seed valido è solo quello della chiave attuale.
- **Pacchetto archiviato senza invio (D-M2-30):** spostato in `artifacts/expert_listening/archivio_CP2_F4a_non_inviato/`. Le ipotesi della sessione ([CP2_S1_Listening_Hypotheses.md](../../CP2_S1_Listening_Hypotheses.md)) restano registrate e non verificate.

## Adozione a +8 dB (2026-11-16)

Dopo D-M2-31 l'offset 0 dello scenario corrisponde alla nuova produzione (Pulso +2 dB) e il golden di P1 è quello della fixture. La corsa archiviata del 2026-11-16 resta l'evidenza sulla base −6 dB. Esiti dell'adozione: [risultati](../../M2_5_2_F4_Kick_Level_Results.md) §5.

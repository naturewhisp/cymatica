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

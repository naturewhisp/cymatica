# Pacchetto di misura 2b — Registro delle dissolvenze (`fade_events_v1`)

Pre-registrazione: [M2_5_Pkg2b_Fade_Event_Log_Preregistration.md](../../M2_5_Pkg2b_Fade_Event_Log_Preregistration.md). Risultati: [M2_5_Pkg2b_Fade_Event_Log_Results.md](../../M2_5_Pkg2b_Fade_Event_Log_Results.md).

- `fade_events_v1_2026-11-17.json`: manifest con la tabella per ruolo e causa, E4–E6. Si rigenera con `cymatica_game --cacp-run fade_events` (Release, pochi secondi).
- **Sorgente:** [fade_event_log.h](../../../../engine/audio/fade_event_log.h) (registro), `VoiceAllocator::logFade` in [voice_allocator.cpp](../../../../engine/audio/voice_allocator.cpp), [fade_events.cpp](../../../../engine/cacp/fade_events.cpp) (scenario).

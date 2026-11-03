# Esperimento Inc-5 fase 5e.3 — `release_ab_v1`

Confronto controllato del release dei NoteOff ordinari (ADR-0006 5e-08, regole fissate in [release_ab.h](../../../../engine/cacp/release_ab.h) prima della misura).

- `release_ab_v1_2026-11-03.json`: manifest completo dello scenario. Contiene:
  - le misure per insieme (set A, set B, `planner_probe_v1`, fixture) e variante (2, 15, 30, 50, 80 ms);
  - per ruolo: riferimenti, rilevazioni, abbinamenti, rilevazioni sui rilasci, precisione e richiamo a ±50 ms;
  - i confondenti di voce (voice drop, release tagliati, sovrapposizioni);
  - l'hash degli eventi applicati;
  - gli esiti di H1, H2 ed R2;
  - l'elenco delle clip.

  Si rigenera con `cymatica_game --cacp-run release_ab_v1` (Release, circa 90 s). Il `build_id` `f649786` è quello compilato: al momento della misura il working tree conteneva la fase 5e.3, non committata.
- `clips_sha256_2026-11-03.txt`: SHA-256 delle 10 clip A/B in `artifacts/cacp/release_ab_v1/`. Sono le frasi 5 e 10 della fixture per ogni variante, prese dal PCM finale con dissolvenze di 10 ms. Le clip non sono versionate (circa 14 MB) e si rigenerano con lo scenario.
- **Sorgente:** [release_ab.cpp](../../../../engine/cacp/release_ab.cpp), [voice_allocator.cpp](../../../../engine/audio/voice_allocator.cpp) (override del release e contatori dei confondenti).

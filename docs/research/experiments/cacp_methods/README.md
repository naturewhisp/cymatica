# Esperimento CACP — metodi di valutazione (2026-10-21)

Dati del dossier [M2_4_CACP_Evaluation_Methods.md](../../M2_4_CACP_Evaluation_Methods.md) §3.2.

- `cacp_methods_experiment.py`: analisi con solo numpy e libreria standard. Calcola gli onset (spectral flux) confrontati con gli intent, gli spettrogrammi mel in PNG e le finestre per il test alla cieca. Uso:
  ```
  python cacp_methods_experiment.py <fixture_probe_v1.wav> fixture_intents_seed1337.csv <out_dir>
  ```
  Variabili opzionali per il turno 2: `CACP_BLIND_BARS=4,12,20,28,52,84`, `CACP_BLIND_SEED=777`, `CACP_BLIND_LABELS=GHJKLM`, `CACP_BLIND_KEY=blind_key_round2.json`.
- `fixture_intents_seed1337.csv`: NoteOn pianificati della fixture-m2-4-v1 (planner offline, seed 1337, `candidateCount` 8, orizzonte 0, epoch = sezione + 1). Le righe `#noteoff` sono le NoteOff dei pad. Esportato con un test Catch2 temporaneo, poi rimosso. Equivale a `planFixtureForm` in `tests/unit/test_music_planner.cpp`, il cui hash canonico è un golden.
- `onset_report_2026-10-21.json`: risultati E1 (δ = 0,05 / 0,10 / 0,20).
- `blind_key_round1.json`, `blind_key_round2.json`, `blind_judgements_2026-10-21.md`: test E2 alla cieca.
- `png_sha256_2026-10-21.txt`: hash dei PNG generati. I PNG non sono versionati e si rigenerano con lo script.
- WAV d'ingresso: `fixture_probe_v1.wav` da `cymatica_game --cacp-run fixture_test` (build `9def52c`, `pcm_content_hash` 15126969907236845529, SHA-256 `dcb667ea…36ec`).
- `onset_parity_v1_2026-10-27.json`: parità del descrittore C++ `descriptors_v1` (Inc-5, fase 5b) con E1 sullo stesso ingresso (fixture renderizzata in processo, quantizzata come il WAV a 16 bit, solo frame di contenuto). Rilevati, P, R, scarti e richiami per ruolo coincidono con `onset_report_2026-10-21.json` alle tre δ. Si rigenera con `cymatica_game --cacp-run onset_parity_v1`.

# Esperimento Inc-5 fase 5e.4a — `full_m2_suite` ed `evaluation_check_selftest_v1`

Regole ed esiti attesi in [ADR-0006](../../../adr/0006-m2-4-inc5-entry-decisions.md) («5e.4a»), registrati prima dell'implementazione.

- `full_m2_suite_manifest_2026-11-03.json`: manifest aggregato della suite, eseguita con `cymatica_game --cacp-run full_m2_suite --pack-seed 1618033988` (Release, hardware del titolare, `build_id` `31b6c43` più il working tree di 5e.4a).
  - **Esito:** **PASS**, uscita 0, circa 201 s.
  - **Vincolanti, tutti PASS:** `tone_test`, `voice_test`, `timbre_test`, `planner_test`, `fixture_test` con golden PCM PASS, `profile_test`, `evaluation_check_selftest_v1`, `procedural_v1`.
  - **Misure, tutte valide:** `onset_parity_v1`, `acao02_v1`, `release_ab_v1`, `trama_miss_diagnosis_v1`.
  - **Pacchetti, tutti `GENERATED` con ripetizione identica:** v1, v2 (14 gruppi), v3 (8 gruppi).
  - **Esiti esplorativi**, riportati e mai vincolanti: H1 `NOT_SUPPORTED`, H2 `SUPPORTED`, R2 short-IOI `NOT_SUPPORTED`, M-NORM `SUPPORTED`.
- `full_m2_suite_manifest_2026-11-03_dif5e4a.json`: la suite dopo la micro-remediation DIF-5e.4a-01/02 e OSS-5e.4a-01; il file precedente resta invariato.
  - **Esito:** **PASS** in Release, 202 s, verdetti identici.
  - **Nuove regole:** la validità di `onset_parity_v1` e `acao02_v1` richiede il manifest scritto; `profile_test` vale solo con i tempi vincolati, quindi in Debug la suite fallisce.
  - **Note esplorative aggiunte:** generalizzazione multi-seed `INCONCLUSIVE` (1 contenuto distinto), specificità `INSUFFICIENT_DATA` attesa su v3 (n_eff 3,769 < 4), diversità `NULL_DIVERSITY`.
- `evaluation_check_selftest_v1_manifest_2026-11-03.json`: **PASS** su 14 casi.
  - S1–S3 su v1, v2 e v3; S6 su v1; S4 ed S5 su v2 e v3.
  - Stati di presenza osservati: `MEASURED` (v2), `INSUFFICIENT_DATA` (v3), `DESCRIPTIVE_ONLY` ed `EXCLUDED_BY_CATCH_RULE`.
  - Statistiche identiche entro 10⁻¹² a un ricalcolo indipendente.
- **Seed:** quello usato è di verifica e annotato, quindi i pacchetti non valgono per una sessione alla cieca, rinviata comunque a M2.5.
- **Sorgente:** [m2_suite.cpp](../../../../engine/cacp/m2_suite.cpp), [check_selftest.cpp](../../../../engine/cacp/check_selftest.cpp).

# Pacchetto di misura 5 — Descrittori timbrici (`timbre_descriptors_v1`)

Pre-registrazione: [M2_5_Pkg5_Timbre_Descriptors_Preregistration.md](../../M2_5_Pkg5_Timbre_Descriptors_Preregistration.md). Risultati: [M2_5_Pkg5_Timbre_Descriptors_Results.md](../../M2_5_Pkg5_Timbre_Descriptors_Results.md).

- `timbre_descriptors_v1_2026-11-17.json`: descrittori per nota, controlli D1–D3, D5, separabilità e distanze. Si rigenera con `cymatica_game --cacp-run timbre_descriptors`, che scrive anche i WAV delle note in `artifacts/cacp/timbre_descriptors_v1/`.
- `oracle_numpy_centroid.py` e `oracle_numpy_centroid.output.txt`: prova D4, con l'ambiente fissato di `tools/pyenv`.

# Ricerca R5 — calcoli della statistica delle affermazioni raggruppate

Dati del dossier [M2_4_Inc5_Clustered_Claim_Statistics.md](../../M2_4_Inc5_Clustered_Claim_Statistics.md) (2026-11-02).

- `stats.py`, `stats2.py`: calcoli analitici ed esatti per enumerazione, senza simulazione Monte Carlo. Python puro senza dipendenze; si eseguono con `python -I stats.py` e `python -I stats2.py`. Coprono:
  - Wilson e Clopper–Pearson;
  - `n_eff`;
  - regola di baseline e MOVER;
  - dimensionamento;
  - concordanza dei duplicati.
- `stats_out.txt`: output dei due script, in quest'ordine. Rieseguito il 2026-11-02 dalla sessione principale: l'output coincide al byte.

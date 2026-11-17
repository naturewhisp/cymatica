# M2.5.2 — F4c, decadimento del kick

Pre-registrazione: [M2_5_2_F4c_Kick_Decay_Preregistration.md](../../M2_5_2_F4c_Kick_Decay_Preregistration.md). Risultati: [M2_5_2_F4c_Kick_Decay_Results.md](../../M2_5_2_F4c_Kick_Decay_Results.md).

- `kick_decay_ab_v1_2026-11-17.json`: manifest della scala k = 1,00…2,00, con limiter, loudness del mix e di Pulso, furti di voce di Pulso, identità di eventi, Trama e Vettore. Si rigenera con `cymatica_game --cacp-run kick_decay_ab` sulla produzione precedente: dopo l'adozione k = 1 corrisponde al vecchio k = 1,5. I WAV stanno in `artifacts/cacp/kick_decay_ab_v1/` e non sono versionati.
- `kick_decay_criterion.py`, `kick_decay_criterion_2026-11-17.json` e `kick_decay_criterion.output.txt`: criterio N, C, S con MoSQITo, nell'ambiente fissato `tools/pyenv`.

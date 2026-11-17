# Pacchetto di misura 4 — Loudness specifica e mascheramento

Pre-registrazione: [M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md](../../M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md). Risultati: [M2_5_Pkg4_Specific_Loudness_Masking_Results.md](../../M2_5_Pkg4_Specific_Loudness_Masking_Results.md).

- `stem_export_v1_2026-11-17.json`: manifest dell'esportazione: 27 WAV float a 32 bit in `artifacts/cacp/stem_export_v1/`, non versionati. Si rigenera con `cymatica_game --cacp-run stem_export`.
- `loudness_masking.py`: prove L1–L5 e misure, con MoSQITo 1.2.1 nell'ambiente fissato `tools/pyenv`. La corsa dura circa 14 minuti, perché calcola due volte la loudness tempo-variante. Uscite: `loudness_masking_2026-11-17.json` (completo) e `loudness_masking.output.txt` (riassunto).
- **Nota:** la corsa archiviata ha scritto il JSON completo e poi si è interrotta alla stampa, su un valore indefinito (la Trama tace nell'estratto di A). Dopo la corsa lo script è stato corretto solo nella stampa e nella pendenza di uno stem muto, che ora è indefinita invece di dare un avviso. Il riassunto è generato dal JSON archiviato.

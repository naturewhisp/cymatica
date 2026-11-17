# M2.5.2 — F2, immagine stereo della Trama (`trama_stereo_v1`)

Pre-registrazione: [M2_5_2_F2_Trama_Stereo_Preregistration.md](../../M2_5_2_F2_Trama_Stereo_Preregistration.md). Risultati: [M2_5_2_F2_Trama_Stereo_Results.md](../../M2_5_2_F2_Trama_Stereo_Results.md).

- `trama_stereo_v1_before_2026-11-16.json`: manifest prima del cambio (pan L 1,0 / R 0,5), build dal working tree su `9f4e982`.
- `trama_stereo_v1_after_2026-11-16.json`: manifest dopo il cambio (centro a potenza costante), con il golden PCM ancora da aggiornare (nota nel campo `reasons`).
- Si rigenera con `cymatica_game --cacp-run trama_stereo_v1` (Release, pochi secondi). Sorgente: [trama_stereo.cpp](../../../../engine/cacp/trama_stereo.cpp).

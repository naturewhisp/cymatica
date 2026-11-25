# H-03 — `release_null_v1`

- Pre-registrazione: [M2_5_H03_Release_Null_Test_Preregistration.md](../../M2_5_H03_Release_Null_Test_Preregistration.md). Risultati: [M2_5_H03_Release_Null_Test_Results.md](../../M2_5_H03_Release_Null_Test_Results.md).
- `release_null_v1_2026-11-25.json`: manifest, build dal working tree su `dd42dd9`. Si rigenera con `cymatica_game --cacp-run release_null` (Release, circa 75 s). Sorgente: [release_null.cpp](../../../../engine/cacp/release_null.cpp).
- La lista `flagged_list` è stata aggiunta dopo la prima corsa, solo come diagnostica. Tolta quella, la seconda corsa è identica alla prima.
- Manifest rigenerato il 2026-11-25 dopo `DIF-H03-01`: i 10 `median_db` della parte B erano lo 0,5° percentile invece della mediana. Ogni altro valore è identico.

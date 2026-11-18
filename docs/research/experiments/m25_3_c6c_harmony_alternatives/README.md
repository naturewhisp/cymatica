# M2.5.3–4 — Pacchetto 6c: alternative armoniche di B

Pre-registrazione: [C6c](../../M2_5_3_C6c_Harmony_Alternatives_Preregistration.md). Risultati: [M2_5_3_C6c_Harmony_Alternatives_Results.md](../../M2_5_3_C6c_Harmony_Alternatives_Results.md). Riferimento: il piano di C6b (commit `791f29c`).

- `harmony_alternatives_v1.json`: scenario `cymatica_game --cacp-run harmony_alternatives_v1`. Sono 64 seed; per ognuno il cammino estratto viene pianificato tre volte, con `bHarmony` = 0, 1, 2. Contiene le misure H-FR, H-IC, H-CT, H-AD, H-RP, H-GR e H-LT della sezione B, più le estrazioni di `bHarmony` sui seed.
- `procedural_diversity_c6c.json`: blocco di diversità dello scenario `procedural --pack-seed 1618033988`, cioè la scansione dei seed da 1 a 64 sulla tabella con la nuova dimensione. Contiene esito, brani e cammini distinti, quote di frasi identiche per ruolo e cammino di ogni seed (`b_harmony`).

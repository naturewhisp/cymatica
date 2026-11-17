# M2.5.3–4 — Pacchetto 6b: melodia lunga una frase

Pre-registrazione: [C6b](../../M2_5_3_C6b_Phrase_Melody_Preregistration.md). Risultati: [M2_5_3_C6b_Phrase_Melody_Results.md](../../M2_5_3_C6b_Phrase_Melody_Results.md). Riferimento: il piano di C6 (uguale a quello di [C5b](../m25_3_c5b_chain_ops/README.md) per la fixture di riferimento, commit `5570cac`).

- `fixture_intents_c6b.csv`: piano con la progressione melodica, da `cymatica_game --cacp-run plan_export`.
- `bar_melody.py <csv>`: successioni di altezze di Vettore per battuta. Conta le frasi con almeno 3 battute diverse e stampa la melodia della prima frase di ogni sezione.
- `bar_melody_c6b.output.txt`, `contradicted_bars_c6b`, `chord_reading_c6b`, `dominant_register_c6b`: uscite sul piano C6b.
- `lineage_measures_c6b.json`: scenario `lineage_measures_v1` sul piano C6b.

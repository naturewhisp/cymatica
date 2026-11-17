# M2.5.3–4 — Contratto minimo, punto 2: Vettore legge l'accordo

Pre-registrazione: [C2](../../M2_5_3_C2_Vettore_Reads_Chord_Preregistration.md). Risultati: [M2_5_3_C2_Vettore_Reads_Chord_Results.md](../../M2_5_3_C2_Vettore_Reads_Chord_Results.md). Riferimento: il piano di C1c ([`fixture_intents_c1c.csv`](../m25_3_c1_corpo_dyad/fixture_intents_c1c.csv), commit `b11b9ff`), identico al piano con l'ablazione `vettoreReadsChord = false`.

- `fixture_intents_c2.csv`: piano con il punto 2, da `cymatica_game --cacp-run plan_export`.
- `chord_reading.py <csv>`: note dell'accordo sui tempi 1 dei cambi d'accordo, e note di Vettore in classe d'intervallo 1 o 6 con una voce di Corpo.
- `chord_reading_*.output.txt`, `role_clashes_c2.output.txt`, `ctnctr_c2.output.txt`, `vettore_corpo_registers_c2.output.txt`: uscite degli script, sul piano C1c e sul piano C2.

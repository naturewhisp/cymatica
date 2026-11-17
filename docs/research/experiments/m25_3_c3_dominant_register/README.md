# M2.5.3–4 — Contratto minimo, punto 3: registro del ruolo dominante

Pre-registrazione: [C3](../../M2_5_3_C3_Dominant_Register_Preregistration.md). Risultati: [M2_5_3_C3_Dominant_Register_Results.md](../../M2_5_3_C3_Dominant_Register_Results.md). Riferimento: il piano del punto 2 ([`fixture_intents_c2.csv`](../m25_3_c2_vettore_chord/fixture_intents_c2.csv), commit `fc07a98`).

- `fixture_intents_c3.csv`: piano con il punto 3, da `cymatica_game --cacp-run plan_export`.
- `dominant_register.py <csv>`: Vettore rispetto alla voce alta di Corpo che suona all'attacco, pad tenuti dalla frase precedente inclusi; registro per sezione; note di Vettore entro 2 semitoni da una nota di Trama che suona insieme.
- `dominant_register_*.output.txt`: uscite sul piano C2 e sul piano C3.
- `interval_identity.output.txt`: le successioni di intervalli e di intervalli d'attacco di Vettore, per frase, sono le stesse in C2 e in C3. M5 e M5r ne dipendono soltanto.

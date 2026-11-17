# M2.5.3–4 — Contratto minimo, punto 5: variazione derivata e ripresa

Pre-registrazione: [C5](../../M2_5_3_C5_Derived_Variation_Preregistration.md). Risultati: [M2_5_3_C5_Derived_Variation_Results.md](../../M2_5_3_C5_Derived_Variation_Results.md). Riferimento: il piano del punto 4 ([`fixture_intents_c4.csv`](../m25_3_c4_role_masks/fixture_intents_c4.csv), commit `656cf23`).

## File

- **Piano:** `fixture_intents_c5.csv`, da `cymatica_game --cacp-run plan_export`.
- **Misure:**
  - `lineage_measures_c5.json`: il nuovo scenario `lineage_measures_v1` (`cymatica_game --cacp-run lineage_measures`). Calcola i criteri K3, K4 e K5 con l'implementazione C++ di M5r su Vettore e con i genitori dichiarati nei trace;
  - `vettore_groups.py <csv>`: gruppi di Vettore a successioni identiche di intervalli e di intervalli d'attacco, sulla griglia di sedicesimi di ogni sezione. È equivalente alla distanza M5r 0; serve a contare C4, che lo scenario non può più produrre. La prima versione misurava gli attacchi in frame e separava frasi uguali in B (a 140 BPM i frame oscillano di ±1): è stata corretta prima di registrare i numeri, e le due misure ora coincidono su C5;
  - `vettore_groups_*.output.txt`: le uscite sul piano C4 e sul piano C5.
- **Decisioni della catena:** `chain_decisions_c5.output.txt`, cioè per frase origine, ripartenza, vincitore, trasformazione, margine e motivo vincente in gradi. Prodotto con un test temporaneo, poi rimosso.
- **Contratto precedente:** `contradicted_bars_c5`, `chord_reading_c5` e `dominant_register_c5`, le misure dei punti 1–3 sul piano C5.

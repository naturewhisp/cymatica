# Esperimento Inc-5 fase 5e.3b — diagnostica dei 10 attacchi mancati della Trama

Previsioni e regole sono fissate in [ADR-0006](../../../adr/0006-m2-4-inc5-entry-decisions.md) («5e.3b») prima della misura. Sorgente: [trama_diagnosis.cpp](../../../../engine/cacp/trama_diagnosis.cpp).

- `trama_miss_diagnosis_v1_2026-11-03.json`: manifest dello scenario `trama_miss_diagnosis_v1` con i due rendering della Trama nel set sintetico B (2 ms e 30 ms). Contiene:
  - per rendering: normalizzatore (massimo del flusso grezzo), posizione del massimo, richiamo passo per passo e richiamo di `evaluateRoleOnsets`, ricalcolo controfattuale;
  - per riferimento: velocity, timbro, intervallo precedente, distanza dal NoteOff precedente, picco normalizzato e grezzo, soglia e margine al picco, massimo locale, candidati fusi, esito controfattuale;
  - gli esiti di P1–P4 e l'esito causale.

  Si rigenera con `cymatica_game --cacp-run trama_miss_diagnosis_v1` (Release, circa 1 s). Il `build_id` `c4857b2` è quello compilato, più il working tree di 5e.3b.
- `trama_miss_diagnosis_v1_curves_2026-11-03.csv`: curve per frame (3.119 frame, hop 256): novelty e soglia a 2 ms, novelty controfattuale a 2 ms, novelty e soglia a 30 ms.

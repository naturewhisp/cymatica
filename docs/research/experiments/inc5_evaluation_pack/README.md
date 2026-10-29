# Esperimento Inc-5 fase 5d — evaluation_pack_v1

Evidenze della fase 5d ([ADR-0006](../../../adr/0006-m2-4-inc5-entry-decisions.md) D-M2-15, precisazioni 5d-01..16). Generate con `cymatica_game --cacp-run evaluation_pack_v1 --pack-seed 2718281828` (Release, `build_id` `3f026c2` più il working tree della fase 5d, dopo le correzioni AG-5d). La presenza per frase è identica a quella della prima esecuzione (seed 8675309): non dipende dal seed di selezione.

- `evaluation_pack_v1_manifest_2026-10-29.json`: manifest dello scenario, cioè conteggi, impegno del seed, cartelle e ripetizione identica. Non contiene nulla sulle singole finestre.
- `presence_calibration_2026-10-29.json`: per ogni frase della fixture e ogni ruolo, sull'estratto alla cieca (6,0 s dall'inizio della frase, fade di 10 ms):
  - presenza pianificata, livello, picco, RMS e rapporto stem/mix;
  - è l'evidenza della precisazione 5d-03;
  - non contiene ID delle finestre né il seed di selezione.
- `baseline_always_present_check_2026-10-29.json`: riepilogo aggregato del checker per la baseline banale «sempre presente».
- `antigravity_check_2026-10-29.json`: riepilogo aggregato del checker sul risultato di Antigravity (secondo parere, prima esecuzione, seed 8675309). Non è alla cieca: accessi al repository e al codice dichiarati, seed pubblico. Il checker è quello precedente alle correzioni AG-5d.
- **Esclusi:**
  - i file pubblici (circa 64 MB);
  - la verità sigillata;
  - l'elenco per affermazione del checker, che rivelerebbe la chiave del pacchetto con questo seed.

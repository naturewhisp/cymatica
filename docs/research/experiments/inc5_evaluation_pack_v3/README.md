# Esperimento Inc-5 fase 5e.3c — `evaluation_pack_v3`

Gruppi musicali con il fingerprint simbolico per frase. Decisione R-01 del titolare; regola e previsione in [ADR-0006](../../../adr/0006-m2-4-inc5-entry-decisions.md) («5e.3c»), registrate prima della misura.

- `evaluation_pack_v3_manifest_2026-11-03.json`: manifest dello scenario. Si rigenera con `cymatica_game --cacp-run evaluation_pack_v3 --pack-seed 1618033988` (Release); `build_id` `df0bcbe` più il working tree di 5e.3c.
  - **Esito:** `GENERATED`, ripetizione identica, regola `per_phrase_symbolic_fingerprint`.
  - **Composizione:** 8 gruppi musicali, 3 gruppi di riferimento, 12 finestre alla cieca (5 di test, 5 negative per rimozione, duplicato, catch), 5 diagnostiche.
  - **`n_eff` atteso:** 4,93 per i positivi, **3,77 per i negativi**. Con v2 e lo stesso seed erano 10,92 e 8,89.
- **Seed:** quello usato qui è di verifica e appare in chiaro nella documentazione, quindi questo pacchetto **non** è valido per una sessione alla cieca. Per la sessione si genera v3 con un seed nuovo e non annotato.
- **Sorgente:** [diversity.cpp](../../../../engine/cacp/diversity.cpp) (`symbolicPhraseFingerprint`), [evaluation_pack.cpp](../../../../engine/cacp/evaluation_pack.cpp).

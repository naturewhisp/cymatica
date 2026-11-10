# Verifiche della revisione d'insieme (2026-11-09)

Script e output della [revisione d'insieme](../../CYMATICA_Analysis_Consolidation_2026-11-09.md), eseguiti su `72a1c8e`. Strumenti offline, non parte del motore e non gate.

- **Dipendenze:** Python 3.14; numpy 2.4 solo per `float32_gain_residual.py`. `doicheck*.py` usano la rete (Crossref e arXiv).
- **Comandi**, dalla radice del repository:
  - `python docs/research/experiments/consolidation_2026-11-09/linkcheck.py .`
  - `python docs/research/experiments/consolidation_2026-11-09/doicheck.py . <file di output>`
  - `python docs/research/experiments/consolidation_2026-11-09/doicheck_parens.py`
  - `python docs/research/experiments/consolidation_2026-11-09/float32_gain_residual.py`

| File | Cosa fa | Output |
|---|---|---|
| `linkcheck.py` | Controlla i link relativi e le ancore dei documenti Markdown versionati | `linkcheck.output.txt` |
| `doicheck.py` | Risolve ogni DOI su Crossref e ogni identificativo arXiv, e li affianca alla riga che li cita | `doicheck.output.txt` |
| `doicheck_parens.py` | Secondo passaggio per i DOI con parentesi | `doicheck_parens.output.txt` |
| `float32_gain_residual.py` | Residuo di una moltiplicazione in float32 per un guadagno costante, su segnale sintetico | `float32_gain_residual.output.txt` |

**Limiti.**
- `linkcheck.py` segnala come rotti i 311 link assoluti `file:///`: sono link locali di due documenti di Antigravity, non link relativi rotti.
- `doicheck.py` tronca i DOI con parentesi alla prima «)»: quei sette sono verificati da `doicheck_parens.py`. I DOI `10.48550` (arXiv su DataCite) e Zenodo non sono su Crossref e risultano non risolti: non sono verificati.
- Il controllo conferma esistenza, autori e titolo. Non conferma i valori citati dai dossier.

**Script di misura rieseguiti** e trovati identici agli output archiviati: in `m25_ensemble_baseline`, `lhl_syncopation`, `n2_power`, `fm_sethares`, `ctnctr_check`, `clash_bars`, `vertical_check`, `voice_use`, `plan_steps`, `role_clashes`, `vettore_corpo_registers`; in `inc5_claim_stats`, `stats.py` seguito da `stats2.py`. Controlli negativi di `m25_device_baseline`: 15/15, 20/20 e 15/15.

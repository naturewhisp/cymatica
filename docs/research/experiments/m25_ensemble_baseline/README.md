# Misure d'insieme sulla fixture (2026-11-03)

Script e output delle misure citate nell'[ascolto del 2026-11-03](../../../m2_4_human_listening_2026-11-03.md) e nella [proposta per l'ingresso di M2.5](../../M2_5_Entry_Music_Quality_Proposal.md) §8. Sono strumenti temporanei di analisi offline, non parte del motore e non gate.

- **Materiale:** intent del seed 1337 ([fixture_intents_seed1337.csv](../cacp_methods/fixture_intents_seed1337.csv)), valido perché il PCM ha lo stesso hash del golden (15126969907236845529); manifest di `procedural_v1` con build `fa8ac3c`; pacchetto d'ascolto in `artifacts/expert_listening/`, non versionato.
- **Dipendenze:** Python con numpy, solo per `expert_analysis.py`. Nessuna dipendenza del progetto.
- **Comando:** dalla radice del repository, `python docs/research/experiments/m25_ensemble_baseline/<script>.py .`

| Script | Cosa misura | Output |
|---|---|---|
| `expert_analysis.py` | Integrità del pacchetto, differenza fra le clip del rilascio, pattern ai confini di sezione | `expert_analysis.output.txt` |
| `role_levels.py` | `stem_to_mix_db` per ruolo e porzione, dal manifest | `role_levels.output.txt` |
| `plan_steps.py` | Passi da sedicesimo per ruolo, sezione e frase | `plan_steps.output.txt` |
| `role_clashes.py` | Classi d'intervallo fra ruoli pesate sul tempo di sovrapposizione, registri, note di Vettore in comune con Corpo | `role_clashes.output.txt` |
| `clash_bars.py` | Note di Corpo e Vettore battuta per battuta in una frase per sezione | `clash_bars.output.txt` |
| `vertical_check.py` | Note d'accordo di Vettore per tempi e accordi, tempi 1 con cambio d'accordo (dossier R-M25-7) | `vertical_check.output.txt` |
| `ctnctr_check.py` | CTnCTR e PCS di Vettore per sezione e tempi | `ctnctr_check.output.txt` |
| `lhl_syncopation.py` | Sincope LHL ed E0 dei pattern | `lhl_syncopation.output.txt` |
| `fm_sethares.py` | Dissonanza di interferenza dei rapporti FM, modello di Sethares | `fm_sethares.output.txt` |
| `vettore_corpo_registers.py` | Vettore rispetto alla voce alta di Corpo. Le righe della sezione B su tempi 1 e 3 sono un artefatto in virgola mobile: fanno fede `vertical_check.py` e `ctnctr_check.py` | `vettore_corpo_registers.output.txt` |
| `n2_power.py` | Potenza binomiale con due ascoltatori | `n2_power.output.txt` |
| `voice_use.py` | Voci realmente usate per ruolo, durata delle note, note più gravi | `voice_use.output.txt` |

Gli script che leggono il CSV degli intent ricevono il percorso come argomento: `python docs/research/experiments/m25_ensemble_baseline/vertical_check.py docs/research/experiments/cacp_methods/fixture_intents_seed1337.csv`.

**Limiti:**

- gli intervalli sono classi d'altezza: non considerano l'ottava, la dinamica o il timbro;
- le note che Corpo non suona (ha due voci) non contano come note d'accordo, quindi la quota di Vettore in comune con Corpo sottostima le note d'accordo;
- i pad aperti sono estesi fino al successivo attacco di Corpo, un'approssimazione;
- i confronti con classe d'intervallo sono misure sul piano, non su ciò che si sente. Nessuna di queste dissonanze è stata segnalata dall'esperto.

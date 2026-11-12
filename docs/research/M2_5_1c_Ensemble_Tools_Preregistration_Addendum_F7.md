# M2.5.1c — Strumento d'insieme: aggiunta alla pre-registrazione per F7

- **Stato:** aggiunta scritta il 2026-11-12, **prima** di ricevere il controllo positivo. Non modifica la [pre-registrazione](M2_5_1c_Ensemble_Tools_Preregistration.md), che resta invariata (hash `30ac2cb5…`). L'hash di questo file è registrato in `docs/progress.md` prima della ricezione.
- **Contesto:** la prima esecuzione ha già mostrato due limiti della M4 (categorica sugli intervalli; cieca solo alla trasposizione cromatica). Questa aggiunta li conosce, e lo dichiara.

## Fonte di F7

Una melodia di circa 24 frasi da 4 battute, scritta da una persona secondo le [istruzioni](experiments/m25_ensemble_tools/F7_istruzioni_per_l_esperto.md), consegnata come MIDI a una traccia o come tabella di testo. La conversione nel JSON dello strumento è meccanica (inizio e durata in sedicesimi della frase, nota MIDI) e non modifica le note; lo script di conversione e i suoi controlli vanno versionati insieme al file. Le note di intento dell'autore restano chiuse fino alla misura.

Non si usa un subagente né una trascrizione da audio come F7: il primo condivide il modello di chi conosce la misura, la seconda introduce errori di trascrizione proprio sulla grandezza misurata.

## Predizioni

| Grandezza | Predizione | Motivo |
|---|---|---|
| Classificazione a lag 1 | `ORDERED` | In uno sviluppo scritto da una persona le frasi vicine derivano l'una dall'altra |
| ρ di Spearman, k = 1..P/2 | nessuna predizione di segno | Un brano reale ha ritorni e contrasti: V(k) può scendere ai ritorni, quindi la monotonia di F3 non è attesa |

**Esiti e loro lettura**, fissati ora:
- `ORDERED`: lo strumento vede l'ordine di uno sviluppo umano. Rafforza la validazione delle fixture F1–F6.
- `ORDER_FREE` o `ANTI_ORDERED`: lo strumento non vede quello sviluppo. Prima di ogni uso diagnostico si cerca la causa con le note d'intento dell'autore, distinguendo i limiti già noti della M4 (sviluppo graduale delle altezze, trasposizione modale) da un difetto nuovo. Non si ritocca la melodia né la misura per far tornare il risultato.
- `DEGENERATE`: frasi tutte uguali per la M4; si verifica la conversione prima di ogni altra lettura.

Nessuna soglia, nessun uso nello scoring del planner.

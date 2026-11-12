# M2.5.1c — Strumento d'insieme: aggiunta alla pre-registrazione per F8 e F9

- **Stato:** aggiunta scritta il 2026-11-12, **prima** di acquisire F8 e F9. Non modifica la [pre-registrazione](M2_5_1c_Ensemble_Tools_Preregistration.md) né l'[aggiunta F7](M2_5_1c_Ensemble_Tools_Preregistration_Addendum_F7.md). L'hash di questo file è registrato in `docs/progress.md` prima dell'acquisizione.
- **Decisione del titolare (2026-11-12):** tre controlli distinti invece di uno: F7 melodia dell'esperto, F8 brano storico di pubblico dominio, F9 melodia scritta da un subagente alla cieca.

## F8 — brano storico, umano, di pubblico dominio

- **Scelta:** un movimento **monodico** di J. S. Bach per strumento solo, da una fonte libera (Mutopia Project o equivalente), preferendo un brano lungo almeno circa 24 frasi da 4 battute. Si preferisce la monodia a un pezzo per tastiera per non dover estrarre a mano una linea melodica.
- **Conversione:** dal MIDI della fonte, senza ritocchi: frasi di 4 battute secondo il metro dichiarato nel file; attacchi e durate quantizzati al sedicesimo; dove suonano più note insieme (bicordi) si tiene la più acuta. Le regole valgono per tutto il brano e sono applicate dallo script, non a mano.
- **Limite dichiarato prima:** in Bach una linea sola spesso suggerisce più voci (polifonia implicita); la regola della nota più acuta non la separa. Lo stile è lontano da CYMATICA.

| Grandezza | Predizione | Motivo |
|---|---|---|
| Classificazione a lag 1 | `ORDERED` | Le progressioni e le sequenze di Bach trasformano la figura della frase precedente |
| ρ di Spearman | nessuna predizione di segno | Ritorni e cadenze rendono V(k) non monotono |

## F9 — subagente alla cieca, non umano

- **Procedura:** un subagente nuovo riceve le stesse istruzioni scritte per l'esperto, con l'ordine di non leggere file né usare strumenti e di rispondere solo con la tabella di testo (formato B). Non vede il codice, la pre-registrazione, i risultati né questa aggiunta.
- **Etichetta:** **non umano**. È un confronto, non una validazione: lo stesso tipo di modello che ha scritto la misura potrebbe tendere a uno sviluppo per operazioni discrete, che la M4 vede bene.

| Grandezza | Predizione | Motivo |
|---|---|---|
| Classificazione a lag 1 | `ORDERED` | La richiesta è uno sviluppo per variazioni successive |
| ρ di Spearman | nessuna predizione di segno | Le istruzioni chiedono anche ritorni e contrasti |

## Lettura degli esiti, fissata ora

- La predizione si confronta solo con la classificazione a lag 1, come per F7.
- Un esito diverso si riporta così com'è. Prima di ogni uso dello strumento se ne cerca la causa: limiti noti della M4 (categorica sugli intervalli, cieca solo alla trasposizione cromatica), regole di conversione, oppure un difetto nuovo. Non si ritoccano né le melodie né la misura.
- Se F9 risulta `ORDERED` e F7 o F8 no, è un segnale che la misura premia lo stile di scrittura di un modello più di quello umano: va dichiarato in ogni uso.
- Nessuna soglia, nessun uso nello scoring del planner.

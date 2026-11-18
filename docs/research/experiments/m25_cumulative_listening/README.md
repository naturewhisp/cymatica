# M2.5.3–4 — Ascolto cumulativo: materiali

Disegno e ipotesi: [M2_5_Cumulative_Listening_Hypotheses.md](../../M2_5_Cumulative_Listening_Hypotheses.md), hash in `progress.md`. Decisione: ADR-0007 D-M2-50.

## Come si genera il pacchetto

Tutti i passi usano la stessa build, con il commit indicato nella chiave.

1. Si generano i due brani completi:
   ```bash
   build/x64-release/apps/cymatica_game/cymatica_game.exe --cacp-run cumulative_listening_v1
   ```
   I file vanno in `artifacts/cacp/cumulative_listening_v1/`:
   - brano 1, la fixture di riferimento (seed 1337, golden PCM PASS);
   - brano 2, il seed 34 scelto dalla regola dichiarata, renderizzato offline sul suo percorso della tabella (golden NOT_APPLICABLE);
   - il manifest, con hash, loudness e inizio delle sezioni.
2. Si rigenerano le clip chiptune:
   ```bash
   build/x64-release/apps/cymatica_game/cymatica_game.exe --cacp-run chiptune_ab_v1
   ```
3. Si costruiscono il pacchetto e la chiave:
   ```bash
   tools/pyenv/.venv/Scripts/python.exe -I docs/research/experiments/m25_cumulative_listening/make_cumulative_package.py --date <data> --commit <sha>
   ```
   L'uscita, non versionata, va in `artifacts/expert_listening/`:
   - `cymatica_ascolto_cumulativo_<data>/` e il suo `.zip`, da inviare;
   - `CHIAVE_cumulativo_<data>_NON_INVIARE.txt`, che resta a chi conduce e non si invia mai.

## Il pacchetto

- `ascolto.html` è una pagina sola, nello stile visivo di CYMATICA: palette Sintetico e figura di Chladni, prese da `expert_music_guide/_style.py`.
  - Contiene i cinque blocchi del disegno e le domande del secondo ascoltatore.
  - Le risposte si salvano nel browser mentre si scrive, e alla fine si copiano o si scaricano come testo.
  - Nessun dato lascia il computer.
- `audio/` contiene:
  - `brano_1.wav` e `brano_2.wav`;
  - `coppia_1..3_prima/seconda.wav`. Le coppie sono: chiptune contro produzione; la coppia identica di controllo; il duplicato della prima coppia con l'ordine invertito. Il seed di mescolamento sta solo nella chiave.
- Nel pacchetto non c'è una lista di hash, che rivelerebbe la coppia identica e il duplicato. Gli hash stanno nella chiave.

## Verifica fatta sulla bozza

La pagina, servita in locale, è stata controllata nel browser integrato:
- 9 lettori, tutti i file rispondono 200;
- durata del brano 2: 183,86 s;
- salvataggio automatico ed esportazione del testo, su 49 campi;
- disposizione delle scale 1–7 corretta dopo una correzione.

Due cose non sono state verificate:
- **Ricerca nell'audio:** il server di prova non accetta richieste parziali, quindi i salti fra le parti e «Segna adesso» non si sono potuti provare. Da file locale il browser legge il file direttamente, e la ricerca dovrebbe funzionare, ma non l'ho controllato.
- **Contenuto della pagina:** non contiene etichette di versione.

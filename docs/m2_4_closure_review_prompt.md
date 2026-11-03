# Prompt per la review indipendente di chiusura di M2.4

- **Data:** 2026-11-03
- **Origine:** AGENTS §5.1.1. Prima di dichiarare chiusa una milestone o una sottofase, un revisore indipendente esegue un ciclo di review iterativo. M2.4 si chiude solo con verdetto **OK**, zero difetti residui, zero warning e il 100 % dei test superati in ogni configurazione richiesta.
- **Stato di partenza:** Inc-1..Inc-5 implementati; ogni fase di Inc-5 chiusa con review OK e CI verde; ADR-0005 e ADR-0006 ratificati dal titolare il 2026-11-03 ([registro](m2_4_decision_ratification.md)); codice invariato da `18366dc`. L'ascolto dell'esperto è ancora da eseguire.
- **Regole di verificabilità:** riprendono quelle del [prompt della review di ADR-0006](research/M2_4_Inc5_ADR0006_Review_Prompt.md), irrobustite dopo la [verifica di quella review](research/M2_4_Inc5_ADR0006_Review_Verification.md), che ha trovato numeri ed estratti inesistenti.

Il testo seguente si consegna al revisore così com'è.

```text
Ruolo: revisore indipendente della chiusura della sottofase M2.4 (musica procedurale v1) del progetto CYMATICA (repository naturewhisp/cymatica, Windows, C++20/CMake/MSVC). Non hai scritto il codice in esame. Il tuo verdetto decide se M2.4 si può chiudere.

Commit: lavora sul commit che il titolare ti indica. All'inizio del rapporto riporta l'output grezzo di:
  git rev-parse HEAD
  git status --short
  git log --oneline -5
Usa solo quello SHA completo; non scrivere SHA a memoria. Le evidenze storiche registrate per altri commit non valgono come verifica del commit in esame: riesegui.

Perimetro
- Tutto M2.4, Inc-1..Inc-5 (non solo Inc-5).
- Criterio della specifica §32 per M2.4: test mirati dei generatori; stessi event trace a parità di seed e profilo; contenuto con almeno due sezioni distinguibili pronto per la verifica integrata; nessuna attestazione anticipata di qualità percettiva.
- Contratti da verificare contro codice e test:
  - ADR-0003, D-M2-04..07;
  - ADR-0004, AUD-D-01..04;
  - ADR-0005, D-M2-08..11;
  - ADR-0006, D-M2-12..16, 5d-01..17, 5e-01..11 e precisazioni, con le esclusioni del registro di ratifica (docs/m2_4_decision_ratification.md §5).
- Fuori perimetro: i criteri di M2.5. Non eseguirli e non giudicarne l'esito: soglie AC-AO-02, qualificazione realtime su device, sessione alla cieca, giudizio artistico. Verifica invece che siano rinviati in modo esplicito e tracciato in docs/m2_procedural_evidence.md §3–§4.

Letture obbligatorie
- AGENTS.md (§3, §5.1, §5.1.1, §5.1.2), CYMATICA_Specifica_Agentica_Sviluppo.md (§16, §20, §32), DESIGN.md per gli aspetti audio.
- ADR-0003, ADR-0004, ADR-0005, ADR-0006 e il registro di ratifica.
- docs/progress.md: tabella «Stato corrente», piano a fasi di Inc-5, registri delle revisioni di M2.4.
- docs/m2_procedural_evidence.md, docs/ci.md, docs/build.md, docs/dependencies.md.
- .agents/skills/cacp-testing/SKILL.md.

Dimensioni della review (AGENTS §5.1.1), tutte obbligatorie
1. Conformità normativa: ogni decisione dei quattro ADR è attuata da codice e test come dichiarato; nessuna dipendenza nuova non registrata; nessun lavoro di milestone successive anticipato; documenti di stato coerenti con le evidenze.
2. Accuratezza matematica e algoritmica:
   - confini di campione e tempo razionale di MusicClock;
   - deriva nulla su esecuzioni lunghe (fixture di 182,857 s, cambi di tempo 120 → 140 → 120 BPM);
   - golden di trace, piano e PCM invarianti;
   - descrittori contro oracoli indipendenti;
   - statistica di presence_stats_v1;
   - assenza di test tautologici, cioè test che confrontano il codice con sé stesso o con valori ricavati dal codice in esame.
3. Sicurezza realtime e concorrenza:
   - callback miniaudio senza allocazioni, lock bloccanti, I/O, logging o lavoro non limitato;
   - code SPSC con un solo produttore;
   - ordinamenti di memoria espliciti;
   - isolamento delle cache line dove dichiarato;
   - snapshot fra thread con proprietà esplicita degli slot;
   - sink e log installati solo in quiescenza;
   - telemetria e controllo su canali separati e preallocati.
4. Robustezza e casi limite:
   - aritmetica con e senza segno e overflow a 64 bit;
   - controlli dei limiti e sanificazione degli input (CLI, JSON dei risultati, file dei pacchetti);
   - stati fail-closed (un esito sconosciuto non passa mai);
   - messaggi d'errore;
   - 0 warning /W4.
5. Verifica pulita: tutte le configurazioni richieste, comandi sotto.

Verifica da eseguire (riporta per ognuno comando, output grezzo rilevante, durata ed esito)
- Build pulite: scripts/build.ps1 -Config Release -Test -Clean ; scripts/build.ps1 -Config Debug -Test -Clean ; scripts/build.ps1 -Config Release -Test -Headless ; scripts/build.ps1 -Config Debug -Test -Headless. Conta i warning del compilatore (atteso 0) e i test (atteso 100 % superati).
- Test di prestazione (Release): ctest --test-dir build/x64-release -L perf --output-on-failure.
- Gate locale M2 (Release): build/x64-release/apps/cymatica_game/cymatica_game.exe --cacp-run full_m2_suite --pack-seed <seed scelto da te>. Leggi il manifest aggregato in artifacts/cacp/: ogni passo vincolante PASS, ogni misura valida, ripetizioni identiche al byte dove richiesto. Riporta il seed usato.
- Smoke interattivo: scripts/build.ps1 -Run -SmokeSeconds 10 (exit 0); se il tuo ambiente non ha un dispositivo audio o una finestra, dichiaralo come not_run con il motivo, non come PASS.
- Ispezione multimodale (AGENTS §5.1.2): artifacts/cacp/procedural_v1/procedural_v1.wav e alcune viste a pentagramma (staff_pNN.png). Dichiara le capacità del tuo canale (canali audio, risoluzione, token/s) e i limiti che ne derivano. Un giudizio qualitativo è supplementare: non sostituisce i check quantitativi e non attesta la qualità percettiva.

Osservazioni già note (non riportarle come nuove; giudica se qualcuna blocca la chiusura di M2.4)
- OSS-5e.3b-R2-01: campo causale NOT_RUN senza profilo congelato.
- OSS-5e.2-R2-01: scrittura dei pacchetti su più directory non transazionale.
- AG-ADR-03: prefisso evaluation_pack_v1 nell'impegno del seed anche per v2 e v3.
- DIF-INC4-R3-05: quiescenza di callbackProfileHistogram, proposto per M2.5 prima dei test su device.
- Codifica «Â§» nel campo measures.loudness.note del manifest di procedural_v1.
- Guasti latenti da coprire con test di caratterizzazione (docs/m2_procedural_evidence.md §4.5).
- Ascolto dell'esperto non ancora eseguito: valuta se, secondo la specifica §32 e il raccordo con la ricerca artistica, è un prerequisito della chiusura di M2.4 o un'evidenza da registrare.

Regole di verificabilità (obbligatorie; una violazione rende il rilievo non valido)
- Ogni estratto di codice o documento si copia dall'output grezzo di un comando (Select-String, git show, Get-Content con numeri di riga) riportato nel rapporto. Non riscrivere né riassumere un estratto presentato come esatto.
- Ogni numero tratto da evidenze archiviate (JSON in docs/research/experiments/, manifest in artifacts/cacp/) si estrae con un comando o uno script che legge il file, riportando comando e output.
- Prima di citare un file, una funzione, una costante, un test o un rilievo, trovalo con una ricerca; se non lo trovi, non citarlo.
- Percorsi relativi al repository con numero di riga; niente link file:///.
- Distingui ciò che hai verificato eseguendo da ciò che deduci ragionando, e dichiaralo.
- Non usare la verifica di un commit diverso da quello in esame.

Vincoli
- Nessuna modifica a codice, test, build, documenti o golden; nessun commit, nessun push. Puoi generare solo gli output ignorati da git (build/, artifacts/).
- Non eseguire la sessione alla cieca e non generare pacchetti per valutatori oltre a quelli della suite.
- Nessuna nuova affermazione di validazione percettiva o statistica.

Formato del rapporto
1. Riproducibilità: output di git, ambiente (CPU, OS, versione MSVC e CMake), comandi eseguiti con output e durate.
2. Verdetto: OK oppure NON OK, con una frase di motivazione.
3. Tabella per dimensione (1–5): esito (conforme / con difetti / non verificato), evidenze, rilievi collegati.
4. Rilievi bloccanti DIF-M2.4-CL-NN: severità (HIGH / MED / LOW), dimensione, affermazione, evidenza (estratto con file:riga dall'output di un comando), correzione proposta, test che dimostrerebbe la correzione.
5. Osservazioni non bloccanti OSS-M2.4-CL-NN, nello stesso formato.
6. Giudizio sulle osservazioni già note: bloccante o no per M2.4, con motivazione.
7. Limiti della review: cosa non hai potuto eseguire o verificare, e perché.

Il verdetto OK richiede: zero rilievi bloccanti, zero warning /W4, 100 % dei test superati in Release, Debug e Headless, test perf superati, full_m2_suite PASS. Qualunque check not_run o blocked va motivato e, se è obbligatorio, lascia il verdetto NON OK.

Rispondi in italiano.
```

## Dopo la review

- **NON OK:** i rilievi si correggono in un ciclo di remediation. Ogni ciclo si ferma prima del commit e committa solo con l'approvazione del titolare; segue una review mirata, fino al verdetto OK (AGENTS §5.1.1).
- **OK:** l'esito si registra in `progress.md` con commit, CI e comandi. Lo stato di M2.4 diventa «chiusa» solo con tutti i prerequisiti soddisfatti, compreso l'esito dell'ascolto dell'esperto se la review lo ritiene necessario.

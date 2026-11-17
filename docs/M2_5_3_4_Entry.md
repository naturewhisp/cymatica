# M2.5.3–4 «Struttura e sviluppo» — Ingresso di fase

- **Stato:** preparato il 2026-11-17 (AGENTS §1.1). **E1, E2 ed E3 approvati dal titolare il 2026-11-17** («Approvo tutto»); registro in ADR-0007 §11, D-M2-38. E4–E6 restano come annotati.
- **Fonti:** ADR-0007 §3 (sotto-fase M2.5.3–4), §4 (CP-3, CP-4), §5 (calendario), D-M2-19 (metodo), D-M2-20 (livelli di qualità), D-M2-21 (metro), D-M2-30 (verifica autonoma), D-M2-32 (perimetro); [dossier R-M25-7](research/M2_5_F5_Music_Director_Research.md) (contratto minimo, definizioni di sviluppo, batteria d'ascolto); [dossier delle lacune di misura](research/M2_5_Measurement_Gaps_Research.md) §6; specifica §32, riga M2.5; [registro delle domande per l'ascolto cumulativo](m2_5_cumulative_listening_questions.md).
- **Prerequisito:** M2.5.2 chiusa (CI 44094198129 verde su `678a336`).

## 1. Perimetro (D-M2-32)

1. **F3:** continuità dei pattern ai confini di sezione, prima di qualunque rampa di tempo (R-M25-4).
2. **F1:** àncora metrica dichiarata per frase (R-MET-1: `ancorato`, `spostato`, `ritorno`; R-MET-2: in `ancorato` un ruolo articola il tempo 1).
3. **Contratto minimo di R-M25-7 (opzione B)**, un cambiamento per pacchetto, in quest'ordine:
   1. diade di Corpo;
   2. Vettore che legge l'accordo, con ablazione;
   3. `dominantRole` e registro di Vettore in A′;
   4. entrate e uscite dei ruoli;
   5. variazione derivata e ripresa (lignaggio dei motivi a capacità fissa);
   6. seed oltre i pareggi su tabella chiusa d'autore.
4. **D-M2-21:** scenario tecnico ripetibile del cambio di metro 4/4 → 3/4 → 4/4 con manifest, promosso dalla prova di fattibilità. Va fatto al più tardi prima del gate.
5. **In chiusura di fase:** un solo ascolto umano cumulativo (D-M2-30), con il registro delle domande, e CP-3.

**Fuori perimetro:** §16.3–§16.4 completo (resta a M6); rampa di tempo (solo se F3 non basta, con un ADR per `MusicClock`); metro musicale (R-M25-11, non scelto); rilascio H-03 e normalizzazione H-04 (CP-4).

## 2. Criteri che la fase non deve rompere

Dalla riga M2.5 della specifica §32 e da ADR-0007:
- tre minuti di musica continua senza crackle;
- due sezioni distinguibili con transizione di tempo nel contenuto;
- sincronismo fra intent e audio;
- trace deterministico; callback che esegue soltanto;
- piano ricostruibile da seed, versione della policy e trace, con stream casuali isolati.

Ogni pacchetto aggiorna i golden di piano, trace e PCM con il motivo, e chiude con Release, Debug, `full_m2_suite` e CI verde.

## 3. Punti aperti e proposte

| # | Punto | Serve ora? | Proposta |
|---|---|---|---|
| **E1** | **Come verificare lo sviluppo in autonomia.** M4 non è validata: Bach e la tabella F9 risultano `ORDER_FREE`. Senza una misura validata, ogni «sviluppo» andrebbe giudicato solo all'ascolto | **Sì**, prima del punto 3.5 (variazione derivata) | **Pacchetto di misura 6** (dossier §6, D1 + D2): nuove misure dello sviluppo, pre-registrate e provate sugli stessi controlli F1–F9 e sulle fixture avverse (loop letterale, rumore nella scala, rimescolamento). Una misura che non ordina correttamente i controlli non si usa. IDyOM (D3) solo se D1–D2 non bastano |
| **E2** | **Versione della policy del planner.** Il piano dev'essere riproducibile da seed, versione della policy e trace | Sì, al primo pacchetto che tocca il planner | Introdurre una versione esplicita (per esempio `planner-v2-m25`) al primo cambio, registrata nel manifest. Non conservare il percorso v1: i golden storici restano documentati, non eseguibili |
| **E3** | **Ordine dei pacchetti** | Sì | (0) D-M2-21 scenario tecnico del metro, piccolo e indipendente; (1) pacchetto di misura 6; (2) F3; (3) F1; (4) contratto minimo 1→6. F1 dopo F3, perché la continuità ai confini cambia anche le frasi di Pulso |
| **E4** | **F7, la melodia dell'esperto** | No | Non blocca. Quando arriva entra come controllo positivo del pacchetto 6, con l'addendum già pre-registrato |
| **E5** | **Annotazioni di F8** (lignaggio dei motivi in Bach) | Solo per E1 | Il dossier le chiede a una persona. In alternativa il pacchetto 6 usa soltanto controlli il cui ordine è noto per costruzione (fixture avverse e tabella F9) |
| **E6** | **Seed nuovi per la sessione alla cieca di M2.5.5** | No | Si decide a CP-4. Il punto 3.6 è il primo a produrre varietà fra seed |

## 4. Rischi

- **Varietà fittizia:** è il rischio principale (dossier R-M25-7). Si mitiga con E1 e con l'ablazione di ogni componente.
- **Calibrazione AC-AO-02:** ogni cambio del piano cambia gli stem. `acao02_v1` resta `INCONCLUSIVE` fino al ricongelamento di M2.5.5, che va fatto sul contenuto congelato.
- **Misure del mix:** le misure di M2.5.2 (livelli, Trama, kick) valgono per il brano attuale. Dopo la fase vanno ripetute, ed è automatico con gli scenari esistenti.

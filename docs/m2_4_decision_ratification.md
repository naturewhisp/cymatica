# M2.4 — Registro per la ratifica delle decisioni (ADR-0005 e ADR-0006)

- **Data di preparazione:** 2026-11-03
- **Origine:** ADR-0005 e ADR-0006 richiedono la ratifica esplicita del titolare prima della chiusura di M2.4. Per ADR-0005 la richiesta riguarda in particolare la statistica di PASS di D-M2-10. È l'attività 3 di chiusura di Inc-5 in [progress.md](progress.md).
- **Stato:** **ratificato dal titolare il 2026-11-03** («Ratifico come proposto»), con il perimetro e le esclusioni descritti qui (§7).

## 1. Cosa significa la ratifica

- **Conferma:** le decisioni elencate (§3, §4) diventano i contratti su cui M2.4 si chiude e su cui M2.5 costruisce. Restano `accepted for implementation`, con l'annotazione «ratificato dal titolare».
- **Non valida nulla:** nessun parametro diventa `validated`. Profili, soglie e tuning restano iniziali e versionati; si valutano al gate M2.5 o dopo (§5).
- **Non chiude M2.4 da sola.** Servono anche:
  - la review indipendente di chiusura di M2.4 (AGENTS §5.1.1);
  - la registrazione dell'ascolto dell'esperto, anche se negativo;
  - la verifica finale dell'handoff (§6).
- **Non dipende dall'ascolto dell'esperto:** l'ascolto alimenta solo decisioni di M2.5 (H-03, F1–F6), nessuna delle quali è soggetta a questa ratifica.

## 2. Provenienza delle evidenze

- **Codice:** invariato da `18366dc` (chiusura di 5e.4a); i commit successivi toccano solo documenti. Verificato con `git diff --stat 18366dc HEAD -- engine apps tests CMakeLists.txt CMakePresets.json .github scripts`, che non riporta differenze.
- **CI:** l'ultima verificata è su `b8b811f` ([run 41661221144](https://github.com/naturewhisp/cymatica/actions/runs/41661221144)), verde. Release 231/231 con `tone_test` e `fixture_test` PASS; Debug rapida 221/221.
- **Gate locale `full_m2_suite`:** verdetto **PASS** in 201,8 s, nel [manifest](research/experiments/inc5_full_m2_suite/full_m2_suite_manifest_2026-11-03_dif5e4a.json) (remediation DIF-5e.4a, poi committata in `18366dc`).
  - Vincolanti PASS: `tone_test`, `voice_test`, `timbre_test`, `planner_test`, `fixture_test`, `profile_test`, `evaluation_check_selftest_v1`, `procedural_v1`.
  - Misure valide: `onset_parity_v1`, `acao02_v1`, `release_ab_v1` e `trama_miss_diagnosis_v1` sono `MEASURED`; `evaluation_pack_v1/v2/v3` sono `GENERATED`.
- **Review indipendenti:** OK per ogni fase da 5.0 a 5e.4b (tabella del piano a fasi in `progress.md`). La review di ADR-0006 di Antigravity è [verificata](research/M2_4_Inc5_ADR0006_Review_Verification.md): nessun difetto nuovo bloccante.

## 3. ADR-0005 (ingresso Inc-4)

| Decisione | Contenuto | Attuazione ed evidenza | Proposta |
|---|---|---|---|
| D-M2-08 | Espansione di `fixture-m2-4-v1` in 24 frasi, confini a 3.072.000 / 5.705.142 / 8.777.142 frame | Inc-4 (`b0f466f`); `fixture_test` PASS in ogni CI e nella suite; golden di trace, piano e PCM | ratificare |
| D-M2-09 | Pianificazione JIT, epoch, `SetTempoMap` con frame esplicito, revoca dallo staging | Inc-4; ack ed epoch 1 → 2 → 3 nei check di `fixture_test`; in 5a, hash JIT = offline = golden e catena O3 esatta (2.784/2.784) | ratificare |
| D-M2-10 | Profiling: p99.9 ≤ 2,0 ms normalizzati a 512 frame e nessun blocco oltre il periodo, su W e F, con CACP ON e OFF, 3 ripetizioni, solo Release | `profile_test` PASS nella suite (Release, gate dei tempi; in Debug la suite fallisce per costruzione, DIF-5e.4a-02) | ratificare **la statistica di PASS**, con il limite R6-03: non è la qualificazione realtime su device, che resta un gate distinto di M2.5 |
| D-M2-11 | Scenari separati `fixture_probe_v1` e `profile_probe_v1`; check vincolanti; metodi di valutazione dell'agente supplementari | Inc-4; esteso in Inc-5 da D-M2-15 (pacchetto e checker) | ratificare |
| Nota multi-seed | Il seed agisce solo sui pareggi | Confermata dal Teorema D e da `NULL_DIVERSITY` sui seed 1..64 (5e.2) | presa d'atto (non è una decisione) |

## 4. ADR-0006 (ingresso Inc-5)

| Decisione | Contenuto | Attuazione ed evidenza | Proposta |
|---|---|---|---|
| D-M2-12 | Hash canonico di provenienza e fingerprint del contenuto distinti; registro O3 con disposizioni; controllo esatto della catena | 5a (`be91198`): fault injection, overflow, profilazione con sink nullo; catena esatta in `fixture_test` | ratificare |
| D-M2-13 | Stem stereo per ruolo offline, ricostruzione del mix dei bus | 5c (`3f026c2`); ricostruzione bit-exact (0 campioni discordanti) | ratificare |
| D-M2-14 | `descriptors_v1`: STFT, onset, mel, Side/Mid, LUFS BS.1770-5 / EBU 3341, PNG, SHA-256 | 5b (`d86f6bb`), test contro oracoli e vettori di riferimento; parità in `onset_parity_v1` | ratificare **come versione congelata**. La scelta della normalizzazione (M-NORM) resta aperta per un `descriptors_v2` (H-04) |
| D-M2-15 e 5d-01..17 | Pacchetto `public_blind` / `public_diagnostic` / `sealed`, `evaluation_result_v0`, checker e qualifica «alla cieca» | 5d (`c8c1086`); test di leakage, positivi e negativi; `evaluation_check_selftest_v1` PASS (5e.4a) | ratificare |
| D-M2-16 | Calibrazione R6, AC-AO-02 **proposto**, multi-seed, LUFS esplorativo, nessun gate di suite in CI | 5c, 5e.1, 5e.2, 5e.4a; calibrazione `MEASURED`, generalizzazione `INCONCLUSIVE` | ratificare **il metodo**. Le soglie AC-AO-02 restano proposte (H-01, H-02) |
| 5e-01..06 | `procedural_v1`, `full_m2_suite`, `fixture_test` in CI, diversità, LUFS, vista a pentagramma | 5e.1, 5e.2, 5e.4a | ratificare |
| 5e-07 | Ablazione rinviata a M2.5 con F5 | non attuata, per decisione | presa d'atto del rinvio |
| 5e-08 | Strumento sperimentale del release, solo offline; nessun valore adottato | 5e.3 (`c4857b2`): `release_ab_v1` `MEASURED`, H1 `NOT_SUPPORTED`, H2 `SUPPORTED` | ratificare lo strumento; il valore resta a M2.5 (H-03) |
| 5e-09 | Osservazioni del checker (OSS-5d-R3-01, OSS-5d-01) | 5d, 5e.2 | ratificare |
| 5e-10 | Evidenze e handoff | 5e.4b: [dossier delle evidenze](m2_procedural_evidence.md). **Modificata** dalla decisione del titolare del 2026-11-03: l'intera sessione alla cieca va a M2.5; in Inc-5 restano la review di ADR-0006 e l'ascolto dell'esperto | ratificare con la modifica |
| 5e-11 | Soglie `n_eff` ≥ 4 e almeno 4 gruppi; composizione del pacchetto | 5e.2; `presence_stats_v1` | ratificare (decisione già del titolare) |
| Precisazioni di attuazione | 5e.3, 5e.3b (M-NORM `SUPPORTED`, retrospettiva), 5e.3c (R-01, `evaluation_pack_v3`, previsione di 8 gruppi `CONFIRMED`), 5e.4a (regole della suite e del self-test) | fasi omonime, review OK | ratificare |

## 5. Escluso dalla ratifica (resta proposto o non validato)

- **Decisioni del gate M2.5:** H-01..H-05 del [dossier delle evidenze](m2_procedural_evidence.md) §4.1. Riguardano soglie AC-AO-02, precisione grezza contro precisione senza rilasci, release per ruolo, normalizzazione del rivelatore e dipendenza fra gruppi.
- **Profili non validati** (§4.3 del dossier): `kPlannerScoringProfileV1`, `stem_onset_profile_v1`, `presence_profile_v1`, `presence_stats_v1`, `PlayerProfile_v1`.
- **Qualificazione realtime su device:** corsa WASAPI ≥ 180 s con periodo effettivo, MMCSS e underrun (D-M2-10, fuori da Inc-4).
- **Giudizio artistico** e questioni musicali F1–F6.

## 6. Verifica finale dell'handoff M2.5

Il [dossier delle evidenze](m2_procedural_evidence.md) §3–§4 è stato confrontato con la specifica §32 (M2.4 e M2.5), con la definizione normativa di AC-AO-02 ([agent_control_observability](agent_control_observability.md), tabella delle estensioni) e con i rinvii registrati in `progress.md`.

| Controllo | Esito | Azione |
|---|---|---|
| Criteri di M2.5 della specifica §32 (tre minuti senza crackle, due sezioni con transizione di tempo o metro, sincronismo, trace deterministico, callback che esegue soltanto, build e review) | tutti mappati; la mancanza del cambio di **metro** nella fixture è dichiarata | nessuna |
| AC-AO-02 normativo: cattura bounded, overflow e lifecycle testati, onset correlati all'intent, adapter che dichiara capacità e limiti | la riga della mappatura copriva solo O3 e gli onset | **corretto:** riga divisa nei quattro elementi, con l'evidenza di ciascuno |
| DIF-INC4-R3-05 (quiescenza di `callbackProfileHistogram`, «proposto per M2.5 prima dei test su device») | assente dall'handoff | **aggiunto** al §4.4 |
| Qualificazione realtime su device (D-M2-10) | citata solo nella riga della callback | **resa esplicita** al §4.4 |
| AG-ADR-03 e proposte M2.5 della review di Antigravity | assenti | **aggiunti** al §4.5, con rimando alla verifica |
| OSS-5e.3b-R2-01, OSS-5e.2-R2-01, H-01..H-05, F1–F6, profili non validati, sessione alla cieca, ablazione | presenti | nessuna |
| Record di stato non aggiornati: intestazione di ADR-0006 («in review del titolare come fase 5.0»), stato corrente e piano a fasi di `progress.md` (fermi a 5e.2 o 5e.3c), [agent_control_observability](agent_control_observability.md) §12 («in review dal 2026-10-26») | non aggiornati | **aggiornati** allo stato verificato, senza anticipare la ratifica |

## 7. Ratifica del titolare

```text
Data: 2026-11-03
Formula del titolare: «Ratifico come proposto»
ADR-0005 (D-M2-08..11): ratificato come proposto nel §3 (D-M2-10 con il limite R6-03)
ADR-0006 (D-M2-12..16, 5d-01..17, 5e-01..11 e precisazioni): ratificato come proposto nel §4
  (D-M2-14 come versione congelata; D-M2-16 nel metodo, soglie AC-AO-02 escluse;
   5e-10 con la modifica del 2026-11-03; presa d'atto di 5e-07)
Riserve o modifiche: nessuna; resta escluso quanto elencato nel §5
```

Le intestazioni di ADR-0005 e ADR-0006 riportano «`accepted for implementation`, ratificato dal titolare il 2026-11-03», con un rimando a questo registro. Nessun valore diventa `validated`.

# M2.5.3–4 — Contratto minimo, punto 6: il seed sceglie fra alternative d'autore. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni del titolare (2026-11-17):**
  - la fixture di riferimento resta il profilo d'autore («ha senso che resti fissa per i test»);
  - procedere con il punto 6, aggiungendo alla tabella la cella melodica di AURA ([note sulle fonti](AURA_Music_Source_Notes.md), voce A1).
- **Principio** dal [dossier R-M25-7](M2_5_F5_Music_Director_Research.md): «lo stato decide quando, il seed sceglie quale percorso fra alternative d'autore».
- **Diagnosi:** il seed agisce solo sui pareggi, e nella fixture non ce ne sono; i 64 seed danno lo stesso brano (`NULL_DIVERSITY`, Teorema D).

## 1. Variabile

1. **Tabella chiusa `fixture-variants-v1`:** 5 dimensioni, tutte già dichiarate nel contratto. Fanno **256 combinazioni**. L'alternativa 0 di ogni dimensione è il profilo d'autore attuale.

   | Dimensione | Alternative | Vincoli derivati |
   |---|---|---|
   | A, entrata della Trama | frase **4**, 2, 6, 3 | A′ la riprende: Trama assente in 16 … 16 + e − 1 |
   | B, inizio dello spostamento metrico | frase **12**, 10, 11, 13 | `Spostato` d, d + 1 (2 sedicesimi); `Ritorno` d + 2; Trama assente in d − 2, d − 1 |
   | A′, pausa del kick | frase **22**, 21, 20, nessuna | Mai in `Spostato` né con Corpo ritardato (D-M2-44) |
   | Motivo d'apertura di A (e A′) | **motivo attuale**, cella di AURA | La cella è `0 · 0 2 4 · 3 2` in ottavi; ancoraggi sui gradi 0 e 4 |
   | Motivo d'apertura di B | **motivo attuale**, cella di AURA | La stessa cella nel modo di B |

2. **Estrazione:** per ogni dimensione, `randomU64(RandomKey{seed, StreamId::Director, decisione della dimensione, 0}, policy)` modulo il numero di alternative. Lo stream `Director` è separato da quello dei pareggi (`MusicForm`). Il resto è senza distorsione, perché 2⁶⁴ è divisibile per 4 e per 2.
3. **Fixture di riferimento invariata:** `makeFixtureM24V1Phrases(clock)` e `makeFixtureM24V1Form()` restituiscono l'alternativa 0 di ogni dimensione. Le varianti si ottengono solo con le nuove funzioni che ricevono una variante esplicita.
4. **Uso:**
   - lo scenario di diversità pianifica ogni seed da 1 a 64 con la sua variante (`planFixtureM24V1Procedural`);
   - le misure e i golden restano sulla fixture di riferimento (`planFixtureM24V1`);
   - la variante estratta entra nel manifest dello scenario di diversità.

## 2. Criteri e regola d'adozione

| ID | Criterio | Superato se |
|---|---|---|
| **S1, riferimento** | Fixture di riferimento | Golden di piano, trace e PCM della fixture, della forma di prova e di `meter_change_v1` **invariati** |
| **S2, determinismo e isolamento** | Estrazione | Stesso seed, stessa variante. La variante dipende solo da seed e policy. I pareggi restano sullo stream `MusicForm` |
| **S3, validità** | Tutte le 256 varianti pianificate | Nessun fallback; nessuna violazione del piano; nessun `vettoreRegisterUnmet`; in ogni battuta non `Spostato` un attacco di Pulso o Corpo sul tempo 1 |
| **S4, diversità** | Scenario sui seed da 1 a 64 | Esito `DISTRIBUTIONS`, cioè almeno 8 contenuti distinti, e almeno 40 varianti distinte su 64 seed (attese circa 57) |
| **S5, la cella di AURA** | Sui 64 seed | Entrambi i motivi compaiono come apertura sia di A sia di B |
| **S6, build** | Release, Debug, `full_m2_suite` | Verdi, salvo il limite noto `acao02_v1` |

**Adozione:** tutti superati. Altrimenti l'esito è negativo, e il seed resta sui soli pareggi.

**Descrittivi:**
- battute di Corpo contraddette, per variante;
- distanze M2–M4 fra seed;
- quota di frasi identiche per ruolo.

## 3. Limiti

- La varietà è limitata a 256 percorsi, su dimensioni d'orchestrazione e di apertura del motivo. Melodie di frase (6b), armonie alternative e motivi dell'esperto la allargheranno.
- La cella di AURA è materiale del titolare, riscritto da me nei modi di CYMATICA: va sentito, prove B1–B3.
- Che 64 seed diano piani diversi non dice che i brani siano percepiti come diversi: lo dirà l'ascolto.

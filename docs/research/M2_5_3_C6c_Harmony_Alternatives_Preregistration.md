# M2.5.3–4 — Pacchetto 6c: alternative armoniche di B, con la sensibile dichiarata. Pre-registrazione

- **Stato:** scritto il 2026-11-18, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Entrambe in tabella» (2026-11-18): un'alternativa diatonica e una con la sensibile dichiarata.
- **Fonte:** [note su AURA](AURA_Music_Source_Notes.md), voce A5: giro i–VI–iv–V con la sensibile sul V.
- **Decisione prevista:** D-M2-49, che estende la tabella d'autore `fixture-variants-v1` di D-M2-47.

## 1. Variabile

1. **Alterazione dichiarata della terza.** `ChordSpec` riceve il campo `thirdChromaticOffset`, in semitoni, con valore 0 o +1 e default 0. Tutte le note dell'accordo passano per una funzione sola, `chordToneMidi`, che aggiunge l'alterazione quando il grado è la terza dell'accordo, in qualunque ottava. La usano:
   - le due voci di Corpo e il pad di ripiego L2;
   - l'arpeggio della Trama;
   - la riparazione della diade di Corpo (C1c);
   - le note dell'accordo lette dal Vettore (C2) e il riconoscimento del cambio d'accordo;
   - il conteggio delle note dell'accordo negli strumenti d'insieme.
2. **Regola della falsa relazione.** È un'estensione della lettura dell'accordo del punto 2. In una battuta il cui accordo altera la terza, una nota del Vettore sulla classe d'altezza naturale di quel grado si sposta sulla nota dell'accordo più vicina, con la regola già in uso: a parità di distanza, verso il basso. Lo spostamento conta come adattamento ed entra nel trace. Con l'ablazione `vettoreReadsChord = false` anche questa regola si spegne.
3. **Nuova dimensione della tabella,** `bHarmony`. Viene estratta con l'id di decisione `kVariantDecisionBase + 5`, sul flusso Director, modulo 3. Le estrazioni delle altre cinque dimensioni non cambiano: ogni seed tiene lo stesso cammino di prima, più l'armonia di B.

   | Alternativa | Frasi 8–14 di B | Frase cadenzale 15 |
   |---|---|---|
   | 0, riferimento | i–VI–VII–i (Sol m, Mi♭, Fa, Sol m) | come le altre, con il pad che attraversa il confine |
   | 1, diatonica | i–VI–iv–v (Sol m, Mi♭, Do m, Re m) | i–iv–v–i |
   | 2, sensibile | i–VI–iv–V (Sol m, Mi♭, Do m, Re con Fa♯) | i–iv–V–i |

   - Nelle alternative 1 e 2 le frasi 8–14 finiscono sul v o sul V, che si risolve nel i della frase seguente, come nel giro di AURA.
   - La frase cadenzale 15 torna sul i. Così il pad che entra in A′ resta Sol minore, come nel riferimento: nessun Re maggiore tiene contro il Re dorico di A′.
   - Funzioni armoniche: i Stabilità, VI Preparazione, iv Preparazione, v o V Tensione; nella frase 15, i Stabilità, iv Preparazione, V Tensione, i Risoluzione.
   - La progressione melodica di 6b segue le fondamentali: 0, −2, +3, −3 nelle frasi 8–14, e 0, +3, −3, 0 nella frase 15.
4. **Il riferimento non cambia.** La fixture di riferimento usa l'alternativa 0 e tutte le alterazioni restano a 0. Le sezioni A e A′ non cambiano in nessuna alternativa.
5. **Il trace non registra gli accordi,** come già prima. L'alternativa si ricava da seed e versione della politica attraverso l'estrazione, e l'alterazione compare negli eventi pianificati.

## 2. Misure

Le misure sono simboliche, sui token in sedicesimi del piano (`planTokens`), su un disegno appaiato. Per ogni seed da 1 a 64 si prende il cammino estratto e se ne pianificano tre copie, una per alternativa (`bHarmony` forzato a 0, 1, 2). Lo scenario è `harmony_alternatives_v1`. Il JSON va in `artifacts/cacp/` e in [`experiments/m25_3_c6c_harmony_alternatives/`](experiments/m25_3_c6c_harmony_alternatives/).

| ID | Misura | Definizione |
|---|---|---|
| **H-FR** | False relazioni | Note di qualunque ruolo sulla classe d'altezza naturale della terza alterata che suonano dentro una battuta con l'accordo alterato, per attacco o per durata |
| **H-IC** | Conflitti col Corpo | Attacchi di Vettore in classe d'intervallo 1 o 6 con una voce di Corpo che suona in quell'istante |
| **H-CT** | Note dell'accordo sui cambi | Quota degli attacchi di Vettore al passo 0 di una battuta di B con cambio d'accordo che sono note dell'accordo, alterazione compresa |
| **H-AD** | Adattamenti | Note spostate dal punto 2 e dalla regola della falsa relazione, nelle frasi di B |
| **H-RP** | Riparazioni | Battute di Corpo riparate da C1c, nelle frasi di B |
| **H-GR** | Gruppi di Corpo | Gruppi simbolici distinti delle frasi di Corpo di B, sui 64 seed |
| **H-LT** | Sensibile | Attacchi di Fa♯ per ruolo nell'alternativa 2 |

## 3. Criteri e regola d'adozione

| ID | Criterio | Superato se |
|---|---|---|
| **H1, riferimento** | Fixture e forme che non usano le alternative | Golden di trace, piano e PCM della fixture invariati; eventi di `planner_probe_v1` e `meter_change_v1` invariati; nessun golden esistente cambia |
| **H2, falsa relazione** | H-FR nell'alternativa 2 | 0 su 64 seed |
| **H3, contratto dei punti 1–4** | Alternative 1 e 2 | H-IC = 0; H-CT = 100%; nessun `vettoreRegisterUnmet` |
| **H4, tabella** | Scansione procedurale dei seed da 1 a 64 | Esito `DISTRIBUTIONS`; brani distinti ≥ 58 (previsti 61–63); cammini distinti ≥ 50 (previsti circa 61); ogni alternativa estratta almeno 10 volte (prevista circa 21) |
| **H5, build** | Release e Debug `/W4 /WX` | Tutti i test passano; `full_m2_suite` verde salvo il limite noto `acao02_v1` |

**Adozione:** si adotta se H1–H5 sono superati. Altrimenti l'esito è negativo: la tabella resta a 256 cammini e il campo d'alterazione non entra.

**Previsioni descrittive,** riportate ma non vincolanti per l'adozione:
- H-AD dell'alternativa 2 supera quello dell'alternativa 1, per i Fa naturali spostati;
- H-LT è maggiore di 0 in Corpo, Trama e Vettore;
- H-GR delle alternative 1 e 2 è almeno pari a quello dell'alternativa 0.

## 4. Limiti

- È un controllo simbolico: dice che la sensibile non contraddice le altre voci, non che il giro suoni meglio. Il giudizio d'ascolto va nel registro delle domande per l'ascolto cumulativo.
- La sensibile introduce un tono fuori modo in un linguaggio finora modale. Se l'ascolto dirà che rompe l'identità di B, si toglie l'alternativa 2 dalla tabella e il campo d'alterazione resta inerte.
- Solo B riceve alternative: in A e A′ il re dorico ha già il IV maggiore, e un V maggiore (Do♯) sarebbe una decisione a parte.

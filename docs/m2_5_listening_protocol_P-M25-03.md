# Protocollo d'ascolto di M2.5 (P-M25-03) — bozza per CP-1

- **Stato:** **proposto**, da decidere a CP-1 e da fissare **prima** dell'ascolto di CP-2 (ADR-0007, P-M25-03). Non è ancora adottato.
- **Fonti:** [proposta d'ingresso](research/M2_5_Entry_Music_Quality_Proposal.md) (P-M25-03 e precisazioni), [dossier R-M25-7](research/M2_5_F5_Music_Director_Research.md) (estensione per CP-1, batteria B0–B7), [ascolto del 2026-10-28](m2_4_human_listening_2026-10-28.md) (scale 1–7), [ascolto del 2026-11-03](m2_4_human_listening_2026-11-03.md) (indicazioni per il pacchetto successivo), ADR-0007 §8.1 (volume per giudicare il kick), [prova del volume USB](research/experiments/cp1_volume_loopback/README.md) (volume dell'app).
- **Perimetro:** gli ascolti umani previsti da ADR-0007: CP-2, CP-3 e il gate. L'agente non ascolta e non conta come partecipante.

## 1. Partecipanti

- **Esperto** (ascoltatore principale), come negli ascolti precedenti. Il suo nome non compare nei documenti.
- **Secondo ascoltatore**, senza formazione musicale richiesta (indicazione del titolare del 2026-11-03): risponde solo alle domande semplici del §6.
- Chi conduce (il titolare) prepara l'ordine delle clip e tiene la chiave fino a fine sessione.

## 2. Prima dell'ascolto

1. **Pacchetto:** materiali rigenerati dalla build indicata; per ciascuno, il commit e l'hash. Una sezione dice **che cosa è cambiato** rispetto all'ascolto precedente, senza anticipare quale versione sia «quella nuova».
2. **Ipotesi registrate prima:** per ogni A/B, che cosa ci si aspetta di sentire e quale esito cambierebbe una decisione. Le ipotesi stanno in un file con hash registrato in `progress.md` prima della sessione. Il livello 2 di qualità resta un obiettivo dichiarato **non vincolante** (P-M25-03).
3. **Ordine e chiave:** l'ordine delle clip è mescolato con un seed scritto nel file della chiave; le etichette dei file non si mostrano.
4. **Apparecchiatura e volume:**
   - cuffie e diffusori annotati per modello;
   - **volume fisso** per tutta la sessione, scelto all'inizio in modo che il kick sia giudicabile (suggerimento dell'esperto);
   - cursore dell'applicazione nel mixer di Windows al **100 %**, verificato; volume di sistema annotato. Su questo PC il cursore dell'app attenua il segnale con legge quadratica (prova R-M25-12) e rende i confronti illeggibili se non è noto.
5. **Durata:** al massimo 45 minuti, con una pausa a metà.

## 3. Ordine dentro la sessione

1. **Descrizione libera prima di tutto:** l'ascoltatore ascolta il brano e scrive che cosa sente, senza domande guida, per non suggerire i termini.
2. **Scale 1–7**, le stesse del 2026-10-28, per confrontare i giudizi nel tempo:
   - **coerenza** (1 = le parti non stanno insieme, 7 = suonano come un brano unico);
   - **interesse** (1 = annoia subito, 7 = tiene l'attenzione fino alla fine);
   - **ripetitività** (1 = mai ripetitivo, 7 = ripetitivo in modo fastidioso).
3. **Istante del calo:** durante un secondo ascolto completo, l'ascoltatore segna minuto e secondo in cui l'interesse cala, se cala.
4. **Domande specifiche** (§5) e **A/B** (§4).

## 4. Confronti A/B e controlli

- **Costrutti separati.** Per ogni coppia si chiedono tre cose distinte, in quest'ordine: si sente una differenza? (rilevazione); quale preferisci? (preferenza); quale funziona meglio musicalmente, e perché? (giudizio motivato). Una preferenza senza rilevazione non conta come differenza udibile.
- **Controlli (B0), in ogni sessione:**
  - almeno una **coppia identica**, presentata come una coppia qualsiasi;
  - almeno un **duplicato nascosto** di una clip già ascoltata.
  Le risposte ai controlli si riportano **sempre** e misurano quanto è rumoroso il giudizio della sessione.
- **Rilevazione (B1)** con ABX su estratti di 8–16 battute quando serve sapere se una differenza si sente; 16–24 prove per giudizio.
- Le altre prove della batteria R-M25-7 (B2–B7) si usano solo quando una decisione le richiede, con il materiale indicato nel dossier.

## 5. Domande specifiche per l'esperto

Riprendono Q1–Q8 del [pacchetto precedente](m2_expert_listening_package.md), con queste modifiche:

| Domanda | Modifica |
|---|---|
| Q3, kick e metro | **Per sezione**: A, B e A' separatamente («il tempo forte si percepisce?»), con il punto in cui cambia |
| Q5, ripetitività e sviluppo | Dopo le scale 1–7, non prima |
| Q7, A/B | Con i costrutti separati del §4 |
| Q8, osservazioni libere | Resta alla fine; la descrizione libera iniziale del §3 è un'altra cosa |

## 6. Domande semplici per il secondo ascoltatore

Linguaggio quotidiano, nessun termine tecnico; si può rispondere «non so».

1. Che cosa hai sentito? Descrivilo con parole tue.
2. Ti è piaciuto? Da 1 a 7.
3. Ti sei annoiato? Se sì, più o meno quando?
4. Hai sentito dei cambi di «parte» nel brano? Quanti, e più o meno quando?
5. Riuscivi a battere il piede a tempo? Sempre, a tratti, mai?
6. Hai sentito rumori strani, scatti o fruscii? Quando?
7. In questa coppia, le due versioni ti sembrano uguali o diverse? Se diverse, quale preferisci?

## 7. Modulo di risposta

```text
Data e ora, sessione:
Ascoltatore (esperto / secondo ascoltatore; nessun nome nei documenti):
Apparecchiatura (cuffie / diffusori / interfaccia):
Volume di sistema e cursore dell'app (verificato al 100 %):
Build / commit e hash dei materiali:
File delle ipotesi e suo hash:
Descrizione libera:
Coerenza 1–7:      Interesse 1–7:      Ripetitività 1–7:
Istante del calo (mm:ss), se c'è:
Q1 … Q8 (esperto) oppure domande 1–7 (secondo ascoltatore):
A/B — per ogni coppia: ordine presentato, rilevazione, preferenza, giudizio motivato:
Controlli B0 — coppia identica: risposta;  duplicato nascosto: risposta:
Chiave rivelata a fine sessione:
Esito complessivo (libero, anche negativo o inconcludente):
```

## 8. Lettura dei risultati

- Si confrontano i giudizi con le ipotesi registrate prima; un esito diverso si riporta così com'è.
- Se i controlli B0 falliscono (coppia identica giudicata diversa, duplicato non riconosciuto), i confronti della sessione valgono come indicazione debole.
- Con due ascoltatori, un risultato vale per quelle persone e quelle clip; la specificità si dichiara solo con gruppi indipendenti, altrimenti `INSUFFICIENT_DATA` (dossier R-M25-7).

## 9. Limiti

- Pochi ascoltatori: il protocollo orienta le decisioni, non le valida statisticamente.
- Chi conduce conosce la chiave: l'ascolto non è in doppio cieco.
- Le scale 1–7 sono le stesse del 2026-10-28 per la confrontabilità, non perché siano validate.

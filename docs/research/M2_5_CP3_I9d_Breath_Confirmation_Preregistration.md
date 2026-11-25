# M2.5 CP-3 — I9 quater, conferma del respiro pieno. Pre-registrazione

- **Stato:** scritto il 2026-11-25, **prima** di codice ed estratti. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Origine:** [I9 ter](M2_5_CP3_I9c_N2_Revision_Results.md): (a) confermata ma duplicato incoerente, quindi serve una conferma. Solo R1 contro Y; la casa ridotta non si prova più. Nulla adottato.

## 1. Materiale

- `piece_Y` e `piece_R1` di I9 ter, senza nuovi render (seed 1337). Le due versioni differiscono solo per la melodia nelle battute 23, 55 e 87.
- **Due punti non ascoltati in I9 ter:** battuta 23 (parte 1, 46–48 s) e battuta 87 (parte 3, 164,857–166,857 s).
- **Otto estratti di 24 s:** 2 punti × 2 versioni × 2 ripetizioni. In ogni estratto la battuta comincia a un secondo scelto a caso fra 8 e 14, diverso fra le due ripetizioni di almeno 2 s: si sa che cosa cercare, non quando. Ordine casuale, mai due estratti uguali per punto e versione di seguito. Loudness alla più debole, dissolvenze di 0,5 s.

| ID | Verifica del materiale |
|---|---|
| Z1 | Y e R1 identici campione per campione negli estratti prima della battuta, a parità d'inizio (prima della pareggiatura); diversi dentro la battuta |
| Z2 | Estratti entro 0,1 LU; picco ≤ −1 dBFS |
| Z3 | Chiave separata, non stampata; nessuna etichetta nei nomi |

Z1–Z3 falliti bloccano la consegna.

## 2. Domande, per ogni estratto

1. C'è un momento in cui la musica si svuota o perde interesse? Se sì, a che secondo? (sì / no)
2. C'è un passaggio che suona forzato? Se sì, a che secondo? (sì / no)

Nessun voto: in I9 ter il rumore dei voti era almeno 3 punti.

**Conteggio:** *vuoto segnalato* se il secondo indicato cade fra 1 s prima dell'inizio della battuta e la sua fine, arrotondando al secondo. *Forzato*: stessa finestra. Indicazioni fuori finestra si riportano ma non contano.

## 3. Previsioni e regola

| ID | Previsione |
|---|---|
| Q1 | Vuoto segnalato in almeno 3 dei 4 estratti Y |
| Q2 | Vuoto segnalato in 0 dei 4 estratti R1 |
| Q3 | R1 forzato in al massimo 1 dei 4 |

- **Tutte e tre confermate:** il respiro pieno diventa **candidato** offline.
- **Q1 o Q2 mancata:** non confermato, resta Y. Non si ripete con altri estratti.
- **Q3 mancata:** la risposta di tre note si riscrive, con una nuova pre-registrazione.

Se ogni estratto avesse la stessa probabilità p di essere segnalato, a caso, Q1 e Q2 insieme avrebbero probabilità al massimo del 2,3 % (p ≈ 0,43). Il calcolo suppone risposte indipendenti, che per un solo ascoltatore è un'approssimazione.

## 4. Limiti

- Un ascoltatore che conosce l'ipotesi e sa che si tratta di due punti; un seed.
- **Battuta 87:** subito dopo (battute 88–91) il kick manca in entrambe le versioni, per la tabella d'autore. Un vuoto indicato dopo la fine della battuta non conta; se cade a cavallo, rumore in entrambe le versioni.
- Sentire un vuoto non dice se sia un difetto: il respiro può essere voluto. La conferma riguarda la percezione; tenerlo o riempirlo resta una scelta artistica del titolare, da prendere con le adozioni.

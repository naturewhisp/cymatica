# M2.5.3–4 — Contratto minimo, punto 5b: operazioni pensate per la catena. Risultati

- **Pre-registrazione:** [M2_5_3_C5b_Chain_Operations_Preregistration.md](M2_5_3_C5b_Chain_Operations_Preregistration.md), SHA-256 (LF) `b27807047c888bb6de896b3dee0ca30510bd43e4a1c3240429f68c1c0ba986b7`.
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c5b_chain_ops/README.md).
- **Decisione:** D-M2-46.

## 1. Cosa cambia

Due trasformazioni nuove:
- **ornamento di passaggio:** divide una nota e ci inserisce una nota di passaggio, o una volta superiore;
- **liquidazione:** toglie l'ultima nota non d'ancoraggio.

Entrambe hanno invarianti verificati e limiti (al massimo 8 note, durate di almeno un passo). Le frasi la cui base è il vincitore precedente usano tabelle di catena senza inversione. Il profilo di punteggio passa alla versione 2: Ornamento 180, Liquidazione 50, e gli altri valori invariati.

**Precisazione d'implementazione:** una frase dichiarata in catena ma ripartita per il limite usa la tabella della sezione, perché la sua base è il motivo della sezione. Nel profilo accade solo alla frase 17.

## 2. Esiti

| ID | Criterio | Risultato | Esito |
|---|---|---|---|
| L1 | Origini, limite, invarianza | Sì. Ripartenza solo alla 17, contro 4 nel punto 5. Test `[motif_lineage]` e `[chain_ops]` | Superato |
| L2 | Contratto dei punti 1–4 | 0 battute contraddette; 25/25 note dell'accordo sui cambi; 0 conflitti Vettore–Corpo; Vettore sopra Corpo in 187/188, 141/172 e 155/156 attacchi; R-MET-2 | Superato |
| L3 | Alternanze A-B-A ≤ 25% | **0 su 15**; con il punto 5 erano 5 su 12 | Superato |
| L4 | Genitore più vicino della mediana ≥ 75% | **14 su 15 (93%)**; con il punto 5 erano 5 su 12 | Superato |
| L5 | Più di 5 gruppi e un percorso cumulativo di 3 frasi | **14 gruppi** (prima 5); percorsi in A (1–3), in B (9–14) e in A′ (18–20) | Superato |
| L6 | Ritorno della 16 | Base = vincitore della 0; la 0 è la più vicina, **senza parità** (0 contro 0,039) | Superato |
| L7 | Picchi di Corpo alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r (con M5 e M5r ora il più alto è il 17); M5 e M5r ancora ammesse | Superato |
| L8 | Scenari | Fixture PASS con i golden aggiornati; `meter_change_v1` PASS, PCM invariato | Superato |

**Regola d'adozione:** superati L1, L2, L3, L4, L5, L7 e L8, quindi il pacchetto **si adotta**. Anche L6 è superato.

## 3. Cosa si sente nel piano

Motivi vincenti in «grado/durata» nei passi; dati in `chain_decisions_c5b.output.txt`.

- **A:** la figura `A F D E` si arricchisce a ogni frase sull'ultimo tempo: E → E F → E F F → E F F G. Poi resta ferma per tre frasi, e alla cadenza (frase 7) torna al motivo della sezione.
- **B:** tre ornamenti successivi, poi due liquidazioni (frasi 12–13), poi un nuovo ornamento. È la sequenza classica di sviluppo, liquidazione e ripresa del moto.
- **A′:** richiama l'apertura di A, riparte dopo l'ottava (frase 17) e **ripercorre il cammino di A**. Le frasi 18 e 19 sono identiche, in tutti i ruoli, alle frasi 1 e 2: i gruppi simbolici della fixture le uniscono. Poi una liquidazione e un ornamento nuovi.

È uno sviluppo derivato, cumulativo, dipendente dall'ordine e ripreso, come lo definisce il dossier. Resta da sentire se l'orecchio lo percepisce (prove B2–B5).

**Rischi:**
- gli ornamenti portano la melodia fino a 8 note per battuta, quindi più densa;
- le note ripetute (F F) nascono dalla volta superiore su note vicine;
- la vicinanza alla Trama in A′ sale a 30 note su 156.

## 4. Test e golden aggiornati, con la storia

- **Test dei punti precedenti:**
  - vincitori, riparazioni della diade, adattamenti (47), spostamenti di registro e ripartenze;
  - versione del profilo di punteggio;
  - tabelle di B (ammesse le operazioni di catena nelle frasi in catena);
  - note della fixture: 1300 → 1396;
  - gruppi simbolici: 18 → 20; per frame 19 → 20.
- **Test nuovo:** `[chain_ops]`, con le operazioni, i limiti e gli invarianti violati di proposito.

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 5749962721823883068 | 92717741569625359 |
| Piano della fixture | 0xf2ce734ee88095a7 | 0x39017615657f1fbd |
| PCM della fixture | 682170244392377824 | 16057152926442900680 |
| Piano della forma di prova | 0x62600d91a28e15e3 | 0x6497ef98eda7ac5f; cambia solo per la versione del profilo nel trace, gli eventi sono gli stessi |
| PCM di `meter_change_v1` | — | invariato |

## 5. Build e scenari

Release e Debug `/WX` 304/304, con un test nuovo, `[chain_ops]`; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.

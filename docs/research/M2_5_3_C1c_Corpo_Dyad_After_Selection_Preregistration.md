# M2.5.3–4 — Contratto minimo, punto 1, variante C1c: diade di Corpo riparata dopo la scelta. Pre-registrazione

- **Stato:** scritto il 2026-11-17, dopo C1 e C1b e prima del codice di C1c. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Sì, C1c» (2026-11-17), dopo l'[approfondimento di C1b](experiments/m25_3_c1_corpo_dyad/README.md).
  - C1b ripara tutte le battute, ma la riparazione entra nel punteggio dei candidati.
  - In A e A′ il vincitore supera il secondo dello 0,4%, quindi cambia, e con lui la melodia.
  - La melodia nuova non è più consonante, toglie un ostinato ad A e porta Vettore dentro il registro di Trama.
- **Informata dai dati:** come C1b, questa variante nasce dagli esiti precedenti sulla stessa fixture, e lo dichiaro.

## 1. Variabile

1. **Selezione invariata:** i candidati si generano, si validano e si valutano senza alcuna riparazione, come in F1. Il vincitore è quindi lo stesso di F1, con lo stesso punteggio.
2. **Riparazione del solo vincitore:** si rigenera il candidato vincente, con lo stesso indice e gli stessi parametri, applicando la regola simmetrica di C1b in ogni battuta. Se la voce alta di Corpo (terza o quinta) forma classe d'intervallo 1 o 6 con un'altezza di Vettore che attacca nella battuta, e l'altra nota dell'accordo no, le due si scambiano. La catena della condotta delle parti prosegue dal voicing riparato.
3. **Sicurezza:** il candidato riparato passa per gli stessi vincoli duri e per la simulazione delle voci. Se viola un vincolo, si tiene il vincitore non riparato e il trace lo registra. Il punteggio registrato resta quello della selezione.
4. **Trace:** due campi nuovi per frase. `corpoDyadRepairedBars` conta le battute in cui la regola ha scambiato la voce; `corpoDyadRepairRejected` dice se la riparazione è stata respinta. Entrano nell'hash canonico del piano, formato 1 → **2**, e nell'hash del trace della fixture.
5. Policy 1.0.2.0.

## 2. Predizioni

| ID | Predizione |
|---|---|
| **R1, selezione** | Vincitore, punteggio vincente e punteggi di tutti i candidati identici a F1 in tutte le 24 frasi; 0 pareggi in testa |
| **R2, riparazione** | `contradicted_bars.py`: 0 battute in ogni sezione. `role_clashes`: Corpo–Vettore ic6 = 0 e ic1 = 0 in A e A′; ic1 in B 6,2% (± 0,5) |
| **R3, cosa non cambia** | Pulso, Vettore e il basso di Corpo sono identici a F1 in tutte le 24 frasi. Corpo di B è identico. La voce alta di Corpo cambia **esattamente nelle 16 battute di Sol** di A e A′ (Si → Re) |
| **R4, contatore** | `corpoDyadRepairedBars` = 2 in ognuna delle frasi 0–7 e 16–23, e 0 in B: 32 in tutto. Dopo Sol + Re la condotta delle parti porta Mi sul La minore, e la regola lo riporta a Do. Per questo i passaggi sono due ma la voce alta, rispetto a F1, cambia in una battuta sola. Nessuna riparazione respinta |
| **R5, Trama** | Trama cambia solo nelle battute di Sol delle frasi che hanno Trama (4–7 e 16–23) |
| **R6, sezioni** | Novità di Foote su Corpo: i due picchi più alti alle frasi 9 e 17, con M4, M5 e M5r |
| **R7, scenari** | Fixture PASS con i golden di piano, trace e PCM aggiornati col motivo; `meter_change_v1` PASS in M1–M5 |

Senza predizione, descrittivi: Trama–Vettore per classe d'intervallo; i golden della forma di prova e di `meter_change_v1`.

## 3. Limiti

- La riparazione non entra nel punteggio: un candidato con una diade riparabile non è preferito né penalizzato. È voluto, per «un cambiamento per pacchetto».
- La fragilità dei margini in A e A′ (0,4%) resta: è un rilievo per il punto 6.
- «Vettore sopra Corpo in A′» resta un'ipotesi del punto 3, da dichiarare e misurare insieme alla separazione da Trama.

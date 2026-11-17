# M2.5.3–4 — Contratto minimo, punto 5: variazione derivata e ripresa. Risultati

- **Pre-registrazione:** [M2_5_3_C5_Derived_Variation_Preregistration.md](M2_5_3_C5_Derived_Variation_Preregistration.md), SHA-256 (LF) `25e61d3346e9add396e19c63003c8abc48f9eb3109833bc85aeedfd359b95911`.
- **Scelta del titolare:** «Catena con limiti».
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c5_lineage/README.md).
- **Decisione:** D-M2-45.

## 1. Cosa cambia

Ogni frase dichiara da dove prende il motivo base: il motivo della sezione, il vincitore della frase precedente, oppure il richiamo dell'apertura di una sezione. La memoria dei motivi ha capacità fissa: il vincitore precedente e l'apertura di ogni sezione.

Se il motivo richiesto si allontana dal motivo della sezione di oltre 3 gradi nel baricentro, la frase riparte dal motivo della sezione. Origine e ripartenza sono nel trace (hash canonico del piano, formato 5). Il controllo d'invarianza del motivo verifica che la base sia il vincitore dichiarato.

Il nuovo scenario `lineage_measures_v1` misura i criteri con l'implementazione ammessa di M5r.

## 2. Esiti

| ID | Criterio | Risultato | Esito |
|---|---|---|---|
| K1 | Origini come dichiarato, limite rispettato, invarianza superata | Sì. Ripartenze nelle frasi 12, 17, 21 e 22 (test `[motif_lineage]`) | Superato |
| K2 | Contratto dei punti 1–4 | 0 battute contraddette; note dell'accordo sui cambi 25/25; 0 conflitti Vettore–Corpo; Vettore sopra Corpo in 122, 87 e 125 attacchi su 128, nessun «unmet»; R-MET-2 | Superato |
| K3 | (a) più gruppi di Vettore che in C4; (b) materiale nuovo e riusato in almeno 2 sezioni | (a) 5 contro 4; (b) in tutte e 3 le sezioni | Superato, di misura |
| K4 | Genitore più vicino della mediana nel 75% delle derivate | 5 su 12 (42%) | **Non superato** |
| K5 | La 16 richiama l'apertura di A ed è la più vicina alla 0 | Base della 16 = vincitore della 0; distanza M5r 0, ma **a pari merito** con altre frasi di A | Superato, per parità |
| K6 | Picchi di Corpo alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r; M5 e M5r ancora ammesse | Superato |
| K7 | Scenari | Fixture PASS con i golden aggiornati; `meter_change_v1` PASS, PCM invariato | Superato |

**Regola d'adozione:** superati K1, K2, K3, K5, K6 e K7, quindi il pacchetto **si adotta** secondo la regola pre-registrata. K4, riportato in ogni caso, non è superato.

## 3. Lettura onesta: lo sviluppo c'è, ma è poco

La predizione qualitativa si è avverata: con l'inversione del contorno nelle tabelle, le catene **alternano due stati**.

- **In A:** le frasi 0–3 passano da `A F D E` a `D F A G` e ritorno. Prima suonavano 4 volte la stessa figura.
- **In B:** la catena fa nascere una figura nuova, `0 4 5 4` in gradi, che non esisteva in C4 e torna più avanti nella sezione. È il materiale realmente nuovo del pacchetto, ed è ciò che fa superare K3.
- **In A′:** la ripresa riparte dall'apertura di A, poi alterna di nuovo.

**K4 manca per la stessa ragione:** con l'alternanza la frase è identica alla «nonna» (n−2), non alla madre. La misura lo vede correttamente: il ricordo c'è, ma salta una generazione.

**K5 passa solo per parità:** la 16 è identica alla 0, ma anche alle altre frasi di A con la stessa figura.

In termini del dossier lo sviluppo è derivato e dipendente dall'ordine, ma poco cumulativo. Siamo al confine della varietà fittizia: due figure già note si alternano. Per uno sviluppo davvero progressivo servono operazioni pensate per la catena (ornamento, frammento, estensione), che la pre-registrazione aveva lasciato fuori. È il seguito naturale, da decidere: punto 5b qui, oppure M6, dove la specifica mette la memoria dei motivi con lignaggio.

## 4. Test e golden aggiornati, con la storia

- **Test dei punti precedenti:**
  - vincitori: la frase 10 torna al 6;
  - riparazioni della diade: anche le frasi 9, 11 e 13 di B;
  - adattamenti: 40;
  - spostamenti di registro;
  - note della fixture: 1308 → 1300;
  - gruppi simbolici: 12 → 18; per frame 16 → 19.
- **Golden:**

  | Golden | Prima | Dopo |
  |---|---|---|
  | Trace della fixture | 3614853450997319296 | 5749962721823883068 |
  | Piano della fixture (formato 5) | 0xeb6a2fd17e8f0722 | 0xf2ce734ee88095a7 |
  | PCM della fixture | 871685324087719297 | 682170244392377824 |
  | Piano della forma di prova | 0xb13b4a4e01a8fa6d | 0x62600d91a28e15e3; cambia solo per il formato 5, gli eventi sono gli stessi |
  | PCM di `meter_change_v1` | — | invariato |

## 5. Build e scenari

Release e Debug `/WX` 303/303, con un test nuovo, `[motif_lineage]`; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.

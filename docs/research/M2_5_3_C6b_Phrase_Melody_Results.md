# M2.5.3–4 — Pacchetto 6b: melodia lunga una frase. Risultati

- **Pre-registrazione:** [M2_5_3_C6b_Phrase_Melody_Preregistration.md](M2_5_3_C6b_Phrase_Melody_Preregistration.md), SHA-256 (LF) `37535ff8a836c42a6313eca457f34a4d09e2f8f7da70370c73818b5c6dab449d`.
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c6b_phrase_melody/README.md).
- **Decisione:** D-M2-48.
- **Fonte:** [note su AURA](AURA_Music_Source_Notes.md), voci A2 e A3.

## 1. Cosa cambia

In ogni battuta della frase la melodia si sposta sulla fondamentale dell'accordo, con lo spostamento ridotto all'intervallo più vicino in [−3, +3] gradi: in A e A′ 0, +3, −3, 0; in B 0, −2, −1, 0.

Il motivo resta di una battuta, e catena, trasformazioni e memoria non cambiano. La riparazione della diade e la lettura dell'accordo leggono le altezze spostate. Il campo è nel trace (hash canonico del piano, formato 6), e il controllo d'invarianza ricostruisce lo spostamento.

Prima frase di ogni sezione, come suona ora (registro e adattamenti compresi):

| Frase | Melodia |
|---|---|
| 0 (A) | `A F D E ∣ D B G A ∣ E C A B ∣ A F D E` |
| 8 (B) | `G D C D ∣ E♭ B♭ B♭ B♭ ∣ F C B♭ C ∣ G D C D` |
| 16 (A′) | `A F D E ∣ D B G A ∣ E C A B ∣ A F D E` |

La frase sale e scende con l'armonia, e alla quarta battuta torna alla figura d'apertura. Prima era la stessa battuta ripetuta quattro volte.

## 2. Esiti

| ID | Criterio | Risultato | Esito |
|---|---|---|---|
| T1 | Invarianza e trace | Controllo d'invarianza superato; campo nel trace (test `[phrase_melody]`) | Superato |
| T2 | Almeno 22 frasi su 24 con ≥ 3 battute diverse | **24 su 24**; con C6 erano 8 | Superato |
| T3 | Contratto dei punti 1–4 | Note dell'accordo sui cambi 25/25; 0 conflitti Vettore–Corpo; nessun `vettoreRegisterUnmet`; R-MET-2. Battute di Corpo contraddette: 0 | Superato |
| T4 | Sviluppo | Picchi di Corpo a 9 e 17; K4 14 su 15; gruppi di Vettore 14 come prima; vincitori invariati | Superato |
| T5 | Meno adattamenti | **8 contro 47**: la melodia parte dalla fondamentale di ogni accordo | Superato |
| T6 | Diversità | `DISTRIBUTIONS`, 58 brani distinti | Superato |
| T7 | Scenari | Fixture PASS con i golden aggiornati; forma di prova e `meter_change_v1` invariati negli eventi | Superato |

**Adozione:** tutti superati, quindi il pacchetto si adotta.

## 3. Descrittivi e rischi

- **Registro:** la gamma della melodia si allarga. In A e A′ va da 69 a 86 MIDI, cioè fino al Re6 (prima 72–81); in B da 58 a 79. Il registro resta sopra Corpo in 188/188 e 156/156 attacchi di A e A′.
- **Vicinanza alla Trama:** 15 note su 188 in A, 6 su 172 in B, 23 su 156 in A′ (prima 16, 7 e 30).
- **Diversità:** fra due seed le frasi di Corpo identiche scendono dall'83% al 70%, perché le riparazioni della diade seguono la melodia. Vettore 32%, Trama 54%, Pulso 61%.
- **Rischio da sentire:** in B la battuta su Mi♭ diventa `E♭ B♭ B♭ B♭`, con note ribattute che vengono dalla lettura dell'accordo sulla figura spostata. Le note acute di A vanno sentite (voce 15 del registro).

## 4. Golden, test, durata della suite

- **Golden aggiornati con la storia:**

  | Golden | Dopo |
  |---|---|
  | Trace della fixture | 462136604501102608 |
  | Piano della fixture (formato 6) | 0x416d343bb5dc66b9 |
  | PCM della fixture | 13522047511299799422 |
  | Piano della forma di prova | 0x2edb6f713b792cdb (solo il formato) |

- **Test aggiornati con la storia:** riparazioni della diade, adattamenti (8), spostamenti di registro, sensibilità dell'hash. Nuovo test `[phrase_melody]`.
- **Durata della suite** (segnalata dal titolare): il test di diversità che avevo aggiunto con il punto 6 ripeteva la scansione dei 64 seed già fatta dal test esistente. Ora i 64 seed danno brani diversi, quindi le distanze M2–M4 si calcolano su 2009 coppie, e quel test costava 1060 s in Debug e 110 s in Release. I suoi controlli sono passati nel test esistente, e la scansione si esegue una volta sola.
- **Restano lenti:** la loudness per stem (1393 s in Debug) e la scansione di diversità (1044 s). La distanza M4 è quadratica sulle sequenze di note: ottimizzarla è un seguito possibile, senza cambiare le misure.

## 5. Build

- Release e Debug `/WX` 307/307, dopo l'unione dei test di diversità. Prima dell'unione erano 308/308 in entrambe le configurazioni.
- `full_m2_suite` verde salvo `acao02_v1`.
- **Durata:** test Release 2 min; test Debug 22 min (23:18–23:40), fissati dal test della loudness per stem, che esegue anche la scansione di diversità. Le correzioni proposte sono un'implementazione esatta e più veloce di M4 (Myers) e niente scansione nel test della loudness. Sono da decidere.

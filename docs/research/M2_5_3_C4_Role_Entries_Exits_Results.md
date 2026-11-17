# M2.5.3–4 — Contratto minimo, punto 4: entrate e uscite dei ruoli. Risultati

- **Pre-registrazione:** [M2_5_3_C4_Role_Entries_Exits_Preregistration.md](M2_5_3_C4_Role_Entries_Exits_Preregistration.md), SHA-256 (LF) `2e371d1068fe2abda55b31e10449e3c7bfa8d5cdac3ec1c3a5942edf392d243b`.
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c4_role_masks/README.md).
- **Decisione:** D-M2-44.

## 1. Cosa cambia

`PhrasePlan` dichiara `maskPulso` accanto a `maskTrama`. Il pad di Pulso non può tacere in `Spostato`, né quando Corpo è ritardato in battuta 0. La Trama complementare, senza Pulso, suona il suo pattern intero.

Il profilo `fixture-orchestration-v1` toglie la Trama nelle frasi 10–11 e 16–19, oltre alle 0–3 di prima, e Pulso nella 22.

## 2. Esiti

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| E1 | Maschere come dichiarate; pieno organico in 13 frasi su 24 | Sì; 20 → 13 | Confermata |
| E2 | `maskPulso` in `Spostato` o con Corpo ritardato: violazione strutturale | Sì (test `[role_masks]`) | Confermata |
| E3 | In ogni battuta non `Spostato`, attacco di Pulso o Corpo sul tempo 1 | Sì, nella 22 è Corpo | Confermata |
| E4 | Nella 22 la Trama ha più attacchi | 20 → 32 | Confermata |
| E5 | Frasi 0–9, 12–15, 20–21 identiche a C3 | Sì; identica anche la 23 | Confermata |
| E6 | Vincitori, descrittivo con margine | Cambiano in 10–11 (6 → 5), 17–19 (6 → 5) e 22 (6 → 1), sempre verso un'inversione del contorno; margini 2,5–3,6% (§3) | Descrittivo |
| E7 | Picchi di novità di Corpo alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r; M5 e M5r ancora ammesse | Confermata |
| E8 | Fixture PASS con golden aggiornati; `meter_change_v1` PASS | Sì. `meter_change_v1` e la forma di prova restano **identici**: non dichiarano maschere | Confermata |

## 3. Perché cambiano i vincitori

Senza Trama, o senza Pulso, la tensione stimata della frase scende, e la scelta premia un altro candidato. È la regola fissata prima di misurare: la maschera è un dato dichiarato, e la selezione deve vederla.

I margini dei nuovi vincitori:
- 3600 punti su 100 000 nelle frasi 10–11;
- 2500 nelle 17–19;
- 1025 nella 22.

Sono più robusti dello 0,4% che decide la melodia in A e nelle altre frasi di A′. Corpo resta identico ovunque. Pulso cambia dove cambia il numero di impulsi del vincitore, e la melodia dove cambia la trasformazione.

**Test dei punti precedenti aggiornati, con la storia:** i vincitori fissati nei test di F1, del punto 1 e del punto 3; i conteggi di riparazioni, adattamenti e spostamenti; il numero di note della fixture (1392 → 1308); la partizione dei gruppi simbolici (9 → 12; per frame 14 → 16). Le regole dei punti precedenti valgono ancora:
- note dell'accordo sui tempi 1 dei cambi 25/25 in ogni sezione;
- nessun conflitto Vettore–Corpo;
- Vettore sopra Corpo in 124, 85 e 124 attacchi su 128.

## 4. Lettura

- **A′ diventa davvero una ripresa di A.** L'orchestrazione è la stessa: la Trama rientra alla quinta frase. Anche l'ostinato ha ora la stessa distribuzione di A: `A F D E` 12, `D F A G` 11, più le varianti sugli accordi. Prima A′ suonava solo `D F A G`. È un effetto non predetto, che viene dal cambio di vincitore delle frasi 17–19.
- **Vicinanza a Trama:** in A′ scende da 32 a 16 note su 128.
- **In B:** le frasi 10–11, senza Trama, hanno la melodia invertita e un'ottava sopra (+12). B ha quindi un registro che si apre a metà sezione, poi torna giù con lo spostamento metrico.
- **Il prezzo:** l'organico cambia a ogni confine di frase dichiarato, e la pausa del kick nella 22 è una scelta forte. Tutto va all'ascolto cumulativo, voce 12 del registro.

## 5. Golden e build

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 7620653259624784472 | 3614853450997319296 |
| Piano della fixture | 0x17aade206b9ac268 | 0xeb6a2fd17e8f0722 |
| PCM della fixture | 11240606778404684750 | 871685324087719297 |
| Forma di prova A→B, `meter_change_v1` | invariati | invariati |

Release e Debug `/WX` 302/302, con un test nuovo, `[role_masks]`; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.

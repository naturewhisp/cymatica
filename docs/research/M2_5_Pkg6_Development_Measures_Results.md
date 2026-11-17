# Pacchetto di misura 6 — Misure dello sviluppo: risultati

- **Pre-registrazione:** [M2_5_Pkg6_Development_Measures_Preregistration.md](M2_5_Pkg6_Development_Measures_Preregistration.md), SHA-256 (LF) `156e7a43cdd62da03047debd96344b019d5a15de63f7a52e3c549f79675c4c41`.
- **Esecuzione, 2026-11-17:** Release `development_measures_v1`; Release e Debug headless `/WX` 296/296. Dati: [esperimento](experiments/m25_pkg6_development/README.md).
- **Esito:** **M5 e M5r ammesse** per l'uso in M2.5.3–4 (V1–V5 superate). Q1–Q5 confermate, **Q6 mancata**: anche la M4 supera le prove di lignaggio e di sezione.

## 1. Validazione

| Prova | M4 (riferimento) | M5 | M5r |
|---|---|---|---|
| **V1**, cavallo F1–F6 | superata | superata | superata |
| **V2**, ranghi del genitore in F9 (3, 5, 13, 15, 21, 22) | 1, 1, 4, 1, 1, 1 → 5/6, MRR 0,875 | 1, 2, 6, 1, 1, 1 → **4/6, MRR 0,778** | 1, 1, 8, 1, 1, 2 → **4/6, MRR 0,771** |
| **V3**, sezioni di F9 (7, 11, 15, 19, 21) | picchi 21, 7, 19, 12, 14 → 5/5 | picchi 21, 7, 19, 11, 16 → **5/5** | 21, 7, 19, 11, 16 → **5/5** |
| **V4**, sezioni di Corpo nella fixture (9, 17) | 9, 17 | **9, 17** | **9, 17** |
| **V5**, ripetizione | identica | identica | identica |
| Inversioni 6 ← 1 e 14 ← 6 (descrittive) | 0,857 e 0,933 | 0,275 e 0,374 | 0,157 e 0,450 |
| F8, variogramma a lag 1 (descrittivo) | `ORDER_FREE` | `ORDER_FREE` | `ORDER_FREE` |
| **Ammessa** | riferimento, non candidata | **sì** | **sì** |

| ID | Predizione | Esito |
|---|---|---|
| Q1 | M5 e M5r superano V1 | Confermata |
| Q2 | M5r supera V2 | Confermata (4/6, 0,771) |
| Q3 | M5 supera V2 | Confermata (4/6, 0,778) |
| Q4 | Almeno una supera V3 | Confermata (entrambe 5/5) |
| Q5 | Entrambe superano V4 | Confermata |
| Q6 | La M4 non supera V2 | **Mancata** (5/6, MRR 0,875) |

## 2. Lettura

- **Correzione della lettura di M2.5.1c.** La M4 non aveva «mancato lo sviluppo» in quanto distanza: aveva mancato **il variogramma a lag 1**, cioè la prova «le frasi vicine si somigliano più del caso». Con la verità dichiarata di F9 la M4 trova i genitori (5/6) e le sezioni (5/5). In F8 il variogramma resta `ORDER_FREE` con tutte e tre le misure. Era la prova a essere inadatta: in una composizione reale una frase deriva spesso da una frase lontana (il tema), non dalla precedente, e la media a lag 1 non lo vede.
- **Le misure graduate** vedono le inversioni come vicine (0,16–0,45 contro 0,86–0,93 della M4), perché il costo cresce con l'ampiezza dello scarto d'intervallo. Non le cercavano: è un effetto collaterale, da tenere presente nell'uso.
- **M5r vede il ritmo**, quindi l'aumentazione (13 ← 1) le appare più lontana: rango 8 contro 6 di M5 e 4 di M4.
- **Uso in M2.5.3–4:** il lignaggio (rango del genitore) e la novità di Foote (sezioni) con M5 e M5r, sempre insieme alla M4 come riferimento. Il variogramma a lag 1 non si usa più per diagnosticare lo sviluppo. Nessuna misura entra nel punteggio del planner.
- **Limite principale:** un'unica verità annotata (F9), scritta da un modello. Con F7, la melodia dell'esperto annotata, la validazione si rafforza.

## 3. Codice

- `development_measures.{h,cpp}`: M5, M5r, rango del genitore, novità di Foote, picchi, scenario `development_measures_v1`.
- `ensemble_tools`: `variogramOfMatrix`, estratto da `roleVariogram` senza cambiarne i valori (rieseguito: stessi esiti M4).
- Test `[development_measures]`.

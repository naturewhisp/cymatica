# M2.5.1c — Strumento offline d'insieme e di sviluppo: risultati

- **Stato:** misure descrittive per CP-1 e CP-3. Non normativo, mai un gate, non entra nello scoring del planner.
- **Pre-registrazione:** [M2_5_1c_Ensemble_Tools_Preregistration.md](M2_5_1c_Ensemble_Tools_Preregistration.md), SHA-256 (LF) `30ac2cb52c488b2a3ba35badda4a0f116142d77dc3b61164237fe5f50113e0a7`, registrato in `progress.md` prima di scrivere il codice. Lo scenario ricalcola l'hash del file e lo confronta: `matches = true` alla prima esecuzione.
- **Esecuzione:** `cymatica_game --cacp-run ensemble_tools_v1`, manifest `artifacts/cacp/ensemble_tools_v1_manifest.json` (rigenerabile, non versionato). Codice in [`ensemble_tools.cpp`](../../engine/cacp/ensemble_tools.cpp), test in [`test_ensemble_tools.cpp`](../../tests/unit/test_ensemble_tools.cpp). Nullo di 1000 permutazioni, seed dichiarati nel codice.

## 1. Fixture avverse

V(1) = distanza media fra frasi vicine; il nullo è la banda 2,5–97,5 % sotto permutazione dell'ordine.

| ID | Fixture | Attesa | Ottenuta | V(1) | Banda del nullo | p | ρ di Spearman |
|---|---|---|---|---:|---|---:|---:|
| F1 | Loop letterale | `DEGENERATE` | `DEGENERATE` | 0,000 | [0, 0] | — | — |
| F2 | Rumore nella scala | `ORDER_FREE` | `ORDER_FREE` | 0,967 | [0,946; 0,989] | 0,545 | 0,68 |
| F3 | Sviluppo reale | `ORDERED` | `ORDERED` | 0,248 | [0,675; 0,871] | 0,001 | 1,00 |
| F4 | Sviluppo rimescolato | `ORDER_FREE` | `ORDER_FREE` | 0,812 | [0,680; 0,874] | 0,744 | 0,47 |
| F5 | Trasposizione casuale | `DEGENERATE` | **`ORDER_FREE`** | 0,337 | [0,293; 0,478] | 0,127 | 0,44 |
| F6 | Ornamenti casuali | `ORDER_FREE` | `ORDER_FREE` | 0,491 | [0,448; 0,509] | 0,694 | −0,33 |
| F7 | Controllo positivo umano | — | `NOT_RUN` | — | — | — | — |

**Criterio di cavallo (pre-registrazione §3): soddisfatto.** F1 degenera, F3 è ordinata con ρ > 0, F2 e F4 non lo sono: lo stesso materiale di F3, rimescolato, perde l'ordine. Il nullo distingue l'ordine dal contenuto.

**Predizione mancata, F5.** Avevo previsto che la M4 fosse cieca alla trasposizione. Lo è solo per la trasposizione **cromatica**: F5 traspone per gradi del modo dorico, e gli intervalli in semitoni cambiano con la posizione nel modo. La predizione era sbagliata nel ragionamento, non nella misura; la fixture non è stata ritoccata. Conseguenza d'uso: la M4 vede una trasposizione modale come materiale diverso.

**Ordinamento atteso di V(1)** (F1 = F5 = 0 < F3 < F6 < F2): rispettato per F1 < F3 < F6 < F2; F5 non vale 0 per la ragione sopra, e si colloca fra F3 e F6.

**Proprietà trovata durante i test, non prevista.** La M4 è **categorica sugli intervalli**: un intervallo diverso costa 1 qualunque sia la sua ampiezza. Una deriva graduale di altezza produce frasi tutte equidistanti, quindi la misura non vede uno sviluppo graduale delle altezze; vede bene uno sviluppo per operazioni discrete (F3). Fissata dal test «M4 is categorical on intervals». Va dichiarata in ogni uso, insieme ai limiti già noti (posizione assoluta della prima nota ignorata).

**F7 resta da fare.** Il dossier chiede un controllo positivo scritto da una persona. Formato in [pre-registrazione §2](M2_5_1c_Ensemble_Tools_Preregistration.md) e nel codice: un file JSON con 24 frasi circa di Vettore. Finché non c'è, la validazione dello strumento è parziale: le fixture F1–F6 sono scritte dall'agente che conosce la misura.

## 2. Fixture reale (seed 1337), descrittiva

Valori di V(k) per ruolo, distanza M4.

| Ruolo | Frasi con note | V(1) | V(2) | V(4) | V(8) | V(12) | V(16) | Banda del nullo a k = 16 | p a k = 16 | Classificazione | ρ |
|---|---:|---:|---:|---:|---:|---:|---:|---|---:|---|---:|
| Pulso | 24 | 0,04 | 0,09 | 0,19 | 0,24 | 0,32 | 0,48 | [0,12; 0,48] | 1,000 | `ORDERED` | 1,00 |
| Corpo | 24 | 0,11 | 0,17 | 0,31 | 0,64 | 0,47 | **0,05** | [0,16; 0,55] | 0,003 | `ORDERED` | 0,73 |
| Trama | 20 | 0,06 | 0,13 | 0,28 | 0,40 | 0,40 | 0,50 | [0,12; 0,58] | 0,895 | `ORDERED` | 0,89 |
| Vettore | 24 | 0,11 | 0,22 | 0,49 | 0,69 | 0,42 | 0,47 | [0,22; 0,73] | 0,457 | `ORDERED` | 0,52 |

Lettura, senza soglie:
- Tutti i ruoli sono `ORDERED` a lag 1: le frasi vicine si somigliano più del caso. Coerente con sezioni omogenee di 8 frasi.
- **Corpo** ha la firma del ritorno descritta dal dossier: distanza che cresce fino a V(8) = 0,64 (A contro B) e cade a V(16) = 0,05 (A contro A′), sotto il nullo. Il ritorno A → A′ di Corpo è visibile.
- **Vettore** non mostra un ritorno oltre il caso a lag 16 (p = 0,457): la ripresa A′ usa ricette diverse da A (`RepriseAPrime`), e la M4 le vede come materiale diverso.
- **Pulso** a lag 16 sta al bordo alto del nullo: in A′ il pattern di Pulso differisce da A più di quanto farebbe una coppia a caso.
- Su 24 frasi i dati sono pochi: è una diagnosi, non una prova di percezione.

## 3. Statistiche verticali, griglia di sedicesimi

| Sezione | Onset Pulso/Corpo/Trama/Vettore | Coincidenze non nulle | Trama: note dell'accordo | Vettore: note dell'accordo sul battere | Vettore: altrove |
|---|---|---|---|---|---|
| A | 160 / 64 / 80 / 128 | Pulso–Vettore 64, Corpo–Vettore 32, Pulso–Corpo 16 | 80 su 80 | 24 su 32 | 48 su 96 |
| B | 128 / 64 / 160 / 128 | Trama–Vettore 64, Corpo–Vettore 32, Corpo–Trama 31 | 160 su 160 | 24 su 32 | 40 su 96 |
| A′ | 128 / 64 / 160 / 128 | Trama–Vettore 64, Corpo–Vettore 32, Corpo–Trama 31 | 160 su 160 | 24 su 32 | 48 su 96 |

- Pulso e Trama non coincidono mai: è la regola del planner (Trama salta gli step di Pulso).
- Trama suona solo note dell'accordo; Vettore sul battere ne suona 3 su 4, altrove circa la metà.
- In A Pulso coincide con Vettore 64 volte, in B e A′ mai: la rotazione di Pulso cambia fra le sezioni.

Sostituisce `vertical_check.py`, che leggeva un CSV non versionato; i numeri non sono stati confrontati riga per riga con quello script.

## 4. Costo logico dei progetti (per frase)

| Progetto | Formula | Valutazioni di passo | Parametri |
|---|---|---:|---|
| A, attuale | C × B × S | 512 | dichiarati |
| B, sequenziale con lettura | C × R × B × S | 2.048 | dichiarati |
| D, fascio per ruolo | L × K × R × B × S | 4.608 | L = 3, K = 6 **ipotetici** |
| G, revisione a K passaggi | K × R × C × B × S | 4.096 | K = 2 **ipotetico** |

L'unità è lavoro logico, non tempo. Moltiplicare D e G per B × S è una scelta nostra per rendere confrontabili le formule del dossier. Ordine di grandezza per CP-3, non un budget.

## 5. Correzioni dopo la review (2026-11-12)

- **Pre-registrazione fail-closed (DIF-M2.5.1c-STEP4-01):** se il file manca o l'hash differisce, lo scenario è `INCONCLUSIVE` ed esce con codice 1; il manifest resta per diagnosi. Prima bastava il campo `matches`, da controllare a mano.
- **F7 senza note (DIF-M2.5.1c-STEP4-02):** un controllo con meno di 2 frasi con note viene rifiutato, perché nessuna distanza è calcolabile; F7 è `MEASURED` solo se il variogramma è calcolabile. Non è una soglia di qualità: le circa 24 frasi restano un requisito delle istruzioni.
- **p = 0,003 del ritorno di Corpo (OSS-M2.5.1c-STEP4-01):** è descrittivo e non corretto per i confronti multipli fra ruoli e lag.

Le misure di F1–F6 e della fixture reale non cambiano.

## 6. Controlli umani e di modello: F8 e F9 (2026-11-12)

Predizioni registrate prima dell'acquisizione nell'[aggiunta F8/F9](M2_5_1c_Ensemble_Tools_Preregistration_Addendum_F8_F9.md) (`ORDERED` a lag 1 per entrambi); provenienza e conversione in [controls/README](experiments/m25_ensemble_tools/controls/README.md).

| ID | Fonte | Frasi | Attesa | Ottenuta | V(1) | Banda del nullo | p | ρ |
|---|---|---:|---|---|---:|---|---:|---:|
| F8 | Bach, BWV 1002, Double della Borea (umano) | 17 | `ORDERED` | **`ORDER_FREE`** | 0,821 | [0,798; 0,866] | 0,205 | 0,29 |
| F9 | Subagente alla cieca (non umano) | 24 | `ORDERED` | **`ORDER_FREE`** | 0,878 | [0,855; 0,947] | 0,119 | 0,29 |

**Entrambe le predizioni mancano.** Le distanze M4 stanno fra 0,8 e 0,95 a ogni lag, per entrambi: due frasi reali differiscono quasi sempre nella maggior parte degli intervalli, e la M4, categorica, le vede tutte quasi ugualmente lontane. In F9 la frase 21 è identica alla frase 1 secondo la nota d'intento, ma una sola coppia identica non sposta la media a lag 20.

**Conseguenza, secondo la lettura fissata nell'aggiunta.** Il criterio di cavallo su F1–F4 resta soddisfatto, ma le fixture sintetiche erano troppo facili: lo sviluppo di F3 procede per una modifica alla volta, esattamente ciò che la M4 conta. Su musica reale, umana o di modello, il variogramma M4 non distingue uno sviluppo dal caso. **La M4 non va usata per diagnosticare lo sviluppo musicale.** Resta descrittiva per ripetizioni letterali e ritorni identici, come quelli di Corpo nella fixture del gioco, il cui materiale si ripete esattamente.

Non si è ritoccata né la misura né la conversione. Una misura diversa (per esempio graduata sugli intervalli, o la M3 per punti già esistente) richiede una nuova pre-registrazione e va provata sugli stessi controlli F1–F9: proposta per CP-3, non adottata.

## 7. Limiti

- F7 non eseguita: la validazione contro un controllo umano manca.
- Una sola distanza (M4), con due limiti dichiarati: categorica sugli intervalli, cieca solo alla trasposizione cromatica.
- Nessun ascolto. Le misure non dicono se lo sviluppo si sente.

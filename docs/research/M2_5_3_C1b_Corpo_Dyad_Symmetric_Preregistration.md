# M2.5.3–4 — Contratto minimo, punto 1, variante C1b: diade di Corpo simmetrica. Pre-registrazione

- **Stato:** scritto il 2026-11-17, dopo la misura di C1 e prima del codice di C1b. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Origine:** la [pre-registrazione di C1](M2_5_3_C1_Corpo_Dyad_Preregistration.md) e il suo esito, che si registra come negativo prima di questo documento. La regola di C1 riparava solo la terza. Le 16 battute contraddette restano 16, spostate dall'accordo di Sol (Si contro il Fa di Vettore, tritono) a quello di La minore (Mi contro Fa, seconda minore, 8,0 s come prima). Il motivo è la condotta delle parti: dopo Sol + Re, sulla battuta seguente la voce più vicina è la quinta Mi. Dati: [esperimento C1](experiments/m25_3_c1_corpo_dyad/).
- **Questa variante nasce dall'esito di C1**, quindi non è indipendente dai dati. Lo dichiaro: le predizioni sotto valgono per la fixture, che ha già guidato la scelta. La loro forza è la verifica della regola, non una scoperta.

## 1. Variabile

Sostituisce la regola di C1. Per ogni battuta di ogni candidato, qualunque sia la politica di voicing:
- si calcola il voicing di Corpo come oggi;
- se la voce alta di Corpo (terza o quinta) forma classe d'intervallo 1 o 6 con l'altezza di almeno una nota di Vettore che attacca nella battuta, **e se l'altra voce possibile (quinta o terza) non la forma con nessuna**, Corpo scambia la voce alta con l'altra, e il ruolo superiore riceve quella lasciata.

La catena della condotta delle parti prosegue dal voicing riparato. Policy 1.0.2.0. La regola vale anche per i candidati `RootFifth`: in quelle battute la politica diventa una preferenza, non un obbligo, e lo dichiaro.

## 2. Predizioni

| ID | Predizione |
|---|---|
| **Q1, riparazione** | Nessuna battuta della fixture ha la voce alta di Corpo in classe 1 o 6 con Vettore (`contradicted_bars.py`: 0 in ogni sezione). In `role_clashes`, Corpo–Vettore ic6 = 0 in A e A′; Corpo–Vettore ic1 = 0 in A e A′ |
| **Q2, il limite** | Corpo–Vettore ic1 in B resta 6,2% (± 0,5): è col basso Mib, spetta al punto 2 |
| **Q3, basso** | Il basso di Corpo resta identico in ogni frase, rispetto a F1 |
| **Q4, vincitori** | Almeno 20 frasi su 24 mantengono il vincitore di F1. Nessun pareggio in testa |
| **Q5, sezioni** | Novità di Foote su Corpo: i due picchi più alti alle frasi 9 e 17, con M4, M5 e M5r |
| **Q6, scenari** | Fixture PASS con i golden aggiornati; `meter_change_v1` PASS in M1–M5, golden del PCM aggiornato se cambia |

Senza predizione, descrittivi: Trama–Vettore per classe d'intervallo; il numero di battute in cui cambia la voce alta.

## 3. Limiti

- Come C1, la regola guarda le altezze e non le durate.
- Se entrambe le voci contraddicono, nulla cambia: è il caso del punto 2 (Vettore che legge l'accordo).
- La variante è informata dai dati della fixture. Una verifica indipendente servirà con le forme di seed diversi del punto 6.

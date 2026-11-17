# M2.5.3–4 — Contratto minimo, punto 3: `dominantRole` guida il registro. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Melodia sopra Corpo» (2026-11-17), dopo la [diagnosi in `progress.md`](../progress.md), fra due alternative: questa e «A′ come A».
- **Regole:**
  - R-ROL-1: `dominantRole` deve guidare la generazione;
  - R-REG-1 e R-REG-3: melodia sopra, riempimento in mezzo, basso in basso, salvo eccezione dichiarata.

  Fonte: [note sulle regole d'insieme](experiments/m25_f5_research_notes/regole_insieme.md).
- **Decisioni collegate:**
  - D-M2-41 e D-M2-42: riparazioni del solo vincitore, dopo la scelta;
  - D-M2-39: policy 1.0.2.0.

## 1. Diagnosi (piano `fc07a98`)

`dominantRole` vale Vettore in ogni frase e non è mai letto. Le note di Vettore stanno sopra la voce alta di Corpo che suona al loro attacco in 24 casi su 128 in A, 71 su 127 in B e 0 su 128 in A′. In A′ il registro (MIDI 48–57) viene dal vincitore OctaveShift −1, scelto per un margine dello 0,4%.

## 2. Variabile

Nel solo vincitore, dopo la scelta, la diade e la lettura dell'accordo, e solo se `phrase.dominantRole == Vettore`:

- si conta la quota di note di Vettore **strettamente sopra** la voce alta di Corpo che suona al loro attacco;
- si prova lo spostamento di tutta la Vettore della frase di 0, poi +12, poi +24 semitoni. Si tiene **il primo con cui la quota è almeno la metà**. Se nessuno basta, si tiene +24 e il trace lo segnala;
- lo spostamento cambia solo l'ottava: classi di altezza, attacchi e durate restano invariati. Il trace registra lo spostamento in `vettoreRegisterShift`, che entra nell'hash canonico del piano (formato 3 → **4**) e nell'hash del trace. Il controllo d'invarianza del motivo lo ricostruisce.

Ruoli diversi da Vettore come dominante non sono trattati: nessuna frase li dichiara.

**Caso limite dichiarato:** nella frase 8 la quota a spostamento 0 è esattamente 8 su 16. Con la soglia «almeno metà» la frase resta dov'è; con una maggioranza stretta salirebbe di un'ottava. Ho scelto «almeno metà» prima di misurare l'effetto: così B resta tutta nello stesso registro.

## 3. Predizioni

Calcolate sul piano `fc07a98` con la stessa regola applicata agli intent esportati.

| ID | Predizione |
|---|---|
| **D1, spostamenti** | +12 in ognuna delle frasi 0–7, 0 nelle 8–15, +24 nelle 16–23 |
| **D2, sopra Corpo** | A 124 / 128 (4 uguali, 0 sotto); B 71 / 127 (invariata); A′ 120 / 128 (8 uguali, 0 sotto) |
| **D3, registri** | Vettore 72–81 in A e in A′, 60–67 in B |
| **D4, cosa non cambia** | Selezione, Pulso, Corpo e Trama identici; in Vettore cambiano solo le ottave (classi di altezza, attacchi e durate invariati) |
| **D5, vicinanza alla Trama** | Note di Vettore entro 2 semitoni da una nota di Trama che suona insieme: da 0 a 16 in A, da 0 a 32 in A′, 3 in B |
| **D6, sviluppo** | Le distanze M5 di Vettore fra le frasi restano identiche, perché gli intervalli non cambiano. La novità di Foote su Corpo ha ancora i picchi alle frasi 9 e 17 |
| **D7, scenari** | Fixture PASS con i golden aggiornati col motivo; `meter_change_v1` PASS in M1–M5 |

**Descrittivo:** spostamenti scelti in `meter_change_v1` e nella forma di prova.

## 4. Rischi e limiti

- **Salti di registro ai confini:** al confine A→B la melodia scende di un'ottava (da 72–81 a 60–67), e a B→A′ risale. Il passaggio A→B è già giudicato brusco: il salto potrebbe peggiorarlo, o renderlo più leggibile. Va all'ascolto cumulativo.
- **Mascheramento e separazione da Trama:** la vicinanza misurata è solo simbolica. La separazione percettiva si misura con gli strumenti di mascheramento (pacchetto 4) sul render, come descrittivo.
- **Soglia:** «almeno metà» è un valore iniziale d'autore, non derivato da una fonte.

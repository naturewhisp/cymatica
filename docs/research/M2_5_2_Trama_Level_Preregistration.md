# M2.5.2 — Livello della Trama: pre-registrazione del criterio

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura nuova. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Perimetro:** chiusura di M2.5.2 (D-M2-32), verifica autonoma secondo D-M2-30. Il titolare ha chiesto il criterio il 2026-11-17 («Sì, preparalo»).
- **Fonti:**
  - DESIGN §8.3: la Trama è «densità e atmosfera», «pattern secondari», quindi presente ma subordinata. DESIGN §8.4: Vettore è la «direzione espressiva». DESIGN §8.2: Corpo è «massa e struttura»;
  - [pacchetto 4b](M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Results.md): perdita della Trama 0,85–0,92 a 0 dB, contro 0,73–0,84 di Corpo e Vettore (pacchetto 4), e incremento crescente con il livello.

## 1. Criterio

**Scala:** offset sul bus della Trama g ∈ {0, +2, +4, +6, +8} dB, override offline (pacchetto 4b). Si misura su ciascuno dei tre estratti (A: frasi 5–6; B e A′: frasi 3–4), con MoSQITo 1.2.1, la stessa calibrazione e lo stesso mono del pacchetto 4. Per ogni g e per ogni sezione si calcolano, per **Trama, Corpo e Vettore**:
- perdita per mascheramento: 1 − ΔN / N(ruolo da solo);
- incremento ΔN = N(mix) − N(mix senza il ruolo), entrambi al livello g della Trama.

Il valore si sceglie così:

1. **Presenza:** perdita della Trama ≤ max(perdita di Corpo, perdita di Vettore) nella stessa sezione e allo stesso g, cioè non più coperta del ruolo tonale più coperto.
2. **Subordinazione:** ΔN della Trama < ΔN di Corpo **e** < ΔN di Vettore nella stessa sezione e allo stesso g, cioè contribuisce al mix meno della struttura e della voce espressiva.
3. **Sicurezza:** riduzione del limiter 0 dB e loudness del mix non oltre +0,5 LU rispetto a g = 0.

**Scelta:** il **g più piccolo** che soddisfa 1–3 **in tutte e tre le sezioni**. Se g = 0 li soddisfa già, il livello resta −9 dB. Se nessun g li soddisfa tutti, il livello resta −9 dB e la scelta va all'ascolto cumulativo, con i valori misurati.

Pulso è escluso: è percussivo, non tonale, e il suo livello è già deciso (D-M2-31).

## 2. Validazione

Lo strumento è già validato: pacchetto 4 per L1–L5, pacchetto 4b per K1–K3. In più:

| ID | Prova | Esito atteso |
|---|---|---|
| **V1** | Gli stem di Corpo e Vettore da soli sono identici a ogni g | Hash del PCM uguali (l'override tocca solo la Trama) |
| **V2** | A g = 0 i valori di Trama, Corpo e Vettore coincidono con quelli del pacchetto 4b e del pacchetto 4 per B e A′, entro 0,005 sone | Stessi WAV di partenza, stesso strumento |

## 3. Predizioni

| ID | Predizione |
|---|---|
| **T1** | A g = 0 la presenza (1) **non** è soddisfatta in nessuna sezione |
| **T2** | Il g scelto è +2, +4 o +6 dB |
| **T3** | Al g scelto la subordinazione (2) è soddisfatta con margine: ΔN della Trama ≤ metà del minore fra quelli di Corpo e Vettore |

## 4. Adozione

Se il criterio sceglie g > 0: `kTramaGain` = −9 + g dB, golden PCM aggiornato con il motivo, e le stesse verifiche di regressione dei passi precedenti (piano, trace ed eventi invariati; Pulso, Corpo e Vettore identici prima del bus; limiter 0 dB).

## 5. Limiti

- La perdita include la compressione della loudness nella stessa banda (pacchetto 4). Il criterio è **relativo** fra ruoli proprio per questo: i confronti reggono meglio dei valori assoluti.
- «Presente ma subordinata» è la nostra traduzione di DESIGN §8.3, non una norma: è dichiarata qui prima di misurare e resta verificabile all'ascolto cumulativo.

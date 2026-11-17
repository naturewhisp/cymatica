# M2.5.2 — Livello della Trama: risultati

- **Pre-registrazione:** [M2_5_2_Trama_Level_Preregistration.md](M2_5_2_Trama_Level_Preregistration.md), SHA-256 (LF) `00a0680ce0fb382d6cd0025b10d1cfc90f41dd4f69e1cef0cb08b6ebd5e1a2b9`.
- **Esecuzione, 2026-11-17:** Release `stem_export_v3`: golden PCM PASS a 0 dB, nessuna riduzione del limiter a nessun offset, V1 confermata. Poi `trama_level_criterion.py`. Dati: [esperimento](experiments/m25_2_trama_level/README.md).
- **Esito:** **nessun offset soddisfa il criterio in tutte e tre le sezioni.** Per la regola pre-registrata il livello **resta −9 dB** e la scelta va all'**ascolto cumulativo**, con i valori misurati qui sotto. Il suono di produzione non cambia.

## 1. Misure

Perdita per mascheramento (Trama / Corpo / Vettore) e controlli:

| Offset | A | B | A′ | Mix (BS.1770) |
|---:|---|---|---|---|
| 0 | 0,865 / 0,778 / 0,794 | 0,916 / 0,822 / 0,731 | 0,848 / 0,758 / 0,843 | 0 |
| +2 | 0,830 / 0,780 / 0,797 | 0,891 / 0,823 / 0,737 | **0,810** / 0,763 / 0,845 | +0,11…+0,12 LU |
| +4 | **0,791** / 0,783 / 0,801 | 0,862 / 0,825 / 0,743 | 0,768 / 0,769 / 0,846 | +0,28…+0,29 LU |
| +6 | 0,749 / 0,786 / 0,805 | **0,827** / 0,828 / 0,750 | 0,723 / 0,774 / 0,848 | **+0,54…+0,56 LU** |
| +8 | 0,704 / 0,789 / 0,809 | 0,788 / 0,830 / 0,757 | 0,677 / 0,778 / 0,850 | +0,91…+0,95 LU |

In grassetto, il primo offset in cui la presenza è soddisfatta in quella sezione.
- **Presenza** (Trama non più coperta del ruolo tonale più coperto): A da +4, B da +6, A′ da +2.
- **Subordinazione** (incremento della Trama minore di quelli di Corpo e Vettore): soddisfatta a tutti gli offset. Per esempio a +6 dB in B la Trama dà 1,38 sone, contro 4,20 di Corpo e 6,67 di Vettore.
- **Sicurezza** (mix ≤ +0,5 LU): soddisfatta fino a +4, superata da +6.

**Conflitto:** in B la presenza richiede +6 dB, e +6 dB supera di poco il limite di sicurezza (+0,54 LU). In A e A′ basterebbero +4 dB.

## 2. Predizioni e prove

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| T1 | A 0 dB la presenza manca in tutte le sezioni | Manca in A, B e A′ | Confermata |
| T2 | Il g scelto è +2, +4 o +6 | Nessun g scelto | **Mancata** |
| T3 | Al g scelto, subordinazione con margine | Non applicabile (nessuna scelta). A +4 e +6 l'incremento della Trama resta sotto la metà del minore fra Corpo e Vettore in ogni sezione | Non valutabile |
| V1 | Corpo e Vettore identici a ogni offset | Identici (hash) | Confermata |
| V2 | A 0 dB, valori uguali al pacchetto 4b entro 0,005 sone | Trama da sola identica; incremento entro 0,0003; loudness del mix **fino a 0,015 sone** di scarto | **Mancata in parte**: fra le due misure il kick è cambiato (F4b, Pulso one-shot), e la pre-registrazione non ne teneva conto |

## 3. Lettura

- La regola ha funzionato come doveva: non ha scelto un valore che violasse uno dei vincoli fissati prima. Il limite di +0,5 LU sulla loudness del mix era arbitrario, ed è proprio lui a bloccare +6 dB. Cambiarlo ora sarebbe adattarlo all'esito: non lo si fa.
- **Per l'ascolto cumulativo** si porta una scala già misurata: −9 dB (attuale), −5 dB (+4: presente in A e A′, mix +0,29 LU) e −3 dB (+6: presente ovunque, mix +0,55 LU). La domanda: la Trama si sente come «densità e atmosfera» presente ma secondaria (DESIGN §8.3)?
- In alternativa il titolare può decidere un criterio diverso, per esempio un limite di sicurezza più largo o una presenza misurata sul brano intero, **con una nuova pre-registrazione**, non adattando questa.

## 4. Codice

- `stem_export_v3` (scenario e manifest con la loudness BS.1770 degli estratti).
- Release headless `/WX` 293/293.

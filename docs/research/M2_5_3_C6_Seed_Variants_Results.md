# M2.5.3–4 — Contratto minimo, punto 6: il seed sceglie fra alternative d'autore. Risultati

- **Pre-registrazione:** [M2_5_3_C6_Seed_Variants_Preregistration.md](M2_5_3_C6_Seed_Variants_Preregistration.md), SHA-256 (LF) `d8f36b7e1957c8409913f91d8d4f230c1dc9373a09793d422aabd6a05ba74009`.
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c6_seed_variants/README.md).
- **Decisione:** D-M2-47.
- **Fonte della cella melodica:** [note su AURA](AURA_Music_Source_Notes.md), voce A1.

## 1. Cosa cambia

- **Tabella `fixture-variants-v1`:** 5 dimensioni, 256 combinazioni.
  - frase d'entrata della Trama in A, ripresa in A′;
  - inizio dello spostamento metrico in B;
  - pausa del kick in A′;
  - motivo d'apertura di A, attuale o cella di AURA;
  - motivo d'apertura di B, attuale o cella di AURA.
- **Estrazione:** il seed sceglie ogni dimensione sullo stream `Director`, separato dai pareggi.
- **Uso:** la fixture di riferimento, le misure e i golden restano sul profilo d'autore. Lo scenario di diversità pianifica ogni seed sul suo percorso.
- **Cella di AURA:** `0 · 0 2 4 · 3 2` in ottavi; in re dorico `D · D F A · G F`, in sol eolio `G · G B♭ D · C B♭`.

## 2. Esiti

| ID | Criterio | Risultato | Esito |
|---|---|---|---|
| S1 | Riferimento invariato | Golden di piano, trace e PCM della fixture, della forma di prova e di `meter_change_v1` invariati: nessun golden aggiornato | Superato |
| S2 | Determinismo e isolamento | Stesso seed, stessa variante; estrazione su `Director`; i pareggi restano su `MusicForm` (test `[seed_variants]`) | Superato |
| S3 | Le 256 varianti sono sicure | Tutte senza fallback, senza violazioni del piano, senza `vettoreRegisterUnmet`, con l'àncora sul tempo 1 in ogni battuta non spostata | Superato |
| S4 | Diversità sui seed da 1 a 64 | `DISTRIBUTIONS`; **58 contenuti distinti** (prima 1) e **58 percorsi distinti** (attesi circa 57) | Superato |
| S5 | La cella di AURA compare | Apertura di A con la cella in 31 seed, di B in 43 | Superato |
| S6 | Build | Release e Debug `/WX` 307/307; `full_m2_suite` verde salvo `acao02_v1` | Superato |

**Adozione:** tutti i criteri superati, quindi il pacchetto si adotta.

## 3. Misure descrittive

| Ruolo | Frasi identiche fra due seed (M1) | Distanza di edit M4, mediana |
|---|---|---|
| Vettore | 32% | 0,63 |
| Trama | 55% | 0,50 |
| Pulso | 61% | 0,30 |
| Corpo | **83%** | 0,06 |

- Coppie di seed con lo stesso brano: 0,35%. Distanza delle caratteristiche M2: mediana 0,12.
- **Lettura:** la varietà è soprattutto nella melodia e nell'orchestrazione. **L'armonia è quasi la stessa per tutti i seed:** il giro d'accordi non è nella tabella. È il limite dichiarato, e la prossima dimensione da aggiungere: per esempio il giro i–VI–iv–V di AURA, voce A5, con la decisione sulla sensibile.
- Che i piani siano diversi non dice che i brani siano percepiti come diversi: lo dirà l'ascolto, voce 14 del registro.

## 4. Build

Release e Debug `/WX` 307/307, con 3 test nuovi `[seed_variants]` (2 del planner e 1 di diversità) e senza aggiornamenti di golden; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.

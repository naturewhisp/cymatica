# Spike R-M25-9: palette chiptune 8-bit, offline. Risultati

- **Pre-registrazione:** [M2_5_Chiptune_Palette_Spike_Preregistration.md](M2_5_Chiptune_Palette_Spike_Preregistration.md), SHA-256 (LF) `ed096c32c2eff2a51c8de706a5a139086197df8d4c29dbf166f1abb9dadae85e`.
- **Esecuzione:** 2026-11-18.
- **Dati:** [esperimento](experiments/m25_chiptune_spike/README.md).
- **Natura:** spike; il suono di produzione non cambia, la decisione resta a CP-3 dopo l'ascolto cumulativo.

## 1. Cosa c'è

- **Interruttore solo offline:** `AudioEngine::setTimbrePalette(TimbrePalette::Chiptune8Bit)`, rifiutato dai motori con dispositivo, con la stessa regola degli altri override offline.
- **Timbri:**
  - Pulso: triangolo con la stessa discesa di frequenza, click di rumore senza filtro;
  - Corpo: triangolo sotto 165 Hz, impulso al 25% sopra, passa-basso a 6 kHz;
  - Trama: impulso al 12,5%, passa-basso a 8 kHz;
  - Vettore: impulso con PWM, ciclo 50% ± 15% a 0,7 Hz, glide e vibrato invariati, passa-basso a 8 kHz.
- **Forme d'onda:** gli impulsi sono la differenza di due denti di sega PolyBLEP, quindi limitati in banda.
- **Scenario:** `chiptune_ab_v1` rende la fixture con le due palette, misura loudness per stem, true peak e limiter, misura la riconoscibilità dei ruoli con il pacchetto 5 e scrive le clip.

## 2. Esiti

| ID | Criterio | Risultato | Esito |
|---|---|---|---|
| X1 | Produzione intatta | Tutti i golden invariati; Release e Debug verdi (§4) | Superato |
| X2 | Solo offline, percorso di produzione identico | Test `[chiptune_palette]`: palette esplicita «produzione» identica al default in tutti i ruoli; chiptune diverso, finito e non nullo; valori non validi rifiutati | Superato |
| X3 | Sicurezza | Render chiptune finito e identico se ripetuto; true peak −5,5 dBTP (produzione −6,2); riduzione del limiter 0 dB | Superato |
| X4 | Loudness dopo la calibrazione | Stem: Pulso −0,16, Corpo −0,30, Trama −0,19, Vettore +0,24 LU; mix −0,003 LU | Superato |
| X5 | Ruoli riconoscibili (≥ 0,75 per coppia) | Chiptune 1,00 / 1,00 / 1,00 (Trama–Corpo, Trama–Vettore, Corpo–Vettore); produzione 1,00 / 1,00 / 0,92 | Superato |
| X6 | Clip | 7 clip: A, B, A′ in produzione e chiptune allineato; brano intero chiptune | Superato |

**Calibrazione, come dichiarata:**
- passaggio 1 a 0 dB: stem chiptune più bassi di 1,66, 1,70, 6,19 e 1,76 LU;
- costanti: +1,5, +1,5, +6,0 e +2,0 dB;
- passaggio 2: entro ± 0,31 LU.

La Trama chiptune richiede +6 dB perché l'impulso al 12,5% ha poca energia rispetto alla FM.

## 3. Lettura

- **I ruoli restano riconoscibili:** su note isolate le forme d'onda diverse (triangolo, impulsi al 25%, al 12,5% e con PWM) distinguono i ruoli quanto in produzione, o di più. Era il criterio di R-M25-9 più a rischio, perché tutti i ruoli diventano onde pulsanti.
- **Cosa non dice:**
  - la misura è su note isolate, non nel mix;
  - non dice se il chiptune piaccia, né se suoni «generico», che è l'errore da evitare secondo DESIGN §12.1.

  Lo dirà l'ascolto cumulativo, voce 16 del registro.
- **Per CP-3:** questo spike mostra che il motore può cambiare palette senza toccare piano, eventi e mix. È la base tecnica per una palette per archetipo (M7).

## 4. Build

- Release e Debug `/WX` 310/310, con un test nuovo, `[chiptune_palette]`, e senza aggiornamenti di golden.
- `full_m2_suite` verde salvo `acao02_v1`, il limite noto.

## 5. Primo riscontro, da un modello (non umano)

Il titolare ha fatto ascoltare le clip A a Gemini. La verifica con la misura è in `progress.md`, in sintesi:
- confermato che il click di rumore del kick chiptune si sente come un rullante (+11 dB sopra 6 kHz);
- confermato che il kick di produzione si sente poco;
- non confermati i «raddoppi d'ottava» (note identiche) e il «mix più denso nelle medie» (medie uguali);
- proposta da tenere: alternare le palette secondo la sezione, voce A4 delle note su AURA.

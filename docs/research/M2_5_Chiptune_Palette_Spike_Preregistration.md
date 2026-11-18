# Spike R-M25-9: palette chiptune 8-bit, offline. Pre-registrazione

- **Stato:** scritto il 2026-11-18, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Sì, chiptune prima dell'ascolto cumulativo» (2026-11-17); R-M25-9 in ADR-0007, aggiornamento dello stesso giorno.
- **Natura:** spike limitato. Il suono di produzione non cambia. Il confronto serve all'ascolto cumulativo e a CP-3, che decide l'obiettivo della palette.
- **Riferimenti:**
  - DESIGN §12.1, archetipo Sintetico/Matriziale: «timbri digitali con dettaglio analogico», evitando il generico;
  - [note su AURA](AURA_Music_Source_Notes.md): onde pulsanti e PWM del titolare.

## 1. Variabile

Un interruttore **solo offline**, `AudioEngine::setTimbrePalette(TimbrePalette::Chiptune8Bit)`, con la stessa regola degli altri override offline: è rifiutato da un motore con dispositivo fisico. Cambia solo la forma d'onda e il filtro di ciascun ruolo. Restano uguali piano, eventi, inviluppi, panoramica, bus e master.

| Ruolo | Produzione | Chiptune 8-bit |
|---|---|---|
| Pulso | sinusoide con discesa di frequenza e click filtrato | triangolo con la stessa discesa; click di rumore senza filtro, come il canale di rumore di una console |
| Corpo | dente di sega PolyBLEP filtrato | sotto 165 Hz triangolo (basso); sopra, impulso al 25% PolyBLEP, passa-basso fisso a 6 kHz |
| Trama | FM a 2 operatori, 1:1 | impulso al 12,5% PolyBLEP, passa-basso fisso a 8 kHz |
| Vettore | dente di sega PolyBLEP con glide e vibrato | impulso PolyBLEP con PWM (ciclo 50% ± 15%, LFO a 0,7 Hz), glide e vibrato invariati, passa-basso fisso a 8 kHz |

Il passa-basso fisso è il «dettaglio analogico»: smussa gli spigoli senza togliere il carattere.

**Calibrazione della loudness, dichiarata qui:**
1. si rende la fixture con la palette chiptune a guadagno 0 dB;
2. per ogni ruolo il guadagno di compensazione è la differenza di loudness integrata dello stem rispetto alla produzione, arrotondata a 0,5 dB, e diventa una costante del codice;
3. si rende di nuovo e si verifica.

La compensazione fa sì che il confronto riguardi il timbro e non il volume, come nella scelta del rapporto FM (D-M2-33).

## 2. Criteri

| ID | Criterio | Superato se |
|---|---|---|
| **X1, produzione intatta** | Senza override | Tutti i golden invariati; Release e Debug verdi |
| **X2, solo offline** | Interruttore | Rifiutato da un motore con dispositivo; senza override il PCM della fixture coincide con il golden |
| **X3, sicurezza** | Render chiptune della fixture | Nessun campione non finito; render ripetuto identico; true peak e riduzione del limiter riportati |
| **X4, loudness** | Dopo la calibrazione | Ogni stem chiptune entro ± 1 LU dallo stem di produzione; il mix entro ± 1 LU |
| **X5, ruoli riconoscibili** | Pacchetto 5, note isolate dei ruoli | Accuratezza leave-one-out ≥ 0,75 per ognuna delle tre coppie (Trama–Corpo, Trama–Vettore, Corpo–Vettore). Si riportano anche i valori di produzione |
| **X6, materiale d'ascolto** | Clip | Battute 9–16 di A, B e A′ in produzione e chiptune, allineate in loudness; brano intero chiptune |

**Esito dello spike:** se X1–X5 superano, le clip vanno all'ascolto cumulativo (voce 16 del registro) e la decisione resta a CP-3. Se X5 non supera, lo si registra come limite del timbro chiptune per la leggibilità dei ruoli, che è un criterio di R-M25-9.

## 3. Limiti

- Una sola variante chiptune. Duty, filtri e PWM sono scelte d'autore iniziali.
- Gli inviluppi non cambiano: un chiptune «vero» userebbe inviluppi a gradini, che sono fuori da questo spike.
- La riconoscibilità è misurata su note isolate, non nel mix.

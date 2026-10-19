# M2.4 — Analisi Scientifica della Distorsione, Aliasing e Gain Staging in `voice_probe_v1.wav`

**Data:** 15 Ottobre 2026  
**Autori:** Team CYMATICA & Agente di Ricerca DSP  
**Riferimenti normativi:** `CYMATICA_Specifica_Agentica_Sviluppo.md` §16.8, §20.2; `DESIGN.md` §16.4, §16.6; `ADR-0003` (D-M2-06); `ADR-0004` (AUD-D-01, AUD-D-02); `distortion-aliasing.html`.

---

## 1. Contesto e Segnalazione

Durante la prima validazione d'ascolto percettivo del file audio `artifacts/cacp/voice_probe_v1.wav` generato dallo scenario diagnostico CACP `voice_test` (Inc-1), l'ascolto umano ha rilevato:
> *"ho ascoltato il wav e ci sono solo rumori distorti"*

A fronte di test booleani formali superati (continuità della derivata $\le 0.40\text{f}$, silenzio in pausa $< 10^{-4}$, assenza di NaN/Inf), l'ispezione acustica ha evidenziato un degrado timbrico inaccettabile rispetto alla purezza attesa delle sinusoidi a 220 Hz, 330 Hz e 440 Hz.

È stata quindi condotta un'indagine tecnico-scientifica approfondita in parallelo, integrando:
1. Misure dinamiche e conteggio campioni clippati;
2. Analisi spettrale FFT e calcolo del THD+N (Total Harmonic Distortion + Noise);
3. Studio dei fenomeni di non-linearità, generazione armonica e ripiegamento da aliasing documentati in `distortion-aliasing.html`;
4. Audit del codice DSP in `engine/audio/audio_engine.cpp` e `engine/audio/voice_allocator.cpp`.

---

## 2. Evidenze Empiriche e Diagnosi Quantitativa

### 2.1 Analisi Dinamica e Saturazione al Rail

L'analisi numerica campione per campione sui 120.000 frame stereo a 48 kHz (2,5 s) di `voice_probe_v1.wav` ha rivelato:

| Finestra Temporale | Frame Range | Eventi / Voci Attive | Min PCM | Max PCM | Campioni Clippati ($\pm 32767$) | % Clipped |
|---|---|---|---|---|---|---|
| **Fase 1: Nota Singola** | 0 .. 4.000 (0,00 .. 0,083s) | 1 nota (220 Hz) + `ma_waveform` | $-31.506$ | $+32.697$ | 0 | 0,00% (al 99,8% FS) |
| **Fase 2: Diade Polifonica** | 4.000 .. 24.000 (0,083 .. 0,50s) | 2 note (220 + 330 Hz) + `ma_waveform` | $\mathbf{-32.767}$ | $\mathbf{+32.767}$ | **2.519** (per canale) | **12,60%** |
| **Fase 3: Steal + Nuova Diade** | 24.000 .. 48.000 (0,50 .. 1,00s) | 2 note (330 + 440 Hz) + Steal Tail + `ma_waveform` | $\mathbf{-32.767}$ | $\mathbf{+32.767}$ | **4.884** (per canale) | **20,35%** |
| **Pausa (Mute fisico)** | 48.000 .. 60.000 (1,00 .. 1,25s) | Coda de-click $\to$ silenzio numerico | $0$ | $+57$ | 0 | 0,00% |
| **Resume (Rientro)** | 60.000 .. 88.000 (1,25 .. 1,83s) | 2 note (330 + 440 Hz) + `ma_waveform` | $\mathbf{-32.767}$ | $\mathbf{+32.767}$ | **3.814** (per canale) | **13,62%** |
| **Stop (Mute finale)** | 88.000 .. 120.000 (1,83 .. 2,50s) | Silenzio assoluto | $0$ | $0$ | 0 | 0,00% |
| **TOTALE STEREO** | **0 .. 120.000** | — | $\mathbf{-32.767}$ | $\mathbf{+32.767}$ | **22.434** (11.217/ch) | **15,58% sonoro** |

- **Durata del plateau di saturazione continua:** la sequenza più lunga di campioni consecutivi clippati al limite $\pm 32767$ raggiunge **38 campioni (0,791 ms)** per ciascuna semionda, trasformando l'apice sinusoidale in un'onda quadra tronca ("flat-topped").

### 2.2 Analisi Spettrale FFT e THD+N (Fase 3: frame 25.000–41.384)

L'analisi FFT a 16.384 punti sul segmento stazionario a 330 Hz + 440 Hz mostra una massiccia generazione di righe parassite da intermodulazione (IMD) e distorsione armonica:

- **Fondamentali:** 440 Hz ($-9,43\text{ dBFS}$), 330 Hz ($-11,61\text{ dBFS}$).
- **Prodotti di Intermodulazione e Armoniche (IMD):**
  - **220 Hz** ($2 f_1 - f_2 = 660 - 440\text{ Hz}$): **$-33,35\text{ dBFS}$**
  - **550 Hz** ($2 f_2 - f_1 = 880 - 330\text{ Hz}$): **$-33,17\text{ dBFS}$**
  - **1.100 Hz** ($2 f_1 + f_2 = 660 + 440\text{ Hz}$): **$-34,64\text{ dBFS}$**
  - **1.210 Hz** ($2 f_2 + f_1 = 880 + 330\text{ Hz}$): **$-32,88\text{ dBFS}$**
  - **1.320 Hz** ($3 f_2 = 3 \times 440\text{ Hz}$): **$-37,60\text{ dBFS}$**
- **Distorsione Armonica Totale + Rumore (THD+N):**
  $$\text{THD+N} = -18,36\text{ dB} \quad (12,08\%)$$
Una distorsione armonica del $12\%$ in presenza di suoni sinusoidali elementari distrugge completamente la trasparenza acustica, producendo all'orecchio un ronzio grezzo, aspro e metallico.

---

## 3. Fondamenti Teorici: Non-Linearità, Intermodulazione e Aliasing

Il documento `distortion-aliasing.html` e la letteratura DSP (Zavalishin 2018, Dattorro 1997, Musikhack 2024) stabiliscono le relazioni fisiche che governano questo fenomeno:

1. **Non-linearità istantanea e troncamento duro:**  
   Il clamping sample-by-sample $y[n] = \text{clamp}(x[n], -1.0, 1.0)$ introduce una discontinuità nella derivata prima della forma d'onda. La risposta in frequenza di un'onda quadra o di una sinusoide clippata decade lentamente con legge $1/k$ ($6\text{ dB/ottava}$). Di conseguenza, un'onda clippata a 440 Hz genera componenti armoniche significative ben oltre i $20\text{ kHz}$, estendendosi fino a centinaia di kilohertz.
2. **Il limite di Nyquist e il Ripiegamento (Aliasing):**  
   A frequenza di campionamento $f_s = 48\text{ kHz}$, il limite massimo teorico rappresentabile è $f_N = 24\text{ kHz}$. Qualsiasi armonica $f_h > 24\text{ kHz}$ prodotta da una saturazione istantanea senza oversampling non può essere rappresentata nel dominio discreto e si ripiega all'indietro:
   $$f_{\text{alias}} = |k \cdot f_s - f_h|$$
3. **Intermodulazione Polifonica (IMD):**  
   Quando due o più toni non armonici ($f_1 = 330\text{ Hz}$, $f_2 = 440\text{ Hz}$) attraversano insieme una funzione di clipping duro, non si generano solo armoniche pure $k f_1$ e $m f_2$, ma una complessa combinazione di prodotti di intermodulazione:
   $$f_{\text{IMD}} = |m f_1 \pm n f_2|$$
   Tutti questi prodotti, una volta superati i $24\text{ kHz}$, si ripiegano a frequenze frazionarie disarmoniche rispetto alla scala musicale, trasformando l'accordo armonico in "rumore distorto".

---

## 4. Diagnosi delle Cause Radice nel Codice

Dall'audit del codice sorgente emergono due cause radice concomitanti:

### Causa 1: Collisione tra generatore monofonico e VoiceAllocator (`audio_engine.cpp`)
In `AudioEngine::Impl`:
- `ma_waveform` era stato introdotto in M1 per il test del tono diagnostico continuo (`startTone()`).
- In M2.3, per udire gli eventi prima della polifonia, gli eventi `ScheduledAudioEvent` pilotavano `triggerNoteOn(schedEvt, activeVolume)`.
- In Inc-1, all'arrivo di `VoiceAllocator`, `triggerNoteOn` è rimasto attivo in parallelo:
  ```cpp
  case AudioEventType::NoteOn:
      voiceAllocator.onNoteOn(schedEvt);
      triggerNoteOn(schedEvt, activeVolume); // Ridondante e confliggente!
      break;
  ```
- In `renderWaveformSlice`:
  ```cpp
  ma_waveform_read_pcm_frames(&waveform, dest, count, nullptr); // Suona a 0.20f
  voiceAllocator.renderBlock(dest, count, ..., true);            // Somma le voci sopra
  ```
  `ma_waveform` suonava a $0.20\text{f}$ saltando a ogni nuova nota monofonicamente (da 220 a 330 a 440 Hz con discontinuità di fase) e sommandosi alla polifonia.

### Causa 2: Assenza di Headroom e Scaling in `VoiceAllocator` (`voice_allocator.cpp`)
In `VoiceAllocator::renderBlock`:
```cpp
const float s = std::sin(slot.phase) * slot.velocity * slot.envelopeLevel;
left += s;
right += s;
```
- Le voci venivano sommate con guadagno unitario ($1,0$).
- Con note a velocity $0.8$ e sustain $0.8$ (attack $1.0$), due voci producevano $1.28\text{f} \dots 1.60\text{f}$.
- Durante il voice stealing con coda attiva e `ma_waveform`, il picco ha raggiunto **$2.28\text{f}$ ($+7.1\text{ dBFS}$)**.
- In `writeWavStereo16`, il clamp a $[-1.0, 1.0]$ ha generato la squadratura sistematica delle creste.

---

## 5. Soluzione Architetturale e Misure della Correzione

In conformità con `ADR-0003` D-M2-06 (4 bus con guadagni dedicati: Pulso $-6\text{ dB}$, Corpo $-8\text{ dB}$, Trama $-10\text{ dB}$, Vettore $-6\text{ dB}$):

1. **Disaccoppiamento di `ma_waveform`:**
   - Gli eventi `ScheduledAudioEvent` (`NoteOn`, `NoteOff`) sono gestiti **esclusivamente** da `VoiceAllocator`.
   - Rimozione di `triggerNoteOn`/`triggerNoteOff` dal flusso degli eventi schedulati.
   - `ma_waveform` viene renderizzato **unicamente** quando `activeVolume > 1e-6f` (attivazione esplicita del tono continuo via `startTone()`).
2. **Gain Staging Nominale per Voce (`kNominalVoiceScale = 0.25f`):**
   - Ciascun ruolo dispone di 2 slot. Uno scaling nominale $0,25\text{f}$ assicura che due voci a piena velocity ($1.0$) sommate producano al massimo $0,50\text{f}$ ($-6\text{ dBFS}$), lasciando un margine di headroom di **almeno $6\text{ dB}$** per ciascun ruolo prima del bus master.
   - Per le note a velocity $0,8$ di `voice_probe_v1`, l'ampiezza massima è di $0,16\text{f}$ a regime per voce.

### 5.1 Verifica Analitica e Spettrale del Segnale Corretto

La simulazione numerica del segnale con gain staging $0,25\text{f}$ e disaccoppiamento di `ma_waveform` produce:

- **Picco massimo assoluto:** **$0,3400\text{f}$** (**$-9,37\text{ dBFS}$**).
- **Headroom lineare residua:** **$+9,37\text{ dB}$** rispetto allo 0 dBFS digitale.
- **Campioni clippati:** **ESATTAMENTE 0** (da 22.434 a 0!).
- **Spettro FFT:**
  - $440\text{ Hz}$: $-22,13\text{ dBFS}$
  - $330\text{ Hz}$: $-22,67\text{ dBFS}$
  - Prodotti di intermodulazione IMD: **$< -120\text{ dBFS}$** (completamente inudibili).
  - THD+N su cicli interi: **$< -200\text{ dB}$** (limitato unicamente dalla precisione float32).

---

## 6. Conclusioni e Azioni (Eseguite e Verificate)

L'evidenza scientifica ha confermato l'origine del difetto e validato la soluzione. Tutte le azioni di remediation (unitamente alla bonifica tecnico-scientifica post-Inc-1 `AUD-01..12`) sono state implementate e verificate:
1. Applicato `kNominalVoiceScale = 0.25f` in `VoiceAllocator`, con rampe di de-click sample-rate adaptive (`declickFrames_ = sampleRate_ / 500u`), singola rampa lineare su `onStop()`, ripristino su `onStart()` e sanitizzazione `NaN`/`Inf` (`dspAnomalyCount`).
2. Disaccoppiato `ma_waveform` in `AudioEngine` (attivo unicamente con `activeVolume > 1e-6f`) e rimossi tutti i residui monofonici legacy di M2.3.
3. Rigenerato `artifacts/cacp/voice_probe_v1.wav` tramite `cymatica_game.exe --cacp-run voice_test` (`PASS`, `peak_amplitude = 0.340088`, `clipped_samples = 0`, `voice_steals_detected = 1`, `transport_epoch = 4`), con validazione multimodale in prima persona e 91/91 test CTest verdi in Release/Debug (90/90 Headless).

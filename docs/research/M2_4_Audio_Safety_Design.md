# CYMATICA — Studio progettuale su protezione del master, loudness e safe listening

- **Data:** 2026-10-15 (aggiornato 2026-10-19 con chiusura formale del Gate pre-Inc-2).
- **Stato:** **studio tecnico e specifica di qualifica pre-Inc-2**; il principio di protezione finale, la separazione dei domini di monitoring e le specifiche del **Gate tecnico pre-Inc-2 (`AUD-D-02`)** (topologia della catena master, algoritmo `MasterLimiter` zero-latency $L = 0$, ceiling operativi e meter true-peak offline indipendente a 48 tap ITU-R BS.1770-5) sono **ratificati e chiusi in [ADR-0004](../adr/0004-m2-4-audio-renderer-safety-proposal.md)** per l'implementazione in Inc-2.
- **Baseline:** `main` (Inc-1 completato e verificato: 91/91 test Release/Debug, 90/90 Headless).
- **Nota 2026-10-20:** documento storico di ricerca. I valori normativi correnti del limiter (soglia −1,80 dBFS, pesi 0,65/0,35, detector a 3 interpolatori, limiter unico, rampa a monte) e del meter (zero padding) sono in [ADR-0004 §4.3](../adr/0004-m2-4-audio-renderer-safety-proposal.md); lo stato operativo è in [progress.md](../progress.md). I valori sotto restano come traccia della proposta originale.
- **Contesto:** [review audio](M2_4_Audio_Renderer_Research_Review.md), [ADR-0003 D-M2-06/07](../adr/0003-m2-decisions-contracts.md), [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md).

## 1. Oggetto e limiti di garanzia

Il titolare ha **ratificato il principio** di uno stadio digitale terminale sempre attivo e fail-closed sulle sorgenti audio sotto controllo CYMATICA e l'architettura di monitoring progressivo a tre domini. La motivazione è la gestione di musica adattiva e, in futuro, la somma con SFX/UI. Con la chiusura del **Gate pre-Inc-2 (2026-10-19)** sono stati formalmente qualificati in [ADR-0004](../adr/0004-m2-4-audio-renderer-safety-proposal.md) la topologia esatta della catena master, l'algoritmo `MasterLimiter` zero-latency ($L = 0$) con soglia interna $C_{\text{lim}} = -1,50\text{ dBFS}$ ($0,8414\text{f}$) e hard safety ceiling $C_{\text{max}} = -1,0\text{ dBFS}$ ($0,89125\text{f}$), e il meter true-peak offline indipendente a 48 tap ITU-R BS.1770-5, pronti per l'implementazione C++20 in Inc-2.

La mitigazione software può contenere picchi e loudness digitali, ma **non certifica** l'esposizione acustica alle orecchie: volume OS/dispositivo, amplificazione, sensibilità delle cuffie, caratteristiche del trasduttore e durata di ascolto non sono misurate dal PCM. Non descrivere il sistema come in grado di impedire sempre danni uditivi o hardware.

ITU-T [H.872 (2024)](https://www.itu.int/rec/T-REC-H.872-202410-I/en) è il riferimento principale pertinente al gaming, con funzioni anche di dynamic-range setup, categoria/master volume e altre misure. In §9.3.1 raccomanda, per il gioco, output scaling con **30-minute integrated non superiore all'intorno −23 LUFS ±2 LU** e **true peak ≤−1 dBTP**. Il documento include una previsione particolare per handheld con altoparlante a dinamica insufficiente (−18 LUFS ±2 e raccomandazione di headphone safety mode); l'applicabilità concreta va valutata, non estesa automaticamente a tutte le piattaforme.

**I valori `−14 LUFS` e `−4` ricevuti informalmente non sono contratti CYMATICA.** `−4` non specifica LUFS, dBFS o dBTP: mantenerlo come domanda aperta, senza codificarlo. Non chiamare `−14 LUFS` una soglia di protezione acustica.

## 2. Pipeline di riferimento e Topologia Qualificata Pre-Inc-2 (`AUD-D-02`)

```text
  [1] 8 VoiceSlot (kNominalVoiceScale = 0.25f) + StealTails (declickFrames_)
       |
  [2] 4 Role Buses (Pulso -6 dB, Corpo -8 dB, Trama -10 dB, Vettore -6 dB)
      + FX bounded per ruolo + Micro-ducking di Corpo da Pulso (-3 dB, att 2 ms, rel 80 ms)
       |---> [3] ChannelFrame Telemetry Extraction (pre-master, indipendente da volume utente)
       v
  [4] Music Mix Sum + Soft-Clipper Timbrico Opzionale (bypass di default)
       + (Futuri SFX / UI Audio in M3+)
       v
  [5] Master Finite Guard: !std::isfinite(L/R) -> 0.0f + ++dspAnomalyCount
       v
  [6] Terminal MasterLimiter Zero-Latency (L = 0 campioni, sempre attivo, fail-closed):
      Detector ibrido causale (FIR polifase 4x12 tap + bound di curvatura Nyquist 1°/2° ordine),
      Soglia interna C_lim = -1.50 dBFS (0.8414f) [proposta originale; ora -1.80 dBFS, ADR-0004 §4.3], attacco 0 ms, hold 2 ms, release 50 ms
       v
  [7] Rampa Fisica di De-Click del Trasporto (Pause/Resume/Stop, declickFrames_ = sampleRate / 500)
      & Gate di Silenzio Fisico (0.0f esatto oltre n_device,pause + N_tail)
       v
  [8] Attenuazione Utente Finale (<= 1.0f) & Hard Safety Clamp [-0.89125f, +0.89125f] (-1.0 dBFS)
       v
  [9] Scrittura nel Buffer Device (pOutput) & Tap AudioProbe (osservazione 1:1 dell'uscita device)
```

- **Assenza di SRC post-limiter nel motore:** `VoiceAllocator` e l'intera catena master operano direttamente alla frequenza di campionamento effettiva del device (`impl->sampleRate = impl->device.sampleRate` in modalità online, es. 48 kHz o 44.1 kHz, oppure `config.sampleRate` in modalità offline), mentre la riconciliazione con la timeline canonica a 48 kHz avviene a monte tramite `deviceToInternalFrames` / `internalToDeviceFrames`. Non esiste alcuno stadio di sample-rate conversion software tra il `MasterLimiter` e il buffer consegnato a `miniaudio`.
- **Ordinamento Limiter $\to$ Rampa Fisica di Trasporto:** posizionando il `MasterLimiter` (stadio `[6]`) a monte della rampa fisica di trasporto (stadio `[7]`) e scegliendo una topologia zero-latency ($L = 0$ campioni), il limiter non possiede alcuna linea di ritardo che possa riemettere energia sonora dopo la chiusura del gate di pausa, preservando l'invariante di silenzio numerico assoluto ($0,0\text{f}$) al frame $n_{\text{device, pause}} + N_{\text{tail}}$ e l'onset sample-accurate al campione 0 di tutti i test esistenti.
- Dopo lo stadio di protezione non sono ammissibili processori o guadagni **amplificanti** che possano invalidare il ceiling; attenuazioni a unità o inferiori sono consentite. Se il dispositivo applica guadagno hardware o post-elaborazione OS, questo rimane fuori dal controllo CYMATICA.
- **Tutte le sorgenti sonore controllate dal gioco devono attraversare una protezione terminale comune.** In M2.4 esiste solo la porzione pertinente alla musica; i futuri SFX/UI dovranno essere collegati al master protetto quando verranno introdotti. Non richiedere il completamento anticipato degli SFX in M2.4.

## 3. Specifiche Qualificate dello Stadio Protettivo (Gate Pre-Inc-2 Chiuso)

Con la chiusura del Gate pre-Inc-2 in [ADR-0004](../adr/0004-m2-4-audio-renderer-safety-proposal.md), lo stadio protettivo da implementare in Inc-2 risponde alle seguenti specifiche:

1. **Finite guard** prima del DSP terminale, per intercettare `NaN`, `Inf`, parametri fuori range e stati instabili, con contatore telemetrico bounded (`dspAnomalyCount`). In caso di anomalia non finita sul campione master, il valore viene immediatamente sostituito con `0.0f` prima di entrare nel detector del limiter, evitando qualsiasi contaminazione numerica dello stato di rilascio.
2. **Gain staging conservativo** sui quattro bus (`Pulso` $-6\text{ dB}$, `Corpo` $-8\text{ dB}$, `Trama` $-10\text{ dB}$, `Vettore` $-6\text{ dB}$) con `kNominalVoiceScale = 0.25f`; la somma massima teorica in fase di tutte le 8 voci contemporanee a `velocity = 1.0` è $\approx 1,314\text{f}$ ($+2,37\text{ dBFS}$), lasciando al limiter una riduzione massima nel caso pessimo degenerato di soli $\approx 3,87\text{ dB}$.
3. **Limiter terminale bounded zero-latency ($L = 0$)** senza heap, lock bloccanti, file I/O o logging nella callback (`engine/audio/master_limiter.h/.cpp`):
   - **Detector di picco ibrido causale:** combina (a) l'interpolazione polifase causale $4\times$ a 12 tap per sotto-fase sul buffer circolare locale degli ultimi 12 frame stereo `float historyL_[12], historyR_[12]` e (b) il bound analitico di curvatura alle alte frequenze basato sulle differenze finite prime e seconde ($\Delta x[n] = x[n] - x[n-1]$, $\Delta^2 x[n] = x[n] - 2x[n-1] + x[n-2]$):
     $$\hat{p}_{\text{curv}, c}[n] = \sqrt{x_c[n]^2 + 0,55\,(\Delta x_c[n])^2 + 0,30\,(\Delta^2 x_c[n])^2}, \qquad \hat{p}[n] = \max_{c \in \{L, R\}} \max\!\left(|x_c[n]|,\, \hat{p}_{\text{FIR4x}, c}[n],\, \hat{p}_{\text{curv}, c}[n]\right)$$
   - **Legge di guadagno stereo-linked:** soglia interna conservativa $C_{\text{lim}} = 0,8414\text{f}$ ($-1,50\text{ dBFS}$), attacco istantaneo $0\text{ ms}$, hold $N_{\text{hold}} = \lfloor 0,002 \times f_s \rfloor$ ($2\text{ ms}$), release esponenziale $\tau_{\text{rel}} = 50\text{ ms}$ ($\alpha_{\text{rel}} = \exp(-1 / (0,050 \cdot f_s))$), seguita da hard safety clamp a $C_{\text{max}} = 0,89125\text{f}$ ($-1,0\text{ dBFS}$).
4. **Meter True-Peak Offline Indipendente (`analysis::estimateTruePeakDbtp`):** implementato in `AudioProbe` secondo **ITU-R BS.1770-4/-5 Annex 2** (filtro FIR polifase standard a 48 tap, $4 \times 12$ coefficienti a fase lineare non correlato al bound di curvatura causale del limiter), verificato sulla suite di segnali avversari (`T-ADV-01..06`).
5. **Comportamento in saturazione riproducibile**, comprensivo di Attack/Hold/Release, gain reduction tracciata (`maxGainReductionDb`), fade di `Pause`/`Resume`/`Stop` sample-rate adaptive e continuità cross-epoch.
6. **Deadline integra**: target D-M2-07 di $\le 2,0\text{ ms}$ per l'intera callback Release a 512 frame / 48 kHz; il costo del `MasterLimiter` zero-latency è $\approx 60$ MAC/frame stereo ($\approx 0,64\text{ }\mu\text{s}$ per blocco da 512 frame, $< 0,04\%$ del budget).
7. **Nessuna dipendenza di sicurezza dai worker CACP/LUFS**: se il consumer è lento, il limiting realtime continua a funzionare. Capture/drop possono rendere **incompleta** l'evidenza, non disattivare la protezione e non produrre falsa attestazione `PASS`.

## 4. Metriche: definizioni e domini

| Grandezza | Intervallo/metodo | Uso | Non dimostra |
|---|---|---|---|
| **Momentary loudness** | K-weighted, 400 ms (EBU Mode) | reattività a cambi rapidi di mix | true peak, esposizione a lungo termine |
| **Short-term loudness** | K-weighted, 3 s | intensità percepita su frasi | spike campionario, dose fisica |
| **Integrated loudness della run** | BS.1770 gated, dal reset/run start | confronto intere composizioni | finestra rolling 30 min |
| **Integrated rolling 30 min** | BS.1770 gated sull'intervallo mobile nel dominio di riproduzione | audit rispetto alla feature H.872 §9.3.1 | livello in dBA alle cuffie |
| **Sample peak** | massimo `|x[n]|` dopo processing | hard clipping digitale | picchi intersample |
| **True peak** | ricostruzione/oversampling secondo BS.1770 | stima del picco fra campioni | loudness media / SPL |
| **Gain reduction limiter** | attenuazione imposta, dB e time history | quanto spesso opera il protettore | corretta calibrazione fisica |
| **dB SPL/dBA** | misurazione acustica dedicata | esposizione reale | disponibile dal solo `AudioProbe` |

Le misure LUFS non sono una semplice media aritmetica del livello dB; Integrated applica blocchi, pesatura K e gating assoluto/relativo. Le definizioni di riferimento sono [ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I) ed [EBU Tech 3341](https://tech.ebu.ch/publications/tech3341). Un confronto fra tracce deve usare compensazione di loudness e mantenere separato il controllo dinamico dell'intero mix.

## 5. Monitoraggio continuo senza violare il realtime

**Obiettivo:** raccolta di dati sufficienti per M/S/I, rolling 30 min, sample peak, true peak, limiter gain reduction e qualità dell'acquisizione; il monitoring non è un controllo critico del limiter.

- L'**AudioProbe esistente** è una ring SPSC a capacità limitata, sufficiente a diagnosi controllate; non se ne può presumere, senza prova, l'acquisizione lossless di una sessione illimitata. Evitare di usare una ring overflow-prone come unico input per attestare la conformità su 30 min.
- Nella callback sono consentiti solo primitive incrementali bounded e/o copie su ring preallocata; nessun FFT, gating globale, file I/O, allocazioni o lock. Prevedere **separazione dei ruoli**: DSP di protezione nel thread audio, accumuli diagnostici su consumer/worker, export o report offline.
- Per il **30-minute rolling gated**, definire il dominio temporale come output effettivamente riprodotto (device time), non come frame logico durante una Pause. Stabilire convenzioni di warmup (**una finestra completa non esiste prima dei 30 min**), silenzio, underrun, reset di run, cambio device/sample rate e consumer disconnesso.
- Gating conforme a BS.1770 non equivale a media delle letture Momentary. Il progetto può usare blocchi di energia K-weighted bounded e strutture temporali a dimensione fissa sul worker; confrontare l'algoritmo con riferimento indipendente e segnali ufficiali di test EBU.
- Se mancano campioni/finestre nella misura, il risultato corrispondente è **`inconclusive`/`not_run`**, non `passed`. Tenere distinti il contatore di drop della cattura e il contatore di deadline miss audio.
- Prima di dichiarare monitoraggio continuo affidabile, profilare l'effetto CACP ON/OFF, con consumer lento, device rate 44.1/48 kHz, burst di eventi e run estesa; evitare false affermazioni di trasparenza.
- La presentazione di un HUD audio/loudness o di controlli volume avanzati non è deliverable M2.4, salvo approvazione separata. È invece importante preservare il futuro requisito di volume master/per-categoria e profili prudenziali secondo H.872.

## 6. Piano di test e gate progressivi proposti

| Fase | Valutazione tecnicamente giustificata | Stato normativo |
|---|---|---|
| **Prima di Inc-2** | Qualificare algoritmo del limiter (`MasterLimiter` zero-latency), posizione rispetto a SRC/de-click tail, e meter true-peak BS.1770-5 nel budget callback. | **CHIUSO E ACCETTATO PER IMPLEMENTAZIONE (ADR-0004, 2026-10-19)** |
| **Inc-2** | Implementazione C++20 di 4 ruoli DSP, 4 bus, `MasterLimiter` e finite guard; test `test_synth_dsp.cpp` con segnali avversari (`T-ADV-01..06`); scenario CACP `synth_test` (`timbre_bus_probe_v1`). | **Pronto per l'implementazione in Inc-2** |
| **Inc-4** | Fixture A–B–A' con measure stage/post process; metriche M/S/I, true peak, gain reduction, deadline misses. | Evidence addizionale proposta |
| **Inc-5 / M2.5** | WAV completo e manifest CACP; confronto meter indipendente; ascolto umano e misure con limiter attivo. Durata della fixture ≈183 s, quindi **non** test rolling 30m a piena finestra. | Gate originali invariati finché non emendati formalmente |
| **M5 o milestone di durata appropriata** | Sessioni con SFX/UI reali, dispositivi differenziati e 30 min effettivi, controlli volume e headphone safety; oltre al software valutazione acustica dove praticabile. | Decisioni future da calendarizzare, non claim M2.5 |

### Dataset minimo per limiter e metering (`T-ADV-01..06` per Inc-2)

1. `T-ADV-01` — Silenzio, DC e somma in fase sui quattro bus di tutte le 8 voci contemporanee ($\approx +2,37\text{ dBFS}$ pre-limiter).
2. `T-ADV-02` — Cosinusoide a $f_s / 4$ sfasata di $\pi / 4$ con ampiezza campionaria $0,75\text{f}$ e intersample peak teorico $+3,01\text{ dB}$ ($1,06066\text{f}$).
3. `T-ADV-03` — Tono prossimo a Nyquist ($0,45 f_s$) e salto improvviso di `timbreParam` / impulso isolato di `Pulso`.
4. `T-ADV-04` — Iniezione diretta di `NaN`, `+Inf`, `-Inf` e subnormali (`1e-38f`) per verifica del Finite Guard e recupero immediato in $< 100\text{ ms}$.
5. `T-ADV-05` — Sequenze `Pause`/`Resume`/`Stop` durante limitazione attiva a $+6\text{ dBFS}$ (verifica assenza di code post-gate oltre $n_{\text{device, pause}} + N_{\text{tail}}$ a 48 kHz e 44.1 kHz).
6. `T-ADV-06` — A/B a parità di loudness per verificare assenza di pumping o distorsione sui 4 ruoli in regime nominale ($G[n] = 1,0\text{f}$).

**Criterio epistemico:** un limiter qualificato può rispettare un *ceiling digitale misurato* nel profilo testato; nessun test di questo elenco certifica la sicurezza delle cuffie dell'utente in assenza di dati acustici.

## 7. Decisioni ratificate e dettagli differiti a fasi successive

- AUD-D-01: **ratificato e chiuso per Inc-2** — renderer C++20 nativo puro autocontenuto per i 4 ruoli di Inc-2 con **zero dipendenze esterne** (`docs/dependencies.md` invariato).
- AUD-D-02: **ratificato e chiuso per Inc-2** — protezione terminale comune fail-closed con `MasterLimiter` zero-latency ($L = 0$), soglia interna $-1,50\text{ dBFS}$, hard ceiling $-1,0\text{ dBFS}$, posto a monte della rampa fisica di de-click e verificato offline tramite filtro FIR a 48 tap ITU-R BS.1770-5.
- AUD-D-03: **ratificata la separazione callback/worker/offline**; `−1 dBTP` e `−23 LUFS ±2 LU` restano **target sperimentali**, non limiti mandatory; `−4` rimane privo di unità e quindi non utilizzabile.
- AUD-D-04: livello di monitoring esteso per le run di 3 minuti (Inc-4/Inc-5) vs milestone di sessione lunga (M5+ per la finestra rolling 30 min).

Non trasformare risposte non ancora ricevute in campi di configurazione predefiniti o nuovi mandatory gate. Il `MusicClock` e la semantica del transport restano invariati.

## 8. Fonti

- [ITU-T H.872 (10/2024)](https://www.itu.int/rec/T-REC-H.872-202410-I/en): §9 sulle funzioni audio per software gaming.
- [WHO/ITU, Global standard for safe listening video gameplay and esports (2025)](https://www.who.int/publications/i/item/9789240108028).
- [ITU-R BS.1770-5 (11/2023)](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I).
- [EBU Tech 3341 (21/11/2023)](https://tech.ebu.ch/publications/tech3341), [EBU loudness reference/test signals](https://tech.ebu.ch/loudness/).

Questa nota è materiale di supporto alla decisione e **non** una dichiarazione di conformità ITU/EBU già ottenuta.

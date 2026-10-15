# CYMATICA — Studio progettuale su protezione del master, loudness e safe listening

- **Data:** 2026-10-15.
- **Stato:** **studio tecnico**; il principio di protezione finale e la separazione dei domini di monitoring sono **ratificati in ADR-0004 (2026-10-15)**, mentre algoritmo, topologia, ceiling e metriche di conformità sono ancora da qualificare. Nessuna feature è implementata.
- **Baseline:** `090dc2a45cf3785a2055c492415f9151321ea18a`.
- **Contesto:** [review audio](M2_4_Audio_Renderer_Research_Review.md), [ADR-0003 D-M2-06/07](../adr/0003-m2-decisions-contracts.md), [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md).

## 1. Oggetto e limiti di garanzia

Il titolare ha **ratificato il principio** di uno stadio digitale terminale sempre attivo e fail-closed sulle sorgenti audio sotto controllo CYMATICA e l'architettura di monitoring progressivo a tre domini. La motivazione è la gestione di musica adattiva e, in futuro, la somma con SFX/UI. D-M2-06 è stato emendato coerentemente; **un algoritmo true-peak e un ceiling numerico non sono ancora stati ratificati o implementati**.

La mitigazione software può contenere picchi e loudness digitali, ma **non certifica** l'esposizione acustica alle orecchie: volume OS/dispositivo, amplificazione, sensibilità delle cuffie, caratteristiche del trasduttore e durata di ascolto non sono misurate dal PCM. Non descrivere il sistema come in grado di impedire sempre danni uditivi o hardware.

ITU-T [H.872 (2024)](https://www.itu.int/rec/T-REC-H.872-202410-I/en) è il riferimento principale pertinente al gaming, con funzioni anche di dynamic-range setup, categoria/master volume e altre misure. In §9.3.1 raccomanda, per il gioco, output scaling con **30-minute integrated non superiore all'intorno −23 LUFS ±2 LU** e **true peak ≤−1 dBTP**. Il documento include una previsione particolare per handheld con altoparlante a dinamica insufficiente (−18 LUFS ±2 e raccomandazione di headphone safety mode); l'applicabilità concreta va valutata, non estesa automaticamente a tutte le piattaforme.

**I valori `−14 LUFS` e `−4` ricevuti informalmente non sono contratti CYMATICA.** `−4` non specifica LUFS, dBFS o dBTP: mantenerlo come domanda aperta, senza codificarlo. Non chiamare `−14 LUFS` una soglia di protezione acustica.

## 2. Pipeline di riferimento

```text
  Pulso/Corpo/Trama/Vettore -> FX di ruolo -> quattro bus -> MUSIC MIX
                                                        |
  SFX futuri --------------------------------------------+---> GAME MASTER
  UI audio futuri ---------------------------------------+        |
                                                        finite guard
                                                             |
                                                  gain staging / mix policy
                                                             |
                                                   terminal limiter (*)
                                                             |
                                                  output gain <= 1
                                                             |
                                              conversione finale dispositivo
                                                             |
                                              output digitale / AudioProbe
                                                             |
                                                    dispositivo audio
```

`(*)` La topologia definitiva deve identificare **se il resampling è prima o dopo la protezione**: un SRC a valle può produrre nuovi true peak. Il ceiling true-peak e la misura devono riferirsi alla **rappresentazione PCM effettivamente inviata al device**, verificando anche la ricostruzione intersample. Dopo lo stadio di protezione non sono ammissibili processori o guadagni **amplificanti** che possano invalidare il ceiling; attenuazioni a unità o inferiori sono consentite. Se il dispositivo applica guadagno hardware o post-elaborazione OS, questo rimane fuori dal controllo CYMATICA.

**Tutte le sorgenti sonore controllate dal gioco devono attraversare una protezione terminale comune.** In M2.4 esiste solo la porzione pertinente alla musica; i futuri SFX/UI dovranno essere collegati al master protetto quando verranno introdotti. Non richiedere il completamento anticipato degli SFX in M2.4.

## 3. Requisiti candidati dello stadio protettivo

I seguenti elementi vanno letti in conformità alla ratifica di ADR-0004: il **principio terminale** è obbligatorio, mentre la scelta dell'algoritmo, della topologia e dei parametri numerici costituisce un gate tecnico prima dell'implementazione del master in Inc-2:

1. **Finite guard** prima del DSP terminale, per intercettare `NaN`, `Inf`, parametri fuori range e stati instabili, con contatore telemetrico bounded. In caso di anomalia severa prevedere una policy deterministicamente definita che azzeri o attenui il solo output non valido, evitando propagazione della contaminazione numerica.
2. **Gain staging conservativo** sui quattro bus e sul master; il limiter è una protezione eccezionale, non un sostituto di un mix progettato male.
3. **Limiter terminale bounded** senza heap, lock bloccanti, file I/O o logging nella callback. Studiare sample-peak limiter, lookahead brickwall, true-peak con oversampling e soft-clipper + limiter; la scelta concreta dipende dai costi misurati, non dalla sola etichetta commerciale.
4. **Ceiling di prova −1 dBTP** come candidato H.872; alla qualifica, stimare il true peak con un meter *indipendente* dalla logica del limiter e segnali difficili (impulsi, sinusoidi vicino a Nyquist, in-phase bus sum, cambiorate e transitori). Il clamp a ±1 non costituisce prova true peak.
5. **Comportamento in saturazione riproducibile**, comprensivo di Attack/Release, lookahead, gain reduction, latency deterministica, fade di Pause/Resume/Stop, stadi di resampling e continuità di input/output.
6. **Deadline integra**: target D-M2-07 di <=2.0 ms per l'intera callback Release a 512 frame / 48 kHz; all'eventuale aggiunta di true-peak oversampling/latency, aggiornare gli scenari del benchmark, non creare un budget di 2 ms aggiuntivo per il limiter.
7. **Nessuna dipendenza di sicurezza dai worker CACP/LUFS**: se il consumer è lento, il limiting realtime continua a funzionare. Capture/drop possono rendere **incompleta** l'evidenza, non disattivare la protezione e non produrre falsa attestazione `PASS`.

Scegliere parametri numerici di attack/release/lookahead e oversampling soltanto dopo prova sotto carico; non sono congelati in questo documento. I picchi da software non qualificato o plugin terzi richiederebbero anche adeguata limitazione/finite guard prima dell'host graph.

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
| **Prima di Inc-2** | Qualificare algoritmo del limiter, posizione rispetto a SRC, compensazione latenza e ceiling di prova nel budget callback. | **Principio ratificato (ADR-0004)**; qualifica tecnica pre-master |
| **Inc-2** | Gate pre-codice: scegliere algoritmo/topologia, poi test DSP limiter e finite guard con segnali avversari; budget callback completo e regressione sample-peak. | **Principio ratificato**; algoritmo e ceiling ancora da qualificare |
| **Inc-4** | Fixture A–B–A' con measure stage/post process; metriche M/S/I, true peak, gain reduction, deadline misses. | Evidence addizionale proposta |
| **Inc-5 / M2.5** | WAV completo e manifest CACP; confronto meter indipendente; ascolto umano e misure con limiter attivo. Durata della fixture ≈183 s, quindi **non** test rolling 30m a piena finestra. | Gate originali invariati finché non emendati formalmente |
| **M5 o milestone di durata appropriata** | Sessioni con SFX/UI reali, dispositivi differenziati e 30 min effettivi, controlli volume e headphone safety; oltre al software valutazione acustica dove praticabile. | Decisioni future da calendarizzare, non claim M2.5 |

### Dataset minimo per limiter e metering

1. Silenzio, DC e passaggi graduali; segnali in fase sommati sui quattro bus.
2. Impulsi isolati, click/transient Pulso e salto improvviso di `timbreParam`.
3. Oscillazioni prossime a Nyquist, intersample peak con sample peak basso, cambi di frequenza e canali stereo sfasati.
4. Eccesso di gain, segnali fuori range, `NaN`, `Inf`, subnormali e feedback patologico.
5. Pause/Resume/Stop e tail cross-epoch; OS/device sample rate conversion e sequenze di buffer brevi/variabili.
6. A/B a parità di loudness per verificare pump/distorsione, senza scambiare maggiore livello con maggiore qualità.

**Criterio epistemico:** un limiter qualificato può rispettare un *ceiling digitale misurato* nel profilo testato; nessun test di questo elenco certifica la sicurezza delle cuffie dell'utente in assenza di dati acustici.

## 7. Decisioni ratificate e dettagli ancora differiti

- AUD-D-01: **ratificato** renderer nativo + ammissione selettiva di DSP esterno auditato (nessuna dipendenza automaticamente adottata).
- AUD-D-02: **ratificato il principio** di protezione terminale comune e policy fail-closed; algoritmo, posizionamento rispetto a SRC e verifica true-peak **differiti al gate pre-Inc-2**.
- AUD-D-03: **ratificata la separazione callback/worker/offline**; `−1 dBTP` e `−23 LUFS ±2 LU` restano **target sperimentali**, non limiti mandatory; `−4` rimane privo di unità e quindi non utilizzabile.
- AUD-D-04: livello di monitoring atteso in M2.4 vs milestone di sessione lunga; integrità dell'acquisizione e modello di gating/versioning.

Non trasformare risposte non ancora ricevute in campi di configurazione predefiniti o nuovi mandatory gate. Il `MusicClock` e la semantica del transport restano invariati.

## 8. Fonti

- [ITU-T H.872 (10/2024)](https://www.itu.int/rec/T-REC-H.872-202410-I/en): §9 sulle funzioni audio per software gaming.
- [WHO/ITU, Global standard for safe listening video gameplay and esports (2025)](https://www.who.int/publications/i/item/9789240108028).
- [ITU-R BS.1770-5 (11/2023)](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I).
- [EBU Tech 3341 (21/11/2023)](https://tech.ebu.ch/publications/tech3341), [EBU loudness reference/test signals](https://tech.ebu.ch/loudness/).

Questa nota è materiale di supporto alla decisione e **non** una dichiarazione di conformità ITU/EBU già ottenuta.

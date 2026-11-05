# M2.5 — ricerca R-M25-8: nessun segnale unico prova l'assenza di glitch su Windows, ma tre strati si controllano senza privilegi

- **Data:** 2026-11-05 (fonti consultate lo stesso giorno)
- **Origine:** ricerca R-M25-8 di [ADR-0007](../adr/0007-m2-5-entry-decisions.md), prerequisito del criterio PASS di CP-1.
- **Status:** ricerca **non normativa**. Ogni indicazione è una proposta per CP-1. Nessuna soglia è validata.
- **Ambiente:** Windows 11 Pro 10.0.26300, miniaudio 0.11.25, Windows SDK 10.0.26100, utente non elevato. Nessun comando ha richiesto privilegi.
- **Materiale:** sorgente, analisi e CSV in [m25_glitch_calibration](experiments/m25_glitch_calibration/README.md); le catture non sono versionate. Le tabelle qui sotto riportano i risultati.

## Conclusioni

1. **Windows non espone a un'applicazione WASAPI alcun contatore di glitch.** Nel SDK 26100 la parola compare in commenti e in un solo campo pubblico, `XAUDIO2_PERFORMANCE_DATA.GlitchesSinceEngineStarted`, che riguarda il motore XAudio2. `AudioGraph` e `AudioStateMonitor` non offrono statistiche sui glitch.
2. **Esistono eventi ETW espliciti, non documentati.** Il provider `Microsoft-Windows-Audio` ha 24 eventi nel canale `Microsoft-Windows-Audio/GlitchDetection`, spento di default. I nomi sono verificati sul manifest locale. Il significato è solo dedotto dai nomi: nessuna fonte Microsoft letta li descrive, e nessuno è stato osservato scattare su questa macchina, perché attivarli richiede privilegi.
3. **`IAudioClock` rivela l'underrun del client.** In 87 stalli iniettati, un intervallo che ha prodotto silenzio ha fermato la posizione del dispositivo per lo stesso tempo, con errore entro 0,15 ms su HDMI. Su USB una volta su 11 il segnale è mancato. La documentazione non descrive questo comportamento, Chromium lo usa in produzione.
4. **`AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY` in loopback non segnala i glitch di render.** Il flag è comparso 0 volte in 87 stalli, 47 dei quali con silenzio reale. È comparso sul primo pacchetto di ogni corsa e quando è stata la cattura stessa a perdere dati. Serve a validare il lato cattura, non a rilevare un underrun. Il flag `SILENT` si comporta meglio: il numero di pacchetti silenziosi coincide col silenzio inserito, ma vale solo se il nostro è l'unico flusso.
5. **Il confronto del contenuto è il rilevatore più forte.** Un underrun del client inserisce silenzio a multipli di 480 frame e non perde campioni: il ritardo cresce, poi il segnale riprende identico. Una perdita in cattura lo riduce. Il segno distingue i due casi, quindi la prova loopback già impostata su `ma_device_type_loopback` resta valida anche senza i flag, che miniaudio non inoltra. Gli zeri esatti però non bastano: con quel percorso `SILENT` viene ignorato, quindi contano gli slittamenti di allineamento.
6. **Il software non vede tutta la catena.** Oltre l'uscita del motore restano driver, HDMI o USB, DAC e casse. Lì valgono solo cattura fisica o ascolto. L'ascolto umano non è sostituibile (AGENTS §5.1).
7. **Proposta per CP-1:** controllo positivo su ogni uscita, poi quattro controlli software senza privilegi e ascolto umano. ETW entra solo come quinto controllo se il canale reagisce al controllo positivo.

## Il modello a strati

| Strato | Dove | Guasto tipico | Controllo proposto | Copertura |
|---|---|---|---|---|
| L0 | Callback | PCM errato, callback lenta | Hash contro il golden (già fatto) | Provata |
| L1 | Client → buffer endpoint | Il client non riempie in tempo | Intervalli, `IAudioClock`, loopback | Misurata in questo dossier |
| L2 | Motore audio (`audiodg`, APO, mix) | Passo del motore in ritardo | Loopback, eventi ETW | Plausibile, non provocata |
| L3 | Motore → driver | Dispositivo affamato | Eventi ETW, in parte `IAudioClock` | Non verificata |
| L4 | Driver → hardware | Errore DMA, USB, HDMI | `eMINIPORT_GLITCH_REPORT` se il driver lo emette | Non verificata |
| L5 | DAC, casse, ambiente | Disturbo fisico | Cattura analogica, ascolto | Solo fisica |

Microsoft definisce il glitch come discontinuità percepita. La telemetria di Windows 7 contava invece quando «the underlying audio hardware was being starved for data» ([Carr 2009](https://learn.microsoft.com/en-us/archive/blogs/e7/improving-audio-glitch-resilience-in-windows-7)). Sono due grandezze diverse: un evento può non essere udibile e un disturbo udibile può non generare eventi.

## Misure nostre: controllo positivo

`glitchcal` è uno strumento minimo in WASAPI puro. Imita lo schema di scrittura di miniaudio: `Start` prima della prima scrittura, buffer da 30 ms, `GetBuffer(480)` dopo ogni evento, thread MMCSS Games. Emette un tono di 997 Hz a -80 dBFS. A istanti noti blocca il thread di render per una durata nota e registra padding, `IAudioClock` e cattura loopback con flag e posizione. Non è il motore di CYMATICA.

Cinque corse: tre su `LG TV SSCR2 (NVIDIA High Definition Audio)`, una su `Realtek Digital Output (Realtek USB Audio)`, una di controllo sul lato cattura. Formato di mix 48 kHz float32 stereo su entrambe, periodo del motore 480 frame (default, minimo e massimo coincidono), buffer di render 1.440 frame, volume master 1,0, muto spento.

**Quanto stallo serve per sentire un buco.** A riposo il padding al risveglio è 960 frame.

| Stallo | HDMI, corse con silenzio | HDMI, silenzio inserito | USB, corse con silenzio | USB, silenzio inserito |
|---:|---:|---:|---:|---:|
| 15–38 ms | 0 su 30 | 0 | 0 su 9 | 0 |
| 40 ms | 6 su 6 | 10 ms | 2 su 3 | 10 ms |
| 42–46 ms | 15 su 15 | 10 ms | 3 su 3 (solo 45 ms) | 10 ms |
| 50 ms | 3 su 3 | 20 ms | non provato | |
| 60 ms | 6 su 6 | 30 ms | 3 su 3 | 30 ms |
| 100 ms | 6 su 6 | 60–70 ms | 3 su 3 | 70 ms |

Con buffer di 30 ms e periodo di 10 ms il primo buco arriva a circa 40 ms di intervallo, cioè buffer più un periodo. La stima `suspected_underruns` scatta a 30 ms: è prudente di un periodo. L'intervallo massimo della baseline (19,55 ms) lascia circa 20 ms di margine, se il modello vale anche per miniaudio. È una deduzione: `glitchcal` non è miniaudio.

**Sensibilità dei segnali** su 87 stalli (47 con silenzio, 40 senza), silenzio nel contenuto come riferimento.

| Segnale | Silenzio rilevato | Falsi allarmi | Lettura |
|---|---:|---:|---|
| Intervallo ≥ 30 ms | 47 su 47 | 28 su 40 | Prudente, non distingue |
| Padding 0 al risveglio | 47 su 47 | 28 su 40 | Necessario, non sufficiente |
| Scarto `IAudioClock` > 5 ms | 46 su 47 | 0 su 40 | Il più selettivo fra quelli senza contenuto |
| `DATA_DISCONTINUITY` in cattura | 0 su 47 | 0 su 40 | Cieco all'underrun di render |
| Salto di posizione in cattura | 0 su 47 | 0 su 40 | Cieco all'underrun di render |

Lo scarto è la differenza fra il tempo QPC e il tempo di posizione trascorsi, sommata per 300 ms dopo lo stallo. Su HDMI coincide col silenzio entro -0,13 e +0,15 ms. Su USB il caso mancato è uno stallo da 40 ms con scarto di -7,45 ms e silenzio di 10 ms. Sugli altri 10 casi USB l'errore resta entro 0,3 ms. Quell'unico caso basta a non usare `IAudioClock` da solo.

**Flag `SILENT` in cattura.** Conteggio per corsa, non per singolo stallo. Il periodo è 10 ms.

| Corsa | Pacchetti `SILENT` | Attesi da silenzio e coda finale |
|---|---:|---:|
| HDMI 1 | 71 | 71,0 |
| HDMI 2 | 56 | 56,0 |
| HDMI 3 | 72 | 72,0 |
| USB | 75 | 74,0 |
| Cattura (stallo in lettura) | 39 | 39,0 |

Il flag segue quindi il silenzio del mix. Non distingue un buco da un silenzio voluto nel contenuto, e un altro flusso sullo stesso endpoint lo coprirebbe. È un segnale utile solo con riferimento noto e macchina silenziosa, e miniaudio non lo inoltra.

**Fedeltà del loopback.** `glitchcal` scrive zeri nei pacchetti `SILENT`, come chiede la documentazione: gli zeri del confronto vengono da lì, non da una lettura del buffer.
- **HDMI:** 5.407.677 frame del canale 0 identici al bit al riferimento in tre corse, 0 diversi. `IAudioEffectsManager` riporta 0 effetti. Dopo ogni buco il segnale riprende esattamente dal campione successivo: nessun campione perso.
- **USB:** nessuna identità al bit. Guadagno 1,00000004, errore massimo 1,4·10⁻⁹ su 4.800 frame, un primo campione a 4·10⁻¹⁰ prima del tono. Il manager riporta 9 effetti tutti spenti, con `canSetState` falso. C'è quindi un'elaborazione residua, a circa -185 dBFS, che la lista degli effetti non spiega.
- **Lato cattura:** con uno stallo di 2,5 s e buffer di 2 s sono arrivati 1 flag di discontinuità oltre al primo, 1 salto di posizione e circa 0,5 s persi.
- **Inizio corsa:** la cattura comincia con 480 frame di silenzio su HDMI e 960 su USB. È compatibile col render che parte a buffer vuoto, come in miniaudio. Va escluso dal conteggio.

**Costo delle chiamate** nel thread di render, su circa 16.000 chiamate: `GetCurrentPadding` media 1 µs, massimo 16 µs; `GetPosition` media 1 µs, massimo 24 µs. La documentazione non garantisce che siano senza lock, quindi AGENTS §3 sconsiglia di metterle nella callback.

**Attività parallela.** La cattura in corso per M2.5.1d, non ancora committata, riporta sulla fixture reale `residual_db` -179,18, ritardo 960, 0 slittamenti e 0 silenzi inattesi. Il controllo con suoni di sistema mostra 121 raffiche di residuo. Questo risultato è suo, non riprodotto qui. Indica che a pieno livello il loopback resta fedele nella sua configurazione.

## Domanda 1: quali eventi di sistema segnalano i glitch

Verifica sul manifest locale con `Get-WinEvent -ListProvider`, `wevtutil gp` e `logman query providers`, senza privilegi.

**Provider con manifest:** `Microsoft-Windows-Audio` `{AE4BD3BE-F36F-45B6-8D21-BDD6FB832853}`, 137 eventi. Anche `Microsoft-Windows-MMCSS` `{36008301-E154-466C-ACEC-5F4CBD6B4694}` e `Microsoft-Windows-XAudio2` `{1EE3ABDB-C1FC-4B43-9E56-11064ABBA866}`.

**Non trovati:** nessun provider con manifest chiamato AudioEngine, audiodg o PortCls in `logman query providers`. WPR usa un alias `AudioEngine` che non ho potuto ricondurre a un GUID.

| Canale | Stato qui | Tipo | Eventi | Keyword |
|---|---|---|---:|---|
| `/GlitchDetection` | spento | Operational | 24 | `0x0400000000000000` |
| `/Performance` | spento | Analytic | 43 | `0x1000000000000000` |
| `/Informational` | spento | Operational | 50 | `0x0200000000000000` |
| `/Operational` | acceso | Operational | 12 | `0x4000000000000000` |

Tutti i 24 eventi di GlitchDetection sono a livello Verbose.

| ID | Nome nel manifest | Strato dedotto | Campi utili |
|---:|---|---|---|
| 33 | `op_EVT_GLITCH_CP_SERVER_INPUT_STARVATION` | L1, flusso del client affamato | Offset, `BytesAvailable` |
| 31, 32 | `..._CP_CLIENT_OUTPUT_SERVER_OVERREAD`, `..._READ_POINTER_OVERWRITE` | L1, lato client del render | Posizioni |
| 29, 30, 34, 35 | `..._CP_CLIENT_INPUT_*`, `..._CP_SERVER_OUTPUT_QUEUE_FULL_PACKET_DROP`, `..._READ_POINTER_OVERWRITE` | Lato cattura | `DroppedBytes` |
| 28 | `op_EVT_GLITCH_APO_FORMAT_CONVERT` | L2 | |
| 36, 37 | `..._KSENDPOINT_IOMGR_NO_OUTSTANDING_PACKETS`, `..._MAX_IOCTL_TIME` | L3 | `DeltaHNS` |
| 41 | `..._KSENDPOINT_BASE_END_GLITCH` | L3 | `GlitchDuration` |
| 44 | `..._KSENDPOINT_RTREN_WRITE_POS_EXCEEDS_TOTAL_POS` | L3, render WaveRT | Posizioni |
| 48 | `..._KSENDPOINT_STREN_DEVICE_STARVED` | L3, render standard | `AvailableFrames` |
| 38–40, 42, 43, 45–47, 49 | Altri casi KS | L3, spesso cattura | |
| 26, 27 | `..._CM_RENDER`, `..._CM_CAPTURE` | Monitor di cattura | |

Fuori da quel canale:

| ID | Canale | Task | Campi | Nota |
|---:|---|---|---|---|
| 118 | Performance | `AudioCore_Pump_MissedDeadline_Task` | `TaskGroup`, `Hns`, `success` | Passo del motore oltre la scadenza |
| 140 | Performance | `AudioCore_Pump_ProcPassDuration_Task` | `WorkDuration`, `MaxWorkDuration` | Durata del passo |
| 19 | Performance | `AudioPerf_Task_GlitchCounter` | `dwGlitchCount`, `hnsTimeWindow` | Conteggio su finestra |
| 18 | Operational | `AudioDG_Glitch_Threshold_Exceeded` | `dwAudioDgTerminationCode` | Warning, soglia ignota |
| 142 | Informational | `AudioCore_AEGlitch_Task` | `GlitchFrameCount`, posizioni | Descrizione vuota |
| 143 | Informational | `AudioCore_AEDrop_Task` | `SampleTime`, `u64ByteCount` | Descrizione vuota |

**Provider TraceLogging.** Il profilo Audio di WPR include `Microsoft.Windows.Audio.Client` (`6e7b1892-5288-5fe5-8f34-e3b0dc671fd2`), `.Client.Streaming`, `.CrossProcess`, `.Engine` (`46df03df-beab-5f0b-7eb2-e471c1bea993`), `.KernelStreamingEndpoint` (`553ca39b-c608-566d-18ae-7b9a03a39acd`) e altri. Nomi e GUID vengono da `WindowsPerformanceRecorderControl.dll`. Il contenuto degli eventi non è documentato. Non sono stati registrati.

**Lato driver.** `EPcMiniportEngineEvent` definisce `eMINIPORT_GLITCH_REPORT`, emesso con `IPortClsEtwHelper::MiniportWriteEtwEvent`. Il terzo parametro distingue underrun del buffer WaveRT, errori del decoder e posizione di scrittura ricevuta due volte. La [pagina Microsoft](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/glitch-reporting-for-offloaded-audio) lo descrive per l'audio in offload. Non ho trovato il provider che lo riceve, e dipende da ciò che il driver implementa.

**Strumenti.**

| Strumento | Stato qui | Uso | Permessi |
|---|---|---|---|
| WPR, profilo `Audio` | Installato. `wpr -profiles` lo descrive come «Audio glitches» | Traccia completa per la causa | [Prompt amministratore](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/optimizing-windows-devices-for-multimedia-experiences-exercise-2) |
| WPA | Installato. Nessun catalogo audio dedicato nella cartella `Catalog` | Tabelle generiche, DPC e ISR | Lettura senza privilegi (dedotto) |
| MXA (`xa.exe`) | Non installato, parte dell'ADK | Vista `Audio Glitches Classic`, strumento raccomandato da Microsoft | Prompt amministratore nell'esercizio |
| GPUView | Presente in `gpuview`, nessun riferimento audio in `log.cmd`, `log.wprp` e binario | Non adatto | |
| `logman`, `tracerpt`, `Get-WinEvent` | Installati | Sessione minima e lettura | Sessione: amministratore o gruppo Performance Log Users |

Il profilo WPR è `Audio.Verbose.Memory` e prevede 1.024 KB per 1.940 e 3.253 buffer nei due collector: fino a circa 5 GB. Registra `AudioEngine` a livello 3. Se quell'alias fosse `Microsoft-Windows-Audio`, gli eventi Verbose non entrerebbero nella traccia, nonostante la pagina dica «tutti i glitch». Non è verificato.

[`StartTrace`](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew) e `EnableTraceEx2` richiedono amministratore, Performance Log Users o account di servizio. L'utente corrente non è nel gruppo. Non ho avviato sessioni. Il log Operational è leggibile: su 1.930 eventi dal 2026-09-21 al 2026-11-05 l'ID 18 non compare. Non è una prova di assenza, perché la soglia è ignota.

## Domanda 2: cosa misurano davvero in modalità condivisa

| Segnale | Buffer non riempito dal client | Glitch del motore | Problema del driver |
|---|---|---|---|
| Intervalli di callback | Sì, prudente | No | No |
| `GetCurrentPadding` | Sì (0 è necessario, non sufficiente) | Non provato | Non provato |
| `IAudioClock` | Sì, misurato | Non provato | Non provato |
| ETW 31–33 | Dedotto | No | No |
| ETW 118, 28 | No | Dedotto | No |
| ETW 36–49 | No | Forse | Dedotto |
| PortCls | No | No | Solo se il driver emette |
| Loopback, contenuto | Sì, misurato | Dedotto | No, salvo pin hardware |
| Ascolto, cattura fisica | Sì | Sì | Sì |

Un `IAudioClock` regolare non prova il silenzio assente a valle. La documentazione lo definisce come posizione del campione in uscita, ma non dice cosa fa in caso di guasto. La [guida Microsoft sui media](https://learn.microsoft.com/en-us/windows-hardware/test/weg/delivering-a-great-media-experience) aggiunge che alcuni glitch nascono prima del driver e che DPC o ISR oltre 1–3 ms possono incidere. L'[esercizio MXA](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/optimizing-windows-devices-for-multimedia-experiences-exercise-4) mostra `audiodg` che salta la sua cadenza di 10 ms.

## Domanda 3: cosa offre WASAPI e cosa è usabile con miniaudio

miniaudio non contiene `IAudioClock` né `IAudioStreamVolume` (0 occorrenze in `miniaudio.h`). `ma_device` espone però `wasapi.pAudioClientPlayback`, e il progetto legge già `device.wasapi.hAvrtHandle`. Si può quindi chiamare `GetService` da codice nostro senza toccare il file vendored.

| API | Cosa dà | Usabile senza modificare miniaudio | Nota |
|---|---|---|---|
| `IAudioClient::GetCurrentPadding` | Frame in coda | Sì | Istantaneo, va campionato spesso |
| `IAudioClock::GetPosition` | Posizione e QPC | Sì, con `GetService` | Il deficit è cumulativo: basta leggerlo a bassa frequenza, fuori dalla callback |
| `IAudioClock2::GetDevicePosition` | Posizione grezza | Sì, da verificare | Solo modalità condivisa, non provata |
| `IAudioRenderClient` | Scrittura | Riservata a miniaudio | Non segnala glitch |
| Flag `DATA_DISCONTINUITY`, `SILENT` | Stato del pacchetto | **No** | `ma_device_read__wasapi` legge i flag, ne usa uno solo in duplex e non ne inoltra nessuno |
| Posizione e QPC del pacchetto | Timestamp di cattura | **No** | miniaudio passa `NULL, NULL` |
| `IAudioStreamVolume` | Volume per canale | Sì | Solo controllo preliminare, non rivela glitch |
| `IAudioEffectsManager` | Elenco effetti | Sì | Build 22000 o successive; su USB non ha spiegato il residuo |
| `IAudioClient3` | Periodi del motore | Sì, in sola lettura | miniaudio non lo usa qui: `sampleRate` esplicito attiva `AUTOCONVERTPCM` e disabilita la modalità a bassa latenza |
| `AudioClientProperties.Options` (RAW, POST_VOLUME_LOOPBACK) | Opzioni del flusso | **No** | Vanno impostate prima di `Initialize`, che avviene dentro `ma_device_init` |
| `AudioGraph` | Motore proprio | No | Eventi di quantum ed errori irrecuperabili: nessuna statistica glitch |
| `AudioStateMonitor` | Muto o attenuazione di sistema | No | API UWP sul livello, non sui glitch |
| `XAudio2.GetPerformanceData` | Glitch del motore XAudio2 | No | Non osserva un flusso miniaudio |

Tre avvertenze per chi aggiunge chiamate dirette.

- **Reroute:** se cambia il dispositivo predefinito, miniaudio ricrea il client e il puntatore salvato non vale più. Nel probe conviene `wasapi.noAutoStreamRouting = true`, con corsa `INVALID` se il dispositivo cambia.
- **Thread:** il servizio va rilasciato dallo stesso thread che lo ha ottenuto, prima di `ma_device_uninit`. La documentazione non dice se le chiamate sono sicure accanto al thread di render. `glitchcal` le esegue nello stesso thread: il caso da un secondo thread non è provato.
- **Offload:** miniaudio tenta `bIsOffload` se la categoria lo consente. Un flusso in offload salterebbe il motore software. Conviene `wasapi.noHardwareOffloading = true` nel probe, o registrarne lo stato.

`ma_device_type_loopback` resta adatto a raccogliere il PCM. Nasconde però i flag e copia il buffer anche quando `SILENT` impone di ignorarne il contenuto. Un rilevatore basato solo su zeri esatti può quindi mancare un buco, se il buffer contiene altro: non ho verificato cosa contenga. Un commento nel sorgente segnala anche casi in cui il flag di discontinuità compare a ogni chiamata.

## Domanda 4: limiti noti del loopback

- **Posizione rispetto al volume:** per default il tap è prima di volume e muto ([`AUDCLNT_STREAMOPTIONS`](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/ne-audioclient-audclnt_streamoptions)). `POST_VOLUME_LOOPBACK` esiste, ma non su tutti gli endpoint, e [da Windows 11 24H2](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/ksproperty-audioloopback) il driver deve dichiararlo. Nelle nostre corse il master era 1,0, quindi non abbiamo distinto i due punti.
- **Volumi prima del mix:** volume di flusso, di sessione e di politica agiscono prima del mix e quindi entrano nel loopback (deduzione da [`IAudioStreamVolume`](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudiostreamvolume)). L'attenuazione automatica del sistema apparirebbe come guadagno diverso da 1.
- **Posizione rispetto agli APO:** SFX prima del mix, MFX dopo il mix del modo, EFX su tutti i flussi ([architettura APO](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/audio-processing-object-architecture)). Nessuna pagina letta dice dove cade il tap software rispetto a EFX. Microsoft parla di uscita del motore e, in offload, di uscita finale salvo codifica non PCM. Il residuo su USB mostra che almeno uno stadio è a monte del tap su quell'uscita.
- **Pin hardware:** se il dispositivo ha un pin di loopback, WASAPI legge da lì, altrimenti copia l'uscita del motore ([Loopback Recording](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording)). Il punto esatto dipende quindi dall'hardware.
- **Limiter di sistema:** fonti secondarie e un commento su un blog Microsoft descrivono un limiter finale prima della conversione in intero, inattivo sotto ±1 circa. Il tap rispetto al limiter non è documentato. Il residuo di -179 dB dell'attività parallela suggerisce che con la fixture non incida.
- **Silenzio:** se nessun flusso è attivo sull'endpoint non arrivano pacchetti ([Windows 11 APO](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects), [van Eerde](https://matthewvaneerde.wordpress.com/2008/12/16/sample-wasapi-loopback-capture-record-what-you-hear/)). Misurato: la cattura parte col render, e i periodi senza segnale arrivano come pacchetti `SILENT`, non come assenza di pacchetti, finché il nostro flusso è attivo.
- **Flag di discontinuità:** il [`GetBuffer`](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudiocaptureclient-getbuffer) dice che «this flag can be used for glitch detection», ma il riferimento è il flusso di cattura. Sul primo pacchetto va ignorato: van Eerde riferisce che Windows 7 lo imposta e Chromium lo tratta come indefinito.
- **Drenaggio:** dopo un'interruzione un lettore che non svuota tutti i pacchetti vede discontinuità persistenti ([Q&A Microsoft](https://learn.microsoft.com/en-us/answers/questions/1188388/persistent-audio-discontinuity-in-wasapi-loopback)). Il caso riguarda il campione di Microsoft, non WASAPI.
- **Formato:** il loopback usa il formato di mix dell'endpoint. Se non è 48 kHz float stereo serve conversione e il confronto col golden non è diretto.
- **Altri suoni:** il loopback contiene il mix di tutte le sessioni. Il [process loopback](https://learn.microsoft.com/en-us/windows/win32/api/audioclientactivationparams/ns-audioclientactivationparams-audioclient_process_loopback_params) (build 20348 o successive, supportato da miniaudio) isola un processo ma non è stato provato: è da capire se vede lo stesso punto della catena.
- **Stringhe in `AudioSes.dll`:** esiste una classe `CLoopbackMixer` con messaggi `GLITCHED` interni. Il sistema ha quindi una nozione propria di problema nel loopback, ma non è esposta. Viene da una lettura delle stringhe, non da una fonte.

## Domanda 5: strumenti esterni

**LatencyMon** non rileva glitch audio. Misura latenze del sistema: durata di ISR e DPC, hard page fault e un tempo da interrupt a processo su una catena simulata con un proprio driver ([documentazione](https://www.resplendence.com/latencymon_interrupt2process)). Usa una sessione ETW del kernel e porta la risoluzione del timer a 1 ms ([note tecniche](https://www.resplendence.com/latencymon_technical)). L'autore sconsiglia di eseguire il test «while an audio program is active». Non può quindi affiancare il probe. Dice se la macchina è adatta, non se una corsa ha avuto un glitch, e un risultato pulito non esclude problemi: in un thread Q&A un utente riferisce click senza DPC anomali (fonte secondaria). Utile per la diagnosi di un FAIL, non per il PASS. Richiede un'installazione, non eseguita.

**Altri strumenti.** WPR e WPA servono per la causa, non per il verdetto. MXA è l'analizzatore raccomandato da Microsoft ma non è installato. Il test HLK *Glitchfree Low-period Full-duplex* verifica playback e cattura senza glitch, ma la pagina non spiega il metodo. La cattura con segnale noto è usata per verificare catene audio ([Bootlin](https://bootlin.com/blog/testing-audio-the-beauty-of-sine-waves/), fonte secondaria), ma non ho letto un metodo specifico per i dropout.

## Procedura raccomandata per CP-1

Nessun passo richiede privilegi, tranne ETW che è facoltativo.

**A. Preliminare, per ogni corsa, nel manifest.** Nome dell'endpoint, formato di mix, periodo del motore, modalità condivisa, offload, volume di flusso e sessione, lista degli effetti, sessioni attive. Formato inatteso, reroute o altra sessione attiva rendono la corsa `INVALID`.

**B. Controllo positivo, per ogni uscita e dopo ogni cambio di driver.** 15 s con stalli di 25 e 60 ms, tre cicli, a -80 dBFS. Atteso con buffer di 30 ms: nessun buco a 25 ms, un buco a 60 ms, visto sia nello scarto `IAudioClock` sia nel contenuto. Se un rilevatore non reagisce, il suo zero non vale per quella uscita. Vale il principio di AGENTS §3: un risultato ignoto non passa.

**C. Corsa `device_probe_v1` con cattura PCM e loopback.** La macchina resta senza altri suoni.

| # | Controllo | PASS proposto | Copre |
|---:|---|---|---|
| 1 | PCM della callback contro golden | Identico | L0 |
| 2 | Intervallo fra callback | `suspected_underruns` 0, margine riportato | L1 |
| 3 | Deficit `IAudioClock`, letto a 20 Hz da un thread non realtime | Nessun salto > 5 ms dopo l'avvio | L1, forse L3 |
| 4 | Contenuto del loopback allineato al riferimento | 0 slittamenti, 0 silenzi inattesi, 0 raffiche, ritardo costante | L1, L2 |
| 5 | Integrità della cattura | Overflow 0 | Validità |

Slittamento in aumento significa silenzio inserito a valle del client, slittamento in calo significa perdita in cattura. Il primo è FAIL, il secondo è `INVALID`. La soglia di residuo va tarata per uscita, non fissata al bit: USB non è identico al bit e HDMI sì.

**D. ETW facoltativo.** Serve una decisione del titolare, perché cambia una configurazione di sistema, anche se reversibile. Comandi **non eseguiti**:

```powershell
# una tantum, da amministratore, reversibile con /e:false
wevtutil sl Microsoft-Windows-Audio/GlitchDetection /e:true /ms:20971520
# ogni corsa, utente normale
Get-WinEvent -LogName 'Microsoft-Windows-Audio/GlitchDetection' -Oldest
```

In alternativa, una sessione con orologio QPC, che non cambia il sistema ma richiede privilegi a ogni corsa:

```powershell
logman start cym_audio -ets -p Microsoft-Windows-Audio 0x1400000000000000 0x5 -o audio.etl -ct perf
logman stop cym_audio -ets
```

ETW entra nel PASS solo se il passo B fa scattare l'evento 33 o un altro evento atteso, e se non risultano eventi persi. Altrimenti resta `NON DISCRIMINANTE`. Se scatta, il criterio proposto è 0 eventi 31–33 e 36–49 fra avvio e arresto del contenuto.

**E. Ascolto umano, obbligatorio.** Almeno una corsa completa per uscita, con registro delle anomalie. Copre L3–L5, che il software non vede.

**Interpretazione dello zero.** Zero eventi su un tempo T dà, al 95%, un limite superiore di circa 3/T (regola del tre). Tre corse da 184 s escludono solo frequenze superiori a circa una ogni 3 minuti. La qualificazione finale di M2.5.6 chiederà più tempo o carico controllato.

**Su FAIL.** WPR da amministratore e, se serve, LatencyMon senza il probe, per cercare la causa.

**Costo stimato.** Monitor `IAudioClock` circa 50 righe; strumento di controllo positivo circa 300, già scritto in prova; analisi già presente nell'attività parallela. Una giornata di lavoro. In esecuzione 15 s per il controllo positivo e 3,1 minuti per corsa. Nessuna nuova dipendenza del runtime: solo API di Windows, salvo `numpy` per l'analisi offline, che lo script già dichiara.

**Limite della procedura.** Copre bene L0–L2 e solo in parte L3. Non dimostra che il suono all'uscita fisica sia privo di disturbi.

## Fonti

Livelli: **FT** testo letto. **FT-parz** letto in parte o tramite riassunto dell'estrattore. **AM** solo metadati. **SEC** nota da fonte secondaria. Le pagine Microsoft con sorgente pubblico sono state lette nei repository `MicrosoftDocs`. Quelle indicate con «estrattore» passano da un modello automatico.

| Fonte | URL | Livello | Usata per |
|---|---|---:|---|
| Loopback Recording | [Learn](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording) | FT | Pin hardware, copia dal motore |
| `_AUDCLNT_BUFFERFLAGS` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/ne-audioclient-_audclnt_bufferflags) | FT | Significato dei flag |
| `AUDCLNT_STREAMOPTIONS` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/ne-audioclient-audclnt_streamoptions) | FT | Tap prima del volume |
| `IAudioCaptureClient::GetBuffer` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudiocaptureclient-getbuffer) | FT-parz | Flag, posizione |
| `IAudioClock`, `GetPosition` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclock-getposition) | FT | Posizione |
| `IAudioClock2::GetDevicePosition` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclock2-getdeviceposition) | FT | Posizione grezza |
| `IAudioClient::GetCurrentPadding` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getcurrentpadding) | FT | Padding |
| `IAudioClient::GetService`, `Initialize` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getservice) | FT-parz | Servizi, thread, loopback |
| `IAudioRenderClient::GetBuffer` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudiorenderclient-getbuffer) | FT-parz | Responsabilità del client |
| `IAudioStreamVolume` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudiostreamvolume) | FT-parz | Quattro fattori di volume |
| `IAudioClient3` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudioclient3) | FT-parz | Periodi del motore |
| `IAudioEffectsManager` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudioeffectsmanager) | FT | Elenco effetti |
| `AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/audioclientactivationparams/ns-audioclientactivationparams-audioclient_process_loopback_params) | FT | Process loopback |
| `XAUDIO2_PERFORMANCE_DATA` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/xaudio2/ns-xaudio2-xaudio2_performance_data) | FT-parz | Campo glitch |
| `AudioStateMonitor` | [Learn](https://learn.microsoft.com/en-us/uwp/api/windows.media.audio.audiostatemonitor) | FT | Muto e attenuazione |
| `AudioGraph`, eventi ed errori | [Learn](https://learn.microsoft.com/en-us/uwp/api/windows.media.audio.audiograph) | FT-parz | Assenza di statistiche |
| Low Latency Audio | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/low-latency-audio) | FT-parz | Stack audio, strumenti |
| Glitch Reporting for Offloaded Audio | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/glitch-reporting-for-offloaded-audio) | FT | Evento del driver |
| `EPcMiniportEngineEvent` | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/portcls/ne-portcls-epcminiportengineevent) | FT-parz | Parametri del glitch |
| `IPortClsEtwHelper::MiniportWriteEtwEvent` | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/portcls/nf-portcls-iportclsetwhelper-miniportwriteetwevent) | FT | Emissione |
| APO Architecture | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/audio-processing-object-architecture) | FT-parz | SFX, MFX, EFX |
| Hardware-Offloaded Audio Processing | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/hardware-offloaded-audio-processing) | FT-parz | Pin di loopback |
| Windows 11 APIs for APO | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects) | FT-parz | Pre e post volume, silenzio |
| `KSPROPERTY_AUDIOLOOPBACK` | [Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/ksproperty-audioloopback) | FT-parz | Tap point, 24H2 |
| `StartTrace`, `EnableTraceEx2` | [Learn](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew) | FT-parz | Permessi ETW |
| Built-in Recording Profiles | [Learn](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/built-in-recording-profiles) | FT, estrattore | Profilo Audio |
| WPT, esercizi 2 e 4 | [Esercizio 2](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/optimizing-windows-devices-for-multimedia-experiences-exercise-2), [Esercizio 4](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/optimizing-windows-devices-for-multimedia-experiences-exercise-4) | FT, estrattore | WPR, MXA |
| Delivering a great media experience | [Learn](https://learn.microsoft.com/en-us/windows-hardware/test/weg/delivering-a-great-media-experience) | FT, estrattore | MXA, soglie DPC e ISR |
| Glitchfree Low-period Full-duplex test | [Learn](https://learn.microsoft.com/en-us/windows-hardware/test/hlk/testref/4c6d68cd-cd62-41b4-ae87-8709d2fe5046) | FT, estrattore | Test HLK |
| Carr 2009, blog Windows 7 | [Learn](https://learn.microsoft.com/en-us/archive/blogs/e7/improving-audio-glitch-resilience-in-windows-7) | FT, estrattore | Definizione, telemetria |
| Audio Fidelity: Clipping, 2007 | [Learn](https://learn.microsoft.com/en-us/archive/blogs/audiofool/audio-fidelity-clipping) | FT, estrattore | Limiter finale, commento anonimo |
| van Eerde, loopback capture | [Blog](https://matthewvaneerde.wordpress.com/2008/12/16/sample-wasapi-loopback-capture-record-what-you-hear/) | FT-parz, estrattore | Silenzio, primo pacchetto |
| Q&A 1188388 | [Learn](https://learn.microsoft.com/en-us/answers/questions/1188388/persistent-audio-discontinuity-in-wasapi-loopback) | FT-parz, estrattore | Drenaggio |
| Chromium, `audio_low_latency_output_win.cc`, `..._input_win.cc` | [GitHub](https://github.com/chromium/chromium/blob/main/media/audio/win/audio_low_latency_output_win.cc) | FT-parz | Rilevatore in produzione, primo pacchetto |
| LatencyMon, interrupt to process | [Sito](https://www.resplendence.com/latencymon_interrupt2process) | FT-parz, estrattore | Cosa misura |
| LatencyMon, note tecniche | [Sito](https://www.resplendence.com/latencymon_technical) | FT-parz, estrattore | ETW, timer |
| Dechamps, WinSoftVol | [GitHub](https://github.com/dechamps/WinSoftVol) | FT-parz, estrattore | `CAudioLimiter` |
| Q&A su clic senza DPC anomali | [Learn](https://learn.microsoft.com/en-us/answers/questions/5491950/asio-audio-glitches-on-asus-rog-flow-x16-need-help) | SEC | Limite di LatencyMon |
| Bootlin, test audio con sinusoidi | [Blog](https://bootlin.com/blog/testing-audio-the-beauty-of-sine-waves/) | SEC | Cattura con segnale noto |
| `nasbench/EVTX-ETW-Resources` | [GitHub](https://github.com/nasbench/EVTX-ETW-Resources) | AM | Eventi `EVT_GLITCH` già nei manifest di Windows 8.1 e 10 |

**Evidenze locali.** `third_party/miniaudio/miniaudio.h` (funzioni `ma_device_read__wasapi`, `ma_device_write__wasapi`, `ma_device_init_internal__wasapi`, `ma_worker_thread`); intestazioni del SDK 26100 in `um`, `shared`, `winrt`; manifest di `Microsoft-Windows-Audio`; `wpr -profiles` e `wpr -profiledetails Audio`; stringhe di `WindowsPerformanceRecorderControl.dll`, `audioeng.dll`, `AudioSes.dll`.

## Limiti

**Non letto o non eseguito.**
- Nessuna sessione ETW, nessun canale attivato, nessun evento osservato scattare: servono privilegi.
- MXA e LatencyMon non sono stati eseguiti né installati.
- La sessione WinHEC 2015 citata da Microsoft sul laboratorio media non è stata letta.
- Non ho trovato una descrizione Microsoft degli eventi `op_EVT_GLITCH_*`.
- Il provider che riceve `eMINIPORT_GLITCH_REPORT` e il GUID dell'alias `AudioEngine` restano ignoti.

**Dedotto, non provato.**
- La mappa evento-strato viene dai nomi.
- La relazione 40 ms su miniaudio.
- La validità di `IAudioClock` per guasti del motore o del driver.
- La posizione del tap rispetto a EFX e al limiter.

**Limiti delle misure.**
- Una macchina, due uscite, 87 stalli di tipo noto. Il guasto è solo un client affamato: nessun guasto del motore o del driver è stato provocato.
- Tono a -80 dBFS, non contenuto a pieno livello, e canale 0 nel confronto al bit. Corse brevi, senza carico controllato. Volume master 1,0.
- Lo strumento è in WASAPI puro. Lo scarto è sommato su 300 ms dopo lo stallo, quindi due guasti ravvicinati si confonderebbero.
- Il legame fra `SILENT` e silenzio è verificato sui totali di corsa, non stallo per stallo.
- `Realtek Digital Output` è un endpoint attivo, ma non so se a valle ci sia qualcosa di collegato. Vale per il passo 4 di M2.5.1d: può evitare hardware nuovo solo se quell'uscita è davvero ascoltabile.

**Riproducibilità.** Sorgente, analisi e CSV sono in [m25_glitch_calibration](experiments/m25_glitch_calibration/README.md); eseguibile e catture non sono versionati.

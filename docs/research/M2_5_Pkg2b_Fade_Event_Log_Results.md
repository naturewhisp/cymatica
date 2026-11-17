# Pacchetto di misura 2b — Registro delle dissolvenze: risultati

- **Pre-registrazione:** [M2_5_Pkg2b_Fade_Event_Log_Preregistration.md](M2_5_Pkg2b_Fade_Event_Log_Preregistration.md), SHA-256 (LF) `fa2a5e95f1a6fec3e85513e6ba834893d511d16f96e54c518aa11a42c59f0a79`.
- **Esecuzione, 2026-11-17:** Release e Debug, scenario `fade_events_v1` sulla fixture (seed 1337). Dati: [esperimento](experiments/m25_pkg2b_fade_events/README.md).
- **Esito: lo strumento è validato** (E1–E6). La tabella sulla fixture è una misura `MEASURED`, descrittiva.

## 1. Validazione

| ID | Prova | Risultato | Esito |
|---|---|---|---|
| E1 | Nota di Corpo al sustain, NoteOff | 1 `release`, 96 frame, A = 0,75 · 0,8 · 0,25 esatta, guadagno di bus = `kCorpoGain`; con il rilascio dello slot a 1440 frame il registro riporta 1440 | Confermata |
| E2 | Furto e taglio di rilascio su Corpo | 1 `steal` e 1 `release_cut`, durata 0, A uguale all'inviluppo del momento; contatori dell'allocatore 1 e 1 | Confermata |
| E3 | Pulso già decaduto prima del NoteOff | Nessun record | Confermata |
| E4 | Coerenza con i contatori, fixture | `steal` 0 = 0 per ogni ruolo; `release_cut` 0 / **190** / 0 / 0 = contatori | Confermata |
| E5 | Ripetizione | Identica | Confermata |
| E6 | PCM con il registro installato | Golden PCM `PASS` | Confermata |

- **Predizione descrittiva:** `release_cut` di Corpo **190**. Confermata.
- **Differenze dichiarate rispetto al §1 della pre-registrazione:**
  - nel codice lo **Stop** non è un rilascio: ferma l'oscillazione e lascia una coda lineare, come il furto, quindi il registro lo riporta con durata 0;
  - la **Pause** è una rampa fisica di 2 ms sull'uscita della voce, e ha una causa propria (`pause`).

  Nella fixture nessuno dei due casi compare: a Stop le voci sono già spente, e non ci sono pause.
- **Variante dell'E1:** il test imposta il rilascio dello slot direttamente, perché l'override vero passa solo dal motore offline. Verifica che il registro riporti il rilascio dello slot, non il percorso dell'override.

## 2. Misura sulla fixture

1 582 interruzioni di voce in 183 s:

| Ruolo | Causa | Eventi | Ampiezza in uscita, mediana / massima | Durata della dissolvenza |
|---|---|---:|---|---|
| Pulso | `release` | 416 | −26,0 / −22,3 dBFS | 2 ms |
| Corpo | `release` | 192 | −24,5 / −24,0 dBFS | 2 ms |
| Corpo | `release_cut` | 190 | −24,5 / −24,0 dBFS | **taglio**, coda lineare di 2 ms |
| Trama | `release` | 400 | −27,3 / −26,7 dBFS | 2 ms |
| Vettore | `release` | 384 | −21,7 / −20,9 dBFS | 2 ms |

**Lettura:**
- **Ogni nota della fixture finisce con una dissolvenza di 2 ms a pieno livello**, fra −21 e −27 dBFS in uscita, non dopo un decadimento. Tutti i 1 582 eventi superano il bin di −40 dBFS. È il dato che mancava a H-03: il sospetto del titolare sulle dissolvenze di 2 ms riguarda ogni nota, non solo i cambi d'accordo.
- **I 190 tagli di Corpo** avvengono a pieno livello di sustain: −24,5 dBFS, la stessa ampiezza dei rilasci. Il NoteOff e il NoteOn successivo cadono sullo stesso frame, quindi la voce appena rilasciata viene ripresa subito dalla nota nuova.
- **Il kick viene troncato dal NoteOff:** 416 rilasci a −26 dBFS di mediana. Le durate pianificate delle note di Pulso sono 53,6 ms (128 note) e 62,5 ms (288 note), mentre il suo decadimento naturale dura 50–120 ms. È esattamente la «durata di 54–62 ms» notata dall'esperto (ADR-0007 §8.1). **Per F4b:** la durata del kick oggi la decide il NoteOff del planner, non il timbro. Allungarla vuol dire cambiare la durata delle note di Pulso oppure il comportamento del rilascio, e va deciso prima di toccare l'inviluppo.
- Lo strumento non dice se queste dissolvenze si sentono. Le domande percettive restano a H-03, al pacchetto 4 per il mascheramento e all'ascolto cumulativo.

## 3. Codice

- **Allocatore:** `FadeEventLog` a capacità fissa (`engine/audio/fade_event_log.h`), `VoiceAllocator::setFadeEventLog` e `logFade`, chiamato su NoteOff abbinato, furto, taglio di rilascio, Stop, Pause e scadenza di sicurezza. Il puntatore è nullo per default: il percorso realtime non cambia, come dimostra E6.
- **Motore e probe:** `AudioEngine::setFadeEventLog` (quiescenza); `ProbeAnalysisCapture::fadeLog` nel probe della fixture.
- **Scenario e test:** `fade_events_v1`; test `[fade_event_log]`.
- **Build e suite:** Release e Debug headless `/WX` 286/286; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.

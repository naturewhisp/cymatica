---
trigger: always_on
description: Invarianti su serializzazione JSON Windows, callback realtime bounded ed elaborazione DSP click-free.
---

# Invarianti di Serializzazione e Realtime — CYMATICA

## 1. Serializzazione JSON e Percorsi Windows
- Tutti i serializzatori JSON (`RunRecord`, `AudioProbe`, manifest CACP, replay `.cymtest`, telemetria) devono produrre output rigorosamente valido secondo RFC 8259.
- I percorsi Windows contenenti backslash (`\`) DEVONO essere sempre serializzati con escape esplicito (`\\`), evitando qualsiasi sostituzione post-hoc o string hacking (`.Replace('\', '/')`) negli script CI o di test.
- Ogni struttura dati che esporta JSON deve avere un test associato che ne verifica il parsing diretto nativo (`nlohmann::json::parse()` in C++ o `ConvertFrom-Json` in PowerShell).

## 2. Callback Realtime Bounded (Hard Realtime)
- Nessun ciclo all'interno di una callback realtime (`processBlock`, rendering audio, fixed-step tick) può iterare in modo illimitato su code o strutture esterne.
- Qualsiasi svuotamento di code SPSC in tempo reale deve essere delimitato da un budget massimo per blocco predeterminato (es. `AudioEngine::kMaxScheduledEventsPerBlock`).
- Gli eventi in eccesso devono essere differiti al blocco successivo e tracciati nella telemetria (`deferredScheduledEvents`), garantendo complessità $O(1)$ rispetto alle risorse computazionali del frame.

## 3. Gestione Voci, Transizioni di Epoch e Micro-fadeout Anti-Click
- Le transizioni di trasporto/epoch (`SetTempoMap`, salti di misura) non devono mai lasciare voci attive prive di `NoteOff` o orfane in stato di oscillazione indefinita.
- Qualsiasi terminazione di voce, cambio di frequenza discontinuo o operazione di voice-stealing DEVE applicare un micro-fadeout lineare a zero di 2 ms (96 campioni a 48 kHz, conforme ad ADR-0003) prima di mutare i parametri o silenziare l'uscita, eliminando pop e click di fase.
- Le verifiche audio di spegnimento note devono testare direttamente i campioni PCM accertando silenzio numerico assoluto (0 crossing, ampiezza 0.0f).

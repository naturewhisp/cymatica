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
- Le transizioni di trasporto/epoch (`SetTempoMap`) sono non distruttive e non devono mai troncare arbitrariamente voci attive o pad in `Sustain`; i `NoteOff` cross-epoch matching vengono regolarmente applicati al loro frame campionario nominale.
- Qualsiasi terminazione forzata di voce, furto di voce (voice stealing su saturazione) o transizione di pausa/stop deve applicare una rampa di de-click lineare di 2 ms (96 campioni a 48 kHz, 88 a 44.1 kHz, calcolata come $\lfloor 0.002 \times f_{s,\text{device}} \rfloor$).
- Nelle steal tails sature, il residuo composito per canale $S_{\text{ch}} = y_{\text{tail, ch}}[0] + y_{\text{voice, ch}}[0]$ decade progressivamente senza clamp prematuro pre-master. L'assenza percepita di click deve essere sempre verificata empiricamente su campioni PCM ed ascolto reale.
- Separazione dei domini di trasporto: `Pause` congela immediatamente ($O(1)$) la timeline logica di simulazione a $t_{\text{pause}}$, mentre la tail di de-click fisica opera esclusivamente nel buffer del device garantendo silenzio numerico ($0.0\text{f}$) a partire da $n_{\text{device, pause}} + N_{\text{tail}}$.

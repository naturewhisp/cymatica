# M2.5.3–4 — Scenario tecnico del cambio di metro (`meter_change_v1`): pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice dello scenario. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione di riferimento:** ADR-0007 D-M2-21 (opzione B + C) e specifica 0.8.5 §32: «il cambio di time signature via `SetTempoMap` [è] qualificato da uno scenario tecnico ripetibile con manifest, distinto dalla fixture musicale». È il primo pacchetto di M2.5.3–4 (D-M2-38, E3).
- **Origine:** la [prova di fattibilità](M2_5_1b_Meter_Feasibility.md) e il suo test [`test_meter_feasibility.cpp`](../../tests/unit/test_meter_feasibility.cpp). Lo scenario promuove quella prova a qualificazione eseguibile, con manifest e verdetto, senza toccare la fixture musicale.

## 1. Scenario

Forma di 24 battute in tre sezioni da 8: A in 4/4, B in **3/4** con il motivo di 12 step, A′ in 4/4. 120 BPM, due frasi per sezione, seed 1, catena offline a 48 kHz con blocchi da 480 frame: planner, scheduler e motore reali. I due `SetTempoMap` vengono inviati una battuta prima del confine, lo Stop una battuta prima della fine.

## 2. Controlli vincolanti (verdetto PASS solo se tutti passano)

| ID | Controllo | Atteso |
|---|---|---|
| **M1** | Orologio: frame delle battute 8, 16, 24 | 768 000, 1 344 000, 2 112 000 |
| **M2** | Ack dei due `SetTempoMap` e dello Stop | `Applied` ai frame 768 000, 1 344 000 e 2 112 000 |
| **M3** | Contabilità della catena | 0 eventi persi, scheduler vuoto, eventi inviati = 2 × intent generati, tutte le 6 frasi pianificate |
| **M4** | Piano in 3/4 | Nessun fallback in nessuna frase, candidati validi = generati. Ogni NoteOn di B sulla griglia di semicrome (6000 frame); un NoteOn di Corpo su ogni battere delle battute 8–15 |
| **M5** | Segnale | Campioni tutti finiti; energia > 0 in ciascuna sezione |
| **M6** | Ripetibilità | Hash canonico del piano e hash del PCM identici in due esecuzioni |

**Predizione:** verdetto PASS, con tutti i controlli M1–M6 superati. Sono gli stessi fatti già verificati dal test di fattibilità, ora riuniti in uno scenario con manifest.

## 3. Uso

- Si esegue con `cymatica_game --cacp-run meter_change_v1` e il manifest si archivia.
- Entra in `full_m2_suite` come voce vincolante, e un test unitario lo esegue in CI.
- L'hash del PCM registrato alla prima esecuzione diventa il golden dello scenario.

## 4. Limiti

- Qualifica il **supporto tecnico** del cambio di metro, non la sua riconoscibilità musicale: in 3/4 non c'è una gerarchia di accenti (dossier di fattibilità, Limiti). Il metro musicale resta fuori perimetro (R-M25-11, non scelto).
- I metri composti e il cambio di `barsPerPhrase` (CL-MM-01) non sono coperti.

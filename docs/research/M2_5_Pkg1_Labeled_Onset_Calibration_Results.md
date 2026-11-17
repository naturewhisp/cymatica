# Pacchetto di misura 1 — Calibrazione degli onset etichettata e vincolata: risultati

- **Pre-registrazione:** [M2_5_Pkg1_Labeled_Onset_Calibration_Preregistration.md](M2_5_Pkg1_Labeled_Onset_Calibration_Preregistration.md), SHA-256 (LF) `3c9b3e4ea03bf70efc4ebe9f8cb76fa01c7265e675dd4297b987d2ef5723a9a5`, registrata prima del codice e delle misure.
- **Esecuzione, 2026-11-17:** Release, due corse di `acao02_v1` sulla fixture con golden PCM `18256941985354732249`; oracolo mir_eval 0.8.2 nell'ambiente fissato `tools/pyenv`. Dati: [esperimento](experiments/m25_pkg1_labeled_calibration/README.md).

## Validazione dello strumento

| ID | Prova | Risultato |
|---|---|---|
| V1 | Attacchi e rilasci con latenza 10 ms | Offset 10 ms; 40 attacchi e 40 rilasci etichettati; precisione e richiamo 1 (`[labeled_calibration]`) |
| V2 | Solo rilevazioni sui rilasci (il caso dei 95 ms) | Nessun punto ammissibile, offset indefinito: la degenerazione è impossibile |
| V3 | Riferimenti spostati di 95 ms | Non ammissibile, richiamo 0 |
| V4 | Due corse di `acao02_v1` | Manifest identici (salvo `build_id`) |
| V5 | mir_eval sulle stesse liste, finestra 0,05 s | **Concorda** su tutti i ruoli: abbinamenti 416 / 95 / 400 / 383, F uguale a 1·10⁻¹² |

Un test in più verifica che un offset di 40 ms, dentro la tolleranza delle etichette ma fuori dalla finestra, sia riconosciuto e non ammesso.

## Predizioni sulla fixture

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| Q1 | Trama: δ 0,03–0,15, offset 10–12 ms, richiamo degli attacchi ≥ 0,98; nessun punto fuori da [−4,33; 25,67] ms | δ **0,15**, offset **10,875 ms**, richiamo 1,0. Il punto δ 0,3 (prima: 95,2 ms) ha 0 attacchi e 400 rilasci, quindi non è ammissibile | Confermata |
| Q2 | Pulso invariato | δ 0,3, −12,67 ms | Confermata |
| Q3 | Vettore invariato | δ 0,2, −9,33 ms | Confermata |
| Q4 | Corpo, offset invariato | −10,67 ms (δ 0,3) | Confermata |
| Q5 | `acao02_v1` resta `INCONCLUSIVE` | Unico motivo: profilo calibrato ≠ congelato | Confermata |

## Lettura

- La calibrazione non può più allinearsi ai rilasci, e le rilevazioni sui rilasci sono contate come tali. Sulla Trama sono 400 a δ 0,03–0,3, cioè una per nota: il problema F6/H-03 resta visibile come numero, non come falso positivo.
- **La correzione del dossier:** il pacchetto 1 *non* sblocca da solo `acao02_v1`. Lo stato resta `INCONCLUSIVE` finché il profilo non si ricongela, compito di M2.5.5 sul contenuto congelato. Il dossier diceva «sblocca».
- **Ambiente dell'oracolo:** Python 3.12.10 in `tools/pyenv/.venv`, con wheel fissati per hash in [`requirements-oracle.txt`](../../tools/pyenv/requirements-oracle.txt): numpy 2.2.6, scipy 1.15.3, mir_eval 0.8.2, decorator 5.3.1. Venv e wheel sono ignorati da git. Installazione senza rete: `--no-index --find-links wheels --require-hashes`.

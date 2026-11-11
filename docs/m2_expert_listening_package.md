# M2 — pacchetto per l'ascolto dell'esperto (fine Inc-5)

- **Data:** 2026-11-03
- **Fase:** M2.4 Inc-5, 5e.4b (ADR-0006 5e-10)
- **Status:** materiale di lavoro per una revisione **qualitativa**. Non è un gate e non è una sessione alla cieca, rinviata a M2.5. L'esito si registra, anche se negativo o inconcludente, nel formato del §4, e alimenta le decisioni di M2.5 ([dossier delle evidenze](m2_procedural_evidence.md) §4).
- **Ascolto precedente:** [2026-10-28](m2_4_human_listening_2026-10-28.md), da cui F1–F4.
- **Esito:** ascolto eseguito il 2026-11-03, [registro](m2_4_human_listening_2026-11-03.md).

## 1. Materiali

I file non sono versionati (dimensioni); si rigenerano in modo deterministico dalla build Release.

| # | Materiale | File | Comando |
|---|---|---|---|
| M1 | Brano completo della fixture (seed 1337, 182,9 s, A–B–A') | `artifacts/cacp/procedural_v1/procedural_v1.wav` | `cymatica_game --cacp-run procedural_v1` |
| M2 | Viste «a pentagramma» per frase (24 PNG): intervalli pianificati, NoteOn applicati, onset rilevati, abbinati e sui rilasci. Da M2.5.1c (P-M25-08) ogni corsia porta il nome di uno strumento riconoscibile (KICK / DRUMS, BASS + CHORDS, ARP SYNTH, LEAD SYNTH) con il ruolo interno sotto, l'intestazione riporta frase, sezione, tempo, metro e numeri di battuta, la griglia mostra battute, quarti e sedicesimi e la legenda dei colori è in fondo all'immagine | `artifacts/cacp/procedural_v1/staff_p00.png` … `staff_p23.png` | idem |
| M3 | Clip A/B del rilascio: frasi 5 (sezione A) e 10 (sezione B), release di 2 (produzione), 15, 30, 50 e 80 ms | `artifacts/cacp/release_ab_v1/fixture_p05_release_02ms.wav` … `fixture_p10_release_80ms.wav` (SHA-256 in [inc5_release_ab](research/experiments/inc5_release_ab/clips_sha256_2026-11-03.txt)) | `cymatica_game --cacp-run release_ab_v1` |

Le clip M3 differiscono **solo** nella durata del rilascio dei NoteOff ordinari:
- eventi, seed e rendering sono identici (5e.3);
- l'RMS del mix è sostanzialmente invariato, −23,9 / −23,8 dBFS nelle clip misurate, cioè entro circa 0,1 dB (rettifica OSS-5e.4b-02: la prima versione diceva «uguale»);
- il segnale differenza rispetto alla variante di 2 ms sta 15–24 dB sotto il mix;
- il picco cresce fino a +1,5 dB a 80 ms.

È un esperimento su rilasci diversi, non un confronto fra PCM identici: le clip differiscono per costruzione proprio nel rilascio.

## 2. Procedura proposta

1. **Ascolto:** in cuffia e su diffusori, a volume fisso per tutta la sessione.
2. **M1:** ascolto completo, prima libero, poi con le domande del §3.
3. **M3:** per ogni frase, la 5 e la 10, le cinque varianti in un **ordine mescolato da chi conduce**, che conserva la chiave fino a fine sessione. L'esperto valuta ciascuna clip e poi indica la preferita. Le etichette dei file non vanno mostrate.
4. **M2:** usate dopo l'ascolto, per localizzare i punti commentati. Mostrano la struttura e non sostituiscono l'orecchio.
5. **Registrazione:** si annotano i giudizi nel modulo del §4, insieme a apparecchiatura e condizioni.

## 3. Domande

| # | Domanda | Collegamento |
|---|---|---|
| Q1 | Nel brano completo si sentono crackle, click o artefatti? Dove (minuto e secondo, sezione)? | criterio M2.5 «senza crackle» |
| Q2 | Le sezioni A, B e A' sono distinguibili? La transizione di tempo è convincente o brusca? | criterio M2.5, F3 |
| Q3 | Il kick (Pulso) sostiene il metro? Il tempo forte si percepisce? | F1 |
| Q4 | L'immagine stereo della Trama è sbilanciata? Disturba in cuffia? | F2, AG-MM-12 |
| Q5 | Ripetitività: dove diventa fastidiosa, se lo diventa? C'è sviluppo del materiale? | F3, F5, CL-MM-03/05 |
| Q6 | Timbro e mix: livelli relativi dei ruoli, FM della Trama, eventuali suoni «dissonanti» | F4 |
| Q7 | Clip M3: si sente la differenza fra i rilasci? Da quale durata? Quale suona più naturale per ciascuna frase? Ci sono effetti indesiderati con rilasci lunghi, come impasto o sovrapposizioni? | F6, decisione H-03 di M2.5 |
| Q8 | Osservazioni libere su musicalità e direzione artistica («ricordi, sviluppi, sorprenda e ritorni») | principi del titolare |

## 4. Modulo di risposta

```text
Data e ora:
Esperto:
Apparecchiatura (cuffie / diffusori / interfaccia):
Volume (fisso, riferimento):
Build / commit dei materiali:
Q1:
Q2:
Q3:
Q4:
Q5:
Q6:
Q7 frase 5 — ordine presentato e chiave:      giudizi per clip:      preferita:
Q7 frase 10 — ordine presentato e chiave:     giudizi per clip:      preferita:
Q8:
Esito complessivo (libero, anche negativo o inconcludente):
```

## 5. Limiti

- È un solo ascoltatore su una sola fixture: il risultato orienta le decisioni di M2.5, non le valida statisticamente.
- L'ordine mescolato riduce l'effetto delle etichette, ma l'ascolto non è alla cieca nel senso del protocollo di valutazione: chi conduce conosce la chiave.
- Le clip M3 sono estratti di 7–8 s. Effetti sul lungo periodo (affaticamento, impasto su sezioni intere) vanno valutati su M1, che però usa solo il rilascio di produzione.

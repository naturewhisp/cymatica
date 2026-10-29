# M2.4 Inc-5 fase 5c — onset per ruolo sugli stem: calibrazione e verifica di AC-AO-02

- **Data:** 2026-10-29
- **Stato:** dossier di misura, non normativo. Attua [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md):
  - D-M2-13: stem per ruolo;
  - D-M2-14: descrittori;
  - D-M2-16: calibrazione R6 e AC-AO-02 proposto.
- **Valori:** tutti `MEASURED`. Le soglie sono **proposte**, da ratificare al gate M2.5; nessun valore è `validated`.
- **Riproduzione:** `cymatica_game --cacp-run acao02_v1` (Release). Il manifest completo è in [experiments/inc5_calibration/acao02_v1_2026-10-29.json](experiments/inc5_calibration/acao02_v1_2026-10-29.json), build `83529f0` più il working tree di 5c. La corsa dura circa 10 s su AMD Ryzen 7 7700X.
- **Basi:**
  - [dossier d'ingresso di Inc-5](M2_4_Inc5_Entry_Research.md) §4: rilevatore, ground truth, ordine calibrazione/verifica, fonti O1–O7;
  - [dossier sui metodi CACP](M2_4_CACP_Evaluation_Methods.md) §3.2: E1 sul master.

## 1. Domande

1. **Q1:** gli onset del Music Intent sono osservabili **per ruolo** sugli stem, con quale precisione, richiamo e scarto?
2. **Q2:** i parametri calibrati sul seed 1337 reggono su contenuti diversi, mai usati per sceglierli?
3. **Q3:** gli stem ricostruiscono il mix dei bus, e la presenza di un ruolo si misura sul PCM e non sullo stato di uno spettrogramma (OSS-5b-01)?

## 2. Procedura (fissata prima delle misure)

1. **Stem:** stereo float32 per ruolo, presi in `RoleBusProcessor::processAndMix` dopo gain, ducking e delay, più il mix dei bus.
   - I frame di silenzio «congelato» (pausa o stop) sono scritti come zeri, così il frame *i* dello stem è il frame *i* del rendering.
   - Il sink è nullo nel percorso realtime e si installa solo in quiescenza.
2. **Ground truth per ruolo:** frame dei NoteOn `Applied` del ruolo, dall'applied-event log di 5a. I NoteOn entro 30 ms sono fusi tenendo il primo.
3. **Rilevatore:** `descriptors_v1` sul mono (L+R)/2 dello stem.
   - Parametri per ruolo: solo δ e uno scarto costante sottratto alle rilevazioni.
   - Abbinamento a cardinalità massima: ±50 ms come valore primario, ±25 ms come diagnostica.
4. **Fase a, latenza intrinseca:** set sintetico A, renderizzato dal synth reale, con δ 0,10 e scarto nullo. Per ogni ruolo: 24 note ogni 0,5 s, altezza, velocity 0,8 e timbro 0,5 fissi.
5. **Fase b, calibrazione sul solo seed 1337:**
   - griglia δ ∈ {0,02, 0,03, 0,05, 0,07, 0,10, 0,15, 0,20, 0,30};
   - per ogni δ, scarto = mediana dell'errore firmato della rilevazione più vicina entro ±100 ms;
   - scelta = F massimo a ±50 ms dopo la correzione; a parità vince la δ maggiore.
6. **Fase c, congelamento:** il profilo `stem_onset_profile_v1` si scrive nel codice con il suo SHA-256 **prima** di eseguire la verifica (`frozenStemProfileV1`, [acao02_calibration.cpp](../../engine/cacp/acao02_calibration.cpp)). Un test ne blocca l'hash.
7. **Fase d, verifica con il profilo congelato, senza ritocchi:**
   - (1) seed della fixture con fingerprint di contenuto diverso dal 1337, da una scansione dichiarata dei seed 1–32 (al massimo 3 renderizzati);
   - (2) `planner_probe_v1`;
   - (3) set sintetico B: 40 note per ruolo, intervalli irregolari 0,18–0,70 s, altezze su due ottave, velocity 0,5–1,0, timbro 0,2–0,9, durate pari al 60 % dell'intervallo.
8. **Soglie proposte:** per ogni ruolo, il valore minimo misurato sugli insiemi di verifica meno 0,05, arrotondato per difetto a 0,05.

## 3. Risultati

### 3.1 Stem, ricostruzione, presenza (Q3)

| Misura | Esito |
|---|---|
| Frame catturati / overflow | 8.825.200 / 0 (tutta la fixture) |
| Memoria degli stem (4 ruoli + mix dei bus) | 356.000.000 byte (≈ 356 MB) |
| Ricostruzione: somma degli stem nell'ordine del bus contro il mix dei bus | **esatta al bit**: 0 campioni diversi |
| Presenza della Trama sul PCM, frasi 0–3 di A | `EXACT_SILENCE` in tutte e 4 (maschera: Trama assente) |
| Presenza della Trama sul PCM, frasi 4–7 di A | presente, picco −24,73 dBFS |

La presenza si misura sul PCM degli stem (picco, RMS, silenzio esatto), non sullo stato del mel: è la richiesta di OSS-5b-01.

### 3.2 Latenza intrinseca (set A, δ 0,10, nessuno scarto)

| Ruolo | Riferimenti | Rilevazioni | R | Errore mediano [p05, p95] | Rilevazioni non abbinate: ritardo dall'attacco |
|---|---|---|---|---|---|
| Pulso | 24 | 24 | 1,00 | −12,0 ms [−14,0; −10,0] | — |
| Corpo | 24 | 48 | 1,00 | −9,3 ms [−11,3; −7,3] | 392 ms (durata nota 400 ms) |
| Trama | 24 | 48 | 1,00 | +9,3 ms [+7,3; +11,3] | 237 ms (durata 250 ms) |
| Vettore | 24 | 48 | 1,00 | −10,7 ms [−12,7; −8,7] | 288 ms (durata 300 ms) |

- **Scarto:** −9…−12 ms per i ruoli con attacco breve. È coerente con la convenzione al centro del frame e con l'anticipo dei rilevatori a flux (O3 del dossier d'ingresso: «one frame earlier») e con E1 (circa −8 ms sul master).
- **Trama:** è in ritardo di circa 9 ms, coerente con il suo attacco di 20 ms (il più lento).
- **Rilevazioni in più:** cadono **sul rilascio** di ogni nota (vedi §4, F6).

### 3.3 Calibrazione sul seed 1337 e profilo congelato

Griglia (F a ±50 ms dopo la correzione dello scarto):

| Ruolo | 0,02 | 0,05 | 0,10 | 0,15 | 0,20 | 0,30 |
|---|---|---|---|---|---|---|
| Pulso | 1,000 | 1,000 | 1,000 | 1,000 | 1,000 | 1,000 |
| Corpo | 0,154 | 0,345 | 0,399 | 0,792 | 0,990 | 0,990 |
| Trama | 0,666 | 0,667 | 0,667 | 0,667 | 0,667 | 0,549 |
| Vettore | 0,434 | 0,936 | 0,997 | 0,997 | 0,997 | 0,996 |

**Profilo congelato `stem_onset_profile_v1`:** SHA-256 `1ad4410052efe85e0fb4d4ea00c7e77383da9a1cea05e4b49fc5fea93bd8b3cd`, uguale al ricalcolo. È congelato il 2026-10-29, prima della verifica.

| Ruolo | δ | Scarto | P / R / F sulla calibrazione |
|---|---|---|---|
| Pulso | 0,30 | −11,67 ms | 1,000 / 1,000 / 1,000 |
| Corpo | 0,30 | −10,67 ms | 0,990 / 0,990 / 0,990 |
| Trama | 0,20 | +9,96 ms | 0,500 / 1,000 / 0,667 |
| Vettore | 0,20 | −9,33 ms | 0,997 / 0,997 / 0,997 |

Gli scarti calibrati coincidono con la latenza intrinseca del set A entro circa 1 ms: le due stime indipendenti si confermano a vicenda. Per la Trama, la metà delle rilevazioni sono rilasci (400 su 800) a ogni δ fino a 0,20; a 0,30 cala il richiamo.

### 3.4 Verifica (profilo congelato)

| Insieme | Ruolo | Rif. | Ril. | P | R | F | Ril. su rilasci | P senza i rilasci |
|---|---|---|---|---|---|---|---|---|
| (1) altri seed | tutti | — | — | — | — | — | — | **INCONCLUSIVE**: nessuno dei seed 1–32 produce un contenuto diverso dal 1337 (1 solo fingerprint distinto) |
| (2) `planner_probe_v1` | Pulso | 40 | 39 | 1,000 | 0,975 | 0,987 | 0 | 1,000 |
| | Corpo | 5 | 5 | 0,800 | 0,800 | 0,800 | 1 | 1,000 |
| | Trama | 36 | 57 | 0,632 | 1,000 | 0,774 | 21 | 1,000 |
| | Vettore | 32 | 32 | 0,969 | 0,969 | 0,969 | 1 | 1,000 |
| (3) sintetico B | Pulso | 40 | 40 | 1,000 | 1,000 | 1,000 | 0 | 1,000 |
| | Corpo | 40 | 69 | 0,580 | 1,000 | 0,734 | 29 | 1,000 |
| | Trama | 40 | 66 | 0,455 | 0,750 | 0,566 | 36 | 1,000 |
| | Vettore | 40 | 77 | 0,519 | 1,000 | 0,684 | 37 | 1,000 |

**In tutti gli insiemi ogni falso positivo è una rilevazione sul rilascio di una nota del ruolo**: la precisione senza i rilasci vale 1,000 ovunque. È una diagnostica dichiarata (`release_detections`, `precision_excl_releases_50`) e non cambia la metrica primaria.

### 3.5 Soglie proposte per AC-AO-02 (±50 ms, regola del §2.8)

| Ruolo | Richiamo | Precisione (tutte le rilevazioni) |
|---|---|---|
| Pulso | 0,90 | 0,95 |
| Corpo | 0,75 | 0,50 |
| Trama | 0,70 | 0,40 |
| Vettore | 0,90 | 0,45 |

**Alternativa da decidere al gate M2.5, non applicata:** dato il §3.4, una precisione calcolata escludendo le rilevazioni entro ±50 ms da un NoteOff applicato del ruolo varrebbe 1,000 in tutti gli insiemi. La soglia proposta con la stessa regola sarebbe 0,95 per ogni ruolo. Anche il NoteOff è un evento dell'intent, quindi la correlazione «onset ↔ intent» lo comprende in modo naturale. La regola dichiarata in anticipo resta però quella del §2.8, e il cambio richiede una decisione.

## 4. Finding

- **F6 — Rilascio di 2 ms (qualità audio, nuovo).** Ogni nota finisce con una rampa lineare di `declickFrames_` (2 ms, 96 frame a 48 kHz) dal livello di sustain a zero ([voice_allocator.cpp](../../engine/audio/voice_allocator.cpp)).
  - *Cosa produce:* un transitorio a banda larga misurabile a ogni NoteOff. Per la Trama (attacco di 20 ms) il transitorio di rilascio è forte quanto l'attacco: precisione 0,50 sulla fixture.
  - *Udibilità:* non è verificata e non va dedotta da questa misura. È plausibile su note sostenute e frequenze basse, ed è coerente con l'impressione di loop «giustapposti» dell'ascolto esperto.
  - *Proposta per il tuning M2.5:* uno stadio di release dell'ADSR proporzionato al ruolo (decine di ms), da misurare con questo stesso scenario e da verificare con l'ascolto. Collegata a F4.
- **Diversità fra seed nulla (risultato negativo, D-M2-16).** I 32 seed scansionati producono lo stesso fingerprint di contenuto del seed 1337: la diversità osservata è `MEASURED` e **nulla**. È il limite già noto, il seed agisce solo sui pareggi, ora misurato. La generalizzazione della calibrazione ad altri seed è `INCONCLUSIVE`. Restano validi gli insiemi (2) e (3), che sono contenuti diversi. Collegato a F3 e F5.
- **Richiamo della Trama nel set sintetico B: 0,75.** Mancano 10 attacchi su 40, con il profilo congelato (δ 0,20). La causa non è stata isolata in 5c. Ipotesi da verificare: attacco di 20 ms a velocity bassa sotto la soglia; vicinanza fra il rilascio di una nota e l'attacco della successiva negli intervalli brevi. La metrica non è stata ritoccata.
- **Il master non basta per la Trama, gli stem sì.** Sul master E1 dava per la Trama un richiamo di 0,31 a δ 0,10; sullo stem il richiamo è 1,00 sulla fixture. È la conferma della scelta O1 (stem offline) del dossier CACP.

## 5. Limiti

- **Insieme (1) vuoto:** la verifica sulla musica della fixture con altri seed non è possibile finché il generatore non produce contenuti diversi.
- **Corpo in `planner_probe_v1`:** solo 5 riferimenti, quindi la sua stima è molto incerta.
- **Insiemi sintetici:** sono contenuti scelti dall'agente. Sono diversi da A e dalla fixture, ma non sono musica.
- **Udibilità:** le soglie proposte misurano la catena «intent → evento sullo stem», non l'udibilità: intent non è percezione (dossier CACP §8).
- **Prestazioni:** il sink degli stem aggiunge un confronto di puntatore per frame nella callback quando è nullo. `profile_test` resta PASS (p99.9 peggiore 0,24 ms) senza un confronto A/B interlacciato.

# M2.5 CP-3 — I1 e I2, kick: indagine. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima del codice e delle misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione:** D-M2-51, [piano di CP-3](../M2_5_CP3_Expert_Plan.md). Il titolare: «Commit e I1–I2» (2026-11-23).
- **Giudizi da avvalorare** (esperto, 2026-11-19): «non ho gradito molto nel primo brano il cambio di nota sul kick»; «gradirei un kick un po' più presente». Alla domanda chiusa ha risposto «è giusto»: la richiesta di presenza è quindi una preferenza moderata.
- **Già noto dal codice:** Pulso suona la tonica due ottave sotto, Re2 (73,4 Hz) in A e A′ e Sol1 (49,0 Hz) in B. Il corpo è un seno con sweep di 8 ms; il click è rumore in banda fra 1,2 e 3,5 kHz per 15 ms.

## 1. Fonti

| Fonte | Letta | Uso |
|---|---|---|
| Deruty, *Harmonic and transposition constraints arising from the use of the Roland TR-808 bass drum*, arXiv:2502.07524 (la pagina dichiara ISMIR 2024) | In parte: corpo v1 fino a circa il 74 %, il resto da un riassunto automatico | Una fondamentale grave si riproduce e si sente meno. Da circa 49,5 a 37 Hz: −6,3 dB per i monitor da vicino (letto). L'equivalente d'udito (circa 5,5 dB) e il totale di 11,8 dB vengono dalla parte riassunta. Riguarda l'808, che suona a lungo; il nostro kick dura 107–122 ms |
| Sound On Sound, *How do I tune my kick drum samples to fit my song?* | Solo l'estratto della ricerca | Opinione contraria: un sub intonato evidente disturba l'armonia se è fuori tonalità |
| Hove e altri (PNAS 2014) e uno studio che lo contesterebbe | **No**, pagine bloccate | Non usati come evidenza |

Nessuna fonte trovata è uno studio d'ascolto controllato sull'accordatura del kick. La fonte di base resta l'esperto; qui si misura ciò che la misura può dire.

## 2. Strumento

- **Override solo offline** sul modello di `setPulsoDecayScale`: altezza fissa di Pulso (nota MIDI) e guadagno del click. Con gli override spenti il PCM non cambia.
- **Scenario** `kick_presence_ab_v1`: fixture di riferimento, seed 1337, stem per ruolo e mix.
- **Altezze fisse:** MIDI 31, 33, 35, 36, 38 (Sol1 49,0; La1 55,0; Si1 61,7; Do2 65,4; Re2 73,4 Hz), click invariato.
- **Click:** +0, +3, +6, +9 dB, altezza invariata.

**Misure, per sezione,** sugli attacchi di Pulso del piano:
- **E_K:** energia media per colpo, con pesatura K (BS.1770), nei 120 ms dopo l'attacco, in dB. Il valore per colpo non dipende dal numero di colpi.
- **Q100:** quota d'energia del kick sotto 100 Hz, nei 120 ms dopo l'attacco.
- **P_body:** energia di Pulso contro la somma degli altri tre stem, banda 40–160 Hz, da 20 a 120 ms dopo l'attacco; mediana, in dB.
- **P_click:** lo stesso, banda 2–5 kHz, da 0 a 15 ms.
- Loudness integrata del mix, riduzione massima del limiter, energia del mix sopra 6 kHz.

## 3. Previsioni

| ID | Previsione |
|---|---|
| K1 | Oggi E_K in B è più bassa che in A di 0,5–3 dB, perché il kick passa da 73 a 49 Hz |
| K2 | P_body > 0 dB in tutte le sezioni; P_click < 0 dB in almeno due sezioni su tre. La presenza manca nel transiente, non nel corpo |
| K3 | Con il click a +6 dB, P_click sale di 6 ± 1 dB e il mix di al più 0,3 LU; limiter fermo |
| K4 | Con l'altezza fissa, la differenza di E_K fra A e B scende a ≤ 1 dB in valore assoluto, per ogni altezza |
| K5 | Con gli override spenti il golden PCM è PASS; con ogni override gli eventi pianificati e gli stem di Trama e Vettore sono identici |

## 4. Regola di selezione (proposta al titolare, non adozione automatica)

- **I1, altezza:** fra le altezze con limiter ≤ 0,1 dB, mix entro ± 0,5 LU da oggi e Q100 ≥ 0,75 in ogni sezione, quella con il minimo di E_K fra le sezioni più alto. A parità entro 0,2 dB, la più grave. Previsione: MIDI 38, cioè A e A′ invariate e B che prende il Re2.
- **I2, click:** il guadagno più piccolo con P_click ≥ 0 dB in tutte le sezioni, mix entro +0,5 LU, limiter ≤ 0,1 dB ed energia del mix sopra 6 kHz entro +3 dB. Quest'ultimo limite c'è perché l'esperto ha trovato «acido» il chiptune, misurato a circa +11 dB in quella banda. Se nessun guadagno passa, non si propone nulla sul click.
- **Se K2 è falsa,** il click non è la leva e I2 si ridiscute.

## 5. Limiti

- Le bande, le finestre e la soglia P_click ≥ 0 dB sono scelte ragionate, non valori validati percettivamente.
- La pesatura K non modella l'ascolto in cuffia né il mascheramento.
- Se il kick ad altezza fissa «suoni meglio» lo dice solo l'ascolto di conferma.

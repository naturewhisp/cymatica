# M2.5.2 — F4a, livello del kick: risultati

- **Pre-registrazione:** [M2_5_2_F4_Kick_Level_Preregistration.md](M2_5_2_F4_Kick_Level_Preregistration.md), SHA-256 (LF) `fc8a15ba1a526e73adb63af6f1ccc0967d736aecfe4781e0e8f38cb8081cefe3`, registrata in `progress.md` prima del codice e della prima corsa.
- **Esecuzione, 2026-11-16:** `cymatica_game --cacp-run kick_level_ab_v1`, Release, binario compilato dal working tree su `ca625ef`. Stato `MEASURED`, nessun motivo d'invalidità, durata circa 18 s. Dati: [esperimento](experiments/m25_2_f4_kick_level/README.md).
- **Nessuna correzione dopo la misura:** varianti, soglie e regole sono quelle pre-registrate.

## 1. Esito delle predizioni

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| P1 | Offset 0 uguale al golden PCM | hash uguale a 15126969907236845529, stato `PASS` | Confermata |
| P2 | Stessi eventi applicati in ogni variante | hash degli eventi identico (identità, frame, disposizione, slot) | Confermata |
| P3 | Null test ≤ 1·10⁻⁶ sugli stem, ≤ 2·10⁻⁶ sul bus | Pulso ≤ 7,6·10⁻⁸; Corpo ≤ 1,12·10⁻⁷; Trama e Vettore 0 esatto; bus ≤ 1,28·10⁻⁷ | Confermata in tutte e quattro le varianti |
| P4 | Pulso +g LU ± 0,05 | +3,000 / +6,000 / +9,000 / +12,000 LU | Confermata |
| P5 | Mix +0,20 / +0,57 / +1,24 / +2,32 LU ± 0,3 | +0,181 / +0,521 / +1,129 / +2,132 LU | Confermata |
| P6 | Limiter 0 dB fino a +9 dB; +12 senza predizione | 0 dB in tutte le varianti, +12 compreso | Confermata (+12: 0 dB, `NO_PREDICTION`) |
| P7 | Due ripetizioni identiche | identiche | Confermata |

- **P3:** su Trama e Vettore lo scarto è zero esatto. Su Corpo è dell'ordine dell'arrotondamento in float (≈ 1·10⁻⁷), dovuto al ducking che segue il guadagno effettivo di Pulso. Il ducking è quindi invariato entro l'arrotondamento, e cambia solo il livello del kick. Il test unitario lo verifica anche con il ducking attivo.
- **P5:** il mix sale sempre un po' meno della predizione: −0,02, −0,05, −0,11 e −0,19 LU, uno scarto che cresce con l'offset. Resta nella tolleranza. Il modello a potenze non correlate sovrastima di poco. La causa non è verificata: correlazione negativa fra kick e basse di Corpo, oppure gating relativo di BS.1770.

## 2. Misure senza predizione

| Offset | Mix (LUFS) | Pulso − bus (LU) | Pulso − Corpo (LU) | True peak (dBTP) | Mix per sezione A / B / A' (LUFS) |
|---:|---:|---:|---:|---:|---|
| 0 | −21,26 | −13,23 | −9,67 | −10,45 | −21,15 / −21,27 / −21,35 |
| +3 | −21,07 | −10,41 | −6,67 | −9,35 | −20,93 / −21,13 / −21,16 |
| +6 | −20,73 | −7,75 | −3,67 | −8,00 | −20,54 / −20,85 / −20,82 |
| +9 | −20,13 | −5,36 | −0,67 | −6,10 | −19,84 / −20,35 / −20,23 |
| +12 | −19,12 | −3,36 | +2,33 | −3,52 | −18,72 / −19,48 / −19,24 |

- Nessuna variante si avvicina al limiter: anche a +12 dB il true peak (−3,52 dBTP) resta 1,7 dB sotto la soglia operativa del limiter (−1,8 dBFS).
- In ogni sezione Pulso sale esattamente di g. I divari fra le sezioni restano quelli di partenza: in B Pulso è 2,0 LU sotto A e 1,4 LU sotto A'.

## 3. Clip per l'ascolto e controllo diretto

- Sono 30 clip: per ciascuna sezione (A, B, A') l'estratto delle battute 9–16 in 5 varianti, ogni clip grezza e allineata. Gli SHA-256 sono nel file dell'esperimento. B dura 13,71 s per il tempo a 140 BPM, A e A' durano 16,00 s.
- **Nessun campione a fondo scala** nelle clip a 16 bit ([controllo](experiments/m25_2_f4_kick_level/clip_check_2026-11-16.txt)).
- **Allineate:** la loudness integrata è uguale a quella dell'offset 0, ma l'RMS non pesato sale fino a +0,85 dB a +12 dB in A. La ponderazione K pesa poco le basse, quindi a loudness uguale il kick porta più energia grave. Lo dice la quota sotto i 150 Hz: in A sale da 0,20 (offset 0) a 0,39 (+9) e 0,49 (+12).
- **Ispezione diretta dell'inviluppo** ([immagine](experiments/m25_2_f4_kick_level/envelope_A_0dB_vs_9dB_aligned.png), primi 4 s di A, allineati):
  - a offset 0 i colpi del kick emergono appena sopra il tappeto di Corpo e Vettore;
  - a +9 dB sono transitori netti con coda breve.
  - Si vede anche la spaziatura irregolare dei colpi (F1, kick spostato dal tempo forte): alzare il kick la renderà più evidente all'ascolto, come previsto da ADR-0007 §5.

  L'agente non ascolta: questa è una lettura del segnale, non un giudizio di bilanciamento.

## 4. Lettura e passi successivi

- Il meccanismo è verificato. L'override cambia solo il livello del kick, la produzione resta invariata, golden compreso, e le varianti sono deterministiche. Il risultato non dice quale livello sia giusto: lo decide l'ascolto a CP-2 (P-M25-03), con le ipotesi L1 e L2 e la regola di decisione già registrate.
- **Pacchetto d'ascolto:** il titolare sceglie dalle clip allineate le coppie A/B, aggiunge i controlli B0 (coppia identica e duplicato nascosto) e prepara ordine e chiave secondo il protocollo §2 e §2bis.
- **Il valore di produzione non cambia** finché non c'è l'esito dell'ascolto. Allora si aggiornano `kPulsoGain`, il golden PCM con il motivo, e questo documento.
- **F4b, durata e peso nelle basse del kick** (suggerimento §8.1): A/B di timbro separato, a loudness allineata, con la sua pre-registrazione. Va fatto dopo l'esito di F4a, o insieme nello stesso ascolto se il titolare lo preferisce: in quel caso con pre-registrazione separata.

## 5. Decisione e adozione (2026-11-16)

- **Decisione del titolare:** ADR-0007 D-M2-30 e D-M2-31. Niente ascolto umano dedicato. Kick a **+8 dB** (Pulso da −6 a +2 dB), centro della stima dell'esperto. La regola di decisione del §5 della pre-registrazione, che affidava il valore all'ascolto, è sostituita da D-M2-31. Il file firmato resta invariato e L1, L2, S1–S3 restano non verificate. Il pacchetto d'ascolto è archiviato senza invio.
- **Predizioni dell'adozione:** [addendum](M2_5_2_F4_Kick_Level_Adoption_Preregistration.md), SHA-256 (LF) `15dbd5c1c5fa21a0b1f7f64e8e8d15d5ba2e5633c89fdb709de6cd48afa52e00`, registrate prima del cambio.

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| A1 | `fixture_test` PASS, cambia solo il golden PCM | PASS. Trace `10949526468471789087` e piano `5693144463743088024` uguali ai golden, hash degli eventi invariato. Unico controllo fallito prima dell'aggiornamento: il PCM | Confermata |
| A2 | Pulso −26,487 LUFS ± 0,05 | −26,487 | Confermata |
| A3 | Corpo, Trama, Vettore ± 0,01 LU | −24,819 / −34,087 / −24,212, invariati | Confermata |
| A4 | Mix +0,98 LU ± 0,3, atteso sotto | +0,890 LU (−20,365 LUFS) | Confermata |
| A5 | Limiter 0 dB, true peak fra −8,00 e −6,10 dBTP | 0 dB; −6,87 dBTP | Confermata |
| A6 | Restano verdi le altre prove, salvo i golden legati al livello di Pulso | Release e Debug headless 270/270; prima dell'aggiornamento fallivano solo `fixture_probe_v1` e la loudness per stem di `procedural_v1`, entrambe per il golden PCM. Nella suite completa però `acao02_v1` diventa `INCONCLUSIVE`: non è un golden PCM | **Mancata** (vedi sotto) |
| A7 | `kick_level_ab_v1` sulla nuova base: P1–P4 e P7 confermate | Confermate; P5 e P6 non si rivalutano, perché valgono per la base −6 dB | Confermata |

- **Golden PCM della fixture:** da `15126969907236845529` a **`365661633327826884`**, con il motivo in `audio_probe.h`. Il vecchio valore resta la baseline storica di M2.4 per i confronti (D-M2-19).
- **Sulla nuova base, senza predizione:**
  - mix per sezione −20,12 / −20,55 / −20,46 LUFS;
  - Pulso per sezione −25,70 / −27,74 / −26,34 LUFS;
  - Pulso rispetto a Corpo −1,67 LU, contro −9,67 prima;
  - con l'override sulla nuova base, +6 e +9 dB in più (+14 e +17 rispetto a M2.4) fanno intervenire il limiter: il margine sopra la scelta attuale c'è, ma non è illimitato.
- **Verificabilità autonoma** (D-M2-30): il cambio di livello è verificato senza ascolto da misure con predizioni registrate prima. Stem di Pulso +8,000 LU esatti, altri ruoli invariati, mix entro la predizione, nessun intervento del limiter, piano ed eventi identici.

### A6 mancata: `acao02_v1` e il profilo di onset congelato

- **Suite completa:** `full_m2_suite --pack-seed 1618033988`. Tutte le voci passano tranne `acao02_v1`, che dà `INCONCLUSIVE` e porta il verdetto della suite a `FAIL`. Una prima corsa senza `--pack-seed` era fallita su `evaluation_check_selftest_v1` per un errore d'invocazione: il seed di default non è mai qualificato «alla cieca». Ripetuta con il seed documentato, quella voce è PASS.
- **Causa:** ricalibrato sul nuovo segnale, l'offset degli onset di Pulso passa da −11,67 ms (profilo congelato il 2026-10-29) a −12,67 ms. Delta e gli altri ruoli restano invariati. La rilevazione di Pulso resta perfetta: 416 riferimenti, 416 rilevazioni, precisione e richiamo 1. Il profilo calibrato ha quindi uno SHA-256 diverso da quello congelato, e lo scenario tiene le misure di verifica come diagnostiche: è la regola fail-closed, che funziona come previsto.
- **Perché la predizione era sbagliata:** A6 considerava solo i golden PCM. ADR-0007 §5 sapeva già che F4 cambia gli stem, e quindi la calibrazione AC-AO-02, ma l'addendum non lo riportava. Errore di pre-registrazione, dichiarato.
- **Cosa resta:** il profilo congelato non si cambia qui. La ricalibrazione AC-AO-02 con verifica su insiemi separati è il compito di M2.5.5, dopo F2, H-03 e H-04, che cambieranno di nuovo gli stem. Fino ad allora `acao02_v1` resta `INCONCLUSIVE` e `full_m2_suite` `FAIL` su quella sola voce. È un limite noto: non va letto come regressione e non va nascosto adattando il profilo.

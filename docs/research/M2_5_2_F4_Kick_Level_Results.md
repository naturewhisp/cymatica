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

# M2.5 CP-3 — I3, basso: risultati dell'indagine

- **Pre-registrazione:** [documento](M2_5_CP3_I3_Bass_Investigation_Preregistration.md), SHA-256 (LF) `430b75a8ef0d776d6c26bdb042c371f9f25cd9d67fa9d17d24272afed6928be2`.
- **Esecuzione:** 2026-11-23. **Dati e script:** [esperimento](experiments/m25_cp3_bass/README.md), solo statistiche aggregate.
- **Stato:** vincoli per il prototipo. Nessun cambiamento al motore.

## A. Il basso dell'esperto

| ID | 808 | Sub | Esito |
|---|---|---|---|
| A1, attacchi insieme al kick (≥ 50 %) | 36 % | 34 % | **Mancata** |
| A2, kick spostato | 17 %, cioè 19 punti sotto | 17 %, 17 punti sotto | Confermata |
| A3, fondamentale (35–70 Hz) | mediana 43,8 Hz (43,8–58,8) | 43,8 Hz (38,8–43,8) | Confermata |
| A4, movimenti di 0–2 semitoni (≥ 40 %) | **11 %**; salti ≥ 7: 59 % | 13 %; 61 % | **Mancata** |
| A5, attacchi sui quarti (≤ 60 %) | **61 %** | 57 % | Mancata per l'808 di poco, confermata per il Sub |

Basso e kick sono **legati ma non sovrapposti**: coincidono il doppio del caso, e due volte su tre il basso suona da solo. La linea si muove soprattutto per salti ampi, non per gradi.

## B. Lo spazio in CYMATICA

| | A | B | A′ |
|---|---:|---:|---:|
| B1, quota del kick in 30–120 Hz | 99,9 % | 91,7 % | 99,7 % |
| B2, tempo con kick sotto −20 dB | 76 % | 71 % | 77 % |

Confermate, e quasi uguali col Do2 (91,0 % e 71 % in B). Le basse sono **vuote per circa tre quarti del tempo**.

## C. Separazione dal kick (kick attuale; col Do2 differenze ≤ 0,2 dB)

| Variante | R minimo | P massima | L minima | Ammessa |
|---|---:|---:|---:|:-:|
| **basso, S0 nessuna** | 9,2 dB | 0 | 0 | **sì, scelta dalla regola** |
| basso, S1 alternanza | 26,6 | 0,7 LU | −0,7 dB | sì |
| basso, S2 ducking −6 | 13,7 | 1,8 | −1,8 | sì |
| basso, S3 ducking −12 | 18,2 | 3,1 | −3,1 | no |
| basso, S4 passa-alto | 16,3 | 4,7 | −8,4 | no |
| alto, S0 | 11,7 | 0 | 0 | sì |
| alto, S1 | 39,7 | 0,7 | −0,7 | sì |
| alto, S2 | 16,3 | 1,8 | −1,8 | sì |
| alto, S3 | 20,8 | 3,1 | −3,1 | no |
| alto, S4 | 13,7 | 1,0 | −2,4 | sì |

C1–C4 confermate. La regola sceglie **registro basso senza separazione**, con entrambi i kick.

**Riserve sulla scelta:**
- **Dipende dal livello** del basso di prova (Corpo −3 LU, pesato K). R scende dB per dB se il basso si alza: con 3,2 dB in più S0 non è più ammessa. Il livello giusto non è noto.
- R è energia, non mascheramento percepito.
- S1 è quella col miglior rapporto: circa 17 dB in più di R per meno di 1 LU. Non vince perché l'ordine della regola preferisce la meno invasiva. Non è una prova che sia la migliore all'ascolto.

## Vincoli per il prototipo

1. **Registro:** MIDI 28–39 (41–78 Hz); la mediana dell'esperto è 44 Hz.
2. **Separazione:** S0 come base, S1 come confronto. Niente S3 e S4.
3. **Rapporto col kick:** legato ma non sovrapposto. Né raddoppio del kick né alternanza rigida.
4. **Movimento:** ammettere salti ampi (ottave, quinte), non solo gradi.
5. **Livello:** da variare, perché la regola ne dipende.
6. **Due kick:** attuale e Do2.

## D. Il basso negli archetipi (lettura di DESIGN §12, domanda del titolare)

| Archetipo | Cosa dice DESIGN sull'audio | Basso plausibile |
|---|---|---|
| Sintetico | «bassi controllati», sequenze, sidechain | Linea sequenziata e asciutta: esplicita |
| Organico | corde, pad respiranti, fraseggio | Linea cantabile, note lunghe: plausibile, non scritta |
| Fratturato | saturazione, transienti aggressivi | Basso distorto e percussivo: plausibile, non scritta |
| Etereo | drone, **sub armonico**, eventi radi | Pedale o silenzio: esplicito il sub, **non** una linea ritmica |
| Sincopato | sincopi, polimetrie, spazio fra gli eventi | Linea sul levare, con pause: plausibile |

- Un registro grave è compatibile con tutti e cinque; una **linea ritmica fissa no**. Servono per archetipo: densità, articolazione, timbro e il diritto di tacere.
- Questo favorisce il ruolo autonomo rispetto alla voce fissa di Corpo, ma è una lettura del design, non una misura. Gli archetipi non esistono ancora nel motore.
- DESIGN §21.4 dice che «Corpo segue il basso continuo»: nel gameplay il basso può proiettarsi su Corpo senza un quinto canale.
- **Da sentire:** in Organico ed Etereo un basso sempre presente potrebbe appesantire. Il prototipo suona con l'estetica attuale e non lo verifica.

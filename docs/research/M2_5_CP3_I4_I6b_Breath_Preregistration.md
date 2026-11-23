# M2.5 CP-3 — I4+I6 bis, salto A→B e respiri. Pre-registrazione

- **Stato:** scritto il 2026-11-23, dopo l'ascolto del [prototipo](M2_5_CP3_I4_I6_Two_Bar_Prototype_Results.md) e **prima** di codice, render e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Variante suggerita dai dati.** Le note sono note: W1–W6 sono verifiche di costruzione, le previsioni vere sono d'ascolto. Solo offline, seed 1337; nulla adottato.

## Versioni

| Versione | Definizione |
|---|---|
| **b** | il prototipo con melodia e ritmo, invariato |
| **c** | b con la tonica delle risposte scelta per rendere minimo il salto più grande fra la nota prima e la prima della frase dopo; si preferiscono le toniche sopra Corpo, a parità la più vicina alla nota sostituita; mai in conflitto 1/6/11 |
| **d** | c con **respiri più radi**: solo ogni 16 battute (frasi 3 e 7 di ogni sezione). Le altre risposte tengono la tonica fino a fine battuta |
| **e** | c con **sostegno del kick**: nelle battute di respiro in cui Pulso suona, un attacco in più sul sedicesimo 15 se libero, velocity 0,85 |

d ed e cambiano una cosa sola rispetto a c.

## Criteri

| ID | Criterio |
|---|---|
| W1 | c cambia solo note di risposta (b%8 = 7); d solo durate del Vettore nelle stesse battute; e solo Pulso |
| W2 | Salto della melodia A→B (battuta 32) in c, d, e ≤ 12 (b 19, oggi 12); nessun salto nuovo oltre 12 rispetto a b |
| W3 | D-M2-43: nessuna frase che in b ha almeno metà note sopra Corpo scende sotto metà; conflitti 0 |
| W4 | Finali dispari sulla tonica 100 % in c, d, e; respiri: b, c, e 12, d 6 |
| W5 | M1 < 60 % e G1 come in b (Vettore) |
| W6 | Render MEASURED, limiter 0 dB, nessuno scarto né evento rimasto, loudness entro 0,5 LU da b |

Se W1 o W6 falliscono non si generano estratti.

## Ascolto

Quattro estratti ciechi di 45,7 s: battute 16–39, con il confine A→B a 0:32, loudness pareggiata, ordine casuale. Domande: il passaggio a 0:32 è naturale, un po' forzato o forzato; i respiri (pause) sono naturali o forzati, a che secondo; ripetitività 1–7; preferito.

**Previsioni:** (a) c, d ed e meno forzati di b a 0:32; (b) e con respiri meno forzati di c; (c) d meno forzato di c, ma più ripetitivo. Nessuna previsione sul preferito.

**Regola:** se d o e non sono preferiti a c, i respiri restano come in b. Se c non è meno forzato di b a 0:32, la correzione del salto non si propone. **Limiti:** il cambio di tempo resta; il titolare conosce il brano e l'ipotesi; un solo seed.

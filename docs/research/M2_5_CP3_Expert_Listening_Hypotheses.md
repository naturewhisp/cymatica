# M2.5 CP-3 — Ascolto di conferma dell'esperto: disegno e ipotesi

- **Stato:** scritto il 2026-11-23, **prima** di codice, render e pacchetto. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica. Protocollo P-M25-03; nessuna adozione.
- **Durata prevista:** circa 20 minuti. Ascoltatore: l'esperto, da solo. Il titolare conduce e tiene la chiave.

## 1. Materiale

Tutto sul seed 1337. Le coppie sono allineate in loudness alla più debole.

**Brano Z** (intero, una volta): piano c di [I4+I6 bis](M2_5_CP3_I4_I6b_Breath_Results.md), più x di [I5d](M2_5_CP3_I5d_Register_Return_Results.md) (Corpo e Vettore un'ottava sotto nelle prime due battute di A′), kick Do2 con click +3 dB, basso V2 ricalcolato sui kick di Z con i guadagni per sezione di [V2m](M2_5_CP3_I1_I3_Kick_Bass_Joint_Results.md). Senza rampa, che non è dimostrata.

| Coppia | Confronto | Estratto |
|---|---|---|
| K | kick di oggi contro Do2 e click +3 dB, piano di oggi, senza basso | 16 s attorno ad A→B |
| V | V1 contro V2m, piano di oggi, kick Do2 | 16 s di A′ (battute 72–79) |
| M | piano di oggi contro c, kick di oggi, senza basso | 32 s di A (battute 16–31) |
| T | c senza rampa contro c con rampa r8 | 16 s, A→B a 10 s |
| R | oggi contro x | 20 s, B→A′ a 8 s |
| I | coppia identica (B0): V1 in un altro punto di A′ | 16 s (battute 80–87) |
| D | duplicato nascosto di V, ordine invertito (B0) | come V |

Ordine fisso K, M, V, T, I, R, D (D non accanto a V); dentro ogni coppia l'ordine è casuale, seme solo nella chiave.

## 2. Ipotesi e conseguenze

| ID | Ipotesi | Se confermata | Se mancata |
|---|---|---|---|
| E1 | Ripetitività di Z ≤ 4 (esperto 2026-11-19: 6) | I4+I6 si propone | si rivede la leva |
| E2 | Melodia «frase con inizio e fine» in Z | sostegno a I6 | non basta |
| E3 | Kick «giusto» in Z; in K si preferisce il candidato | I1–I2 si propongono | si rivede il candidato |
| E4 | Il basso «si sente ed è distinto dal kick» in Z | I3 si propone | si rivede il livello o la linea |
| E5 | In M si preferisce c | come E1 | come E1 |
| E6 | In R si preferisce x | I5d si propone | resta oggi |
| E7 | Passaggio A→B non «brusco» in Z | salto di c aiuta | A→B resta aperto |

**Senza previsione:** V (V1 o V2m) e T (rampa). Regole: se in V si preferisce V1 o non si sente differenza, vince **V1**, più economica; V2m solo se preferita con motivo. Se in T non si preferisce r8, **nessun ADR** per la rampa.

Una preferenza senza rilevazione non conta. Se I è giudicata diversa o D incoerente con V, tutto vale come indicazione debole. Un'ipotesi vale per questo ascoltatore e queste clip.

## 3. Verifiche del materiale, prima di consegnare

| ID | Criterio |
|---|---|
| T1 | Il piano di oggi reso con l'override del kick è uguale a `c2_v0_mix` entro 0,001 |
| T2 | La linea di basso resa con `warped_plan_render_v1` è uguale a quella di `bass_prototype_v1` entro 0,001 (mono, tempi di oggi) |
| T3 | Z: MEASURED, limiter 0 dB, nessuno scarto; picco con il basso ≤ −1 dBFS; nessun conflitto di classe 1/6/11 fra Vettore e Corpo in più rispetto a c |
| T4 | J4 ricalcolata su Z: R sui kick coincidenti ≥ 6 dB in ogni sezione (stima della banda: energia del basso nello stesso render) |
| T5 | Estratti di ogni coppia entro 0,1 LU dopo l'allineamento; nessun campione a fondo scala; la coppia identica uguale campione per campione |
| T6 | Pagina: tutti i file presenti, nessuna etichetta di versione, nessun nome di persona |

T1, T2, T3, T5 o T6 falliti bloccano la consegna; T4 fallita si riporta e il titolare decide.

## 4. Limiti

- Z combina quattro candidati: un giudizio negativo non dice quale. Le coppie servono a separare.
- Il basso è sommato dopo il render; nessun ducking. V e V2m allineate in loudness: V2m attenuata di circa 0,7 dB, basse comprese.
- Un solo ascoltatore, che conosce le versioni precedenti; non è in doppio cieco.

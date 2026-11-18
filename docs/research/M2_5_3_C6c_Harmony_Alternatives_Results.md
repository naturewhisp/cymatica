# M2.5.3–4 — Pacchetto 6c: alternative armoniche di B. Risultati

- **Pre-registrazione:** [M2_5_3_C6c_Harmony_Alternatives_Preregistration.md](M2_5_3_C6c_Harmony_Alternatives_Preregistration.md), SHA-256 (LF) `4ee3320a0fe8fad822d9338a7378ad15d5d7cc343205438971ac8357312aa5a4`.
- **Esecuzione:** 2026-11-18.
- **Dati:** [esperimento](experiments/m25_3_c6c_harmony_alternatives/README.md).
- **Decisione:** D-M2-49.
- **Fonte:** [note su AURA](AURA_Music_Source_Notes.md), voce A5.

## 1. Cosa cambia

- `ChordSpec::thirdChromaticOffset` dichiara l'alterazione della terza. Tutte le note dell'accordo passano da `chordToneMidi`, che la usano:
  - le voci di Corpo e il pad di ripiego;
  - la Trama;
  - la diade C1c;
  - la lettura dell'accordo di C2 e il riconoscimento del cambio d'accordo;
  - gli strumenti d'insieme.
- La lettura dell'accordo sposta anche le note del Vettore che cadono sulla classe naturale di una terza alterata (falsa relazione).
- La tabella `fixture-variants-v1` riceve la dimensione `bHarmony`, con decisione `kVariantDecisionBase + 5`, modulo 3. Le altre dimensioni restano estratte come prima: la tabella passa da 256 a 768 cammini.

| Alternativa | Frasi 8–14 | Frase 15 |
|---|---|---|
| 0, riferimento | Sol m – Mi♭ – Fa – Sol m | uguale, con pad verso A′ |
| 1, diatonica | Sol m – Mi♭ – Do m – Re m | Sol m – Do m – Re m – Sol m |
| 2, sensibile | Sol m – Mi♭ – Do m – **Re (Fa♯)** | Sol m – Do m – **Re (Fa♯)** – Sol m |

## 2. Esiti

Le misure riguardano la sezione B su 64 seed, con il disegno appaiato: ogni seed pianificato con le tre alternative.

| ID | Criterio | Risultato | Esito |
|---|---|---|---|
| H1 | Riferimento invariato | Release 313/313 con tutti i golden esistenti uguali: trace, piano e PCM della fixture, forma di prova, `meter_change_v1` | Superato |
| H2 | 0 false relazioni nell'alternativa 2 | **0** su 64 seed; 0 anche nelle alternative 0 e 1 | Superato |
| H3 | Contratto dei punti 1–4 nelle alternative 1 e 2 | Conflitti Vettore–Corpo 0 e 0; note dell'accordo sui cambi 1984/1984 e 1984/1984; nessun `vettoreRegisterUnmet` | Superato |
| H4 | Tabella | `DISTRIBUTIONS`; **62** brani distinti (previsti 61–63); **62** cammini (previsti circa 61); alternative estratte 19, 27 e 18 volte | Superato |
| H5 | Build | Release e Debug `/WX` 313/313; `full_m2_suite` verde salvo `acao02_v1` | Superato |

**Adozione:** tutti superati, quindi il pacchetto si adotta.

## 3. Descrittivi

| Misura, sezione B, 64 seed | Alt. 0 | Alt. 1 | Alt. 2 |
|---|---|---|---|
| H-AD, note del Vettore adattate | 340 | 862 | 1863 |
| H-RP, battute di Corpo riparate (C1c) | 411 | 698 | 171 |
| H-GR, gruppi distinti di frasi di Corpo | 18 | 16 | 11 |
| H-LT, attacchi sul Fa♯ (Pulso, Corpo, Trama, Vettore) | 0 | 0 | 0, 0, **765**, 0 |
| Cambi reali d'accordo con nota dell'accordo | 1600/1600 | 2048/2048 | 2048/2048 |

**Previsioni descrittive:**
- **H-AD, alternativa 2 sopra l'alternativa 1:** si è verificata (1863 contro 862).
- **H-LT > 0 in Corpo, Trama e Vettore: mancata.** La sensibile suona solo nella Trama.
  - Il Corpo resta sempre su Re e La. Con la condotta delle voci, in questi giri la seconda voce del iv è sempre il Do (la sua quinta), e dal Do il La è più vicino del Fa♯. Con la politica radice-quinta il La è per definizione.
  - Nel seed 1 la riparazione della diade non può passare al Fa♯: la melodia suona un Do, che col Fa♯ forma un tritono, cioè la classe 6 che C1c evita.
  - La melodia non arriva mai sul Fa♯, perché le note spostate scelgono altre note dell'accordo.
- **H-GR delle alternative 1 e 2 almeno pari all'alternativa 0: mancata** (16 e 11 contro 18). Con un giro che cambia la fondamentale in ogni battuta, le riparazioni e gli adattamenti convergono su meno disposizioni di Corpo.

**Diversità fra seed** (quota di frasi identiche fra due seed, prima → dopo):
- Corpo dal 70% al 59%;
- Vettore dal 32% al 26%;
- Trama dal 54% al 49%;
- Pulso dal 61% al 59%.

L'armonia di B ora cambia fra seed.

**Adattamenti:** nell'alternativa 2 sono 5,5 volte quelli del riferimento. La melodia, scritta sul giro modale, viene corretta spesso sul Re maggiore. È un segnale da sentire (voce 17 del registro): la lettura dell'accordo potrebbe rendere la melodia più rigida in B.

## 4. Rischi e seguiti

- **La sensibile c'è solo nell'arpeggio.** Non ho cambiato nulla dopo la misura. Una variante che porti il Fa♯ nel Corpo, ad esempio la terza obbligata sul V con l'alterazione, sarebbe una nuova pre-registrazione, da decidere dopo l'ascolto.
- **Identità di B:** il Re maggiore porta un tono fuori modo. Lo giudica l'ascolto cumulativo, e se rompe il carattere di B l'alternativa 2 esce dalla tabella.
- **Non ci sono clip:** come per la voce 14, i brani delle tre alternative vanno preparati per la sessione d'ascolto.

## 5. Build e test

- **Test nuovi:** `[harmony_alternatives]`, con 3 casi:
  - alterazione in ogni ottava;
  - tabella degli accordi, con A e A′ invariate;
  - false relazioni e contratto su 8 seed appaiati, ripetibili.
- **Test aggiornato:** `[diversity]`, che ora richiede ogni alternativa estratta almeno 10 volte.
- **Release e Debug `/WX`:** 313/313. Durante la build Debug ho cambiato solo l'etichetta testuale `variant_table` del manifest di diversità; la Release, ricompilata dopo, passa 313/313.
- **`full_m2_suite`:** verde salvo `acao02_v1` INCONCLUSIVE, il limite noto.

# Pacchetto di misura 4 — Loudness specifica, perdita per mascheramento e pendenza spettrale: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Autorizzazione:** pacchetto 4 (B1 + B2, §1) del [dossier delle lacune di misura](M2_5_Measurement_Gaps_Research.md), approvato il 2026-11-17, con la regola di validazione comune. **La norma ISO 532-1 non è stata acquistata** (decisione del titolare): la loudness si calcola con MoSQITo, e la riproduzione dei valori della norma è sostituita dalla definizione del sone.
- **Lacuna:** BS.1770 misura la loudness di ogni ruolo da solo, non quanto se ne sente nel mix. D-M2-33 ha mostrato che il guadagno di bus non predice la loudness.

## 1. Strumento

- **Esportazione** (C++, scenario `stem_export_v1`, nessuna dipendenza): per ciascuna sezione (A, B, A′) lo stesso estratto di 8 battute di F4a (frasi 3–4), in WAV float a 32 bit. Si esportano il bus mix, ogni stem e il bus mix senza quel ruolo, cioè 3 × 9 = 27 file.
- **Analisi** (Python offline, ambiente fissato `tools/pyenv`: MoSQITo 1.2.1, numpy 2.2.6, scipy 1.15.3; matplotlib 3.10.3 come dipendenza importata da MoSQITo):
  - **calibrazione dichiarata:** una sinusoide a fondo scala (picco 1,0) vale 100 dB SPL. Mono = media dei due canali, perché ISO 532-1 è monoaurale e la somma binaurale non è modellata;
  - **loudness tempo-variante** ISO 532-1 (`loudness_zwtv`, campo libero), riassunta come media nel tempo, in sone;
  - **incremento di loudness** di un ruolo nel mix: ΔN = N(mix) − N(mix senza il ruolo). È un'approssimazione della loudness parziale, non il modello di Glasberg e Moore 2005;
  - **perdita per mascheramento** = 1 − ΔN / N(ruolo da solo). 0 vuol dire nessuna perdita, 1 ruolo coperto del tutto. La stessa grandezza si calcola per banda Bark, sulla loudness specifica media;
  - **B2, pendenza spettrale:** regressione lineare del livello della densità spettrale (Welch, Hann di 8192 campioni) in funzione di log2(f), fra 50 Hz e 16 kHz, in dB/ottava, per il mix e per ogni ruolo.

## 2. Validazione

| ID | Prova | Esito atteso |
|---|---|---|
| **L1, dato pubblicato** | Definizione del sone: tono a 1 kHz a 40 dB SPL = 1 sone; a 60 dB SPL = 4 sone (raddoppio ogni 10 phon) | MoSQITo stazionario entro ± 5 % |
| **L2, positivo** | Tono a 1 kHz a 60 dB SPL con rumore rosa a 40, 50, 60 e 70 dB SPL | Perdita del tono strettamente crescente con il livello del rumore |
| **L3, negativo** | Toni a 200 Hz e 5 kHz, 60 dB SPL ciascuno | Perdita di ciascuno ≤ 0,15 |
| **L4, invarianza** | Due esecuzioni | Valori identici |
| **L5, pendenza** | Rumore bianco e rumore rosa sintetici | 0 ± 0,3 e −3 ± 0,3 dB/ottava |

## 3. Predizioni sulla fixture (golden PCM `18256941985354732249`)

| ID | Predizione |
|---|---|
| **M1** | In ogni sezione la **Trama** ha la perdita per mascheramento più alta dei quattro ruoli: è il ruolo più basso nel bus (circa −14 LU) e condivide le bande con Corpo e Vettore |
| **M2** | In ogni sezione la perdita della Trama è ≥ 0,3 |
| **M3** | Pendenza del mix fra −6 e −2 dB/ottava |

Tutto il resto è descrittivo: perdita di Pulso, Corpo e Vettore, profilo per banda, pendenze dei ruoli rispetto al rosa (−3) e a Pestana (circa −5).

## 4. Limiti

- La calibrazione a 100 dB SPL è una convenzione: livelli d'ascolto diversi cambiano loudness e mascheramento.
- ΔN non è la loudness parziale di un modello validato. Hafezi e Reiss 2015 avvertono che le misure oggettive di mascheramento quantificano male quello percepito nei mix: è una **descrizione** di come si sposta ciò che si sente di un ruolo, non un criterio di bilanciamento.
- La validazione di MoSQITo non è documentata, e la norma non è stata letta.

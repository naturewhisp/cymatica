# M2.5.2 — Rapporto FM della Trama: pre-registrazione

- **Stato:** scritto il 2026-11-16, prima dello script di contesto, del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Perimetro:** ADR-0007 D-M2-26 (rapporto FM in M2.5.2); verifica autonoma secondo D-M2-30. Un ascolto dedicato non c'è: il giudizio umano già raccolto è «un po' dissonante» (esperto, 2026-10-28).
- **Fonti:** [dossier R-M25-7](M2_5_F5_Music_Director_Research.md), «Timbro: correggere il rapporto FM della Trama»; [script `fm_sethares.py`](experiments/m25_ensemble_baseline/fm_sethares.py) e il suo output; modello di Sethares (codice pubblicato dall'autore, già usato nel dossier); Chowning 1973 (con c:m interi gli spettri sono armonici; i suoni di ottone usano 1:1).

## 1. Variabile

Il rapporto modulante/portante della Trama, `harmonicRatio = 1 + K · timbreParam` in `voice_allocator.cpp`, con K = 1,76 e `timbreParam` = 0,50 dal planner: m/c = 1,88. Cambia solo K. Indice (limite di Carson saturato a 4, picco 3,0), inviluppo, note, piano e trace restano invariati.

**Candidati** (gli stessi dell'esperimento minimo di R-M25-7): m/c = 1,88 (attuale), 1,0, 2,0, √2.

## 2. Regola di scelta, fissata prima

Tre criteri sul modello di Sethares, con spettri teorici dello stesso script (Bessel, kmax 12; Corpo come dente di sega a 10 armoniche).

| Criterio | Definizione | Soglia |
|---|---|---|
| **C1, autodissonanza** | media dell'autodissonanza di una nota Trama alle altezze Trama della fixture, I = 3 | ≤ 0,028, il valore del dente di sega a 10 armoniche |
| **C2, discriminazione degli intervalli** | con il timbro candidato (440 Hz, I = 1,5), dissonanza della quinta giusta e dell'ottava **minori** di quella del tritono | entrambe minori |
| **C3, contesto** | per ogni nota Trama della fixture (seed 1337), aumento di dissonanza dato dalla nota sopra le note di Corpo che suonano al suo attacco, cioè D(Corpo + Trama) − D(Corpo) − D(Trama); media sulle note e sui due indici I = 1,5 e I = 3 | il valore minore |

**Scelta:** fra i candidati che superano C1 e C2, quello con C3 minore. Se nessuno li supera, il rapporto resta 1,88 e la voce si chiude con questo esito. A parità di C3 entro il 5 %, vale il candidato con la variazione minore di K.

Si riporta anche, senza regola, C3 con Corpo e Vettore insieme.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **F1** | Solo 1,0 supera C1 e C2: 2,0 fallisce C2 (tritono 0,07 contro quinta 0,08 nello script del dossier), √2 fallisce C1 (0,042 a D5) e 1,88 fallisce C1 (0,848). Scelta: **m/c = 1,0**, cioè K = 0 |
| **F2** | C3 di 1,0 è minore di quello di 1,88 |
| **F3** | Con K = 0 il golden di piano e trace della fixture resta invariato; gli stem di Pulso, Corpo e Vettore sono **identici al bit**; cambiano lo stem di Trama, il mix e quindi il golden PCM |
| **F4** | Riduzione del limiter 0 dB; true peak entro ± 0,5 dB dall'attuale −6,87 dBTP |
| **F5** | Variazione della loudness integrata del mix entro ± 0,3 LU (la Trama sta circa 13 LU sotto il bus) |

Senza predizione: loudness dello stem di Trama prima e dopo. Se cambia più di 1 LU, la si riporta: il bilanciamento della Trama è il passo generale di F4, non questo.

## 4. Limiti

- Il modello conosce solo l'interferenza fra parziali: niente armonicità percepita, inviluppo o durata. L'indice reale decade con l'inviluppo, e le note durano 107–125 ms.
- Con m/c = 1,0 lo spettro è armonico completo, come quello di Corpo. Se la Trama resti distinguibile da Corpo è una domanda timbrica: va all'ascolto cumulativo (D-M2-30), e qui non viene misurata.

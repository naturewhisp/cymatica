# M2.5.3–4 — Contratto minimo, punto 4: entrate e uscite dei ruoli. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni di riferimento:** ADR-0007 D-M2-32 (contratto minimo, punto 4), D-M2-40 (stato metrico), D-M2-43 (registro del dominante), D-M2-39 (policy 1.0.2.0).
- **Regole** ([note sulle regole d'insieme](experiments/m25_f5_research_notes/regole_insieme.md)):
  - R-ROL-7: tutti e quattro i ruoli solo nei punti forti della forma;
  - R-ROL-1: il dominante è sempre attivo;
  - R-MET-2: in stato ancorato un ruolo-àncora articola il tempo 1.

  Dal [dossier](M2_5_F5_Music_Director_Research.md): «Il silenzio di una parte è un esito legittimo».

## 1. Diagnosi (piano `73d89d8`)

- La tensione dichiarata è costante dentro ogni sezione. I punti forti della forma sono quindi i confini e le cadenze: frasi 7 (`ContrastShift`), 15 (`ContrastShift`) e 23 (`ModalResolution`), più lo spostamento metrico dichiarato in B (12–13).
- L'unica uscita dichiarata è la Trama nelle frasi 0–3 (D-M2-08).
- Tutti e quattro i ruoli suonano in **20 frasi su 24**.

## 2. Variabile

1. **Contratto:** `PhrasePlan` aggiunge `maskPulso`, accanto al `maskTrama` esistente, dichiarato dall'espansore delle frasi.
   - La maschera vale per tutti i candidati, come quella di Trama: è un dato del piano, non una riparazione, e la selezione la vede.
   - Con Pulso assente, la Trama complementare non ha passi da evitare e suona il suo pattern euclideo intero.
   - Le maschere di Corpo e Vettore non si introducono: il profilo non le usa, e il dominante non si toglie.
2. **Vincoli duri,** violazione strutturale del piano:
   - `maskPulso` in stato `Spostato` (lo spostamento è dichiarato su Pulso);
   - `maskPulso` con Corpo ritardato di un battito in battuta 0, perché il tempo 1 resterebbe senza àncora (R-MET-2).
3. **Profilo iniziale `fixture-orchestration-v1`** (valore d'autore, non validato):

   | Frasi | Assente | Motivo |
   |---|---|---|
   | 0–3 | Trama | Esistente: esposizione |
   | 10–11 | Trama | Assottiglia la tessitura prima dello spostamento metrico di 12–13, che si sente meglio |
   | 16–19 | Trama | A′ riprende l'orchestrazione di A: la Trama rientra a metà sezione, come in A |
   | 22 | Pulso | Pausa del kick prima della cadenza finale; il riferimento è esposto in 18–21 e Corpo articola ogni tempo 1 |

   Pieno organico in 13 frasi su 24, comprese le cadenze 7, 15 e 23 e lo spostamento 12–13.
4. **Regola fissata prima di misurare:** se la maschera cambia il vincitore di una frase, il cambio si accetta, perché la maschera è un dato dichiarato che la selezione deve vedere. Lo si riporta con il margine.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **E1, maschere** | Trama assente esattamente nelle frasi 0–3, 10–11, 16–19; Pulso assente esattamente nella 22; pieno organico in 13 frasi su 24 |
| **E2, vincoli** | `maskPulso` in `Spostato`, e `maskPulso` con Corpo ritardato, sono violazioni strutturali (test) |
| **E3, R-MET-2** | In ogni battuta di ogni frase non `Spostato` c'è un attacco di Pulso o di Corpo sul tempo 1 |
| **E4, Trama senza Pulso** | Nella frase 22 la Trama ha più attacchi che nella stessa frase di C3 |
| **E5, cosa non cambia** | Gli intent delle frasi 0–9, 12–15, 20–21 sono identici a C3 |
| **E6, vincitori** | Descrittivo con margine: vincitori invariati o cambiati nelle frasi 10, 11, 16–19, 22, 23 |
| **E7, sezioni** | Novità di Foote su Corpo: i due picchi più alti alle frasi 9 e 17, con M4, M5 e M5r |
| **E8, scenari** | Fixture PASS con i golden aggiornati col motivo; `meter_change_v1` PASS in M1–M5 |

## 4. Limiti

- Il profilo è una scelta d'autore iniziale: la sua posizione e la sua lunghezza si giudicano all'ascolto cumulativo.
- Una variazione di organico non è sviluppo del materiale: non cambia motivi né armonia.
- R-ROL-5 (ingressi in voce esterna) è soddisfatta in modo indiretto: la Trama rientra sopra la melodia, che dal punto 3 sta sopra Corpo. Non si misura qui.

# M2.5 CP-3 — I4, forma: risultati dell'indagine

- **Pre-registrazione:** [documento](M2_5_CP3_I4_Form_Investigation_Preregistration.md), SHA-256 (LF) `63112be030c67f0b2ebf3c8116d7263d4c757f0b3dd69e8bcdacc9f93d67841d`.
- **Esecuzione:** 2026-11-23. **Dati:** [esperimento](experiments/m25_cp3_form/README.md).
- **Stato:** proposte di leve. Nessun cambiamento alla musica.

## 1. Esiti

| ID | Misura | Esito |
|---|---|---|
| G1, stesso ritmo della battuta precedente, dentro la frase | Pulso, Trama e Vettore **100 %** (mediana 100 % sui 64 seed) | Confermata |
| G2, battuta 4 diversa dalle altre tre | **0** frasi per ogni ruolo; 0 su 5383 casi frase-ruolo nei 64 seed | Confermata |
| G3, il ritmo di Pulso cambia fra frasi | 58 % (riferimento); seed: mediana 47 % | Confermata sul riferimento |
| G4, esperto: battute uguali alla precedente | 0 su 111 | Confermata, ma **satura**: non distingue nulla |
| G5, cambi concentrati a fine gruppo di 4 | nessuna differenza (tutte le battute differiscono) | **Mancata** |
| G6, finestre di 8 con entrata o uscita, 64 seed | mediana 50 % (42–58 %) | Confermata |
| G7, battute dall'ultimo cambio agli istanti di calo | mediana 3,5 (7, 4, 0, 10, 3, 3) | **Mancata** (≥ 4) |
| G8, entrate o uscite in più per 9 finestre su 12 | 3 | Confermata (aritmetica) |

Nel riferimento Pulso ripete lo stesso ritmo fino a **20 battute** di fila, Trama 20, Vettore 16; nel seed 34 il Vettore arriva a **44**.

**Analisi aggiunta dopo i dati.** G4 era satura, quindi ho confrontato le battute della batteria dell'esperto a distanza maggiore:

| Distanza | Battute uguali |
|---|---:|
| 1, 2 | 0 % |
| 4 | 28 % |
| **8** | **56 %** |
| 16 | 54 % |

Il suo loop dura **8 battute**, il nostro **una**. Rispetto a 8 battute prima, l'ultima battuta del gruppo cambia nel 75 % dei casi, contro il 17–54 % delle altre: un segno di chiusura, con sole 12 coppie per posizione.

## 2. Lettura

1. **La ripetizione sta dentro la frase.** Il pattern dura una battuta e si ripete quattro volte, senza riempimenti né varianti. Le altezze cambiano con gli accordi, il ritmo no: coerente con «una figura che si ripete».
2. **Cosa dice la regola scritta prima:** G1 e G2 vere indicano la battuta come loop. Il paragone con l'esperto però **non sostiene i riempimenti di fine frase**, perché G5 è mancata. Quello che mostra, dopo i dati, è un pattern più lungo.
3. **Entrate e uscite:** restano al 50 %; il 75 % costa tre cambi in più.
4. **Gli istanti di calo non seguono le entrate e uscite:** il calo a 2:00 cade proprio su un cambio, l'inizio di A′. Con sei istanti di un solo ascoltatore non si conclude.
5. **Fra frasi la varietà c'è già** per Pulso e Trama. Il Vettore del seed 34 invece cambia ritmo solo nel 29 % dei passaggi.

## 3. Leve proposte, in ordine

| Leva | Perché | Come si misura | Rischio |
|---|---|---|---|
| **L1, pattern di 2 o 4 battute** per Pulso e Trama, con risposta (a a′ o a b a c) | G1 al 100 %; loop dell'esperto di 8 | G1 sotto il 50 %; lignaggio ammesso | Varietà fittizia se b e c non derivano da a |
| **L2, motivo di Vettore di 2 battute**, domanda e risposta | «figura che si ripete»; è **I6** | G1 del Vettore, IDyOM | Contratto del motivo e controlli d'invarianza |
| **L3, chiusura a fine gruppo di 8** | 75 % contro 17–54 % nell'esperto, dopo i dati | G2 maggiore di 0 | Sostegno debole: da pre-registrare da capo |
| **L4, tre entrate o uscite in più** | 50 % contro 81 % | finestre ≥ 75 % | D-M2-51 tiene ferma la tabella attuale: l'esperto la approva («danno forma»). Si aggiunge, non si sostituisce |
| **L5, basso che varia** (I3) | linea V2 | finestre del basso | Dipende dal quinto ruolo |

**Raccomandazione:** L1 e L2 come fulcro, cioè **I4 e I6 insieme**, perché attaccano la stessa causa. L4 come complemento. L3 solo con una sua verifica.

## 4. Limiti

- Simbolico; un brano dell'esperto; la lettura a distanza 8 è successiva ai dati.
- Un ritmo che si ripete non è di per sé un difetto (Agres), e nell'ascolto distratto la sazietà può non comparire (Szpunar).

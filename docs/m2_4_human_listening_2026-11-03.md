# M2.4 — Ascolto dell'esperto di fine Inc-5 (2026-11-03)

- **Stato:** evidenza qualitativa **supplementare**, non un gate e non una sessione alla cieca. Alimenta le decisioni di M2.5 ([dossier delle evidenze](m2_procedural_evidence.md) §4). Soddisfa la registrazione richiesta dalla specifica §32 (raccordo con la ricerca) e dal [registro di ratifica](m2_4_decision_ratification.md) §1.
- **Materiale:** [pacchetto del 2026-11-03](m2_expert_listening_package.md), zip SHA-256 `f6b3e079c0d835461aaddc4eb274d6f299ec09d5caeb71e4752f388eb5441f27`.
  - Il brano è identico al byte a `artifacts/cacp/procedural_v1/procedural_v1.wav` (SHA-256 `dcb667ea0f493d2cef0f5feb431bc59d979274d24a38215d2b917745b82836ec`, `pcm_content_hash` 15126969907236845529).
  - Le 10 clip coincidono con gli hash della chiave.
- **Ascoltatore:** lo stesso esperto esterno del [2026-10-28](m2_4_human_listening_2026-10-28.md), come risulta dalle sue risposte. Il nominativo è noto al titolare; qui non è riportato.
- **Condizioni:** cuffie Audio Technica M50x, interfaccia Behringer U-PHORIA UMC204HD, volume confortevole e non misurato (circa 1/4 del potenziometro). Solo cuffie: nessun ascolto su diffusori.
- **Metodo dell'agente:** ogni osservazione verificabile è confrontata con il piano ([intent del seed 1337](research/experiments/cacp_methods/fixture_intents_seed1337.csv)), con il manifest di `procedural_v1` e con i file del pacchetto. Un riscontro conferma che un fenomeno esiste nel piano o nel segnale, non il giudizio estetico. L'agente non ha ascoltato l'audio. Il testo integrale delle risposte è nell'[appendice](#appendice--risposte-dellesperto).

## 1. Esito in sintesi

- **Tenuta tecnica:** nessun click né artefatto nel brano; sezioni tutte distinguibili.
- **Punti critici:**
  - il kick è «troppo basso» nel mix (osservazione con forte enfasi);
  - la percussione FM a sinistra è «ancora fastidiosa»;
  - la transizione A→B è «molto brusca»;
  - dopo il secondo loop il pezzo chiede una «decisa variazione melodica» o un nuovo suono.
- **Rilascio (Q7):** nessuna differenza significativa fra le clip da 2 a 80 ms, e nessuna preferita.
- **Viste a pentagramma:** inutili senza nomi di strumenti riconoscibili e una griglia di riferimento.
- **Giudizio complessivo:** il brano non contiene variazioni tali da fargli cambiare idea rispetto alla valutazione precedente.

**Il giudizio complessivo è esatto e atteso.** Il PCM è identico a quello ascoltato il 2026-10-28: Inc-5 ha aggiunto strumenti di misura e non ha toccato il generatore. Il pacchetto non glielo diceva, quindi l'esperto lo ha rilevato da solo. Non è una regressione, e non è neppure un nuovo giudizio su una versione diversa: **F1–F6 non sono ancora stati affrontati**.

## 2. Domande e riscontri

| Q | Risposta (sintesi) | Riscontro dell'agente | Esito |
|---|---|---|---|
| Q1 | Nessun click o artefatto | Render offline, 0 frame persi, 0 campioni clippati. Il transitorio di 2 ms di F6 esiste nel segnale, ma non è percepito come difetto | **Evidenza favorevole** per «senza crackle», solo sul WAV offline: resta la verifica su device |
| Q2 | Sezioni tutte distinguibili. Transizione A→B «molto brusca», B→A′ «più fluida» | §3.1: il salto di tempo è uguale in valore assoluto (±20 BPM), ma in A→B cambia anche il posizionamento metrico di Pulso e Trama | **Criterio M2.4 sostenuto** (sezioni distinguibili). Asimmetria: spiegazione proposta, da verificare |
| Q3 | Il kick sostiene il metro, il tempo forte si percepisce: «Sì» | Kick sul tempo forte in sole 16 battute su 96 (§3.2). Vettore suona sui passi 0, 4, 8, 12 in tutte le 24 frasi, Corpo attacca sul passo 0 | **Sfuma F1**: il metro regge. Non è ancora noto se lo regga il kick o gli altri ruoli (§3.2) |
| Q4 | Percussione FM a sinistra «ancora fastidiosa» | Audio invariato. Trama −15,5…−15,9 dB rispetto al mix, il ruolo più basso: il fastidio dipende dall'immagine stereo, non dal livello (AG-MM-12, delay 11/17 ms) | **F2 confermato** per la seconda volta |
| Q5 | «Non diventa mai fastidiosa», ma dopo il secondo loop serve una «decisa variazione melodica» o un nuovo suono | Il punto coincide con il 2026-10-28 («dopo il secondo loop»). Nel piano Pulso, Trama e Vettore hanno lo stesso schema ritmico per tutte le 64 battute di B e A′ (§3.3) | **F3 e F5 confermati**. Il giudizio è però più mite del 7/7 di ripetitività: non è fastidio, è mancanza di sviluppo |
| Q6 | Kick «TROOOOOOOOOOOOOOOOOOOOOPPO basso». Ripete i suggerimenti di mix già dati | Misurato: Pulso −10,6…−12,4 dB rispetto al mix, Corpo e Vettore circa −3 dB (§3.4) | **F4 confermato con una misura** (H4 del 2026-10-28, ora misurato) |
| Q7 | Clip 1 «ok», le altre «uguale a prima», nessuna preferita, per entrambe le frasi | §4: clip integre e diverse fra loro; la differenza è misurabile ma non percepita | **Nessuna preferenza per H-03** |
| Q8 | Viste a pentagramma poco utili: servono nomi riconoscibili e una griglia | Verificato sulle immagini del pacchetto: nessuna etichetta di corsia, solo le stanghette di battuta. La legenda sta nel testo di accompagnamento | **Confermata**, §5 |

## 3. Approfondimenti

### 3.1 Perché A→B può suonare più brusca di B→A′

Passi da sedicesimo per battuta, dal piano:

| Confine | Tempo | Pulso | Trama | Vettore |
|---|---|---|---|---|
| A→B (frase 7 → 8) | 120 → 140 BPM | `0,4,8,12` → `2,6,10,14` (dal tempo al controtempo) | `2,5,7,9,14` → `0,3,5,7,12` | `0,4,8,12` invariato |
| B→A′ (frase 15 → 16) | 140 → 120 BPM | `2,6,10,14` invariato | `0,3,5,7,12` invariato | `0,4,8,12` invariato |

Entrambi i confini cambiano anche modo e registro. La differenza è che in A→B il tempo cambia **insieme** al posizionamento metrico di due ruoli, mentre in B→A′ i ritmi proseguono identici.

**Ipotesi dell'agente, non verificata all'ascolto:** il riferimento ritmico che continua rende più accettabile il salto di tempo. Se è corretta, una transizione più morbida non richiede necessariamente rampe di BPM (estensione di `MusicClock`, F3): basterebbe non spostare i pattern sullo stesso confine del tempo.

### 3.2 Q3 rispetto a F1

Il 2026-10-28 lo spostamento del kick risultava «molto strano», con il metro poco chiaro senza un riferimento forte. Oggi alla domanda diretta l'esperto risponde «Sì». Le due risposte non si escludono, ma la seconda ridimensiona il problema.

| Sezione | Passi di Pulso | Battute |
|---|---|---|
| A1 | `2,5,7,10,13,15` | 16 |
| A2 | `0,4,8,12` | 16 |
| B e A′ | `2,6,10,14` | 64 |

**Ipotesi dell'agente:** il metro è sostenuto da Vettore e Corpo, che cadono sempre sul tempo e sono 7–9 dB sopra il kick (§3.4). È la struttura della proposta dell'esperto del 2026-10-28 (§3ter di quel registro): un riferimento stabile separato dallo spostamento del kick.

**Conseguenza pratica: F1 e F4 sono collegati.** Il kick oggi è basso, quindi anche il suo spostamento pesa poco. Alzandolo, il controtempo di B e A′ diventerà più evidente e la risposta a Q3 potrebbe cambiare. Le due questioni vanno valutate insieme, con un nuovo ascolto dopo la modifica del livello.

La risposta è di una sola parola a una domanda doppia, e non distingue le sezioni: va letta con cautela.

### 3.3 Sviluppo del materiale (Q5)

- Pulso, Trama e Vettore hanno un solo schema ritmico per sezione, cambiato soltanto a metà di A. In B e A′ restano fissi per 64 battute; Vettore resta sui quarti per tutte le 96 battute.
- L'unica entrata di un nuovo suono è la Trama alla battuta 16 (32 s). Dopo non entra e non esce più nessun ruolo.

Questi fatti sono coerenti con la richiesta dell'esperto e con F5 (arco orchestrale, entrate e uscite). Non dicono quale variazione funzionerebbe.

### 3.4 Livelli per ruolo (Q6)

`stem_to_mix_db` dal manifest di `procedural_v1` (build `fa8ac3c`, RMS dello stem rispetto al mix per frase):

| Porzione | Pulso | Corpo | Trama | Vettore |
|---|---|---|---|---|
| A1 (frasi 0–3) | −10,9…−10,6 dB | −3,9…−3,6 dB | silenzio | −3,2…−2,9 dB |
| A2 (4–7) | −12,2 dB | −3,5 dB | −15,6…−15,5 dB | −3,0 dB |
| B (8–15) | −11,9 dB | −3,6…−3,5 dB | −15,9 dB | −3,1 dB |
| A′ (16–23) | −12,4 dB | −3,5…−3,4 dB | −15,5 dB | −3,0…−2,9 dB |

- Il kick è **7–9 dB sotto** Corpo e Vettore in RMS, e le sue basse frequenze sono meno percepite a parità di livello (curve isofoniche, ISO 226). È il riscontro quantitativo che H4 del 2026-10-28 rimandava.
- Spazio disponibile: true peak −10,45 dBTP contro un ceiling di −1,0 dBTP, riduzione del limiter 0 dB. Alzare il kick non sembra quindi richiedere interventi sul master. Il valore esatto va provato, perché il picco del mix dipende dalla somma dei ruoli.
- Questa è una misura RMS, non di loudness per stem: gli stem non sono archiviati su disco, solo le loro statistiche.

## 4. Q7: clip del rilascio

Chiave applicata dopo aver ricevuto le risposte. Le clip hanno gli SHA-256 attesi, quindi l'esito non dipende da un errore di confezione.

| Clip | Frase A (fixture frase 5) | Frase B (fixture frase 10) |
|---|---|---|
| 1 | 2 ms (produzione): «ok» | 30 ms: «ok» |
| 2 | 30 ms: «uguale a prima» | 80 ms: «uguale a prima» |
| 3 | 50 ms: «uguale a prima» | 2 ms (produzione): «uguale a prima» |
| 4 | 15 ms: «uguale a prima» | 15 ms: «uguale a prima» |
| 5 | 80 ms: «uguale a prima» | 50 ms: «uguale a prima» |

Differenza fra clip consecutive, in dB rispetto al mix:

| Frase | Coppie (ms) | Differenza |
|---|---|---|
| A | 2→30, 30→50, 50→15, 15→80 | −19,9 / −24,8 / −20,0 / −16,7 dB |
| B | 30→80, 80→2, 2→15, 15→50 | −17,7 / **−14,1** / −22,3 / −18,7 dB |

Fra 2 e 80 ms l'RMS varia di circa 0,1 dB e il picco di +0,7 dB (A) e +1,5 dB (B).

**Lettura.**
- Il confronto più ampio, 80→2 ms in B, ha un segnale differenza a −14 dB dal mix, ed è giudicato «uguale a prima». Una differenza misurabile nella forma d'onda non è quindi bastata a farsi notare, nelle condizioni di questa sessione.
- Con Q1 (nessun click), questo significa che **F6 non emerge come problema percettivo**. Il rilascio di 2 ms resta un problema misurato per il rivelatore di onset (5e.3, 5e.3b).
- **Non è una prova di inaudibilità.** Non è un test ABX, l'esperto parla di variazioni «significative», e un mix diverso potrebbe rendere le differenze più o meno evidenti.

## 5. Viste a pentagramma

Le viste mettono a confronto note pianificate e onset rilevati, ma da sole non sono leggibili per un musicista. L'esperto chiede:

- nomi di strumenti riconoscibili («lead synth, drums, percussion»), non i nomi interni dei ruoli;
- una griglia di riferimento (quarti, ottavi o sedicesimi).

Oggi le immagini non hanno etichette e mostrano solo le stanghette di battuta. La corrispondenza fra corsie e ruoli è nel testo di accompagnamento, non nella vista.

## 6. Conseguenze per M2.5 (proposte, non decisioni)

Le priorità aggiornate sono nella [proposta per l'ingresso di M2.5](research/M2_5_Entry_Music_Quality_Proposal.md) §8. Nessuna è adottata.

| ID | Effetto dell'ascolto |
|---|---|
| F1 | Ridimensionato: «Sì» a Q3. Da valutare insieme a F4 |
| F2 | Confermato, seconda segnalazione |
| F3 | Confermato. A→B è il confine peggiore; si può provare prima a separare il cambio di tempo da quello dei pattern |
| F4 | Rafforzato e misurato: kick 7–9 dB sotto Corpo e Vettore. Suggerimenti di mix ripetuti, nessuno adottato: ciascuno ha un costo DSP da valutare rispetto a D-M2-10 e ADR-0004 |
| F5 | Confermato: variazione melodica o nuovo suono dopo il secondo loop |
| F6 | Ridimensionato: nessuna differenza percepita, nessun click. H-03 può decidere sulle misure, senza un vincolo percettivo |

Per il prossimo pacchetto d'ascolto:

- dire che cosa è cambiato rispetto all'ascolto precedente;
- riusare le scale 1–7 del 2026-10-28, assenti in questo modulo, per poter confrontare i giudizi;
- chiedere la risposta a Q3 per sezione.

## 7. Limiti

- **Campione:** un solo ascoltatore, lo stesso dell'ascolto precedente; nessuna indipendenza fra le due valutazioni.
- **Condizioni:** solo cuffie, volume non misurato, ora della sessione non annotata.
- **Clip:** l'ordine è mescolato ma la chiave è nota a chi conduce; l'esperto poteva ascoltarle nell'ordine preferito. Il confronto fra clip consecutive presume l'ordine numerico, suggerito da «uguale a prima». Sono estratti di 7–8 s.
- **Offline:** il WAV non dimostra l'assenza di crackle sul percorso realtime su device.
- **Riscontri:** confermano fenomeni presenti nel piano o nel segnale. Le spiegazioni dei §3.1 e §3.2 sono ipotesi dell'agente, non verificate all'ascolto.
- **Gate:** l'ascolto formale resta al gate M2.5 (specifica §32), con device reale e protocollo.

## 8. Seguito del 2026-11-04: risposte alla nota sul sintetizzatore

L'esperto ha letto la nota sul sintetizzatore e ha risposto al titolare; il testo è in appendice. Riscontri dell'agente sul [piano](research/experiments/m25_ensemble_baseline/voice_use.output.txt) e sul codice:

| Osservazione | Riscontro | Conseguenza proposta |
|---|---|---|
| Urgente bisogno di una linea di basso | La nota più grave di Corpo è D3 (147 Hz) in A e A′, G2 (98 Hz) in B. Sotto c'è solo il kick: nessun basso sostiene l'armonia | Nuova questione **F7**, da valutare con F4 |
| Al kick manca peso nelle basse | Le note di Pulso durano 54–62 ms: il NoteOff taglia il decay di 70–80 ms, e la nota di base dura pochi cicli. L'esperto aggiunge che è difficile giudicare a questo volume (−21,25 LUFS) | Provare durata e livello insieme, prima di concludere sul solo gain |
| Un kick campionato libererebbe voci | Le voci libere esistono già: Pulso, Trama e Vettore ne usano una sola, quindi 3 voci su 8 sono ferme | Le voci si possono riassegnare anche senza campioni; rilasci e sovrapposizioni vanno verificati |
| Supersaw per il lead: due voci all'unisono, leggermente stonate e aperte | Vettore ha già la seconda voce libera | Candidato economico per M2.5.2 o M2.5.3 |
| LFO sul filtro a quarti o ottavi, poi saturazione | Non esistono oggi. L'LFO si può legare al clock musicale, quindi resta deterministico | Prima scelta dell'esperto per la ricchezza; decidere a CP-1 |
| La FM non è sbagliata di per sé | Il modello indicava ruvidità, l'ascolto non la considera un difetto | Rapporto FM ridimensionato: nessuna priorità in M2.5.2 |
| Campioni per le percussioni: meno CPU | Vero in generale, ma oggi la CPU non è il limite: callback mediana 0,07 ms su 10 ms | Il motivo per valutarli è il suono, non il costo |
| Latenza e CPU contano anche per chi compone | Corretto: l'argomento «tempo reale» della nota non distingue CYMATICA da una DAW. Restano determinismo e controllo | Valutare un VST con preset in R-M25-9, senza darlo per escluso |

**Domanda sul delay.** I documenti riportano i valori 11/17 ms (ADR-0004) come micro-delay stereo, senza una motivazione musicale o una prova d'ascolto. L'intento era allargare l'immagine, non creare un'eco. L'ascolto del 2026-10-28 ne ha mostrato l'effetto collaterale: lateralizza a sinistra (F2).

## Appendice — Risposte dell'esperto

Testo consegnato dal titolare il 2026-11-03, invariato. È omessa solo la riga con il nome.

```text
CYMATICA - risposte all'ascolto esperto (pacchetto del 3 novembre 2026)

Q1: Nessun click o artefatto.
Q2: Sono tutte distinguibili ma trovo la transizione di tempo tra A e B molto brusca. Tra B e A' è più fluida.
Q3: Sì.
Q4: La percussione FM sul lato sinistro è ancora fastidiosa.
Q5: Non diventa mai fastidiosa ma dopo il secondo loop il pezzo ha bisogno di una decisa variazione melodica oppure dell'introduzione di un nuovo suono.
Q6: Il kick è TROOOOOOOOOOOOOOOOOOOOOPPO basso nel mix. Valgono sempre i suggerimenti precedenti sul mix, che reincollo:

Una buona tecnica per ottenere un "rough mix" bilanciato è quella di prendere del pink noise e bilanciare ogni strumento fino a quando questo diventa appena udibile.
Perchè? Perchè il pink noise è già una buona approssimazione di come dovrebbe suonare un brano: tieni a mente che per questioni evolutive, l'orecchio umano è molto sensibile alle frequenze medio-alte e molto poco sensibile alle basse.

Poichè questi brani sono interamente generati in modo algoritmico, un buon riferimento per cercare di bilanciare il mix secondo quello che è l'ascolto dell'orecchio umano è la curva di Fletcher-Munson.

I suoni devono essere inoltre posizionati nello spazio: considerare l'uso di riverberi di diversa durata (OCCHIO AI LIVELLI PERCHÈ MOLTO FACILE STRAFARE) per posizionare meglio gli strumenti.

Sui sintetizzatori si può giocare molto di modulazioni sonore (per esempio mappando il filtro sull'LFO, oppure l'attacco o letteralmente qualsiasi cosa).
Una botta di saturazione sul master può aiutare decisamente la "ricchezza sonora percepita".

Compressori, in questo contesto, se usati, dovrebbero meramente essere una scelta stilistica per aggiungere carattere.

Per creare panorami sonori interessanti si può anche giocare con Flanger, Chorus, Delay (molto figo giocare anche con i feedback).
Q7 Frase A clip 1: ok
Q7 Frase A clip 2: uguale a prima
Q7 Frase A clip 3: uguale a prima
Q7 Frase A clip 4: uguale a prima
Q7 Frase A clip 5: uguale a prima
Q7 Frase A preferita: 
Q7 Frase B clip 1: ok
Q7 Frase B clip 2: uguale a prima
Q7 Frase B clip 3: uguale a prima
Q7 Frase B clip 4: uguale a prima
Q7 Frase B clip 5: uguale a prima
Q7 Frase B preferita: 
Q8: Per quanto riguarda Q7: non riesco ad individuare variazioni significative tra una clip e l'altra. 

Le "Viste a pentagramma" non forniscono informazioni utili senza chiari riferimenti, come il nome dello strumento - che non sia Pulso, Melodio, MimmoPippoCazzo... ma bensì uno strumento riconoscibile, tipo: lead synth, drums, percussion - e una griglia di riferimento (teniamo i quarti? gli ottavi? i sedicesimi?).
Giudizio complessivo: Rispetto a quanto scritto nella valutazione precedente, il brano non mi sembra contenere variazioni significative (nemmeno a livello di timbrica e mix) tali da poter cambiare i miei pensieri.
Data e ora: 2026-11-03
Apparecchiatura: Audio Technica M50x + Behringer U-PHORIA UMC204HD
Volume: Volume d'ascolto non misurato, confortevole. Circa 1/4 del potenziometro.
```

Seguito del 2026-11-04, sulla nota del sintetizzatore. Testo invariato, omessi nome e orari.

```text
chiedigli perchè ha usato un delay così corto: `Poi c'è un delay con 11 ms a sinistra e 17 ms
a destra, feedback 30 %, mix wet circa 32 %.`, cosa cercava di ottenere?

questo (paragrafo che ti incollo dopo) potrebbe tecnicamente avere senso ma considera che anche quando una persona compone e registra della musica ha bisogno di sentire sempre lo stesso audio con una latenza il più bassa possibile e un costo in CPU anche lui, più basso possibile.

[paragrafo «Perché non un synth esistente» della nota]

Non so cosa costi implementare un qualsiasi VST open source preesistente e andare di preset. Per quanto riguarda le percussioni, costa MOLTO DI MENO in termini di cicli di CPU usare dei campioni audio rispetto al sintetizzarli, non a caso quando viene composta la musica in una DAW classica (es. Ableton Live) la maggior parte delle persone carica di drum kit all'interno di un campionatore.

Quale aggiunta porterebbe più ricchezza con meno risorse: un riverbero, una saturazione o una
modulazione?
—- una modulazione sul synth: potrebbe bastare un LFO mappato sul filtro, che magari lo chiude in quarti o ottavi (per introdurre una componente ritmica su qualche pad o lead), seguito da saturazione (anche se non vai in oversampling, in questo momento non ci frega molto), con cui puoi giocare e introdurre anche distorsione, volendo. Se usi un campione di kick, potenzialmente potresti liberare almeno una voce, se riesci a mapparla sulla lead, puoi iniziare ad ottenere una SUPERSAW usando due voci che suonano all'unisono, con pitch leggerissimamente differente tra di loro e se vuoi si potrebbero pannare una leggermente a destra e una leggermente a sinistra.

Il suono FM ti sembra sbagliato di per sé? Una nostra misura indica che il rapporto 1,88 potrebbe
renderlo ruvido, ma è solo un modello: vorremmo confrontare altri rapporti all'ascolto
—— No. Considera che la FM synthesis produce al 90% suoni strani.

Oltre al livello, cosa manca al kick: attacco, durata o peso nelle basse?
Peso nelle basse. Difficile però giudicare con volumi così bassi.

Con 8 voci, a cosa daresti priorità?
In questo momento avete urgente bisogno di una linea di basso.
```

# M2.5 — Lacune della verifica audio autonoma: modelli, strumenti e proposta

- **Stato:** ricerca non normativa, scritta il 2026-11-16. Proposta, non decisione: non modifica codice di runtime, specifica, ADR, contratti, golden né `docs/dependencies.md`. Nessuno strumento è stato installato o eseguito.
- **Perché:** ADR-0007 D-M2-30 stabilisce che una variazione piccola e misurabile si verifica senza ascolto umano, e che una variazione non verificabile in autonomia è un difetto degli strumenti. Questo dossier censisce i difetti degli strumenti emersi in M2.5.2 e propone come colmarli.
- **Base nel repository:** [ADR-0007](../adr/0007-m2-5-entry-decisions.md) §7 (R-M25-1, R-M25-2) e §8.1; [risultati FM](M2_5_2_FM_Ratio_Results.md); [pre-registrazione F2](M2_5_2_F2_Trama_Stereo_Preregistration.md); [strumenti d'insieme](M2_5_1c_Ensemble_Tools_Results.md); [dossier R-M25-7](M2_5_F5_Music_Director_Research.md); [dossier R-M25-8](M2_5_1_Windows_Audio_Glitch_Detection.md); codice in `engine/cacp/`.

## Come leggere il dossier

Ogni lacuna separa **Evidenza nel repository**, **Fonti**, **Strumenti** e **Proposta**. Le proposte sono inferenze nostre: un modello uditivo non è una prova percettiva, e nessun numero qui è una soglia.

Livelli di verifica, come nel dossier R-M25-7, mai alzati:

- **FT**: testo letto direttamente.
- **FT-parz**: come FT, solo nelle parti citate.
- **AM**: solo abstract, metadati o pagina di catalogo, letti tramite ricerca web il 2026-11-16.
- **SEC**: nota solo da fonte secondaria.
- **NV**: non verificata in questa sessione; citata per completezza, da controllare prima di ogni uso.

Per gli strumenti: le pagine dei repository (licenza, linguaggio, dipendenze) sono state lette il 2026-11-16 tramite fetch automatico (**FT** sulla pagina del repository, non sul codice). Versioni e tag non erano visibili sulle pagine: nessuna versione è fissata qui.

**Correzione trovata durante la verifica.** Il DOI `10.1121/1.408170`, che si potrebbe attribuire all'articolo di Sethares del 1993, appartiene a un altro articolo della stessa annata di JASA (Wang e Chu, m-sequence). Il repository non lo cita. Il DOI di Sethares 1993 risulta `10.1121/1.408175` solo da una copia di Wikipedia (SEC): va confermato sull'editore prima di citarlo.

## Sintesi

| # | Lacuna | Oggi | Proposta prioritaria | Dipendenza nuova? |
|---|---|---|---|---|
| 5 | Onset allineati ai rilasci | Calibrazione libera sceglie un offset di 95 ms (rilasci) | Abbinamento etichettato attacco/rilascio dagli eventi, offset limitato | No |
| 7 | Click e crackle | `artifact_present(click)` → `unverifiable`, «no descriptor» | Rilevatore a residuo di predizione lineare, allineato agli eventi, con iniezioni note | No |
| 2 | Lateralizzazione e larghezza | Side/Mid; ILD a banda larga (F2) | ILD, ITD e IACC per banda uditiva, modello d'ascolto in cuffia | No (oracolo facoltativo: AMT) |
| 1 | Bilanciamento percepito | LUFS per stem (BS.1770) | Loudness specifica per banda (ISO 532-1) e perdita di loudness per mascheramento fra stem; pendenza spettrale | No per il nativo; oracolo MoSQITo con approvazione |
| 3 | Rugosità sull'audio reso | Sethares su spettri teorici | Rugosità di Daniel e Weber sul PCM degli stem, con l'oracolo MoSQITo | Sì, per l'oracolo |
| 4 | Distinguibilità timbrica | Nessuna misura | Descrittori per nota a loudness e altezza allineate, separabilità con controlli | No |
| 6 | Sviluppo musicale | M4 non validata (F8, F9) | Nuove misure pre-registrate sugli stessi controlli F1–F9: distanza graduata, novità da autosimilarità, contenuto informativo (IDyOM, offline) | IDyOM solo con approvazione |

Il principio comune: CYMATICA ha una **verità di riferimento** che i lavori pubblicati quasi mai hanno. Il render è deterministico, gli stem sono separati per costruzione, l'event log dà l'istante di ogni NoteOn, NoteOff, furto di voce e dissolvenza di 2 ms, e i golden PCM permettono il null test. Ogni misura proposta sfrutta questa verità: per etichettare gli eventi, per iniettare difetti noti come controlli positivi, e per confrontare A e B con tutto il resto identico.

## Regola di validazione comune a tutti gli strumenti

Nessuno strumento entra in una pre-registrazione finché non ha superato, in una propria pre-registrazione con hash:

1. **Controllo positivo:** un segnale sintetico in cui l'effetto è presente con ampiezza nota, ottenuto dal synth reale o iniettato nel PCM; lo strumento lo deve vedere, con una curva dose-risposta monotona su almeno tre ampiezze.
2. **Controllo negativo:** un segnale in cui l'effetto è assente per costruzione, comprese le insidie note (per i click: l'attacco del kick; per la lateralizzazione: un segnale centrato ma decorrelato); lo strumento non lo deve vedere.
3. **Invarianza:** stesso risultato con render ripetuti e con blocchi di dimensione diversa (bit-exact atteso).
4. **Riproduzione di un dato pubblicato**, dove esiste: un valore di riferimento della norma o dell'articolo (per esempio la loudness ISO 532-1 di un tono di riferimento, o la rugosità di un tono modulato in ampiezza).
5. **Accordo con un oracolo indipendente**, quando c'è: un'implementazione di terzi eseguita fuori dalla build sugli stessi WAV, con la tolleranza fissata prima.

Esiti `MEASURED`, mai `PASS`, finché il titolare non ratifica una soglia; valori indefiniti `null` con `INCONCLUSIVE`, come in `analysis_status.h`.

---

## 1. Bilanciamento percepito fra ruoli (R-M25-1)

**Evidenza nel repository.** `engine/cacp/loudness.h` implementa BS.1770-5 (K-weighting, gating) per mix e stem: misura la loudness di ogni ruolo **da solo**, non quanto se ne sente nel mix. L'esperto chiede rumore rosa e curve isofoniche (ADR-0007 §8.1). Il dossier R-M25-7 ha già osservato che il bilanciamento contro rumore rosa è una pratica senza misure pubblicate a sostegno, e che il rumore rosa (−3 dB/ottava) non coincide con la pendenza di circa −5 dB/ottava misurata da Pestana et al. sui singoli al numero uno: serve come partenza d'orecchio, non come criterio. La predizione mancata di D-M2-33 (Trama −1,03 LU a parità di guadagno) mostra che il livello di bus non predice la loudness; la loudness parziale nel mix è ancora meno prevedibile.

**Fonti.**
- ISO 226:2023, *Normal equal-loudness-level contours*, terza edizione, 2023-03 (AM: catalogo DIN e iTeh). Contorni per toni puri, campo libero frontale, ascolto binaurale, 18–25 anni. Le curve valgono per toni puri stazionari, non per stem musicali: usarle come pesatura è un'approssimazione.
- Suzuki e Takeshima 2004, «Equal-loudness-level contours for pure tones», JASA 116, 918–933, [doi:10.1121/1.1763601](https://doi.org/10.1121/1.1763601) (AM: pagina dell'editore).
- ISO 532-1:2017 (Zwicker, stazionario e tempo-variante) e ISO 532-2:2017 (Moore–Glasberg) (NV: norme a pagamento, non lette).
- Glasberg e Moore 2005, «Development and evaluation of a model for predicting the audibility of time-varying sounds in the presence of background sounds», JAES 53(10), 906–918, [AES e-library 13391](https://www.aes.org/e-lib/browse.cfm?elib=13391) (AM). È il modello di **loudness parziale** tempo-variante: richiede i pattern di eccitazione del segnale e dello sfondo, cioè esattamente ciò che gli stem separati permettono.
- Moore, Glasberg, Varathanathan e Schlittenlacher 2016, «A loudness model for time-varying sounds incorporating binaural inhibition», Trends in Hearing 20, [doi:10.1177/2331216516682698](https://doi.org/10.1177/2331216516682698), testo su [PMC5318944](https://pmc.ncbi.nlm.nih.gov/articles/PMC5318944) (AM).
- Hafezi e Reiss 2015, «Autonomous multitrack equalization based on masking reduction», JAES 63(5), 312–323, doi:10.17743/jaes.2015.0021, [AES e-library 17637](https://www.aes.org/e-lib/browse.cfm?elib=17637) (AM). Secondo l'abstract le misure oggettive di mascheramento esistenti sono poco adatte a quantificare il mascheramento percepito nei mix multitraccia: **un avvertimento diretto contro l'uso di una metrica di mascheramento come criterio**.
- Pestana et al. 2013 e De Man et al. 2014: già nel dossier R-M25-7 (FT-parz là).

**Strumenti.**

| Strumento | Cosa offre qui | Licenza | Maturità | Fuori dal runtime? | Costo |
|---|---|---|---|---|---|
| [MoSQITo](https://github.com/Eomys/MoSQITo) | Loudness ISO 532-1 stazionaria e tempo-variante, loudness ECMA-418-2, sharpness DIN 45692; secondo la [pagina di scopo](https://mosqito.readthedocs.io/en/stable/source/user_guide/scope.html) marcate «Validated», senza dire su quali dati. Moore–Glasberg e ISO 532-2: «To do» | Apache-2.0 | Media: progetto di un'azienda, documentato; la pagina di riferimento e quella di scopo si contraddicono sulla rugosità ECMA | Sì, Python offline | numpy, scipy, pyuff; venv locale da approvare |
| [libebur128](https://github.com/jiixyj/libebur128) | BS.1770/R128, true peak | MIT | Alta, molto diffusa | Sì | Ridondante: `loudness.cpp` esiste già. Utile solo come **oracolo** del nostro BS.1770 |
| AMT | Modelli di loudness e di eccitazione in MATLAB/Octave | GPL-3 (nucleo) | Alta per la ricerca | Sì | Octave: installazione di software, da approvare |
| Programmi di Glasberg e Moore per la loudness parziale | Riferimento degli autori | NV | NV | NV | NV: non verificati in questa sessione |

**Proposta.**
- **B1 (nativo, nessuna dipendenza):** loudness specifica per banda critica degli stem, secondo ISO 532-1 tempo-variante, in `engine/cacp/`. Da essa: (a) profilo di loudness per ruolo e per banda; (b) **perdita di loudness per mascheramento** di un ruolo, definita come loudness del ruolo da solo meno una stima della sua loudness parziale nel resto del mix. La stima parziale nativa sarà un'approssimazione per banda dichiarata come tale, non il modello di Glasberg e Moore 2005, finché non c'è un'implementazione di riferimento verificata.
- **B2:** pendenza dello spettro medio del mix e per ruolo, confrontata con rumore rosa e con la pendenza di Pestana come **descrizione**, non come obiettivo.
- **Validazione:** riproduzione dei valori di riferimento della norma (da leggere: ISO 532-1 è a pagamento, va acquistata o consultata dal titolare); oracolo MoSQITo sugli stessi WAV; controlli: un tono a 1 kHz mascherato da rumore rosa a livelli crescenti (la perdita deve crescere in modo monotono), e due stem in bande disgiunte (perdita vicina a zero).
- **Limite dichiarato:** nemmeno la loudness parziale dice se il bilanciamento è *giusto*; dice solo se un cambio di livello sposta ciò che si sente del ruolo nel mix. Basta per la regola di D-M2-30 sulle variazioni di livello.

## 2. Lateralizzazione e larghezza stereo percepite (F2, R-M25-2)

**Evidenza nel repository.** `stereo.h` misura il Side/Mid a banda larga; la pre-registrazione F2 aggiunge l'ILD a banda larga del primo fronte e dell'intero stem, e dichiara nel §5 che lateralizzazione e larghezza percepite non coincidono con queste misure. L'esperto ha avvertito che il Side/Mid può indicare larghezza mentre si sente un suono spostato. La Trama ha un delay a due tap (11 e 17 ms): una situazione di precedenza, in cui il primo fronte domina la localizzazione.

**Fonti.**
- Breebaart, van de Par e Kohlrausch 2001, «Binaural processing model based on contralateral inhibition. I. Model structure», JASA 110(2), 1074–1088, [doi:10.1121/1.1383297](https://doi.org/10.1121/1.1383297) (AM: [TU/e](https://research.tue.nl/en/publications/binaural-processing-model-based-on-contralateral-inhibition-i-mod/)). La rappresentazione interna codifica insieme differenze di tempo e di intensità interaurali.
- Dietz, Ewert e Hohmann 2011, «Auditory model based direction estimation of concurrent speakers from binaural signals», Speech Communication 53(5), 592–605 (AM: [dblp](https://dblp1.uni-trier.de/rec/journals/speech/DietzEH11.html)). Il DOI corretto risulta `10.1016/j.specom.2010.05.006` (SEC); il DOI `10.1016/j.specom.2010.11.004` non è stato risolto. Secondo i riassunti, costanti di tempo brevi (circa 5 ms) sono decisive con sorgenti concorrenti.
- Hidaka, Beranek e Okano 1995, «Interaural cross-correlation (IACC), lateral fraction (LF), and low- and high-frequency sound levels (G) as measures of acoustical quality in concert halls», JASA 97(5), 3319 (AM; DOI `10.1121/1.412847` SEC). La larghezza apparente della sorgente si lega a 1 − IACC precoce (primi 80 ms) in più bande d'ottava. È un risultato di sale da concerto, non di cuffia: il trasferimento a un delay sintetico è un'inferenza.
- Majdak, Hollomey e Baumgartner 2022, «AMT 1.x: A toolbox for reproducible research in auditory modeling», Acta Acustica 6:19, [doi:10.1051/aacus/2022011](https://doi.org/10.1051/aacus/2022011) (AM).

**Strumenti.**

| Strumento | Cosa offre | Licenza | Fuori dal runtime? | Costo |
|---|---|---|---|---|
| AMT 1.x | Modelli binaurali (fra cui, secondo il paper, molti modelli di localizzazione), filtri gammatone, dati di HRTF | GPL-3 per il nucleo; licenze diverse per singoli modelli e dati, segnalate al primo avvio ([OEAW](https://www.oeaw.ac.at/isf/outreach/software/amt)) | Sì | MATLAB o Octave da installare (approvazione); è l'oracolo più autorevole |
| HRTF pubbliche (formato SOFA) | Simulazione dell'ascolto su altoparlanti | NV per ciascun dataset | Sì | Download da fissare con hash; serve solo per l'ascolto in altoparlante |

**Proposta.**
- **S-B1 (nativo):** banco di filtri gammatone (ERB) sugli stem e sul mix; per banda: ILD, ITD dal massimo della correlazione incrociata normalizzata in ±1 ms, IACC come valore di quel massimo; separatamente per il primo fronte (finestra breve dopo ogni NoteOn, nota dall'event log) e per l'intera nota. In cuffia, che è il caso segnalato («fastidiosa in cuffia»), i due canali arrivano direttamente alle due orecchie: niente HRTF. Per gli altoparlanti serve un'HRTF e un'ipotesi di disposizione, quindi è un secondo passo.
- **Lettura:** lateralizzazione = ILD e ITD del primo fronte pesati per banda con la loudness specifica di B1; larghezza = 1 − IACC. Side/Mid resta riportato, ma mai da solo.
- **Controlli:** positivo, pan di ampiezza a −6, −3, 0, +3, +6 dB e ritardo interaurale a ±0,3 e ±0,6 ms su rumore e su note reali della Trama (dose-risposta monotona); negativo, segnale centrato ma decorrelato (IACC basso, ILD 0: deve dire «largo e centrato», dove il Side/Mid direbbe solo «largo»); negativo per la precedenza: tap ritardato più forte del primo fronte, dove la misura sul primo fronte deve restare dalla parte del primo fronte. Oracolo: un modello binaurale di AMT sugli stessi WAV, se il titolare approva Octave.
- **Limite:** nessun modello sostituisce il giudizio su «troppo stretto» o «troppo largo»; la misura rende verificabile solo la domanda di F2 («è ancora spostata?»).

## 3. Rugosità e dissonanza sensoriale sull'audio reso

**Evidenza nel repository.** D-M2-33 è stata scelta con il modello di Sethares su spettri teorici stazionari (`experiments/m25_2_fm_ratio/fm_ratio_context.py`). I risultati FM §4 dichiarano il limite: il modello conosce solo l'interferenza fra parziali e ignora inviluppo, decadimento dell'indice e durata (note di 107–125 ms).

**Fonti.**
- Sethares 1993, «Local consonance and the relationship between timbre and scale», JASA 94(3), 1218–1228 (AM: [pagina dell'autore](https://sethares.engr.wisc.edu/papers/consance.html); DOI vedi correzione sopra).
- Plomp e Levelt 1965: già nel dossier R-M25-7.
- Daniel e Weber 1997, «Psychoacoustical roughness: implementation of an optimized model», Acustica 83, 113–123 (AM, da più elenchi bibliografici). Deriva dal modello di Aures: indici di modulazione nelle bande critiche. Lavora sul segnale, quindi vede inviluppi e transitori.
- ECMA-418-2 (modello uditivo di Sottek: loudness, tonalità, rugosità) (NV: norma non letta). Un articolo INTER-NOISE 2024 di autori legati a MoSQITo riferisce istruzioni ambigue e informazioni mancanti nella norma per la rugosità (AM: [Ingenta](https://ince.publisher.ingentaconnect.com/contentone/ince/incecp/2024/00000270/00000006/art00024)).

**Strumenti.** MoSQITo: rugosità di Daniel e Weber marcata «Validated» e «Available»; rugosità ECMA «Under dev.» nella pagina di scopo ma elencata nella pagina di riferimento: va usata solo Daniel e Weber finché la contraddizione non è chiarita. PsySound3 contiene Daniel e Weber in MATLAB (SEC). Essentia offre una dissonanza da picchi spettrali, ma è AGPL-3.0 e, secondo la pagina, i pacchetti pip sono per Linux: escluso su questa macchina.

**Proposta.**
- **R1:** rugosità di Daniel e Weber sul PCM reso, per stem e per coppie di stem sommati (Trama + Corpo, Vettore + Corpo), finestre allineate alle note dall'event log. Una variazione di timbro come D-M2-33 si sarebbe verificata così: rugosità di contesto prima e dopo, con il resto identico.
- **Passi:** prima l'oracolo MoSQITo (con approvazione), poi un port nativo in `engine/cacp/` se serve nella CI (Apache-2.0 permette il port con attribuzione; il port va registrato in `dependencies.md` come codice derivato).
- **Controlli:** positivo, tono modulato in ampiezza con frequenza di modulazione e profondità variate (la rugosità ha un massimo intorno a qualche decina di Hz di modulazione: il valore di riferimento esatto va letto nell'articolo, NV); negativo, tono puro e accordo di parziali coincidenti; controllo di coerenza: le misure FM con 1,88 e 1:1 devono ordinarsi come il modello di Sethares, altrimenti la discordanza va riportata, non risolta a favore di uno dei due.
- **Limite:** la rugosità è una dissonanza sensoriale; la consonanza armonica e culturale resta fuori (dossier R-M25-7, Harrison e Pearce 2020).

## 4. Distinguibilità timbrica fra ruoli e mascheramento

**Evidenza nel repository.** Con 1:1 lo spettro della Trama è armonico completo come quello di Corpo (dente di sega). La distinguibilità è stata rinviata all'ascolto cumulativo (D-M2-33, risultati FM §4). Esiste già un set di note sintetiche per ruolo, rese dal synth reale (`SynthNote`, set A e B di `acao02_v1`).

**Fonti.**
- Peeters, Giordano, Susini, Misdariis e McAdams 2011, «The Timbre Toolbox: Extracting audio descriptors from musical signals», JASA 130(5), 2902–2916, [doi:10.1121/1.3642604](https://doi.org/10.1121/1.3642604) (AM; PDF sul [sito McGill](https://www.mcgill.ca/mpcl/files/mpcl/peeters_2011_jasa.pdf), non letto). Circa dieci classi di descrittori relativamente indipendenti; un articolo ISMA 2017 segnala bug del toolbox (AM).
- McAdams et al. 1995, spazi timbrici (NV).

**Strumenti.**

| Strumento | Licenza | Esito |
|---|---|---|
| [Timbre Toolbox](https://github.com/VincentPerreault0/timbretoolbox) | Licenza propria di ricerca, **esclude ogni uso commerciale**, MATLAB con MEX | Escluso: CYMATICA è un prodotto |
| [librosa](https://github.com/librosa/librosa) | ISC | Utilizzabile come oracolo dei descrittori di base (centroide, flusso, MFCC); Python con scipy |
| Essentia | AGPL-3.0 | Escluso per licenza e piattaforma |

**Proposta.**
- **T1 (nativo):** per ogni ruolo, note isolate dal synth reale alla stessa altezza e alla stessa loudness (BS.1770 o B1): descrittori definiti in Peeters 2011, implementati da noi sullo STFT esistente (centroide e dispersione spettrale, rapporto armoniche dispari/pari, tempo d'attacco, decadimento del centroide nella nota, flusso). Distanza fra ruoli = distanza standardizzata nello spazio dei descrittori; separabilità = errore di un classificatore semplice a lasciare-uno-fuori fra note di due ruoli su altezze diverse.
- **Mascheramento timbrico:** nel mix, B1 dice quanto della Trama resta udibile per banda accanto a Corpo; T1 dice se ciò che resta è diverso.
- **Controlli:** negativo, due render dello stesso ruolo con altezze diverse (separabilità al caso); positivo, dente di sega contro sinusoide e FM 1,88 contro dente di sega (separabilità alta); confronto della distanza Trama–Corpo con 1:1 e con 1,88.
- **Limite:** la separabilità nello spazio dei descrittori non prova la distinguibilità percepita: è un'indicazione per l'ascolto cumulativo, che resta il giudice (D-M2-30).

## 5. Onset: calibrazione allineata ai rilasci (AC-AO-02)

**Evidenza nel repository.** `onset.h` usa il flusso spettrale con compressione logaritmica (Bello et al. 2005; Dixon 2006). I risultati FM §3 riportano che la Trama genera due rilevazioni per nota (attacco e rilascio del NoteOff a circa 100 ms), e che a δ 0,3 la calibrazione sceglie un offset di 95 ms con precisione e richiamo pari a 1: si allinea ai rilasci. La nota per M2.5.5 chiede di limitare l'offset o di escludere i rilasci.

**Fonti.**
- Bello, Daudet, Abdallah, Duxbury, Davies e Sandler 2005, «A tutorial on onset detection in music signals», IEEE Trans. Speech Audio Process. 13(5), 1035–1047, [doi:10.1109/TSA.2005.851998](https://doi.org/10.1109/TSA.2005.851998) (AM: [BibTeX FMP](https://www.audiolabs-erlangen.de/resources/MIR/FMP/data/bibtex/FMP_bibtex_BelloDADDS05_OnsetTutorial_IEEE-TSA.txt)).
- La valutazione corrente degli onset usa un abbinamento uno a uno entro una finestra di tolleranza (mir_eval) (NV sulla finestra esatta: va letta nel codice di mir_eval prima di citarla).

**Strumenti.** [mir_eval](https://github.com/mir-evaluation/mir_eval), MIT, Python con numpy e scipy: oracolo dell'abbinamento e delle metriche F. [madmom](https://github.com/CPJKU/madmom): codice BSD ma **modelli CC BY-NC-SA 4.0** (non commerciali): escluso dall'uso dei modelli.

**Proposta (nessuna dipendenza, la più economica e urgente).**
- **O1:** etichettare la verità dall'event log in due classi, attacchi (NoteOn) e rilasci (NoteOff e dissolvenze di 2 ms). L'abbinamento usa un'assegnazione uno a uno; ogni rilevazione è attribuita alla classe dell'evento più vicino. Si riportano separatamente richiamo degli attacchi, rilevazioni sui rilasci e falsi positivi.
- **O2:** la calibrazione dell'offset è vincolata a un intervallo fissato prima, derivato dalla latenza intrinseca misurata nel passo (a) di `acao02_v1` più un margine; fuori dall'intervallo il risultato è `INCONCLUSIVE`, mai un offset scelto. Così la degenerazione dei 95 ms diventa impossibile per costruzione.
- **Controlli:** positivo, note sintetiche con rilascio lungo e con rilascio a 2 ms, dove le rilevazioni sui rilasci devono essere contate come tali; negativo, etichette spostate di 95 ms, che con O2 devono dare `INCONCLUSIVE` e non precisione 1; oracolo mir_eval sugli stessi elenchi di eventi.
- Il ridisegno tocca `acao02_calibration.cpp` (codice CACP offline, non runtime) e va pre-registrato per M2.5.5 come già annotato.

## 6. Metriche di sviluppo musicale

**Evidenza nel repository.** La M4 (distanza categorica sugli intervalli fra frasi) non ha rilevato lo sviluppo nei controlli F8 (Bach) e F9: non è validata e non va usata per diagnosticarlo (OSS-M2.5.1c-F8F9-01). Il risultato §6 propone già una distanza graduata o la M3, da provare sugli stessi controlli F1–F9. Mancano ancora F7 (controllo umano con intenzioni dichiarate) e annotazioni indipendenti dei rapporti fra frasi (OSS-M2.5.1c-F8F9-02).

**Fonti.**
- Pearce 2018, «Statistical learning and probabilistic prediction in music cognition: mechanisms of stylistic enculturation», Ann. N. Y. Acad. Sci. 1423(1), 378–395, [doi:10.1111/nyas.13654](https://doi.org/10.1111/nyas.13654), [PMC6849749](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6849749/) (AM). IDyOM stima la probabilità di ogni evento dato il contesto appreso: il contenuto informativo misura l'imprevisto, che è il lato «sorprenda» della frase del titolare.
- Foote 2000, «Automatic audio segmentation using a measure of audio novelty», ICME 2000, vol. 1, 452–455 (AM: [BibTeX FMP](https://www.audiolabs-erlangen.de/resources/MIR/FMP/data/bibtex/FMP_bibtex_Foote00_segmentationNovelty_ICME.txt); DOI `10.1109/ICME.2000.869637` SEC). Novità dalla matrice di autosimilarità: misura «ritorni» e «sviluppi» come struttura della matrice.

**Strumenti.**

| Strumento | Licenza | Uso | Costo |
|---|---|---|---|
| [IDyOM](https://github.com/mtpearce/idyom) | GPL-3.0 | Offline, sulle sequenze simboliche esportate dal piano; GPL non contamina il gioco se lo strumento resta separato e non distribuito con esso (da confermare dal titolare) | Common Lisp: SBCL, Quicklisp, SQLite; installazione da approvare. Versione 1.8 del 2026-09-21 secondo il README |
| [music21](https://github.com/cuthbertLab/music21) | BSD-3-Clause | Conversione e analisi simbolica (per F8 e corpora di controllo) | Python 3.12+ |
| [librosa](https://github.com/librosa/librosa) | ISC | Autosimilarità e novità sull'audio, come oracolo | numpy e scipy |

**Proposta.**
- **D1 (nativo):** matrice di autosimilarità per ruolo, sia simbolica sia da chroma dell'audio reso, e novità di Foote; la firma A → B → A′ di Corpo, già vista dalla M4, è il primo controllo positivo reale.
- **D2:** distanza graduata sugli intervalli (costo proporzionale all'ampiezza) e una distanza per compressione, provate **sugli stessi F1–F9 con predizioni registrate prima**; nessuna misura nuova si usa senza superare F8.
- **D3 (con approvazione):** contenuto informativo di IDyOM per Vettore, addestrato su un corpus dichiarato; ipotesi: lo sviluppo reale ha contenuto informativo che sale localmente e poi rientra ai ritorni, mentre la varietà fittizia (F2, F4 rimescolati) ha contenuto informativo alto e piatto.
- **Prerequisito non tecnico:** F7 e le annotazioni indipendenti di F8 restano il collo di bottiglia: senza una verità umana sullo sviluppo nessuna misura si valida. Questo è l'unico punto in cui una misura non può sostituire una persona: serve la sua annotazione, non il suo ascolto del nostro brano.
- **Limite:** queste misure descrivono struttura e prevedibilità; «ricordi, sviluppi, sorprenda e ritorni» come esperienza resta all'ascolto cumulativo (D-M2-30).

## 7. Artefatti udibili (click, crackle)

**Evidenza nel repository.** `evaluation_pack.cpp` dichiara `artifact_present` per click, clipping e dropout, ma per i click finisce con `unverifiable`, «no descriptor for click»: esiste solo il descrittore di clipping. `audio_probe` conta NaN, infiniti e denormali (`dspAnomaliesPassed`). Il dossier R-M25-8 copre i glitch del device (underrun), non gli artefatti nel PCM reso. Le dissolvenze di 2 ms (190 tagli di rilascio di Corpo, attacco del ducking) sono sospettate dal titolare (ADR-0007 §8.1, H-03) e il loro effetto udibile non è verificato.

**Fonti.**
- Godsill e Rayner 1998, *Digital Audio Restoration* (Springer) (NV: la pagina dell'editore richiedeva l'accesso). Il metodo classico rileva i click come picchi del residuo di un predittore lineare autoregressivo.
- Chinen et al. 2020, «ViSQOL v3: An open source production ready objective speech and audio metric», QoMEX 2020, [arXiv:2004.09584](https://arxiv.org/abs/2004.09584) (AM). Metrica di qualità con riferimento, pensata per codec.

**Strumenti.** [ViSQOL](https://github.com/google/visqol), Apache-2.0, C++ con Bazel, build Windows sperimentale secondo il README, audio a 48 kHz: misura il degrado rispetto a un riferimento, che qui manca quando si cambia il suono di proposito. Basso valore, alto costo: **rinvio**. Non è emerso uno strumento di terzi dedicato ai click nel PCM che valga la dipendenza.

**Proposta (nessuna dipendenza).**
- **A1:** rilevatore di discontinuità per stem: residuo di predizione lineare (ordine e finestra fissati prima), normalizzato per una stima robusta locale del residuo, più l'energia sopra 8 kHz in finestre di 1 ms; ogni candidato è annotato con l'evento più vicino dell'event log (NoteOn, NoteOff, furto, dissolvenza di 2 ms, ducking). Il risultato è una tabella «candidati per tipo di evento», non un booleano.
- **A2 (A/B):** null test differenziale già previsto da H-03, con A1 applicato al residuo: dice se un cambiamento ha aggiunto o tolto discontinuità.
- **Controlli:** positivo, salti a gradino e impulsi di ampiezza nota (per esempio −60, −40, −20 dBFS) iniettati in posizioni casuali dichiarate nel PCM della fixture, con curva di richiamo per ampiezza; negativo, sinusoide pura con rampe lunghe e attacchi del kick (transitori legittimi, che non devono essere contati come click); invarianza ai blocchi.
- **Uscita:** riempie il descrittore mancante di `artifact_present(click)`, che oggi termina con `unverifiable`.
- **Limite:** la soglia di udibilità di un click dipende da livello e mascheramento: A1 trova discontinuità, B1 dice se cadono in una zona mascherata. L'udibilità resta un'inferenza.

---

## Proposta prioritizzata

Ordine per rapporto fra valore per D-M2-30 e costo, con le dipendenze in fondo.

| Priorità | Pacchetto | Lacuna | Perché ora | Dipendenze | Dove |
|---|---|---|---|---|---|
| 1 | O1 + O2, onset etichettati e offset vincolato | 5 | Sblocca `acao02_v1`, oggi `INCONCLUSIVE`; difetto già documentato | Nessuna | `engine/cacp/`, pre-registrazione per M2.5.5 |
| 2 | A1 + A2, discontinuità | 7 | Riempie un descrittore che oggi è `unverifiable`; serve a H-03 prima di CP-4 | Nessuna | `engine/cacp/` |
| 3 | S-B1, ILD/ITD/IACC per banda | 2 | Rende autonoma la verifica di F2 oltre la banda larga | Nessuna (gammatone nativo) | `engine/cacp/` |
| 4 | B1 + B2, loudness specifica e perdita per mascheramento | 1, 4 | Chiude R-M25-1 per le variazioni di livello; base di S-B1 e T1 | Nessuna per il nativo; norma ISO 532-1 da consultare | `engine/cacp/` |
| 5 | T1, descrittori timbrici | 4 | Prepara la domanda Trama–Corpo per l'ascolto cumulativo | Nessuna | `engine/cacp/` |
| 6 | Oracoli Python | 1, 3, 4, 5 | Validano B1, R1, T1, O1 contro implementazioni indipendenti | venv **locale nel repository**, versioni fissate con hash: numpy, scipy, MoSQITo, librosa, mir_eval | `docs/research/experiments/`, fuori dalla build e dalla CI |
| 7 | R1, rugosità sul PCM | 3 | Richiede l'oracolo per essere credibile | MoSQITo (via 6), poi port nativo | offline, poi `engine/cacp/` |
| 8 | D1 + D2, poi D3 | 6 | Utile per CP-3; bloccato da F7 e dalle annotazioni di F8 | D3: IDyOM con SBCL | offline |
| — | AMT con Octave | 1, 2 | Oracolo binaurale più autorevole; utile ma non indispensabile | Octave (installazione), AMT (GPL-3) | offline |
| Escluso | Timbre Toolbox, modelli madmom, Essentia, ViSQOL | — | Licenze non commerciali o AGPL, piattaforma, poca pertinenza | — | — |

I pacchetti 1–5 non toccano il runtime: sono codice CACP offline, con test, costruiti dalla build esistente senza nuove dipendenze. Ognuno va con una propria pre-registrazione con hash in `progress.md` e i controlli della regola comune.

## Cosa va approvato dal titolare

1. **Se e in che ordine** avviare i pacchetti 1–5 (nessuna dipendenza nuova, ma codice e test nuovi in `engine/cacp/`). Proposta: 1 e 2 subito, perché correggono difetti già registrati; 3 dopo l'esito di F2.
2. **Ambiente Python locale fissato** (pacchetto 6): numpy, scipy, MoSQITo, librosa, mir_eval in un venv dentro il repository o nella cartella degli esperimenti, ignorato da git, con versioni e hash registrati in `docs/dependencies.md` (AGENTS §2). Oggi numpy è già censito come strumento offline **non fissato**: la stessa decisione lo fisserebbe. Serve l'approvazione esplicita del download dei pacchetti.
3. **Octave e AMT** (facoltativo): installazione di software, quindi approvazione esplicita; licenza GPL-3 per uno strumento offline non distribuito.
4. **IDyOM con SBCL** (facoltativo, per CP-3): stesse condizioni; GPL-3.
5. **Norma ISO 532-1:2017**: per riprodurre i valori di riferimento serve il testo, a pagamento. In alternativa ci si affida all'oracolo MoSQITo, con il limite che la sua validazione non è documentata.
6. **Controllo umano F7 e annotazioni di F8** (lacuna 6): non un ascolto, ma un lavoro di annotazione da affidare a una persona, quando il titolare lo ritiene opportuno.
7. **Nessuna misura di questo dossier diventa un gate** senza una soglia ratificata e versioni fissate (`docs/dependencies.md`, ultima riga della sezione «Strumenti offline di ricerca»).

## Limiti del dossier

- Nessuna fonte di letteratura è stata letta per intero in questa sessione: sono AM o SEC. Le affermazioni quantitative delle fonti (soglie, valori di riferimento) sono volutamente assenti o marcate NV; vanno lette nelle fonti prima di ogni pre-registrazione.
- Le pagine degli strumenti sono state lette, il codice no. Le capacità dichiarate (per esempio «Validated» in MoSQITo) sono dichiarazioni dei progetti, non verifiche nostre.
- Nessuna versione o tag è stato fissato: le pagine non li mostravano.
- Tutte le proposte sono inferenze. L'agente non ha ascoltato l'audio.

## Appendice A. Registro delle fonti

| Fonte | Identificativo | Livello | Note |
|---|---|---|---|
| ISO 226:2023 | catalogo DIN/iTeh | AM | Norma non letta |
| Suzuki e Takeshima 2004 | 10.1121/1.1763601 | AM | Metadati dall'editore |
| ISO 532-1:2017, ISO 532-2:2017 | — | NV | |
| Glasberg e Moore 2005 | JAES 53(10) 906–918, AES 13391 | AM | |
| Glasberg e Moore 2002, loudness tempo-variante | JAES 50(5) | SEC | Citata da altre fonti, non verificata |
| Moore et al. 2016 | 10.1177/2331216516682698 | AM | DOI non risolto (403); metadati da PMC e REF |
| Hafezi e Reiss 2015 | 10.17743/jaes.2015.0021 | AM | |
| Breebaart et al. 2001 | 10.1121/1.1383297 | AM | |
| Dietz et al. 2011 | 10.1016/j.specom.2010.05.006 | AM (titolo), SEC (DOI) | Il DOI `…2010.11.004` non è stato risolto |
| Hidaka et al. 1995 | JASA 97(5) 3319; 10.1121/1.412847 | AM, SEC (DOI) | |
| Majdak et al. 2022 (AMT) | 10.1051/aacus/2022011 | AM | |
| Sethares 1993 | JASA 94(3) 1218–1228; 10.1121/1.408175 | AM, SEC (DOI) | `10.1121/1.408170` è un altro articolo |
| Daniel e Weber 1997 | Acustica 83, 113–123 | AM | |
| ECMA-418-2 | — | NV | |
| Wanty, Glesser et al., INTER-NOISE 2024 | Ingenta, incecp 270(6) art. 24 | AM | Autori dal titolo della scheda, non verificati |
| Peeters et al. 2011 | 10.1121/1.3642604 | AM | |
| McAdams et al. 1995 | — | NV | |
| Bello et al. 2005 | 10.1109/TSA.2005.851998 | AM | |
| Pearce 2018 | 10.1111/nyas.13654 | AM | |
| Foote 2000 | ICME 2000; 10.1109/ICME.2000.869637 | AM, SEC (DOI) | |
| Godsill e Rayner 1998 | Springer | NV | Pagina dell'editore dietro login |
| Chinen et al. 2020 (ViSQOL v3) | arXiv:2004.09584 | AM | |

## Appendice B. Strumenti verificati (pagine dei repository, 2026-11-16)

| Strumento | Licenza letta | Linguaggio e requisiti letti | Esito |
|---|---|---|---|
| MoSQITo | Apache-2.0 | Python; numpy, scipy, pyuff | Oracolo candidato |
| libebur128 | MIT (README e registri dei pacchetti, da ricerca) | C | Solo oracolo di BS.1770 |
| AMT | GPL-3 nucleo, altre per i modelli | MATLAB/Octave | Facoltativo |
| librosa | ISC | Python | Oracolo candidato |
| mir_eval | MIT | Python; numpy, scipy | Oracolo candidato |
| Timbre Toolbox (fork Perreault) | Licenza di ricerca non commerciale | MATLAB + MEX | Escluso |
| ViSQOL | Apache-2.0 | C++/Bazel; Windows sperimentale | Rinviato |
| IDyOM | GPL-3.0 | Common Lisp; SBCL, Quicklisp, SQLite | Facoltativo, CP-3 |
| madmom | BSD il codice, CC BY-NC-SA 4.0 i modelli | Python | Escluso per i modelli |
| music21 | BSD-3-Clause | Python 3.12+ | Facoltativo |
| Essentia | AGPL-3.0 | C++ con Python; pip per Linux | Escluso |

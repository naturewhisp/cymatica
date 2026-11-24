# Proposta D-M2-53 — Destinazione delle voci sospese di M2.5

- **Stato:** **approvata** dal titolare il 2026-11-24, con quattro correzioni applicate qui (§2 saturazione, condizione di riapertura e R-M25-3; §3 F2b). Registro: ADR-0007 §11, D-M2-53.
- **Origine:** revisione di Gemini fornita dal titolare il 2026-11-24. Usata solo come segnalazione: ogni voce è stata ricontrollata su ADR-0007, `progress.md` e risultati. Gemini non conta come ascoltatore.
- **Regole:** D-M2-18 (ogni rinvio ha motivo, scadenza e milestone), AGENTS §5.3, D-M2-19, D-M2-52 (difetto, mestiere, gusto).

## 1. Perché sono sospese

- **LFO, saturazione, supersaw:** D-M2-26 e D-M2-29 le adottano come A/B in M2.5.2. D-M2-32 ha poi indicato cosa restava prima di chiudere la fase (F4b e livello della Trama) senza citarle. M2.5.2 è chiusa dal 2026-11-17 e non risultano A/B eseguiti. Il registro le dà ancora per adottate.
- **Modulazioni di altri parametri:** dipendono dall'esito dell'LFO, che non esiste.
- **F2b:** citato come seguito possibile, senza scadenza.
- **R-M25-3, riverbero, chorus, flanger, delay:** scadenza «dopo CP-1», superata e senza milestone.

## 2. Proposta

| Voce | Classe (D-M2-52) | Destinazione proposta | Motivo |
|---|---|---|---|
| LFO sul filtro | Gusto o mestiere, non difetto | **Rinviata a M7**, con R-M25-9, decisa prima del gate | Cambia il timbro mentre il materiale di CP-3 è in prova: due variabili insieme (D-M2-19). Costi misurati: conservati |
| Saturazione | Mestiere | **Rinviata a M7**, con R-M25-9; riferimenti: specifica §16.8, DESIGN §12.3 e §16.4 | È uno strumento del timbro e della dissonanza, legato agli archetipi. La «saturazione» fra i deliverable M7 della specifica §32 è quella della piastra (§17.7), un'altra cosa. Il margine di 0,3 dB è del candidato I3 bis (−3 LU), **non adottato**, e riguarda solo una saturazione sul master o sulle basse, non sul synth |
| Supersaw | Gusto | **Rinviata a M7**, con R-M25-9 | Agirebbe sulla melodia mentre si giudica la melodia |
| Modulazioni di altri parametri | Gusto | **Rinviata a M7**, dopo l'LFO | Dipendenza |
| Riverbero, chorus, flanger, delay (R-M25-3) | Gusto | **Rinviata a M7**, milestone responsabile della ricerca. **Vincolo D-M2-25 invariato:** misura nel profilo D-M2-10 prima dell'integrazione e azzeramento dei denormali nelle strutture a retroazione | Nessun ascoltatore li indica come difetto da correggere; delay e riverbero limitati sono già in specifica §16.8 |
| F2b, delay simmetrico della Trama | Difetto segnalato, trattato da F2 | **Chiusa senza intervento, con condizione di riapertura** | Vedi §3 |

**Condizione di riapertura anticipata per le voci M7:** nel secondo ascolto di CP-3 un ascoltatore segnala la voce, nelle osservazioni libere o nelle descrizioni, come *problema* e non come gusto. Vale **indipendentemente dai controlli B0**: in P-M25-03 §8 il loro fallimento indebolisce i confronti, non le osservazioni libere. La si riesamina a CP-4, prima del congelamento, classificata secondo D-M2-52; il riesame non implica l'adozione.

## 3. F2b

- **Misure, due grandezze diverse:**
  - banda larga sull'intero stem (F2, S3): ILD **−1,47 dB** (era +4,55);
  - per banda (pacchetto 3): primo fronte centrato (ILD −0,001 dB); nota intera **−1,33 dB** verso destra, IACC 0,888, **dispersione fra le bande 2,72 dB**, da +1,8 dB a 100 Hz a **−3,0 dB a 530 Hz**. Lo spostamento quindi non è uniforme, e la media lo attenua.
- **Perché la domanda 3 («centrato o spostato?») non ha una risposta:** D-M2-50 ha lasciato la voce 3 alle misure nell'ascolto condensato. Non è un'omissione degli ascoltatori.
- **Ascolti dopo F2:** il 2026-11-19 l'esperto descrive «un buon gioco di panning» e il 2026-11-23 un «creativo uso del panning»: evidenza spontanea positiva. Non nomina però la Trama, quindi non le è attribuibile con certezza. Nei resoconti registrati non ho trovato nuovi rilievi di lateralizzazione. È un indizio favorevole, non una prova.
- **Riapertura, entro CP-4:** solo per un **rilievo spontaneo** di suono spostato o fastidioso in cuffia, in qualunque ascolto. Non si aggiunge una domanda dedicata, e il secondo pacchetto già preparato non la contiene. Allora pre-registrazione con la misura per banda.
- **Alternativa scartata:** eseguire F2b prima di CP-4. Cambia il PCM e la larghezza della Trama senza un difetto riferito dopo F2.

## 4. Voci che Gemini segnala ma che hanno già una destinazione

| Voce | Stato |
|---|---|
| R-M25-9, obiettivo artistico | CP-3: resta una decisione del titolare, da prendere con le adozioni |
| H-03, H-04, H-05, R-M25-5, R-M25-6 | CP-4 o prima |
| AC-AO-02 | CP-5 |
| R-M25-10 | Prima di M2.5.6 |
| F7, controllo positivo umano | Materiale d'autore dell'esperto, non bloccante (M2.5.1c-F7) |

## 5. Alternative scartate

- **Eseguire i tre A/B ora:** il perimetro si allarga prima di decidere le adozioni di CP-3.
- **Scartarle:** i costi sono misurati e l'esperto le ha proposte; manca solo il momento giusto.
- **Lasciarle aperte:** contrario a D-M2-18.

## 6. Cosa cambia

ADR-0007: riga D-M2-53; §5 (LFO, saturazione, supersaw); §7 (R-M25-3); §10 e §11 (note di emendamento a D-M2-26, D-M2-29 e D-M2-34); §8.1 (LFO, saturazione, supersaw, modulazioni, riverberi, chorus e delay, delay della Trama). D-M2-26 e D-M2-29 risultano emendate solo per queste voci; D-M2-25 resta invariata. Nessun codice, golden o criterio di accettazione cambia. La riassegnazione delle voci libere resta fuori da M2.5 (ADR dedicato).

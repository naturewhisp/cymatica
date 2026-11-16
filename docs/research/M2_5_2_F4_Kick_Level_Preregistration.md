# M2.5.2 — F4, livello del kick: pre-registrazione

- **Stato:** pre-registrazione, scritta il 2026-11-16 prima del codice e di ogni misura. L'hash SHA-256 (LF) di questo file è registrato in `progress.md` prima della prima corsa. Dopo l'hash il file non si modifica: correzioni e predizioni mancate vanno nel file dei risultati.
- **Autorizzazione:** il titolare ha autorizzato M2.5.2 il 2026-11-16 («Sì, avvia F4»), secondo ADR-0007 §6 e D-M2-26.
- **Fonti:** ADR-0007 D-M2-19 (metodo), D-M2-26 (perimetro), §8.1 (suggerimenti sul kick); [loudness per stem](../m2_procedural_evidence.md) di M2.5.1c; [ascolto del 2026-11-03](../m2_4_human_listening_2026-11-03.md) (kick 7–9 dB sotto, stima dell'esperto); [protocollo P-M25-03](../m2_5_listening_protocol_P-M25-03.md).

## 1. Oggetto e variabile

F4 in ADR-0007 è «livello del kick, poi bilanciamento generale». Questo passo isola **una sola variabile: il guadagno di bus di Pulso** (oggi `RoleBusProcessor::kPulsoGain`, −6,0 dB). Tutto il resto resta com'è: timbro, durata e altezza del kick, piano, eventi, altri guadagni.

- **Fuori da questo passo, dichiarato:** durata e peso nelle basse del kick (suggerimento §8.1, «più peso nelle basse, durata oltre i 54–62 ms»). È un A/B di timbro successivo, a loudness allineata, con la sua pre-registrazione. Così si sa se il risultato dipende dal livello o dal timbro.
- **Ducking:** il ducking di Corpo usa come riferimento `kDuckingFullScaleLevel = 0,25 · kPulsoGain`. Nell'esperimento il riferimento segue il guadagno effettivo di Pulso, così profondità e forma del ducking restano le stesse e cambia solo il livello del kick. Lasciarlo fisso cambierebbe due variabili insieme.
- **Produzione invariata:** le varianti si ottengono con un override solo offline, come per l'esperimento di release 5e-08 (`setNoteReleaseFrames`): rifiutato con un device inizializzato o avviato. Il valore di produzione cambia solo dopo l'ascolto (ADR-0007 §5: «Valori di F2 e F4», review di M2.5.2, limite CP-2), con il golden PCM aggiornato e motivato in quel momento.

## 2. Varianti

Scala di offset sul guadagno di Pulso: **0, +3, +6, +9, +12 dB**. La stima dell'esperto (7–9 dB) cade dentro la scala, non al bordo; +12 dB serve a non avere l'optimum sull'ultimo gradino.

Materiale: `fixture_probe_v1`, seed 1337, resa offline completa, due ripetizioni per determinismo.

## 3. Ipotesi e predizioni (prima di misurare)

Valori di partenza da M2.5.1c: mix e bus −21,25 LUFS; Pulso −34,49 LUFS (−13,23 LU rispetto al bus); true peak −10,45 dBTP; riduzione del limiter 0 dB.

| ID | Predizione | Regola d'esito |
|---|---|---|
| **P1** | Con offset 0 non c'è override: PCM finale uguale al golden 15126969907236845529 | hash diverso → `INVALID` dell'intero esperimento |
| **P2** | In ogni variante gli eventi applicati sono gli stessi dell'offset 0 (hash di identità, frame e disposizione) | eventi diversi → `INVALID` |
| **P3, null test** | Gli stem di Corpo, Trama e Vettore sono uguali a quelli dell'offset 0 con scarto massimo assoluto ≤ 1·10⁻⁶; lo stem di Pulso è uguale a k·Pulso₀ (k = 10^(g/20)) con scarto massimo ≤ 1·10⁻⁶; il bus mix prima del master è uguale a bus₀ + (k − 1)·Pulso₀ con scarto ≤ 2·10⁻⁶ | scarto oltre soglia → `NOT_SUPPORTED`, con il ruolo e il campione peggiori |
| **P4** | Loudness integrata dello stem di Pulso: +g LU ± 0,05 rispetto all'offset 0 | fuori tolleranza → mancata |
| **P5** | Loudness integrata del mix finale, aumento rispetto all'offset 0, con somma di potenze di segnali non correlati (frazione di Pulso p = 0,0474): **+0,20 / +0,57 / +1,24 / +2,32 LU** per +3 / +6 / +9 / +12 dB, tolleranza ± 0,3 LU | fuori tolleranza → mancata; la correlazione fra kick e Corpo nelle basse è la causa sospettata, non verificata |
| **P6** | Riduzione del limiter 0 dB per +3, +6, +9. Per +12 nessuna predizione | riduzione > 0 a ≤ +9 → mancata; una variante con riduzione > 0 è marcata «confusa dal limiter» e il suo P3 si legge solo prima del master |
| **P7** | Due ripetizioni identiche (misure e hash) | diverso → `INCONCLUSIVE` |

Si riportano anche, senza predizione: true peak e loudness per sezione (A, B, A'), loudness per stem rispetto al bus e rispetto a Corpo, ducking minimo.

## 4. Clip per l'ascolto (CP-2)

- **Estratti:** uno per sezione, A, B e A', di **8 battute** (frasi 3 e 4 della sezione, battute 9–16), lontani dalle transizioni; dissolvenza di 10 ms. Tre punti diversi del brano, come chiede il §2bis del protocollo.
- **Due serie:**
  - **grezza**, al livello della resa;
  - **allineata**, con un guadagno scalare per clip che porta la loudness integrata della clip a quella della clip con offset 0.

  Il livello del kick resta la variabile sperimentale in entrambe. L'allineamento toglie soltanto la preferenza per il volume complessivo più alto. Per la preferenza fa fede la serie allineata, la grezza resta come riferimento.
- **Controlli B0:** il pacchetto d'ascolto contiene una coppia identica e un duplicato nascosto, come da protocollo. Ordine e chiave li prepara il titolare.

## 5. Ipotesi d'ascolto (per CP-2, non verificate dall'agente)

- **L1:** dalla stima dell'esperto, la variante preferita sulla serie allineata è +6 o +9 dB.
- **L2:** a +3 dB la differenza dall'offset 0 si rileva (ABX, P-M25-03 §4) almeno sugli estratti di A e A', dove il kick è più presente.
- **Regola di decisione:** il valore si adotta a CP-2 dall'esito dell'ascolto, con rilevazione e preferenza separate. Se la preferita è +12 dB, cioè il bordo, la scala si estende prima di adottare un valore. Se nessuna variante è preferita con rilevazione confermata, il valore resta −6 dB e la voce si chiude con questo esito.
- **Interazione con F1:** il kick sul tempo forte cade solo in 16 battute su 96. Alzarlo rende più evidente il suo spostamento (ADR-0007 §5, F1 da rivalutare con F4). Q3 si chiede per sezione, e l'esito di F1 a CP-2 tiene conto del livello scelto qui.

## 6. Limiti

- Le predizioni P4–P6 riguardano il segnale, non il bilanciamento percepito. Quello lo decide l'ascolto, e le curve isofoniche e il rumore rosa restano a R-M25-1.
- Una sola fixture, un solo seed.

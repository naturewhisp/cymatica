# Pacchetto di misura 2 — Rilevatore di discontinuità: risultati

- **Pre-registrazione:** [M2_5_Pkg2_Discontinuity_Detector_Preregistration.md](M2_5_Pkg2_Discontinuity_Detector_Preregistration.md), SHA-256 (LF) `92b1ac5203cab7cb0996d23e8a7bd243fdfc56bb1f7eaac2490c00369dd0c7ad`.
- **Esecuzione, 2026-11-17:** Release, scenario `discontinuity_v1` sulla fixture (golden PCM `18256941985354732249`, PASS). Dati: [esperimento](experiments/m25_pkg2_discontinuity/README.md).
- **Esito: lo strumento A1 non supera la propria validazione.** Lo stato dello scenario è `TOOL_NOT_VALIDATED`, e la tabella sulla fixture non si usa come misura. La funzione differenziale A2 supera la sua prova (W5).

## 1. Validazione

| ID | Atteso | Risultato | Esito |
|---|---|---|---|
| W1, impulsi −60 / −40 / −20 dBFS | Richiamo monotono; 1,0 a −20; ≥ 0,9 a −40 | 0,40 / 0,50 / 0,86 | **Mancata** (monotonia sì, livelli no) |
| W1, gradini di 10 ms | Come sopra | 0,42 / 0,52 / 0,94 | **Mancata** |
| W2, sinusoide pura con rampe | 0 candidati | 2 | **Mancata** |
| W3, stem di Pulso | 0 candidati «inspiegati» | 82 | **Mancata** |
| W4, ripetizione | Tabelle identiche | Identiche | Confermata |
| W5, A2 | 0 su render identici; 1 sull'impulso iniettato | 0; 1 nella posizione esatta | Confermata |

**Correzione d'implementazione prima della misura:** i primi 16 campioni di un buffer non hanno la storia completa per il predittore e davano un residuo grande all'avvio. Il test unitario della sinusoide l'ha mostrato prima della corsa sulla fixture: ora sono esclusi. La pre-registrazione non lo specificava.

## 2. Perché fallisce

- **Forme d'onda con discontinuità vere:** Corpo e Vettore danno decine di migliaia di candidati «inspiegati» (51 157 e 50 857), a intervalli di circa 109 campioni, cioè il periodo di un dente di sega a 440 Hz. Il reset del dente di sega a ogni periodo è una discontinuità legittima della forma d'onda, e un predittore lineare corto (ordine 16, blocchi di 1024 campioni) non la prevede.
  - Nel mix questi candidati coprono i click iniettati: il richiamo di W1 si ferma a 0,5 a −40 dBFS perché i candidati vicini vengono fusi e la scala robusta è dominata dai reset.
- **Kick:** i candidati «inspiegati» di Pulso (82) cadono lontano più di 5 ms dagli eventi applicati. Senza un'analisi dei singoli casi, nell'ascolto non c'è niente da sospettare.
- La regola comune del dossier lo prevede: uno strumento che non supera i controlli non entra in nessuna misura. La tabella resta nel manifest come diagnostica dello strumento.

## 3. Che cosa resta utilizzabile e che cosa propongo

- **A2, differenziale:** funziona dove le forme d'onda sono uguali nei due render, perché i reset del dente di sega si cancellano. È la prova adatta agli A/B, come H-03 sul rilascio. Va però validata anche con iniezioni a più ampiezze sulla differenza, non solo con un impulso: serve un W1 differenziale, da pre-registrare.
- **Rilevatore assoluto, v2, da pre-registrare.** Due strade, entrambe senza dipendenze:
  1. **Strumentazione della verità.** Il dossier insiste che CYMATICA ha la verità di riferimento. Si misura il salto d'ampiezza e di pendenza dell'inviluppo di ogni voce nei punti di furto, taglio di rilascio, dissolvenza di 2 ms e ducking, direttamente nell'allocatore offline, invece di indovinarlo dal segnale. È la misura più diretta per H-03.
  2. **Predittore a lungo termine** (periodo del tono, poi predittore corto) prima della soglia, così i reset periodici vengono previsti. Più costoso, e da validare sugli stessi W1–W5.
- `artifact_present(click)` dell'evaluation pack resta `unverifiable`: non si riempie con uno strumento non validato.

## 4. Codice

`discontinuity.{h,cpp}`: A1, A2, annotazione con l'event log e scenario `discontinuity_v1`, che applica le regole W1–W5 e non dà mai `MEASURED` a uno strumento non validato. Test `[discontinuity]`: sinusoide, impulso, fusione entro 2 ms, A2, annotazione. Release e Debug headless `/WX` 281/281.

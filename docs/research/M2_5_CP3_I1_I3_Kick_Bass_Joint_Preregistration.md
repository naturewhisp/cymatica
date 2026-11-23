# M2.5 CP-3 — I1–I3, kick e basso insieme: pre-registrazione

- **Stato:** scritto il 2026-11-23, **prima** di codice e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Perché:** il [piano](../M2_5_CP3_Expert_Plan.md) chiede di rifare insieme kick e basso (stessa banda), dopo [I1–I2](M2_5_CP3_I1_I2_Kick_Results.md) e il [prototipo del basso](M2_5_CP3_I3_Bass_Prototype_Results.md). E V2 ha più basse di V1: per un ascolto equo vanno pareggiate.
- **Materiale:** render esistenti di `bass_prototype_v1` (seed 1337; kick «now» e «c2» = Do2 con click +3 dB; V0, V1, linea V2 S0). Nessun nuovo render; nulla adottato.

## Versioni

- **V0** senza basso, **V1** Corpo un'ottava sotto, **V2m** = V2 con un guadagno per sezione tale che l'energia **non di kick** in 30–120 Hz sia pari a quella di V1 (kick, Trama e Vettore uguali; per V2m Corpo di V0 più il basso).
- Con entrambi i kick; il candidato è **c2**.

## Misure e criteri (per c2, V1 e V2m, in A, B e A′)

| ID | Misura | Criterio | Previsione |
|---|---|---|---|
| J1 | Somma degli stem di V0 contro mix | ≤ 1·10⁻⁴ | vero |
| J2 | V2m contro V1, energia non di kick in 30–120 Hz | entro 0,5 dB per sezione | vero per costruzione |
| J3 | P_body (40–160 Hz, 20–120 ms dopo l'attacco, kick contro tutto il resto, basso compreso, mediana) | ≥ 6 dB | 8–15 dB |
| J4 | R sui **soli kick coincidenti** con un attacco di basso (entro ±60 ms; energia kick contro basso nei 60 ms, mediana) | ≥ 6 dB | ≥ 6 dB, più basso della mediana su tutti i kick |
| J5 | P_click (2–5 kHz, 0–15 ms) | ≥ 0 dB | cala al massimo 0,5 dB da V0 |
| J6 | Picco della somma ≤ −1 dBFS; loudness del mix V1 e V2m entro 0,5 LU fra loro | vero | vero |
| J7 | E_K del solo kick fra A e B ≤ 0,1 dB; Q100 ≥ 0,75 in ogni sezione | come I1–I2 | vero (0,02; 0,754) |

## Regola

- **Kick candidato e basso compatibili** se J1–J7 valgono per c2. Se J3 o J4 falliscono solo per V2m, resta V1 fra i candidati, e V2 si ripresenta con un livello più basso in una nuova pre-registrazione.
- **Nessuna adozione.** Il pacchetto per l'esperto userà V2m; la preferenza fra V1 e V2m spetta all'ascolto.

## Limiti

- V2 è sommata dopo il render: niente ducking né furti di voce. La linea è scritta per il piano di oggi, non per il kick della versione c di I4+I6, che aggiunge e toglie attacchi: lì la coincidenza andrà ricalcolata.
- Soglie 6 dB e 0 dB come in I1–I3: ragionate, non validate. Un brano, un seed.

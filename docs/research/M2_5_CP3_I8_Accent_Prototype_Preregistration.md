# M2.5 CP-3 — I8, prototipo degli accenti comuni. Pre-registrazione

- **Stato:** scritto il 2026-11-24, **prima** di codice, render e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Scelta d'autore** sulla parola dell'esperto: l'[indagine](M2_5_CP3_I8_Accent_Investigation_Results.md) non soddisfa la sua regola (2 su 4). Solo offline, seed 1337, piano di oggi, kick Do2 con click +3 dB; nulla adottato. Percussioni escluse.

## Versioni

| Versione | Kick | Basso |
|---|---|---|
| **g0** | ritmo di oggi | linea V2 di I3 |
| **g1** | quarti più accenti | accenti della frase |
| **g2** | ritmo di oggi | accenti presi dal kick di oggi |

**Accenti, g1:** ogni frase ha una maschera fra {0,8}, {0,8,11}, {0,6,8}, {0,8,14}: quella con più posizioni nel kick di oggi della prima battuta in cui suona (quota sulla lunghezza; a parità la più lunga, poi la prima). Il kick di ogni battuta in cui oggi suona è maschera ∪ {0,4,8,12}; accenti a velocity piena, altri ×0,8. **Eccezione:** le battute con lo spostamento voluto di F1 (kick di oggi 2,6,10,14) restano come oggi, con accenti {2,10}.
**Accenti, g2:** le tre posizioni metricamente più forti del kick di oggi nella battuta (a parità la prima).
Dove Pulso tace, accenti {0,8}.

**Basso, g1 e g2:** un attacco per accento. Nota: il primo accento è la fondamentale; l'accento 8 (o 10) è la fondamentale nelle battute dispari della frase (1 e 3) e la quinta nelle pari; gli altri l'ottava. **Complesso contro semplice:** se il kick della battuta ha al massimo 4 attacchi si aggiunge una nota sul sedicesimo 14, se libero: la nota della scala sotto la fondamentale seguente. Durata 0,8 dello spazio fino al prossimo attacco, al massimo mezza battuta. Registro, silenzio e pedale come V2. Timbro e velocity come V2.

Livello: energia non di kick in 30–120 Hz pari a V1 per sezione, come V2m, per ogni versione.

## Criteri

| ID | Criterio |
|---|---|
| B1 | Kick con basso oltre il caso (A1): ≥ 15 punti in g1 e g2 (g0 4) |
| B2 | Sincope per attacco: kick di g1 ≤ 0,50 (oggi 0,87); basso di g1 e g2 ≤ 0,60 (oggi 0,77) |
| B3 | P_body ≥ 6 dB e R sui kick coincidenti ≥ 6 dB in ogni sezione (come J3 e J4); picco ≤ −1 dBFS |
| B4 | Ruoli 1, 2, 3 identici a oggi; kick presente nelle stesse battute |
| B5 | Finestre di 4 battute del basso diverse dalla precedente ≥ 50 % in g1 e g2 |
| B6 | Attacchi di basso a un semitono da una nota che suona: al massimo 5 punti sopra g0 |
| B7 | Render MEASURED, limiter 0 dB, nessuno scarto; estratti entro 0,1 LU dopo l'allineamento |

Se B4 o B7 falliscono non si consegna. Gli altri si riportano senza ritocchi. B1 e B2 sono in buona parte **veri per costruzione**.

## Ascolto

Quattro estratti ciechi di 24 s (A, battute 16–27): g0, g1, g2 e **un duplicato nascosto** di uno dei tre, scelto a caso. Domande: kick e basso sono legati (1–7); il ritmo è squadrato o sincopato; il basso è noioso, giusto o frenetico; preferito.

**Previsioni:** (a) g1 e g2 più legati di g0; (b) g1 meno sincopato di g0 e g2. Nessuna sul preferito: il rischio è un basso monotono.
**Regola:** se g1 e g2 non sono più legati di g0, la leva non si propone. Il duplicato si riporta sempre; con uno scarto di 2 punti o più fra i due, i giudizi valgono come indicazione debole.
**Limiti:** un ascoltatore, un seed, una sezione; la regola dell'esperto non è dimostrata dal suo brano; nessun rullante.

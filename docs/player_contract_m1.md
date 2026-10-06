# CYMATICA — Contratto minimo del giocatore per il validatore (M1)

Documento contrattuale richiesto da `CYMATICA_Specifica_Agentica_Sviluppo.md` §32 (Milestone 1) e `AGENTS.md` §1 per riconciliare `DESIGN.md` §6.2.1 e §17.2.
Ultimo aggiornamento: 2026-10-06.

---

## 1. Scopo

Il validatore di raggiungibilità e fairness (Milestone 4) e la prima generazione di pattern giocabili (Milestone 3) richiedono un profilo cinematico e geometrico congelato del giocatore (**Seme**).
Questo documento fissa tali parametri in M1 come **baseline contrattuale per la validazione**. L'implementazione completa del movimento a gamepad/tastiera appartiene a M3, e la rifinitura del game feel a M5; tuttavia, M4 validerà i pattern rispetto a questo profilo congelato.

---

## 2. Geometria e Hitbox (DESIGN §17.2, §13.0)

- **Forma hitbox:** Cerchio bidimensionale centrato sulla posizione del Seme $(x, y)$.
- **Raggio hitbox autoritativo ($R_{hit}$):** $3.0\text{ px}$ (in coordinate logiche dell'arena $800 \times 800$).
- **Regola percettiva vincolante:** il nucleo visibile del Seme deve coincidere esattamente con l'hitbox. L'hitbox non eccede mai il raggio del nucleo visibile a luminanza 100%.
- **Area di Graze ($R_{graze}$):** Cerchio concentrico con raggio $12.0\text{ px}$ ($4 \times R_{hit}$). Un hazard che interseca $R_{graze}$ senza toccare $R_{hit}$ genera evento di Graze autoritativo.

---

## 3. Cinematica del movimento continuo (DESIGN §6.1)

- **Frequenza di aggiornamento autoritativa:** 120 Hz ($dt = 1/120\text{ s} \approx 8.333\text{ ms}$).
- **Velocità lineare massima ($V_{max}$):** $240.0\text{ px/s}$ ($2.0\text{ px/tick}$).
- **Spazio di accelerazione / frenata:** per il modello del validatore M4, il Seme è considerato a risposta lineare immediata o con inerzia trascurabile entro 2 tick ($16.6\text{ ms}$).
- **Bound dell'arena:** coordinate interne logiche comprese in $[-380.0, +380.0]$ su entrambi gli assi, con collisione perimetrale rigida al bordo della Piastra.

---

## 4. Quantum Dash (DESIGN §6.2, §6.2.1, §15.5)

- **Modello cinematico:** Shift di fase quasi-istantaneo (distanza fissa lungo la direzione di input o direzione corrente).
- **Distanza di dash ($D_{dash}$):** $96.0\text{ px}$ ($48\text{ tick}$ di moto lineare nominale).
- **Durata del dash ($T_{dash}$):** $6\text{ tick}$ ($50\text{ ms}$ a 120 Hz).
- **Invulnerabilità durante il dash ($T_{invuln}$):** attiva per l'intera durata del dash ($6\text{ tick}$).
- **Cooldown autoritativo ($C_{dash}$):** $60\text{ tick}$ ($0.5\text{ s}$ a 120 Hz). Durante il cooldown un nuovo comando di dash è rifiutato.
- **Risoluzione rispetto al beat (Open Question 3):** il dash viene eseguito immediatamente al tick di ricezione dell'input; la quantizzazione ritmica interviene sul punteggio di risonanza/coerenza, non ritardando la collisione o la risposta di emergenza.

---

## 5. Riconciliazione e stabilità (Spec §32, AGENTS §1)

Questi parametri costituiscono il profilo `PlayerProfile_v1`. Eventuali modifiche durante la rifinitura di M5 dovranno essere retro-compatibili o accompagnate da riesecuzione completa dei test del validatore M4 per escludere regressioni di fairness.

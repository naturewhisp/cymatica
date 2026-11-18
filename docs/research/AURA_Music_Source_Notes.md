# Spunti musicali da AURA: fonti per CYMATICA

- **Stato:** note di lettura del 2026-11-17, su indicazione del titolare: «annota tutto quello che può essere utile da quello che hai visto in aura». Sono proposte, non decisioni: ogni voce ha una destinazione, e si adotta solo con una decisione registrata.
- **Fonte:** repository AURA del titolare, file `app/lib/src/audio/sound_generator.dart`, letto al commit `c68aaf3`. I commit del file sono del titolare. Non ci sono licenze di terzi: è materiale riusabile, con la provenienza dichiarata qui.
- **Natura della fonte:** la sintesi è procedurale, cioè PCM a 22 050 Hz calcolato in Dart. La musica invece è scritta: un solo tema, riusato in 4 brani. Il seed non entra nella musica; il generatore casuale serve solo al rumore delle percussioni.
- **Intento, dal titolare (2026-11-17):** AURA non è un gioco musicale. La musica sottolinea le fasi di gioco: è la stessa musica, che cambia in modo dinamico con la fase.

## 1. Cosa contiene

| Elemento | Contenuto | Note |
|---|---|---|
| Scala | Do minore naturale, gradi `0 2 3 5 7 8 10`; il Si♭ diventa Si naturale sull'accordo di Sol | Minore armonico solo sul V |
| Giro armonico | Do m – La♭ – Fa m – Sol – Do m – Mi♭ – Fa m – Sol (i–VI–iv–V–i–III–iv–V), un accordo ogni 4 s in `bgm_main` | 8 accordi, 32 s |
| Tema | 64 passi, 8 per accordo, pausa = −1. Cella su ogni accordo: `x · x x+2 x+4 · x+3 x+2` con x = fondamentale dell'accordo (in gradi della scala); l'ultima cella è cadenzale: `15 · 15 13 11 · 13 7` | Progressione melodica: la stessa figura sale con l'armonia |
| Stesso tema, 4 stati | `bgm_main`: 0,5 s per passo. `bgm_ambient`: 1,0 s per passo, con pad, campana ed eco. `bgm_tense`: 0,25 s per passo, con arpeggi veloci e rumore. `bgm_epic`: basso in ottave e arpeggio a schema | Identità del tema attraverso gli stati di gioco |
| Arpeggi | 3 note dell'accordo e salto d'ottava sul 4° passo (`main`); schema `0 1 2 1 2 0 1 2` (`epic`); 8 note al secondo con salto d'ottava ogni 4 (`tense`) | Schemi d'arpeggio dichiarati |
| Basso | Drone triangolare sulla fondamentale (`ambient`); ottave alternate fondamentale/ottava a ottavi (`epic`) | Linea di basso |
| Campana | Parziali 1 : 1,5 : 2 : 2,63, una nota ogni 8 s (`ambient`) | Timbro d'atmosfera inarmonico |
| Eco | Melodia ripetuta dopo 0,75 s al 45% (`ambient`) | Effetto, non contenuto |

## 2. Cosa può servire a CYMATICA, con destinazione

| # | Spunto | Destinazione | Stato |
|---|---|---|---|
| A1 | **Cella di AURA come motivo alternativo d'apertura**, riscritta nel modo della sezione e in ottavi dentro una battuta: `0 · 0 2 4 · 3 2`. In re dorico è `D · D F A · G F`, in sol eolio `G · G B♭ D · C B♭`. Ha le stesse altezze del motivo attuale in A, con un ritmo con pause e un ordine diverso | Punto 6, tabella chiusa del seed | In uso nel punto 6 (pre-registrazione C6) |
| A2 | **Tema intero come prima melodia lunga una frase:** 16 battute di celle che seguono gli accordi, con cadenza finale | Pacchetto 6b, motivi oltre la battuta | Proposto |
| A3 | **Progressione melodica come operazione di catena:** la figura ripetuta e trasposta sull'accordo successivo | 6b, accanto a ornamento e liquidazione | Proposto |
| A4 | **Stesso tema in stati diversi** (tempo e tessitura cambiano, l'identità resta). È l'intento dichiarato di AURA: la stessa musica che cambia in modo dinamico con la fase di gioco. In CYMATICA la musica guida il gameplay, quindi il principio vale due volte: lo stato di gioco cambia la tessitura, il materiale resta riconoscibile | M3, stati di gioco e direttore adattivo (DESIGN §8.5, §9) | Proposto, priorità alta per M3 |
| A5 | **Giro i–VI–iv–V con la sensibile sul V**, come alternativa armonica d'autore. Nei modi di CYMATICA la sensibile è un'alterazione cromatica da dichiarare (in B il Re maggiore porterebbe un Fa♯) | Tabella armonica, dopo il punto 6 | **Adottato** in 6c (D-M2-49): alternative 1 (diatonica) e 2 (Fa♯ dichiarato) di B; ascolto alla voce 17 |
| A6 | **Schemi d'arpeggio dichiarati** per la Trama (`0 1 2 1 2 0 1 2`, salto d'ottava) | Alternative di Trama, dopo il punto 6 | Proposto |
| A7 | **Basso in ottave a ottavi.** È legato alla richiesta «linea di basso (urgente)» dell'esperto, ADR-0007 §8.1 | F7 e linea di basso | Proposto, solo come spunto ritmico |
| A8 | **Campana d'atmosfera con parziali 1 : 1,5 : 2 : 2,63** e una nota rara | Ruolo d'«opportunità» (R-ROL-8), M3 | Proposto |

**Da non prendere:**
- i timbri a onda quadra e PWM a 8 bit, che sono un'estetica chiptune: CYMATICA ha la sua sintesi, già misurata;
- l'eco a 0,75 s, che è un effetto fuori dal perimetro di M2.5 (CP-4).

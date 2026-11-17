# Pacchetto di misura 4 — Loudness specifica, mascheramento e pendenza spettrale: risultati

- **Pre-registrazione:** [M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md](M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md), SHA-256 (LF) `e1fbcdd80cb499cae39938639b383617ab0534b32f3a5b8f4a9e34df33c9c89d`.
- **Esecuzione, 2026-11-17:** Release `stem_export_v1` (golden PCM PASS), poi `loudness_masking.py` con MoSQITo 1.2.1. Dati: [esperimento](experiments/m25_pkg4_loudness/README.md).
- **Esito:** strumento **validato** (L1–L5). M1 e M2 confermate in B e A′ e non misurabili in A; **M3 mancata**.

## 1. Validazione

| ID | Atteso | Risultato | Esito |
|---|---|---|---|
| L1 | 1 kHz a 40 dB SPL = 1 sone, a 60 dB SPL = 4 sone, ± 5 % | 1,006 e 4,09 sone | Confermata |
| L2 | Perdita del tono crescente con il rumore rosa a 40–70 dB SPL | 0,196 / 0,379 / 0,595 / 0,807 | Confermata |
| L3 | Toni a 200 Hz e 5 kHz: perdita ≤ 0,15 | 0,0016 e 0,0009 | Confermata |
| L4 | Ripetizione | Identica | Confermata |
| L5 | Bianco 0 ± 0,3, rosa −3 ± 0,3 dB/ottava | 0,000 e −2,997 | Confermata |

## 2. Fixture (estratti di 8 battute, calibrazione 100 dB SPL a fondo scala)

| Sezione | Mix (sone) | Pendenza del mix | Perdita: Pulso / Corpo / Trama / Vettore |
|---|---:|---:|---|
| A | 35,5 | −8,6 dB/ott | 0,60 / 0,79 / **indefinita** / 0,79 |
| B | 33,7 | −8,7 dB/ott | 0,69 / 0,82 / **0,92** / 0,73 |
| A′ | 34,2 | −8,7 dB/ott | 0,64 / 0,76 / **0,85** / 0,84 |

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| M1 | Trama con la perdita più alta in ogni sezione | B: 0,92, la più alta. A′: 0,848 contro 0,843 di Vettore, di poco. A: la Trama **tace nell'estratto scelto** (frasi 3–4 di A; già noto da `acao02_v1`, presenza `EXACT_SILENCE` nelle frasi 0–3 di A), quindi la perdita è indefinita | Confermata in B e A′, **non misurabile in A** |
| M2 | Perdita della Trama ≥ 0,3 | 0,92 e 0,85 | Confermata in B e A′ |
| M3 | Pendenza del mix fra −6 e −2 dB/ottava | **−8,6 / −8,7 / −8,7** | **Mancata** |

**Lettura:**
- **Il mix è scuro.** La pendenza di −8,7 dB/ottava è più ripida del rumore rosa (−3) e del valore di circa −5 riportato da Pestana per i singoli commerciali. Corpo e Vettore, denti di sega filtrati, scendono a −14…−15 dB/ottava, la Trama a −18. È una descrizione, non un obiettivo, ma è il primo dato misurato che collega il bilanciamento timbrico al giudizio d'ascolto. Va riportato all'ascolto cumulativo.
- **La Trama è il ruolo più coperto** dove suona: si sente meno del 15 % della sua loudness da sola. Kick (0,60–0,69) e Vettore (0,73–0,84) emergono di più.
- **Limite metodologico:** la perdita misura insieme mascheramento e compressione della loudness nella stessa banda, perché la loudness non è additiva dentro una banda critica. Valori alti per tutti i ruoli (0,6–0,9) sono attesi anche senza «mascheramento» in senso stretto. Vale il confronto fra ruoli e fra varianti, non il valore assoluto. Hafezi e Reiss 2015: nessuna misura oggettiva di mascheramento è un criterio di bilanciamento.
- **Errore di disegno dichiarato:** l'estratto pre-registrato di A cade nella metà di A senza Trama. Per misurarla in A serve un estratto diverso, con una nuova pre-registrazione.

## 3. Ambiente e codice

- MoSQITo 1.2.1 e pyuff 2.5.8 aggiunti a `tools/pyenv/requirements-oracle.txt` con hash. MoSQITo importa matplotlib al caricamento senza dichiararlo: matplotlib 3.10.3 e le sue dipendenze sono fissate per hash, con numpy 2.2.6 e scipy 1.15.3 invariati.
- `stem_export.{h,cpp}`: scenario `stem_export_v1` e scrittore di WAV float a 32 bit (`encodeWavStereoFloat32`, usato anche dal pacchetto 5).

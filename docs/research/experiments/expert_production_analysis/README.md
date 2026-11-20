# Analisi della produzione inviata dall'esperto

Pre-registrazione: [M2_5_Expert_Production_Analysis_Preregistration.md](../../M2_5_Expert_Production_Analysis_Preregistration.md). Risultati: [M2_5_Expert_Production_Analysis_Results.md](../../M2_5_Expert_Production_Analysis_Results.md).

Il materiale dell'esperto (stem MP3, MIDI e nota di produzione) **non è nel repository**. È opera sua, inviata «a solo scopo di analisi e raccolta dati», e la nota contiene dati personali. Qui ci sono solo gli script e le statistiche aggregate.

- **`midi_analysis.py --midi <cartella>`:** lettore minimale di file MIDI standard, senza dipendenze, e misure D1–D6. In più, il ritmo armonico delle tracce d'accordi.
- **`stem_analysis.py --gift <wav> --midi <cartella> --cymatica artifacts/cacp/cumulative_listening_v1`:** misure S1–S4 sul brano dell'esperto e S1 sugli stem per ruolo dei due brani dell'ascolto cumulativo. Comprende la loudness BS.1770 in scipy e la correzione dichiarata dell'allineamento del MIDI.
- **`midi_measures.json`, `stem_measures.json`:** uscite.

**Decodifica degli MP3.** Si fa una volta sola, fuori dal repository, con l'FFmpeg già installato sul PC. Il titolare l'ha approvato il 2026-11-20 per questa analisi soltanto; non è una dipendenza di CYMATICA.

```bash
ffmpeg -nostdin -v error -y -i <stem>.mp3 -c:a pcm_f32le <cartella isolata>/<nome>.wav
```

I nomi con parentesi quadre vanno copiati prima con un nome semplice, perché FFmpeg li interpreta come modelli. Il mix si salva come `MIX.wav`, il campione vocale come `Vocal Scream.wav` e il rumore bianco come `White Noise.wav`.

Gli stem per ruolo di CYMATICA li scrive `cymatica_game --cacp-run cumulative_listening_v1`, insieme ai due brani, che restano identici negli hash.

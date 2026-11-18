# Guida musicale per l'esperto

- **Scopo:** il PDF che spiega all'esperto il mood di CYMATICA, il brano di oggi e cosa gli chiediamo: la melodia F7 e le alternative per le tabelle d'autore (motivi d'apertura, giri d'accordi, arpeggi, basso). Le istruzioni F7 complete sono incluse.
- **Fonte del testo:** il documento Google del titolare «Cymatica Audio Design» (2026-11-18), corretto sul confronto con DESIGN.md:
  - tolti i numeri di citazione;
  - Corpo è accordi, pad e basso; Trama è arpeggi e figure;
  - gli archetipi non sono generi;
  - le specifiche di export valgono per brani audio completi;
  - aggiunte le sezioni sul mood con le parole di DESIGN, sul brano di oggi, sulle richieste e sugli archetipi.

  Il testo F7 è quello di [F7_istruzioni_per_l_esperto.md](../m25_ensemble_tools/F7_istruzioni_per_l_esperto.md): vanno cambiati insieme.
- **Uso:** `tools/pyenv/.venv/Scripts/python.exe -I make_music_guide_pdf.py artifacts/expert`. Serve Microsoft Edge. Scrive `artifacts/expert/CYMATICA_guida_musicale.pdf`, che non è versionato.
- **Stile:** `_style.py` è una copia dello stile del PDF di F7 (DESIGN §13, palette Sintetico).
- **Riservatezza:** il nome dell'esperto non compare né qui né nel PDF.

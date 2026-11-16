# CP-1 — Monitor `IAudioClock` nel probe e ascolto controllato sul device: pre-registrazione

- **Stato:** pre-registrazione, scritta il 2026-11-12 **prima** di implementare il monitor e di eseguire le corse. Non normativa. L'hash è registrato in `docs/progress.md` prima della prima corsa.
- **Origine:** [dossier R-M25-8](M2_5_1_Windows_Audio_Glitch_Detection.md): `IAudioClock` ha rivelato 46 stalli su 47 con buco (scarto > 5 ms), 0 su 40 senza buco; su USB un caso mancato su 11, quindi non va usato da solo. Procedura proposta: controllo positivo per uscita (B) e deficit letto a 20 Hz da un thread non realtime (riga 3).

## Monitor

- `AudioEngine::sampleDeviceClock`, **non realtime**: alla prima chiamata ottiene `IAudioClock` con `GetService` dal client WASAPI del device di miniaudio, poi legge `GetPosition` (posizione e istante QPC) e `GetFrequency`. Mai chiamato dalla callback. Rilasciato alla chiusura del device.
- `device_probe_v1` lo legge ogni 50 ms dal ciclo del coordinatore. **Scarto** di un campione = tempo QPC trascorso dal primo campione − posizione trascorsa / frequenza, in ms. **Salto** = aumento dello scarto fra due campioni consecutivi. Il manifest registra numero di campioni, scarto finale, salto massimo e numero di salti oltre 5 ms dopo i primi 500 ms.

## Controllo positivo sull'uscita USB (procedura B del dossier)

`glitchcal` del dossier, compilato dal sorgente versionato, sulle cuffie USB: 15 s, stalli di 25 e 60 ms, tre cicli, tono a −80 dBFS. **Atteso:** nessun buco a 25 ms (buffer di 30 ms), un buco per ogni stallo da 60 ms, visto sia nello scarto `IAudioClock` sia nel loopback. Se il monitor non reagisce ai 60 ms, il suo zero non vale per l'uscita USB e il monitor non si usa come prova di assenza di buchi.

## Corsa del probe

`device_probe_v1 --device-usage games --device-capture-pcm --device-loopback` sulle cuffie USB, cursore dell'app al 100 % verificato nel manifest, macchina silenziosa.

| ID | Predizione | Motivo |
|---|---|---|
| K1 | Campioni validi ≥ 90 % di quelli attesi (circa 3.700 in 185 s) | Lettura a 20 Hz da un thread normale |
| K2 | Nessun salto oltre 5 ms dopo i primi 500 ms | La baseline USB non ha mostrato buchi (loopback pulito, 0 sospetti underrun) |
| K3 | Loopback uguale al golden con guadagno 1 (scarto zero) | Cursore dell'app al 100 % (R-M25-12, H2b) |

## Ascolto controllato

Durante la corsa del probe il titolare ascolta in cuffia, a un volume sicuro scelto prima e annotato, e segna minuto e secondo di ogni click, buco o artefatto (domanda Q1 del pacchetto d'ascolto). **Atteso:** nessuno. Un artefatto sentito e non visto da monitor, loopback e intervalli è un fatto da riportare così, non da spiegare a posteriori.

Esito diverso da una predizione: si riporta. Il criterio PASS del device resta una decisione di CP-1.

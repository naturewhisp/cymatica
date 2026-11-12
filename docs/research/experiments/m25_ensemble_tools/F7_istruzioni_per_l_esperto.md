# Una melodia che si sviluppa — istruzioni

Grazie per l'aiuto. Ti chiediamo di scrivere una melodia, nel modo in cui la scriveresti per un brano vero. Ci serve come esempio umano di «sviluppo» per controllare uno strumento di analisi del nostro gioco musicale.

Non serve conoscere lo strumento: anzi, è meglio di no. Scrivi come ti viene naturale.

## Cosa scrivere

- **Una sola linea melodica**, come un lead synth o una voce: una nota alla volta, senza accordi.
- **Circa 24 frasi**, ognuna di **4 battute in 4/4**. In tutto circa 96 battute.
- Parti da **un motivo breve** e **sviluppalo** nel corso del brano, come faresti tu: variazioni, trasformazioni, ritorni, contrasti. Non deve essere un loop identico e non deve essere casuale.
- Tonalità, tempo e stile sono liberi. Un riferimento possibile: Re dorico, intorno ai 120 BPM.
- La griglia più fine che leggiamo sono i **sedicesimi**: le note più veloci verranno arrotondate al sedicesimo.

## Come consegnarla

Scegli una delle due strade.

**A. File MIDI (consigliato).** Scrivi la melodia in qualsiasi programma (MuseScore, che è gratuito, oppure la tua DAW) ed esporta un file `.mid` con una sola traccia. Ogni frase deve iniziare su una battuta multipla di 4: battuta 1, 5, 9, e così via.

**B. Tabella di testo.** Un file di testo, una nota per riga, con quattro colonne separate da virgole:

```
frase, inizio, nota, durata
1, 0, D4, 4
1, 4, F4, 4
2, 0, D4, 2
```

- **frase:** numero della frase, da 1 a 24 circa.
- **inizio:** in sedicesimi dall'inizio della frase, da 0 a 63 (4 battute × 16 sedicesimi).
- **nota:** nome con l'ottava (C4 è il Do centrale; diesis con `#`, bemolli con `b`: `F#4`, `Bb3`).
- **durata:** in sedicesimi (4 = un quarto, 2 = un ottavo, 16 = una battuta intera).

Le righe sopra sono solo un esempio del formato, non un suggerimento musicale.

## Una nota facoltativa, in un file separato

Se vuoi, scrivi in poche righe come hai sviluppato il motivo: per esempio «frasi 5–8: inversione; frase 17: ritorno del tema». La teniamo **separata** e la apriamo solo dopo la misura, per confrontare quello che hai fatto con quello che lo strumento vede.

## Cosa ne faremo

- Convertiamo la melodia nel formato dello strumento, senza modificarne le note.
- La misura viene confrontata con quello che ci aspettiamo da uno sviluppo reale. Le aspettative sono state fissate prima di ricevere il tuo file.
- Il tuo nome non compare nei documenti del progetto.

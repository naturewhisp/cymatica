# M2.5 CP-3 — I3, prototipo del basso: risultati

- **Pre-registrazione:** [documento](M2_5_CP3_I3_Bass_Prototype_Preregistration.md), SHA-256 (LF) `5e944654c4cc05a92ab21daa42c4e773dead0976de7b3f7319739e6d1670b78e`.
- **Esecuzione:** 2026-11-23. **Dati:** [esperimento](experiments/m25_cp3_bass/README.md).
- **Esito della regola:** V2 **merita un ADR**, con entrambi i kick. Nulla è adottato.

## 1. Criteri (V2 a −3 LU, senza separazione)

| ID | Kick attuale | Do2 e click | Esito |
|---|---|---|---|
| P1 | Golden V0 PASS; in V1 cambia solo Corpo; limiter 0 dB; picco V2 −4,5 dBFS | picco −4,9 | Superato |
| P2, quota non di kick in 30–120 Hz, A / B / A′ | V0 0,1 / 8,3 / 0,3 %; V1 19,8 / 18,2 / 21,0; V2 26,4 / 27,6 / 28,0 | V0 0,1 / 9,0 / 0,3; V1 19,3 / 19,6 / 20,5; V2 25,8 / 29,4 / 27,4 | Superato |
| P3, R di V2 (≥ 6 dB) | 10,6 / 11,7 / 11,4 | 10,7 / 11,8 / 11,4 | Superato |
| P4, attacchi a un semitono | V1 2,1 %, V2 0,8 % | uguale | Superato |
| P5, finestre di 4 battute che cambiano | V1 9,5 %, V2 100 % | uguale | Superato; **V1 è vicino alla soglia del 10 %** |
| P6, finestre di 8 battute con un'entrata o uscita | V0 50 %, V2 58 % | uguale | Superato, **di poco** |
| P7, coincidenza col kick | 34 % | uguale | Superato |

Il master è lineare: somma degli stem e mix differiscono al massimo di 6·10⁻⁵, due passi a 16 bit.

## 2. Che cosa dice, e che cosa no

- **V2 è fattibile senza danni misurabili:** c'è spazio, R resta sopra 6 dB anche a 0 LU (7,6–8,8), gli scontri di semitono sono pochi, il picco resta basso.
- **V1 riempie quasi altrettanto le basse** (circa 20 % contro 26 %) e quasi gratis. Sulle basse da sole il quinto ruolo non si giustificherebbe.
- **La differenza vera è la linea:** V1 ripete il giro, V2 cambia in ogni finestra. Ma P5 e P7 sono in buona parte **veri per costruzione**: ho scritto io la linea perché cambiasse e seguisse il kick.
- **Nulla dimostra che V2 sia musicalmente migliore.** Serve l'ascolto, con V1 fra le alternative.
- **Non risolve la forma:** le finestre di 8 battute con un'entrata o uscita passano solo dal 50 al 58 %. I4 resta necessario.

## 3. Da correggere rispetto all'indagine

**L'alternanza (S1) non mantiene la promessa.** Nell'indagine valeva circa 17 dB, qui 0,4–0,5 dB a pari livello: la linea coincide col kick solo nel 34 % dei casi, e la mediana di R guarda soprattutto i kick già liberi. L'indagine usava ottavi continui. S0 resta la scelta; per S1 servirebbe una misura sui soli kick coincidenti.

## 4. Confronto delle due vie

| | V1, voce di Corpo | V2, quinto ruolo |
|---|---|---|
| Cambiamento | un'opzione del planner, già scritta | ruolo, bus, due voci, strumenti, test, ADR ed emendamento |
| Basse riempite | circa 20 % | circa 26 % (regolabile) |
| Linea propria, silenzio, pedale per archetipo | no | sì |
| Livello e timbro propri | no | sì |
| Rischio | Corpo ha il basso sempre presente | costo e ricalibrazione AC-AO-02 |

## 5. Clip

`artifacts/cacp/bass_prototype_v1/clips/`, non versionate: `now_V0/V1/V2` e `c2_V0/V1/V2`, brano intero, allineate alla loudness della versione più debole. V2 suona con la stessa voce di Corpo: il timbro non è una proposta.

## 6. Limiti

- Un brano, un seed; linea scritta a mano; nessun ducking sul basso.
- La voce bassa di Corpo resta anche in V2: il basso raddoppia la fondamentale un'ottava sotto. Se diventasse un ruolo, Corpo andrebbe ridefinito.
- R è un rapporto d'energia.

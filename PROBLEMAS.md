# Problemas — moonplay

**1.0 (2026-09-10):** MUSIC + AUDIO FM testados (`GHOSTBUS`, `FRAY`, `ZODIACO`, `BRAZIL2` ok). `.MBK` / drums PSG ainda não.

---

## FRAY.MBM — VBlank (resolvido na 1.0)

Com Y8950, o `FRAY` começava ok e depois soava dessincronizado: aceleração breve e o resto lento. Não era tempo no ficheiro (zero cmds 1–23; `start_tempo` = 5 o tempo todo) nem o decoder.

Causa: `WriteOPLLreg` com `DJNZ 256` (~3800 ciclos/registo). O `FRAY` faz 251 drumsets (6 escritas OPLL) e MOD em 4 canais MUSIC a cada VBlank, mais 9 canais AUDIO. O tick passava do quadro; `Halt` saltava frames e o compensador 50 Hz (`×6/5`) às vezes corria dois ticks seguidos.

Conserto: espera `DJNZ 8` + `DJNZ 84` em `WriteOPLLreg` (`mbm_opll.c`).

Notas históricas abaixo (2026-08-27) descrevem o estado **antes** do AUDIO FM e deste wait.

---

Referência de comandos do tracker: `Moonblaster-1.3-app-B (ENGLISH).txt`  
(`^F` = 6 ou 9 canais FM no MSX-MUSIC; `^L` = loop position; `I L M O P S T U V` nas colunas).

Comparar sempre com `emul/dos2/BASIC.BIN` + `MBMPLAY.BAS` na mesma máquina (só YM2413).

O texto que se segue é o ponto de partida da sessão de 2026-08-27.

Referência de comandos do tracker: `Moonblaster-1.3-app-B (ENGLISH).txt`  
(`^F` = 6 ou 9 canais FM no MSX-MUSIC; `^L` = loop position; `I L M O P S T U V` nas colunas).

Comparar sempre com `emul/dos2/BASIC.BIN` + `MBMPLAY.BAS` na mesma máquina (só YM2413).

---

## O que já funciona

- CLI: `MOONPLAY [-i] [-l] file.MBM`, `-h`, sem ficheiro não toca `ESCALA.MBM`.
- Nome `MOONPLAY.COM` (8 caracteres).
- Sem `-l` toca uma vez e volta ao DOS; com `-l` usa `loop_position`.
- F-numbers oficiais do `BASIC.BIN`:
  `0xAD, 0xB7, 0xC2, 0xCD, 0xD9, 0xE6, 0xF4, 0x103, 0x112, 0x122, 0x134, 0x146`
  + oitava `<< 9`. A nota Si (`byte % 12 == 0`) já não lê fora da tabela.
- Decoder de padrões (13 colunas, crunch 243–255) está **em sincronia** com os endereços do ficheiro em todos os `.MBM` de teste (desvio de 1–2 bytes só no último padrão = EOF).
- `sizeof(MBM_HEADER) == 0x178` (struct packed, offsets correctos).
- Escala / Zodíaco / Oxygene IV / Lucifer: afinação e número de canais MUSIC melhoraram (relato do utilizador).
- Wait do YM2413: na 1.0 é `DJNZ 8`+`DJNZ 84` (o `256` antigo não cabia num VBlank com AUDIO; ver FRAY acima).

---

## Bugs confirmados em teste real

### 1. GHOSTBUS.MBM — errado desde o início; “trava” no primeiro padrão

Sintomas (utilizador):

- Toca mal logo no começo.
- Cai num loop no **primeiro padrão** e não segue a melodia.
- Mapear canais AUDIO 7–9 para o OPLL **piorou** isto.

Dados do ficheiro:

| Campo | Valor |
|---|---|
| `song_length` | 77 (78 posições, índices 0–77) |
| `loop_position` | **0** (recomeça do início) |
| `start_tempo` | 8 |
| `sustain` | `0xE0` (ritmo + AM/VIB do byte AUDIO) |
| chip set | `03 03 03 03 03 03 01 01 01 03` → ch 0–5 stereo, **ch 6–8 só AUDIO** |
| notas MUSIC ch 0–5 | 807 |
| notas AUDIO ch 6–8 | 661 (**407 só no canal 7**) |
| MOD / SUS / LINK / PITCH | 109 / 409 / 60 / 6 |
| comandos | tempo 1, 10, 17, 22, 23 (no padrão 4, ainda no intro) |
| ordem | `[1,2,3,4,5,31,5,6,7,8,7,9,…]` — o riff 5–9 **repete-se no próprio arranjo** |

Hipóteses ainda em aberto (não fechadas):

1. **Melodia no canal 7 (AUDIO)** — no modo 6+drums o OPLL não a toca. O que sobra é acompanhamento MUSIC, que no ficheiro já é um riff a repetir. Pode parecer “loop no 1.º padrão” mesmo avançando posições.
2. **`loop_position = 0` + `-l`** — ao acabar as 78 posições volta à posição 0 (padrão 1). Sem `-l` devia tocar as 78 e parar (~2–3 min a tempo 8). Se trava *às primeiras linhas*, não é este caso.
3. **Tempo no intro** — `PlayCommand` faz `tempo = 25 - cmd`. No padrão 4: cmd 1 → espera 24 VBlanks/row (muito lento), depois cmd 22/23 → 3 e 2 (muito rápido). Se `start_tempo` no ficheiro já for a espera em interrupts (e não o número 1–23 do editor), esta conversão está errada.
4. **Pitch −5 a cada VBlank** (cmd 203) no intro, com a espera em 24, faz a nota mergulhar até ao clamp `0x00AD`. A tabela RoboPlay é 10-bit (`<< 1`); no OPLL 9-bit o mesmo delta é ~2× mais agressivo.
5. **Se `song_length` fosse lido como 0**, `seguentPatro` com `-l` e `loop_position = 0` reiniciaria o padrão 1 em *cada* avanço — loop exacto no 1.º padrão. O dump do ficheiro tem `song_length = 77` e o `sizeof` da header está certo; só seria verdade se a RAM corromper o campo.

O decoder **não** perde o sítio: 16 rows do padrão 1 acabam exactamente no endereço do padrão 2 (`0x204 → 0x278`).

### 2. FRAY.MBM — não chega ao fim / não toca de forma correcta

Sintomas (utilizador): corte / não chega ao fim; confirmado que também toca mal, não é só o fade do loop.

| Campo | Valor |
|---|---|
| `song_length` | 31 (32 padrões, 1..32 em ordem linear) |
| `loop_position` | **0** (peça de jogo, feita para repetir) |
| `start_tempo` | 5 |
| `sustain` | `0xE0` |
| chip set | igual ao GHOSTBUS (ch 6–8 AUDIO) |
| notas no ch 8 (AUDIO) | 320 (o canal com mais notas) |
| MOD / SUS | 230 / 144 |
| comandos | **251× drumset 25–27** (quase todas as rows) |
| cmd 24 (fim de padrão) | nenhum |

Sem `-l` o player pára depois da posição 31. São todos os padrões do USER file (~32×16×5/60 ≈ 43 s se `start_tempo` for espera em interrupts). Se 5 for o tempo *do editor* (1–23), a espera correcta seria `25-5 = 20` e a peça duraria ~4× mais — nesse caso estamos a **acabar cedo**.

A tentativa de 9 melodias no OPLL foi especialmente má aqui: `canviDrumset()` escreve `0x16/0x17/0x18` e `0x26/0x27/0x28`, que no modo 9ch **são as frequências das melodias 7–9**. 251 escritas por peça = afinação destruída. Já está barrado (`canviDrumset` só com ritmo ligado), e o modo 9ch-forçado foi revertido.

---

## Tentativa que piorou (não repetir)

Forçar 9 melodias no OPLL quando ch 6–8 têm muitas notas AUDIO, desligando o ritmo:

- O YM2413 **não** faz 9 melodias + drums (`^F` no 1.3).
- GHOSTBUS ficou preso no 1.º padrão.
- FRAY deixou de chegar ao fim.
- Drumset passou a pisar F-num dos canais 7–9.

Modo actual (como o tracker): `sustain & 0x20` → 6+drums; senão → 9 melodias. Só se tocam canais com bit MUSIC no chip set.

---

## Limites de formato (não são bugs do decoder, mas soam a “falta canal”)

- **MSX-AUDIO** (9 FM Y8950 + ADPCM `.MBK`): não implementado. Lucifer (`MUZAK_I`), GHOSTBUS e FRAY perdem a camada AUDIO.
- **PSG drums** (nibble alta de `drum_setup_music_psg`): não.
- **Ficheiros EDIT** (1.º byte `0xFF`): não.
- **Stereo Sunrise / Checkmark**: MUSIC num lado, drums/AUDIO no outro. Ver `LEIA-ME.txt`.
- Um único user-voice no OPLL (registos 0–7). Dois “own voices” ao mesmo tempo (FRAY inst 16 e 17) partilham o mesmo patch — limitação do chip, igual no Moonblaster.

---

## Eventos — estado da implementação

| Evento | Código MBM | Estado |
|---|---|---|
| Nota 1–96 | `data/12` oitava, resto nota | Corrigido (índice `(n-1)%12`, tabela BASIC.BIN) |
| Off | 97 | Key off, sem retrigger |
| Instrumento | 98–113 | Lista 1–16; ≥16 carrega own voice. Volume **por canal** (já não usa o slot da coluna). |
| Volume | 114–176 | `15 - ((cmd-114)>>2)`. **Não confrontado** com o ASM do `BASIC.BIN`. Fusion-C usava `!` (errado). |
| Stereo | 177–179 | Ignorado (só AUDIO/Sunrise) |
| Link | 180–198 | `±(cmd-189)` sem retrigger. GHOSTBUS usa muito `189` (L+0). |
| Pitch | 199–217 | `cmd-208` **por VBlank**. Pode ser 2× mais forte que no OPL/RoboPlay. GHOSTBUS intro usa −5. |
| Brightness −/+ | 218–223, 231–236 | Soma no registo 0x02 (own voice global). |
| Detune (`T`) | 224–230 | Guarda por canal, aplica na nota. |
| Sustain (`U`) | 237 | Escreve `0x30` (S+K). Não está claro se o original faz key-off com S=1. |
| Modulation (`M`) | 238 | Oscila em volta de `base_freq` com `{1,3,5,3,1,0,-2,-4,-2,0}` (ciclo do RoboPlay). FRAY dispara MOD quase em rows alternadas — **ainda soa mal**. |
| Tempo (cmd) | 1–23 | `tempo = 25 - cmd`. **Contradiz** usar `start_tempo` cru como espera (é o que RoboPlay e o Fusion-C fazem no arranque). |
| Fim de padrão | 24 | `seguentPatro()`. GHOSTBUS/FRAY não usam. |
| Drumset | 25–27 | Só se ritmo ligado. |
| Transpose | ≥49 | `transpose = cmd - 55`. Pouco testado. |

`HandleFreqModes()` corre **todos os VBlanks**, não só nas rows. O Fusion-C original só aplicava bend ao processar a linha.

---

## Tempo: pergunta em aberto

Tabela MSX wiki (60 Hz): editor 1 → 24 interrupts/row; editor 23 → 2.

- Arranque: `tempo = capcalera.start_tempo` **sem** converter (FRAY=5, GHOSTBUS=8, Lucifer=6).
- Comando de coluna: `tempo = 25 - cmd`.

Se o byte no ficheiro for o número do editor, FRAY deveria esperar 20, não 5 (peça ~4× mais longa). Se já for a espera em interrupts, os comandos 1/22/23 do GHOSTBUS estão mal convertidos.

**Próximo passo:** desassemblar no `BASIC.BIN` o que faz com o byte a `0xCD` e com os comandos 1–23. O driver está em `emul/dos2/BASIC.BIN` (load `B000h`, tabela F-num no body `+0x1052`).

---

## Ficheiros de teste (o que esperar)

| Ficheiro | Ritmo | O que o OPLL *deve* fazer | Relato actual |
|---|---|---|---|
| `ESCALA.MBM` | sim | 2 vozes, C–F | OK (afinação) |
| `ZODIACO.MBM` | sim | 6ch + drums, 269 Si | OK (afinação) |
| `OXYGENE4.MBM` | não | **9** melodias | OK (canais) |
| `LUCIFER.MBM` | sim | 6ch + drums; samples MBK mudos | melhorou; AUDIO extra mudo |
| `GHOSTBUS.MBM` | sim | 6ch + drums; **lead no ch7 mudo** | **mau** — loop no 1.º padrão |
| `FRAY.MBM` | sim | 6ch + drums; **lead no ch8 mudo** | **mau** — não chega ao fim |

Comando útil: `MOONPLAY -i FRAY` / `MOONPLAY -i GHOSTBUS` (deve dizer `6 ch + FM drums`).

Comparar no mesmo hardware com `MBMPLAY.BAS` (também só MUSIC se não houver Y8950).

---

## Onde mexer na próxima sessão

1. `.MBK` ADPCM (stream para a RAM do Y8950; não meter 32 KB no TPA).
2. Drums PSG (`mbplay.src` `psgdrm`).
3. Não voltar a forçar 9 melodias OPLL em peças com ritmo só para “ouvir o AUDIO”.
4. Tempo: `start_tempo` cru vs `25-cmd` nos comandos — ainda por confrontar com `BASIC.BIN` se alguma peça soar acelerada **sem** sobrecarga de CPU.

Código relevante: `mbm_opll.c`, `moon_player.c`.

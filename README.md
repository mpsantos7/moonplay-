# moonplay 1.0 — player MoonBlaster 1.4 para MSX-DOS 2

Player DOS2 de ficheiros **`.MBM`** (MoonBlaster 1.4) no **YM2413 / MSX-MUSIC** e, se o chip existir, no **Y8950 / MSX-AUDIO FM**.
Não é Moonsound, não é OPL4, não toca `.MFM` / `.MWM`. Kits `.MBK` ainda não.

**Versão 1.0** (2026-09-10). Não havia release 1.0 anterior.

Binário: **`MOONPLAY.COM`** (8 caracteres, limite 8.3 do MSX-DOS).

O replay em C (`mbm_opll.c`) parte do porto [fm_mbm_fusionc](https://gitlab.com/brossaip/fm_mbm_fusionc) e foi alinhado à rotina oficial **`sources/replayer/mbplay.src`** (Remco Schrijvers / MoonSoft, correcções BiFi).

Documentação curta de utilização: `LEIA-ME.txt`.
Notas para continuar no Grok Build: `AGENTS.md`.
Pesquisa histórica (formatos, players, ligações): `RESOURCES.md`.

---

## Estado (1.0, 2026-09-10)

Funciona como aplicação DOS2/Nextor:

| Função | Estado |
|--------|--------|
| Replay MSX-MUSIC (6ch+drums ou 9ch) | feito |
| Notas 1–96 (incluindo B) | feito |
| Padrões, EOP (CMD 24), loop do `.MBM` | feito |
| Pitch, modulação, note link, detune, transpose, tempo | feito |
| CLI silenciosa; `-info` `/INFO`; `-loop` `/LOOP`; `-help` `/HELP` | feito |
| Wildcards `*` `?` via `_FFIRST`/`_FNEXT` | feito |
| Pausa (espaço), parar (ESC) | feito |
| FM-PAC APRLOPLL / PAC2OPLL | feito |
| Compensação 50 Hz (tick ×6/5) | feito |
| MSX-AUDIO FM (Y8950, 9 canais) | feito (se o chip existir) |
| `.MBK` ADPCM, drums PSG | **não** |
| Moonsound / OPL4 | **fora de âmbito** |

Testado em hardware (Panasonic FS-A1ST e MSX2+) e no openMSX (`run_openmsx.bat` / `run_openmsx_audio.bat`). `GHOSTBUS`, `FRAY`, `ZODIACO` e `BRAZIL2` ok na 1.0. A fonte de verdade do replay continua a ser o ASM original.

---

## Requisitos para construir

Este directório tem de estar **dentro da árvore MSXgl**, no sítio dos outros projectos:

```
<MSXGL>/projects/moon_player/
```

Na máquina original: `C:\msxgl\projects\moon_player`.

Precisa de:

- MSXgl (SDCC, scripts Node em `engine/script/js/build.js`, `tools/`)
- Windows: `build.bat` (usa `MSXGL_PATH`, default `..\..`)
- Unix: `build.sh` equivalente

**Não** basta copiar só esta pasta para outro PC sem a MSXgl. Copiar o zip para `<MSXGL>/projects/moon_player/` e correr `build.bat`.

Abrir no Grok Build: workspace = esta pasta (ou a raiz da MSXgl). O `AGENTS.md` é lido automaticamente.

---

## Build e execução

```
cd <MSXGL>/projects/moon_player
build.bat
```

| Ficheiro | Função |
|----------|--------|
| `emul/dos2/moonplay.com` | COM para disquete / HD DOS2 |
| `emul/dsk/DOS2_moonplay.dsk` | imagem 720K com DOS2 + músicas de `content/` |
| `run_openmsx.bat` | openMSX, máquina Panasonic FS-A1ST |

`project_config.js`: `ProjName = "moonplay"`, `ProjModules = [ "moon_player" ]`, `AddSources = [ "mbm_opll.c" ]`, `EmulMSXMusic = true`.

A pasta `out/` é gerada no build; pode apagar-se.

---

## Utilização no MSX-DOS 2 / Nextor

O Nextor é compatível com o MSX-DOS 2.31 nestas APIs. Os wildcards **não** são interpretados pelo player: o kernel faz o matching (`_FFIRST` 40h, `_FNEXT` 41h), na mesma ordem do `DIR`. Opções de comando usam `/` (como as ferramentas Nextor); `-` também é aceite. Separador de pastas: `\`.

```
MOONPLAY                  → ajuda
MOONPLAY LUCIFER.MBM      → uma vez, sem texto
MOONPLAY *.MBM            → todos os .MBM do directório (máx. 48)
MOONPLAY A????.MBM        → nomes de 5 caracteres a começar por A
MOONPLAY 50HZ\*.MBM       → pasta 50HZ
MOONPLAY -info MARIO.MBM
MOONPLAY -loop LUCIFER.MBM
MOONPLAY /LOOP *.MBM
MOONPLAY /HELP
```

- Sem extensão no último componente, acrescenta-se `.MBM` (`*` → `*.MBM`).
- Default: uma passagem, silencioso.
- `-loop` com **um** ficheiro: usa `loop_position` do header (255 = off → recomeça no 0).
- `-loop` com **lista**: cada música uma vez; no fim volta à primeira.
- **Espaço**: pausa / continua (OPLL silenciado na pausa; o padrão não avança).
- **ESC**: pára tudo e sai.

---

## Hardware de som

| Máquina | Notas |
|---------|--------|
| MSX2+ / turbo R (ex. FS-A1ST) | YM2413 interno, portas `7Ch`/`7Dh` sempre ligadas. Não varrer slots nem escrever `7FF6h`. |
| MSX2 + FM-PAC Panasoft SW-M004 (`PAC2OPLL`) | Ligar bit 0 de `7FF6h` **no slot do cartucho**. |
| Checkmark / clones `APRLOPLL` | I/O já ligado. **Nunca** escrever `7FF6h` (num 2+/turbo R isso desliga o FM interno). |
| Checkmark FM Stereo PAK | Jack: L = melodias (MO), R = bateria. Cabo mono ou TV podem parecer mudos. |

MSX-AUDIO FM (Y8950, portas `C0h`/`C1h`) toca se o chip estiver presente (`run_openmsx_audio.bat`). Kits `.MBK` ainda não. Sem Y8950 o comportamento é só MUSIC.

---

## Arquitectura

```
moon_player.c     CLI DOS2, wildcards, pausa, 50 Hz, enable FM-PAC, detecta Y8950
mbm_opll.c/.h     Load .MBM, tick VBlank, YM2413 + Y8950 FM
msxgl_config.h    Módulos MSXgl (dos, input, memory, …)
project_config.js Build MSXgl
```

### API do replay

```
u8   MBM_LoadFile(const c8* name);  /* handle DOS2, caminhos */
void MBM_SetLoop(u8 enable);
void MBM_Start(void);
void MBM_Tick(void);                /* um por VBlank */
void MBM_Mute(void);                /* silêncio sem parar o estado */
void MBM_Stop(void);
u8   MBM_IsPlaying(void);
```

Um `Halt()` + `MBM_Tick()` por frame. A 50 Hz, um tick extra a cada 5 frames.

Cada tick: pitch/modulação em todos os canais; no instante `speed-1` descodifica a linha (crunch 243–255); no instante `speed` toca eventos. Isto copia o ISR do `mbplay.src`.

### Header `.MBM` USER (offset `0x178` = 376)

Ver `MBM_HEADER` em `mbm_opll.h` e https://www.msx.org/wiki/Moonblaster_file_format

Campos críticos:

- `song_length` — último índice de posição (há `song_length+1` entradas)
- `sustain` bit 5 (`0x20`) — ritmo OPLL (6 melodias + drums) vs 9 melodias
- `loop_position` — `0x177`; `255` = off
- tabela de posições em `0x178`
- ponteiros de padrão (u16, offset no ficheiro, padrões **1-based**) a seguir

EDIT mode: byte extra `FFh` no início; suportado de forma simples.

Canais no padrão (13 colunas): 0–8 FM, 9–10 sample (ignorados), 11 drum+sample, 12 CMD.

### Frequências

Tabela oficial `pafreq` (96 × u16): F-num 9 bits + block nos bits 1–3 do byte alto.
Nota B (valores 12, 24, 36, 48, 60, 72, 84, 96) **não** pode usar `byte/12` em C: o resto 0 rebentava a oitava. Indexar `g_freq[note-1]`.

---

## Bugs já corrigidos (relato do hardware)

1. Nome 8.3 → `moonplay.com` (antes `moon_player` truncava para `MOON_PLA`).
2. Default `ESCALA.MBM` e texto de debug — removidos.
3. Nota B desafinada — índice e tabela.
4. Padrões a cortar / loop infinito — EOP avançava o ponteiro cedo demais; o fim da música ignorava `loop_position` e saltava sempre para 0 (`BRAZIL2` nunca acabava).
5. Pitch só na linha; modulação vazia; note link escrevia o índice no OPLL.
6. Tempo `tempo - cmd`; volume `!(byte>>4)` (NOT lógico).
7. Só 6 canais — músicas a 9ch (`ODYSSEY`) perdiam vozes.
8. Registo de ritmo `0xE0` em vez de `0x0E`.
9. **FRAY.MBM lento / fora de sincronia com MSX-AUDIO** — ver abaixo.

### FRAY.MBM — espera do YM2413 (1.0)

Com o Y8950 ligado, o `FRAY` começava bem e depois os canais pareciam dessincronizados: uma aceleração estranha e o resto da peça lento. `GHOSTBUS`, `ZODIACO` e `BRAZIL2` estavam ok.

Não era o decoder nem um comando de tempo: o ficheiro **não tem** cmds 1–23; o `start_tempo` fica em 5 do princípio ao fim. O `FRAY` é a peça mais pesada por VBlank:

- 251 mudanças de drumset (cmds 25–27 em quase todas as outras linhas) → 6 escritas OPLL cada
- MOD em 4 canais MUSIC **a cada** VBlank, mais os 9 canais AUDIO

`WriteOPLLreg` esperava `DJNZ 256` após o dado (~3800 ciclos por registo). Com AUDIO por cima, o `MBM_Tick()` passava do VBlank: o `Halt()` saltava frames (a peça abrandava) e o tick extra a 50 Hz (`×6/5`) às vezes corria dois de seguida (a aceleração). Os canais não iam a relógios diferentes — o relógio comum ia aos solavancos.

**Conserto:** espera `DJNZ 8` (após o endereço) + `DJNZ 84` (após o dado), o mínimo que o YM2413 precisa e que ainda cabe num VBlank com AUDIO. Está em `mbm_opll.c` (`WriteOPLLreg`).

---

## Mapa de pastas

```
moon_player/
  moon_player.c      frontend DOS2
  mbm_opll.c/.h      replay YM2413 + Y8950 FM
  project_config.js  build
  msxgl_config.h
  build.bat / build.sh
  run_openmsx.bat
  LEIA-ME.txt        utilização
  README.md          este ficheiro
  AGENTS.md          regras Grok Build
  RESOURCES.md       ligações e formatos (histórico)
  content/           .MBM empacotados no DSK
  emul/dos2/         deploy + COMMAND2.COM + músicas extra / 50HZ/
  emul/dsk/          DOS2_moonplay.dsk
  manual/            manuais MoonBlaster 1.3/1.4
  sources/           ASM original (replayer/mbplay.src é a referência)
  vendor/fm_mbm_fusionc/  porto C inicial
  moonblaster-1.4.dsk     disco original do tracker
```

Músicas em `content/` (e no DSK): `escala`, `lucifer`, `MARIO`, `BRAZIL2`, `NOLIMIT`, `OXYGENE4`, `ODYSSEY`, `RUNEMAST`.
`emul/dos2/50HZ/` tem variantes a 50 Hz (não vão todas para o DSK; o build só copia `DiskFiles`).

---

## Trabalho futuro

- `.MBK` ADPCM no Y8950 (FM AUDIO já toca se o chip existir).
- Drums PSG (já no `mbplay.src`).
- Lista > 48 ficheiros; EDIT mode completo.
- Não portar para OPL4 — isso é o [RoboPlay](https://gitlab.com/torihino/roboplay).

---

## Licenças e créditos

- MoonBlaster 1.4: Remco Schrijvers / MoonSoft / Sunrise. Fontes libertadas; restrições de uso em novos programas levantadas em 2012 (`sources/readme.txt`).
- Replayer BiFi / MSX Banzai: `sources/replayer/`.
- fm_mbm_fusionc: gitlab.com/brossaip/fm_mbm_fusionc.
- RoboPlay (referência do parser): gitlab.com/torihino/roboplay.
- MSXgl: Guillaume ‘Aoineko’ Blanchard, CC BY-SA.
- Nextor: Konamiman, compatível com MSX-DOS 2.31 (https://github.com/Konamiman/Nextor).

Especificação `.MBM`: https://www.msx.org/wiki/Moonblaster_file_format
Funções DOS2: https://map.grauw.nl/resources/dos2_functioncalls.php (`_FFIRST` 40h, `_PARSE` 5Bh).

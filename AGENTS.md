# moonplay 1.0 — regras para o Grok Build

Player MSX-DOS 2 de MoonBlaster 1.4 no **YM2413 / MSX-MUSIC**, e **Y8950 / MSX-AUDIO FM** se o chip existir (não Moonsound/OPL4; `.MBK` ADPCM ainda não).
O utilizador fala português; responder em português.

## O que é este projecto

- Binário: `moonplay.com` (nome 8.3; **nunca** mais de 8 caracteres no nome).
- Fontes: `moon_player.c` (CLI, DOS2, teclado, FM-PAC) + `mbm_opll.c` / `mbm_opll.h` (replay).
- Motor: MSXgl, `Target = "DOS2"`, `Machine = "2"`.
- Replay alinhado ao `sources/replayer/mbplay.src` (Remco Schrijvers / BiFi), não inventar tabelas de notas.

## Como construir

O projecto vive **dentro** da árvore MSXgl (`C:\msxgl\projects\moon_player`). `build.bat` usa `MSXGL_PATH=..\..` se a variável não existir.

```
cd <msxgl>/projects/moon_player
build.bat
```

Saídas: `emul/dos2/moonplay.com`, `emul/dsk/DOS2_moonplay.dsk`.
Emulador: `run_openmsx.bat` (Panasonic FS-A1ST, YM2413 interno).

`ProjName = "moonplay"` e `ProjModules = [ "moon_player" ]` — o `.c` não se chama `moonplay.c`.

## Comportamento que não se pode partir

- Default: **uma execução**, **sem texto**. Erros e `-help`/`-info` podem imprimir.
- Opções: `-info` `/INFO`, `-loop` `/LOOP`, `-help` `/HELP` (Nextor/DOS2 usa `/`; o `-` também vale).
- Wildcards `*` e `?` via `DOS_FindFirstEntry` / `DOS_FindNextEntry` (`_FFIRST`/`_FNEXT`). Não reimplementar o matching. Ordem = DIR. Até 48 ficheiros. Prefixo de pasta (`50HZ\`) junta-se ao nome do FIB.
- `-loop` com 1 ficheiro: loop interno do `.MBM` (`loop_position`, 255 = off → posição 0).
- `-loop` com lista (`*.MBM`): cada música uma vez, depois recomeça a lista.
- **Espaço** pausa/continua (silencia OPLL e Y8950 na pausa, não zera o estado). **ESC** sai.
- Extensão `.MBM` acrescentada se o último componente não tiver ponto (`*` → `*.MBM`).
- 50 Hz: tick extra a cada 5 VBlanks (andamento de 60 Hz).
- FM-PAC: APRLOPLL = I/O já ligado, **nunca** escrever `7FF6h`. PAC2OPLL = bit 0 de `7FF6h` nesse slot.

## Replay (`mbm_opll.c`)

Fonte de verdade: `sources/replayer/mbplay.src`.

- Notas 1–96 indexam `g_freq[note-1]` (96 entradas oficiais). B = 12, 24, 36… **não** usar `nota/12` com resto 0.
- Header USER em `0x178`. EDIT: byte extra `FFh` no início.
- Posições 0 … `song_length` (inclusive). Padrões 1-based. CMD 24 (EOP) põe `step=15`; o decode seguinte avança o padrão.
- Pitch e modulação **a cada tick**, não só na linha. Note link usa a tabela de frequências, não o índice da nota.
- Tempo comando: `speed = 25 - cmd`. Volume MUSIC: `(byte-114)>>2`.
- `sustain & 0x20`: 6 canais + bateria OPLL; senão 9 canais de melodia.
- Canais com bit MUSIC (`channel_chip_set & 2`) no OPLL; bit AUDIO (`& 1`) no Y8950 se detectado (`IN C0 != FFh`, sem ROM WAVE). `.MBK` / drums PSG: ainda não.

## Ficheiros que não se deve “limpar”

- `sources/` e `manual/` — MoonBlaster 1.4 original (Remco Schrijvers; freeware desde 2012).
- `vendor/fm_mbm_fusionc/` — porto C de origem.
- `content/*.MBM` — músicas de teste (escala, lucifer, MARIO, BRAZIL2, NOLIMIT, OXYGENE4, ODYSSEY, RUNEMAST).
- `emul/dos2/50HZ/` — versões a 50 Hz; o DSK só leva o que está em `DiskFiles`.

## Próximo trabalho (se pedido)

1. `.MBK` ADPCM no Y8950 quando o módulo existir (FM AUDIO já toca).
2. Drums PSG (já no ASM original).
3. Mais de 48 matches; EDIT mode completo.
4. Não converter para OPL4 (isso é o RoboPlay).

## Estilo

- Alterações só no que foi pedido. Comentários curtos e factuais.
- SDCC z80, alignment 1; o header MBM usa `uint8_t id[2]` (não `uint16_t` após `song_length`) para não haver padding.
- `Mem_Copy(src, dst, n)` na MSXgl (ordem invertida vs memcpy).

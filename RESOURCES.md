# Fontes para um player MBM/MBK (DOS2)

Notas de pesquisa **anteriores** à implementação. O player actual é o `MOONPLAY.COM` (ver `README.md`). CLI: `MOONPLAY [opcoes] ficheiro` com wildcards DOS2, não `MOON_PLA`.

Ponto crítico primeiro: **MBM/MBK não são o formato do Moonsound (OPL4).**

## Dois ramos MoonBlaster (confundem-se fácil)

| Extensão | Tracker | Chip | Época |
|----------|---------|------|--------|
| **`.MBM`** + **`.MBK`** | MoonBlaster **1.4** (Moonsoft / Sunrise) | **MSX-MUSIC (YM2413/OPLL)** + **MSX-AUDIO (Y8950/OPL + ADPCM)** + um pouco de PSG | ~1992, Holanda, demos/jogos |
| `.MFM` | MoonBlaster **FM** for MoonSound | OPL4 FM | cartucho Moonsound, 1995+ |
| `.MWM` + `.MWK` | MoonBlaster **Wave** for MoonSound | OPL4 wavetable | idem |

O artigo do Fusion-C que você apontou é sobre **OPLL / MSX-MUSIC** (portas `7Ch`/`7Dh`) e cita explicitamente um player MBM em C. Serve para o ramo **1.4**, não para OPL4.

O `.MBK` do 1.4 é **drumkit ADPCM** (header 56 bytes + ~32 KB de samples Y8950), não o wavekit `.MWK` do Moonsound.

Um player DOS2 de `.MBM` precisa, no mínimo, de MSX-MUSIC; o kit `.MBK` só soa se houver MSX-AUDIO (Music Module / FS-CA1 / AudioWave). Stereo “Sunrise” = MUSIC num canal, AUDIO no outro.

---

## O que já existe (e o que falta em DOS2)

Não há um `.COM` DOS2 pequeno, estável e só-MBM tão famoso quanto o tracker. O que existe:

### 1. RoboPlay — o player DOS2 em C que você lembrava

- **GitLab (canónico):** https://gitlab.com/torihino/roboplay
- Mirror GitHub (só aponta para o GitLab): https://github.com/ToriHino/RoboPlay
- MSX.org: “RoboPlay - Multi format OPL4 music player”
- Thread longo: https://www.msx.org/forum/msx-talk/graphics-and-music/roboplay-multi-format-music-player-in-fusion-c
- Pacote: https://msxhub.com/ROBOPLAY (`hub install ROBOPLAY`)

Escrito em **SDCC + Fusion-C**, **exige DOS2**. Toca MBM 1.4 (USER **e** EDIT, PSG drums, ADPCM), e também MFM/MWM no **OPL4**. O plugin MBM está em `players/src/mbm.h` no repositório.

Limitação para o nosso objetivo: o alvo de som do RoboPlay é **OPL4** (converte MUSIC+AUDIO para o Moonsound). Um player “clássico” no FM-PAC + Music Module é outro produto — mas o **parser C do `.MBM` é o melhor código moderno** para copiar/adaptar.

### 2. Player MBM em C (Fusion-C) — o daquele artigo

- https://gitlab.com/brossaip/fm_mbm_fusionc  
  Citado em https://moltsxalats.wixsite.com/fusionc/post/programming-opll-msx-music  
  Usa a tabela de frequências do BiFi/RoboPlay, `InitVDPInterruptHandler` do Fusion-C, e escreve o OPLL. É o “não estável em C” que você lembrava. Portar de Fusion-C → MSXgl é o caminho mais curto se quisermos **FM-PAC nativo**, não OPL4.

### 3. Replayer oficial em assembly (fonte a usar no ISR)

- **MoonBlaster 1.4 completo (tracker + fontes):**  
  https://www.msx.org/downloads/music/trackers/moonblaster-14  
  (`moonblaster-1.4.zip`, ~8 MB, inclui sources e manuais NL)
- **BiFi / MSX Banzai — player 1.4 para BASIC + ASM “beginner-friendly”:**  
  http://msxbanzai.tni.nl/dev/software.html  
  `MBPLYSRC.LZH` — MBPLAY.SRC + MBLOADER.SRC, comentários em inglês, flags condicionais.  
  Atualização 2019: https://www.msx.org/news/software/en/moonblaster-14-driver-for-msx-basic-update
- **W. Brants replay v1.43** (usado no plugin Winamp / KSS): aparece em conversas do MRC e no `mbm2kss`.
- **Pedido aberto na MSXgl** para portar o BiFi para SDCC:  
  https://github.com/aoineko-fr/MSXgl/issues/4 (artrag, 2022). Ninguém fechou.

O player ASM original **não usa mapper**; o loader testa MUSIC vs AUDIO. Serve como rotina de interrupt. O que falta historicamente é um **frontend DOS2** (listar `*.MBM`, carregar `.MBK`, CLI). Daí o “não há player DOS2” no sentido de um `PLAY.COM` simples — o editor 1.4 **nem DOS2 tem** (só MAP2 como hack).

### 4. TSR / CLI antigos (DOS, difíceis de achar)

- `MBLOAD song.MBM [drum.MBK]` — loader TSR Moonsoft/Sunrise (Manuel tinha no HD; Latok distribuía no MRC).  
  Thread: https://www.msx.org/forum/msx-talk/graphics-and-music/playing-moonblaster-songs-from-msx-dos
- `MBMPLAY.COM` — citado no mesmo thread e na ROM disk do MegaFlashROM SCC+ SD.
- Pascal MSX: `MBPLAYER.INC` + `MBPLAYER.MPC` no manual do MSX Pad (File-Hunter). Exige DOS2 e mapper.

### 5. Moonsound (só se no futuro quisermos MFM/MWM)

- Wave driver DOS1/DOS2: http://www.msx.ch/ftp/Products/MoonSound/wavedrv.lzh  
- Fontes Wave: https://www.msx.org/downloads/moonblaster-wave-replayer-sources  
- FAQ OPL4: https://faq.msxnet.org/opl4.html  
- Manual Wave (PDF): File-Hunter, “Moonblaster for Moonsound User and Edit Manual”

### 6. Spec do ficheiro

- Wiki: https://www.msx.org/wiki/Moonblaster_file_format  
- Thread: https://www.msx.org/forum/development/msx-development/mbm-file-format  

`.MBM` USER (o que o replayer clássico toca):

- offset `0000h`: 3 bytes (tamanho da música + ID)
- `0003h`: 9×16 voices MSX-AUDIO
- `00A3h`: instrumentos/volume MSX-MUSIC
- `00C3h`: que canal vai para que chip
- a seguir: patterns “crunchados”

`.MBK`: 56 bytes de endereços (14 samples × u16) + ADPCM.

**USER vs EDIT:** EDIT começa muitas vezes com `FFh` extra e leva todos os canais; replayers velhos **só USER**. RoboPlay e o MBM Player Windows (Remco Schrijvers, 2026) tocam os dois.

### 7. OPLL em C (o post Fusion-C)

https://moltsxalats.wixsite.com/fusionc/post/programming-opll-msx-music

- OUT `7Ch` (registo), `7Dh` (dado), waits Yamaha (R800 precisa de mais NOPs).
- Na MSXgl já existe `msx-music` — preferir isso a `__naked` do Fusion-C.
- Tempo: **não há IRQ do OPLL**; o player anda no **VBlank 50/60 Hz**.
- Tabela de F-number (8 oitavas × 12 notas) é a mesma no BiFi, RoboPlay e no `fm_mbm_fusionc`.

MSXgl: `engine/src/msx-music.h`, sample `s_lvgm` (FM via LVGM, outro formato).

### 8. Players no PC (para testar ficheiros)

- Winamp / NEZplug: `mbm2kssx3.zip` em nezplug.sourceforge.net  
- https://mbmplayer.mostlyvibes.com — Windows, USER+EDIT, só MBM 1.4 (Remco / Mostly Vibes, 2026)  
- MBMPlay-SMS (GitHub HerrSchatten) — ASM do Remco+BiFi portado ao Master System; API `MBMPlay` / `MBMFrame` útil como contrato C.

---

## Estratégia deste projeto (MSXgl + DOS2)

1. **Alvo:** `.COM` DOS2, `Machine = "2"`, FM-PAC + Music Module no emulador (`EmulMSXMusic` / `EmulMSXAudio`).
2. **Não reinventar o parser:** partir do `mbm.h` do RoboPlay **ou** do `fm_mbm_fusionc`, e ligar o I/O ao `dos.h` da MSXgl.
3. **Som nativo 1.4:** `MSX_Music_*` + módulo MSX-AUDIO da MSXgl (não OPL4), a não ser que queiramos um modo “toca no Moonsound” depois.
4. **ISR:** `Halt()` / hook VBlank; um `MBM_Tick()` por frame, como o `MBMFrame` do BiFi.
5. **CLI:** `MOON_PLA song.MBM [kit.MBK]` — o stub já documenta isso.
6. **Ficheiros USER** primeiro; EDIT se o loader do RoboPlay for fácil de copiar.
7. **MBK** só quando o AUDIO estiver detectado; senão FM-PAC sozinho (muita música 1.4 vive só de MUSIC).

Licenças: tracker 1.4 foi declarado freeware pelos autores; RoboPlay é BSD/WTFPL; o ASM da Sunrise/Moonsoft historicamente veio com o pacote. Ao incorporar ASM, manter os créditos Remco Schrijvers / BiFi / Sunrise.

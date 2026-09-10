// DOS2 player for MoonBlaster 1.4 FM (YM2413 / MSX-MUSIC).
// Y8950 / MSX-AUDIO FM when the chip is present (ADPCM .MBK not yet).
// Replay: port of gitlab.com/brossaip/fm_mbm_fusionc + official MBPLAY.SRC
//
// Internal YM2413 (MSX2+ / turbo R): I/O 7Ch/7Dh is always on.
// PAC2OPLL (SW-M004): enable bit 0 of 7FF6h in the cartridge slot.
#include "msxgl.h"
#include "dos.h"
#include "mbm_opll.h"

#define MAX_SONGS 48
#define NAME_LEN  64

static u8 g_info;
static u8 g_loop;
static u8 g_help;
static c8 g_file[NAME_LEN];
static c8 g_list[MAX_SONGS][NAME_LEN];
static u8 g_nsongs;

static void Out(const c8* s);

static void SelectSlotPage1(u8 slotId) __naked
{
	slotId;
	__asm
		di
		ld		(#0xBFFE), sp
		ld		sp, #0xB000
		ld		c, a

		in		a, (#0xA8)
		and		a, #0xF3
		ld		b, a
		ld		a, c
		and		a, #0x03
		add		a, a
		add		a, a
		or		a, b
		ld		d, a
		out		(#0xA8), a

		bit		7, c
		jr		z, 0001$

		in		a, (#0xA8)
		and		a, #0x3F
		ld		b, a
		ld		a, c
		and		a, #0x03
		rrca
		rrca
		and		a, #0xC0
		or		a, b
		out		(#0xA8), a

		ld		a, (#0xFFFF)
		cpl
		and		a, #0xF3
		ld		b, a
		ld		a, c
		and		a, #0x0C
		or		a, b
		ld		(#0xFFFF), a

		ld		a, d
		out		(#0xA8), a
0001$:
		ld		sp, (#0xBFFE)
		ei
		ret
	__endasm;
}

static bool MatchAt(u16 addr, const c8* s)
{
	while (*s)
	{
		if (Peek(addr++) != (u8)*s++)
			return FALSE;
	}
	return TRUE;
}

static void EnableExternalFMPAC(void)
{
	u8 saved = Sys_GetPageSlot(1);
	u8 p, s, nsub, id;
	u8 found = 0;

	for (p = 0; p < 4; p++)
	{
		nsub = (g_EXPTBL[p] & 0x80) ? 4 : 1;
		for (s = 0; s < nsub; s++)
		{
			id = (nsub == 4) ? (u8)(0x80 | (s << 2) | p) : p;
			SelectSlotPage1(id);
			if (MatchAt(0x4018, "APRLOPLL"))
			{
				found = 1;
				goto restore;
			}
		}
	}
	for (p = 0; p < 4; p++)
	{
		nsub = (g_EXPTBL[p] & 0x80) ? 4 : 1;
		for (s = 0; s < nsub; s++)
		{
			id = (nsub == 4) ? (u8)(0x80 | (s << 2) | p) : p;
			SelectSlotPage1(id);
			if (MatchAt(0x4018, "PAC2OPLL"))
			{
				Poke(0x7FF6, (u8)(Peek(0x7FF6) | 0x01));
				found = 2;
				goto restore;
			}
		}
	}
restore:
	SelectSlotPage1(saved);
	if (g_info)
	{
		if (found == 1)
			Out("OPLL: APRLOPLL (I/O 7C)\n");
		else if (found == 2)
			Out("OPLL: PAC2OPLL (7FF6h)\n");
		else
			Out("OPLL: 7C/7D\n");
	}
}

/* Y8950 at C0h/C1h. 0FFh = empty bus. Skip Moonsound/OPL4 (WAVE). */
static u8 DetectMSXAudio(void)
{
	u8 saved, p, s, nsub, id;
	u8 st, audio_rom, opl4;

	st = g_MSXAudio_IndexPort;
	if (st == 0xFF)
		return 0;

	saved = Sys_GetPageSlot(1);
	audio_rom = 0;
	opl4 = 0;
	for (p = 0; p < 4; p++)
	{
		nsub = (g_EXPTBL[p] & 0x80) ? 4 : 1;
		for (s = 0; s < nsub; s++)
		{
			id = (nsub == 4) ? (u8)(0x80 | (s << 2) | p) : p;
			SelectSlotPage1(id);
			if (MatchAt(0x4018, "AUDIO") || MatchAt(0x4000, "AUDIO"))
				audio_rom = 1;
			if (MatchAt(0x4018, "WAVE") || MatchAt(0x4000, "WAVE"))
				opl4 = 1;
		}
	}
	SelectSlotPage1(saved);
	if (opl4 && !audio_rom)
		return 0;
	return 1;
}

static void Out(const c8* s)
{
	while (*s)
	{
		if (*s == '\n')
			DOS_CharOutput('\r');
		DOS_CharOutput(*s++);
	}
}

static void OutU8(u8 v)
{
	c8 buf[4];
	u8 i = 0;
	if (v >= 100)
	{
		buf[i++] = (c8)('0' + (v / 100));
		v = (u8)(v % 100);
		buf[i++] = (c8)('0' + (v / 10));
		buf[i++] = (c8)('0' + (v % 10));
	}
	else if (v >= 10)
	{
		buf[i++] = (c8)('0' + (v / 10));
		buf[i++] = (c8)('0' + (v % 10));
	}
	else
		buf[i++] = (c8)('0' + v);
	buf[i] = 0;
	Out(buf);
}

static c8 Up(c8 c)
{
	if ((c >= 'a') && (c <= 'z'))
		c -= 32;
	return c;
}

static u8 IsFlag(const c8* a, const c8* name)
{
	if ((*a != '-') && (*a != '/'))
		return 0;
	a++;
	while (*name)
	{
		if (Up(*a++) != Up(*name++))
			return 0;
	}
	return (*a == 0) ? 1 : 0;
}

static void AddExt(void)
{
	u8 i, dot, slash;

	if (g_file[0] == 0)
		return;
	dot = 0;
	slash = 0;
	for (i = 0; g_file[i]; i++)
	{
		if ((g_file[i] == '\\') || (g_file[i] == '/'))
		{
			slash = i;
			dot = 0;
		}
		else if (g_file[i] == '.')
			dot = i;
	}
	if (dot > slash)
		return;
	if (i > 60)
		return;
	g_file[i++] = '.';
	g_file[i++] = 'M';
	g_file[i++] = 'B';
	g_file[i++] = 'M';
	g_file[i] = 0;
}

static void ParseArgs(u8 argc, const c8** argv)
{
	u8 i, n;

	g_info = 0;
	g_loop = 0;
	g_help = 0;
	g_file[0] = 0;

	for (i = 0; i < argc; i++)
	{
		const c8* a = argv[i];
		if (IsFlag(a, "help") || IsFlag(a, "h") || IsFlag(a, "?"))
			g_help = 1;
		else if (IsFlag(a, "info"))
			g_info = 1;
		else if (IsFlag(a, "loop"))
			g_loop = 1;
		else if ((a[0] == '-') || (a[0] == '/'))
			g_help = 1;
		else if (g_file[0] == 0)
		{
			for (n = 0; a[n] && (n < 62); n++)
				g_file[n] = a[n];
			g_file[n] = 0;
		}
	}
	AddExt();
}

static void CopyZ(c8* dst, const c8* src, u8 max)
{
	u8 i;
	for (i = 0; src[i] && (i < (u8)(max - 1)); i++)
		dst[i] = src[i];
	dst[i] = 0;
}

static u8 IsMBMName(const c8* name)
{
	u8 i, dot = 0;
	for (i = 0; name[i]; i++)
	{
		if (name[i] == '.')
			dot = i;
	}
	if (!dot || !name[dot + 1])
		return 1;
	return (Up(name[dot + 1]) == 'M')
		&& (Up(name[dot + 2]) == 'B')
		&& (Up(name[dot + 3]) == 'M')
		&& (name[dot + 4] == 0);
}

/* Path prefix up to the last drive/dir separator. DOS2/Nextor: \ (yen/won). */
static void DirPrefix(const c8* spec, c8* prefix)
{
	u8 i, last = 0;
	for (i = 0; spec[i]; i++)
	{
		if ((spec[i] == '\\') || (spec[i] == '/') || (spec[i] == ':'))
			last = (u8)(i + 1);
	}
	for (i = 0; i < last; i++)
		prefix[i] = spec[i];
	prefix[i] = 0;
}

/* Expand * and ? with _FFIRST/_FNEXT (MSX-DOS 2 / Nextor). Order = DIR. */
static u8 ExpandList(void)
{
	DOS_FIB* fib;
	c8 prefix[52];
	u8 n = 0, i, j;

	DirPrefix(g_file, prefix);
	fib = DOS_FindFirstEntry(g_file, 0);
	while (fib && (n < MAX_SONGS))
	{
		if (!(fib->Attribute & (ATTR_FOLDER | ATTR_VOLUME | ATTR_DEVICE))
			&& IsMBMName((const c8*)fib->Filename))
		{
			i = 0;
			for (j = 0; prefix[j] && (i < (NAME_LEN - 14)); j++, i++)
				g_list[n][i] = prefix[j];
			for (j = 0; fib->Filename[j] && (i < (NAME_LEN - 1)); j++, i++)
				g_list[n][i] = (c8)fib->Filename[j];
			g_list[n][i] = 0;
			n++;
		}
		fib = DOS_FindNextEntry();
	}
	g_nsongs = n;
	if (n == 0)
	{
		const c8* p = g_file;
		while (*p)
		{
			if ((*p == '*') || (*p == '?'))
				return 0;
			p++;
		}
		CopyZ(g_list[0], g_file, NAME_LEN);
		g_nsongs = 1;
	}
	return g_nsongs;
}

static u8 KeyHit(u8 key, u8* prev)
{
	u8 now = Keyboard_IsKeyPressed(key) ? 1 : 0;
	u8 hit = (now && !*prev) ? 1 : 0;
	*prev = now;
	return hit;
}

static void PrintHelp(void)
{
	Out("MOONPLAY 1.0  MoonBlaster 1.4  MSX-MUSIC\n");
	Out("Uso: MOONPLAY [opcoes] ficheiro\n");
	Out("  ficheiro  LUCIFER.MBM  *.MBM  A????.MBM\n");
	Out("            50HZ\\*.MBM   (wildcards DOS2/Nextor)\n");
	Out("  -info /INFO   Titulo e dados\n");
	Out("  -loop /LOOP   Repete (musica ou lista)\n");
	Out("  -help /HELP   Esta ajuda\n");
	Out("ESPACO  pausa/continua\n");
	Out("ESC     para\n");
	Out("Default: uma vez, sem texto.\n");
	Out("MSX-AUDIO FM se o Y8950 existir. .MBK ainda nao.\n");
}

static bool Is50Hz(void)
{
	return (g_RG09SAV & 0x02) != 0;
}

static void PrintTrackInfo(void)
{
	c8 buf[42];
	u8 i, j, k;

	for (i = 0; i < 41; i++)
	{
		c8 c = capcalera.track_name[i];
		if ((c < 32) || (c > 126))
			break;
		buf[i] = c;
	}
	while (i && (buf[i - 1] == ' '))
		i--;
	buf[i] = 0;

	for (j = 0; buf[j]; j++)
	{
		if ((buf[j] == '-') && (buf[j + 1] == '>'))
		{
			k = j;
			if (k && (buf[k - 1] == ' '))
				k--;
			buf[k] = 0;
			j += 2;
			while (buf[j] == ' ')
				j++;
			Out("Title:  ");
			Out(buf);
			Out("\nAuthor: ");
			Out(&buf[j]);
			Out("\n");
			goto rest;
		}
	}
	Out("Title:  ");
	Out(buf);
	Out("\n");
rest:
	Out("File:   ");
	Out(g_file);
	Out("\nTempo:  ");
	OutU8(capcalera.start_tempo);
	Out("\nLength: ");
	OutU8((u8)(capcalera.song_length + 1));
	Out(" pos\nLoop:   ");
	if (capcalera.loop_position == LOOP_OFF)
		Out("off");
	else
		OutU8(capcalera.loop_position);
	Out("\nMode:   ");
	if (capcalera.sustain & 0x20)
		Out("6ch + drums");
	else
		Out("9ch melody");
	{
		u8 n, mus, aud;
		c8 kit[9];
		mus = 0;
		aud = 0;
		for (n = 0; n < NR_OF_CHANNELS; n++)
		{
			if ((capcalera.channel_chip_set[n] & CHANNEL_MUSIC)
				&& (!((capcalera.sustain & 0x20) && (n >= 6))))
				mus++;
			if (capcalera.channel_chip_set[n] & CHANNEL_AUDIO)
				aud++;
		}
		Out("\nChips:  MUSIC ");
		OutU8(mus);
		Out("ch  AUDIO ");
		OutU8(aud);
		Out("ch");
		Out(MBM_HasAudio() ? " (Y8950)\n" : " (Y8950 ausente)\n");
		for (n = 0; n < 8; n++)
		{
			c8 c = capcalera.sample_kit_name[n];
			if ((c < 32) || (c > 126))
				c = ' ';
			kit[n] = c;
		}
		kit[8] = 0;
		n = 8;
		while (n && (kit[n - 1] == ' '))
			n--;
		kit[n] = 0;
		Out("Sample: ");
		Out(kit[0] ? kit : "(none)");
		Out(" (MBK nao tocado)\n");
	}
	Out("VBlank: ");
	if (Is50Hz())
		Out("50Hz (tick x6/5)\n");
	else
		Out("60Hz\n");
}

u8 main(u8 argc, const c8** argv)
{
	u8 extra50;
	u8 pal50;
	u8 song;
	u8 paused;
	u8 prev_sp;
	u8 prev_esc;
	u8 played;
	u8 loop_song;

	ParseArgs(argc, argv);

	if (g_help || (g_file[0] == 0))
	{
		PrintHelp();
		return (g_help || (argc == 0)) ? 0 : 1;
	}

	EnableExternalFMPAC();
	MBM_SetAudio(DetectMSXAudio());
	if (g_info)
	{
		if (MBM_HasAudio())
			Out("AUDIO: Y8950 (C0/C1)\n");
		else
			Out("AUDIO: ausente\n");
	}

	if (!ExpandList())
	{
		Out("Nao abriu: ");
		Out(g_file);
		Out("\n");
		return 1;
	}

	pal50 = Is50Hz() ? 1 : 0;
	loop_song = (g_loop && (g_nsongs == 1)) ? 1 : 0;
	song = 0;
	played = 0;

	for (;;)
	{
		CopyZ(g_file, g_list[song], NAME_LEN);
		if (!MBM_LoadFile(g_file))
		{
			if ((g_nsongs == 1) || g_info)
			{
				Out("Nao abriu: ");
				Out(g_file);
				Out("\n");
			}
			goto next_song;
		}

		MBM_SetLoop(loop_song);
		MBM_Start();
		played = 1;
		if (g_info)
			PrintTrackInfo();

		extra50 = 0;
		paused = 0;
		prev_sp = Keyboard_IsKeyPressed(KEY_SPACE) ? 1 : 0;
		prev_esc = Keyboard_IsKeyPressed(KEY_ESC) ? 1 : 0;
		while (MBM_IsPlaying())
		{
			Halt();
			if (KeyHit(KEY_ESC, &prev_esc))
			{
				MBM_Stop();
				return 0;
			}
			if (KeyHit(KEY_SPACE, &prev_sp))
			{
				paused = (u8)(paused ? 0 : 1);
				if (paused)
					MBM_Mute();
			}
			if (paused)
				continue;
			MBM_Tick();
			if (pal50)
			{
				extra50++;
				if (extra50 == 5)
				{
					extra50 = 0;
					MBM_Tick();
				}
			}
		}
		MBM_Stop();

	next_song:
		song++;
		if (song >= g_nsongs)
		{
			if (g_loop && (g_nsongs > 1))
				song = 0;
			else
				break;
		}
	}

	if (!played)
	{
		Out("Nao abriu: ");
		Out(g_file);
		Out("\n");
		return 1;
	}
	return 0;
}

// DOS2 player for MoonBlaster 1.4 FM (YM2413 / MSX-MUSIC).
//
// Internal YM2413 (MSX2+ / turbo R): I/O 7Ch/7Dh is always on.
// Do not CALL INIOPL or poke 7FF6h on Panasonic 2+ / turbo R.
#include "msxgl.h"
#include "dos.h"
#include "mbm_opll.h"

static void Out(const c8* s);
static void OutHex(u8 v);
static void OutU8(u8 v);

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

static void EnableExternalFMPAC(u8 verbose)
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
				if (verbose)
				{
					Out("OPLL: APRLOPLL slot=");
					OutHex(id);
					Out("h (I/O 7C always on)\n");
				}
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
				if (verbose)
				{
					Out("OPLL: PAC2OPLL slot=");
					OutHex(id);
					Out("h 7FF6h I/O enabled\n");
				}
				found = 2;
				goto restore;
			}
		}
	}
restore:
	SelectSlotPage1(saved);
	if (!found && verbose)
		Out("OPLL: no ROM signature, using 7C/7D\n");
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

static void OutHex(u8 v)
{
	u8 n = v >> 4;
	DOS_CharOutput((n < 10) ? ('0' + n) : ('A' + n - 10));
	n = v & 0x0F;
	DOS_CharOutput((n < 10) ? ('0' + n) : ('A' + n - 10));
}

static void OutU8(u8 v)
{
	u8 h = v / 100;
	u8 t = (v / 10) % 10;
	u8 o = v % 10;
	if (h)
		DOS_CharOutput('0' + h);
	if (h || t)
		DOS_CharOutput('0' + t);
	DOS_CharOutput('0' + o);
}

static bool Is50Hz(void)
{
	return (g_RG09SAV & 0x02) != 0;
}

static void PrintTrackInfo(void)
{
	c8 buf[42];
	u8 i, j, k;
	c8 kit[9];

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
	Out("Tempo:  ");
	OutU8(capcalera.start_tempo);
	Out("   Length: ");
	OutU8((u8)(capcalera.song_length + 1));
	Out(" pos\nLoop:   ");
	if (capcalera.loop_position == 0xFF)
		Out("start");
	else
		OutU8(capcalera.loop_position);
	Out("\nMusic:  ");
	OutU8(g_mbm_nch);
	Out(g_mbm_rhythm ? " ch + FM drums\n" : " melody channels\n");
	for (i = 0; i < 8; i++)
	{
		c8 c = capcalera.sample_kit_name[i];
		if ((c < 32) || (c > 126))
			c = ' ';
		kit[i] = c;
	}
	kit[8] = 0;
	i = 8;
	while (i && (kit[i - 1] == ' '))
		i--;
	kit[i] = 0;
	Out("Sample: ");
	Out(kit[0] ? kit : "(none)");
	Out("\nBytes:  ");
	OutHex((u8)(g_mbm_bytes >> 8));
	OutHex((u8)g_mbm_bytes);
	Out("h\n");
	if (Is50Hz())
		Out("VBlank 50Hz (tick x6/5 so tempo matches 60Hz)\n");
	else
		Out("VBlank 60Hz\n");
}

static void PrintHelp(void)
{
	Out("MOONPLAY  MoonBlaster 1.4  MSX-MUSIC / YM2413\n");
	Out("Usage: MOONPLAY [options] file[.MBM]\n\n");
	Out("  -h   this help\n");
	Out("  -i   print song information\n");
	Out("  -l   loop until ESC (default: play once)\n\n");
	Out("ESC stops. USER-format .MBM only.\n");
	Out("MSX-AUDIO / .MBK samples are not played.\n");
}

static c8 ToUpper(c8 c)
{
	if ((c >= 'a') && (c <= 'z'))
		c -= 32;
	return c;
}

void main(u8 argc, const c8** argv)
{
	c8 name[13];
	u8 extra50;
	u8 pal50;
	u8 want_info = 0;
	u8 want_loop = 0;
	u8 want_help = 0;
	u8 a, n;
	const c8* p;

	name[0] = 0;

	for (a = 0; a < argc; a++)
	{
		p = argv[a];
		if ((p[0] == '-') || (p[0] == '/'))
		{
			for (n = 1; p[n]; n++)
			{
				switch (ToUpper(p[n]))
				{
				case 'H':
				case '?':
					want_help = 1;
					break;
				case 'I':
					want_info = 1;
					break;
				case 'L':
					want_loop = 1;
					break;
				default:
					Out("Unknown option: -");
					DOS_CharOutput(p[n]);
					Out("\n");
					PrintHelp();
					DOS_Exit(1);
				}
			}
		}
		else if (name[0] == 0)
		{
			for (n = 0; p[n] && (n < 12); n++)
				name[n] = p[n];
			name[n] = 0;
		}
	}

	if (want_help)
	{
		PrintHelp();
		DOS_Exit(0);
	}

	if (name[0] == 0)
	{
		PrintHelp();
		DOS_Exit(1);
	}

	EnableExternalFMPAC(want_info);

	if (!MBM_LoadFile(name))
	{
		Out("Cannot open ");
		Out(name);
		Out("\n");
		DOS_Exit(1);
	}

	MBM_SetLoop(want_loop);
	MBM_Start();

	if (want_info)
	{
		Out("File:   ");
		Out(name);
		Out("\n");
		PrintTrackInfo();
		Out("\n");
	}

	Out("Playing ");
	Out(name);
	if (want_loop)
		Out("  (loop, ESC=stop)\n");
	else
		Out("  (ESC=stop)\n");

	pal50 = Is50Hz() ? 1 : 0;
	extra50 = 0;
	while (!Keyboard_IsKeyPressed(KEY_ESC) && !MBM_IsFinished())
	{
		Halt();
		MBM_Tick();
		if (pal50)
		{
			extra50++;
			if (extra50 == 5)
			{
				extra50 = 0;
				if (!MBM_IsFinished())
					MBM_Tick();
			}
		}
	}

	MBM_Stop();
	DOS_Exit(0);
}

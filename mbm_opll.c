#include "mbm_opll.h"
#include "dos.h"
#include "memory.h"

#pragma disable_warning 85
#pragma disable_warning 59

#define STEP_COLS 13
#define DRUM_COL  11
#define CMD_COL   12
#define MAX_STEP  15

typedef struct {
	u8  mode;
	i8  pitch;
	i8  detune;
	u8  note;
	u8  inst_idx;
	u8  inst;
	u8  vol;
	u8  lo;
	u8  hi;
	u8  bright;
	u8  a_lo;
	u8  a_hi;
	u8  a_vol;
	u8  a_bright;
	u8  a_inst;
} CHAN;

MBM_HEADER capcalera;
uint8_t songFile[80 * 16 * 13 + 80 * 2 + 200 + 0x178];
u16 g_mbm_bytes;

static CHAN    g_ch[NR_OF_CHANNELS];
static u8      g_step_buf[STEP_COLS];
static u8      g_chip[STEREO_SETTING_SIZE];
static u8      g_speed;
static u8      g_speed_cnt;
static u8      g_pos;
static u8      g_step;
static u8      g_transpose;
static u8      g_playing;
static u8      g_loop;
static u8      g_rhythm;
static u8      g_melodic;
static u8      g_audio;
static u16     g_pos_off;
static u16     g_pat_off;
static u16     g_pat_ptr;

/* Y8950 operator regs per channel (mbplay.src mmrgad). */
static const u8 g_mmrgad[9][9] = {
	{ 0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xC0 },
	{ 0x21, 0x24, 0x41, 0x44, 0x61, 0x64, 0x81, 0x84, 0xC1 },
	{ 0x22, 0x25, 0x42, 0x45, 0x62, 0x65, 0x82, 0x85, 0xC2 },
	{ 0x28, 0x2B, 0x48, 0x4B, 0x68, 0x6B, 0x88, 0x8B, 0xC3 },
	{ 0x29, 0x2C, 0x49, 0x4C, 0x69, 0x6C, 0x89, 0x8C, 0xC4 },
	{ 0x2A, 0x2D, 0x4A, 0x4D, 0x6A, 0x6D, 0x8A, 0x8D, 0xC5 },
	{ 0x30, 0x33, 0x50, 0x53, 0x70, 0x73, 0x90, 0x93, 0xC6 },
	{ 0x31, 0x34, 0x51, 0x54, 0x71, 0x74, 0x91, 0x94, 0xC7 },
	{ 0x32, 0x35, 0x52, 0x55, 0x72, 0x75, 0x92, 0x95, 0xC8 }
};

// Official MBPLAY pafreq table (F-num 9-bit + block in bits 1-3 of the high byte).
// Index = note 1..96 minus 1. B is 12, 24, 36, ...
static const u16 g_freq[96] = {
	0x00AD, 0x00B7, 0x00C2, 0x00CD, 0x00D9, 0x00E6,
	0x00F4, 0x0103, 0x0112, 0x0122, 0x0134, 0x0146,
	0x02AD, 0x02B7, 0x02C2, 0x02CD, 0x02D9, 0x02E6,
	0x02F4, 0x0303, 0x0312, 0x0322, 0x0334, 0x0346,
	0x04AD, 0x04B7, 0x04C2, 0x04CD, 0x04D9, 0x04E6,
	0x04F4, 0x0503, 0x0512, 0x0522, 0x0534, 0x0546,
	0x06AD, 0x06B7, 0x06C2, 0x06CD, 0x06D9, 0x06E6,
	0x06F4, 0x0703, 0x0712, 0x0722, 0x0734, 0x0746,
	0x08AD, 0x08B7, 0x08C2, 0x08CD, 0x08D9, 0x08E6,
	0x08F4, 0x0903, 0x0912, 0x0922, 0x0934, 0x0946,
	0x0AAD, 0x0AB7, 0x0AC2, 0x0ACD, 0x0AD9, 0x0AE6,
	0x0AF4, 0x0B03, 0x0B12, 0x0B22, 0x0B34, 0x0B46,
	0x0CAD, 0x0CB7, 0x0CC2, 0x0CCD, 0x0CD9, 0x0CE6,
	0x0CF4, 0x0D03, 0x0D12, 0x0D22, 0x0D34, 0x0D46,
	0x0EAD, 0x0EB7, 0x0EC2, 0x0ECD, 0x0ED9, 0x0EE6,
	0x0EF4, 0x0F03, 0x0F12, 0x0F22, 0x0F34, 0x0F46
};

static const i8 g_mod[10] = { 1, 2, 2, -2, -2, -1, -2, -2, 2, 2 };

void WriteOPLLreg(char reg, char value) __naked
{
	reg;
	value;
	__asm
		ld		d, a
		ld		e, l
		ld		a, d
		out		(#0x7C), a
		ld		b, #8
0001$:
		djnz	0001$
		ld		a, e
		out		(#0x7D), a
		ld		b, #84
0002$:
		djnz	0002$
		ret
	__endasm;
}

void WriteAUDIOreg(char reg, char value) __naked
{
	reg;
	value;
	__asm
		ld		d, a
		ld		e, l
		ld		a, d
		out		(#0xC0), a
		ld		a, e
		out		(#0xC1), a
		ex		(sp), hl
		ex		(sp), hl
		ret
	__endasm;
}

static u8 MusicActive(u8 ch)
{
	if (!(g_chip[ch] & CHANNEL_MUSIC))
		return 0;
	if (g_rhythm && (ch >= 6))
		return 0;
	return 1;
}

static u8 AudioActive(u8 ch)
{
	if (!g_audio)
		return 0;
	if (!(g_chip[ch] & CHANNEL_AUDIO))
		return 0;
	return 1;
}

static u8 ApplyTranspose(u8 note)
{
	u8 n = note + g_transpose;
	if (n > (NOTE_ON + DEFAULT_TRANSPOSE))
		n -= NOTE_ON;
	else if (n < (DEFAULT_TRANSPOSE + 1))
		n += NOTE_ON;
	n -= DEFAULT_TRANSPOSE;
	if ((n < 1) || (n > NOTE_ON))
		n = 1;
	return n;
}

static void FreqToRegs(u8 ch, u8 note, u8 key, u8 sus)
{
	u16 f;
	u8  lo, hi;

	f = g_freq[note - 1];
	lo = (u8)f + (u8)g_ch[ch].detune;
	hi = (u8)(f >> 8);
	if (sus)
		hi |= 0x20;
	WriteOPLLreg(0x10 + ch, lo);
	WriteOPLLreg(0x20 + ch, hi);
	if (key)
	{
		hi |= 0x10;
		WriteOPLLreg(0x20 + ch, hi);
	}
	g_ch[ch].lo = lo;
	g_ch[ch].hi = hi;
	g_ch[ch].note = note;
}

static void WriteFreq(u8 ch)
{
	WriteOPLLreg(0x10 + ch, g_ch[ch].lo);
	WriteOPLLreg(0x20 + ch, g_ch[ch].hi);
}

static void AddFreq(u8 ch, i16 delta)
{
	u16 v;

	v = ((u16)(g_ch[ch].hi & 0x0F) << 8) | g_ch[ch].lo;
	v += (u16)delta;
	g_ch[ch].lo = (u8)v;
	g_ch[ch].hi = (u8)((g_ch[ch].hi & 0x30) | ((v >> 8) & 0x0F));
	WriteFreq(ch);
}

static void WriteAudioFreq(u8 ch)
{
	WriteAUDIOreg((char)(0xA0 + ch), g_ch[ch].a_lo);
	WriteAUDIOreg((char)(0xB0 + ch), g_ch[ch].a_hi);
}

static void AddAudioFreq(u8 ch, i16 delta)
{
	u16 v;

	v = ((u16)(g_ch[ch].a_hi & 0x1F) << 8) | g_ch[ch].a_lo;
	v += (u16)delta;
	g_ch[ch].a_lo = (u8)v;
	g_ch[ch].a_hi = (u8)((g_ch[ch].a_hi & 0x20) | ((v >> 8) & 0x1F));
	WriteAudioFreq(ch);
}

/* pafreq << 1, detune*2 on the low byte, then DEC HL (mbplay.src mmple). */
static void AudioFreqToRegs(u8 ch, u8 note, u8 key)
{
	u16 f;
	u8  lo, hi, l;
	i8  d;

	f = g_freq[note - 1];
	f <<= 1;
	d = g_ch[ch].detune;
	l = (u8)f;
	l += (u8)d;
	l += (u8)d;
	f = (f & 0xFF00) | l;
	f--;
	lo = (u8)f;
	hi = (u8)(f >> 8);
	WriteAUDIOreg((char)(0xA0 + ch), lo);
	WriteAUDIOreg((char)(0xB0 + ch), hi);
	if (key)
	{
		hi |= 0x20;
		WriteAUDIOreg((char)(0xB0 + ch), hi);
	}
	g_ch[ch].a_lo = lo;
	g_ch[ch].a_hi = hi;
	g_ch[ch].note = note;
}

static void AudioNoteOff(u8 ch)
{
	g_ch[ch].a_hi &= (u8)~0x20;
	WriteAudioFreq(ch);
}

static void SetAudioInstrument(u8 ch, u8 idx)
{
	u8 i;
	u8 *v;

	if ((idx < 1) || (idx > 16))
		return;
	v = capcalera.voice_data_audio[idx - 1];
	g_ch[ch].a_inst = idx;
	g_ch[ch].a_bright = v[2];
	g_ch[ch].a_vol = v[3];
	for (i = 0; i < 9; i++)
		WriteAUDIOreg((char)g_mmrgad[ch][i], v[i]);
}

static void SetAudioVolume(u8 ch, u8 val)
{
	g_ch[ch].a_vol = (u8)((g_ch[ch].a_vol & 0xC0) | (val & 0x3F));
	WriteAUDIOreg((char)g_mmrgad[ch][3], g_ch[ch].a_vol);
}

static void AudioBrightness(u8 ch, i8 delta)
{
	u8 v;

	v = (u8)((g_ch[ch].a_bright & 0x3F) + (u8)delta);
	v = (u8)((g_ch[ch].a_bright & 0xC0) | (v & 0x3F));
	g_ch[ch].a_bright = v;
	WriteAUDIOreg((char)g_mmrgad[ch][2], v);
}

static void WriteInstVol(u8 ch)
{
	u8 inst = (g_ch[ch].inst > 15) ? 0 : g_ch[ch].inst;
	WriteOPLLreg(0x30 + ch, (inst << 4) | (g_ch[ch].vol & 0x0F));
}

static void LoadOriginal(u8 inst)
{
	u8 i;
	u8 *p = capcalera.original_instrument_data[inst - 16];
	for (i = 0; i < 8; i++)
		WriteOPLLreg(i, p[i]);
}

static void SetInstrument(u8 ch, u8 idx)
{
	u8 inst, vol;

	if ((idx < 1) || (idx > 16))
		return;
	inst = capcalera.instrument_list_music[idx - 1].instrument;
	vol  = capcalera.instrument_list_music[idx - 1].volume;
	g_ch[ch].inst_idx = idx;
	g_ch[ch].inst = inst;
	g_ch[ch].vol = vol;
	if (inst > 15)
	{
		LoadOriginal(inst);
		g_ch[ch].bright = capcalera.original_instrument_data[inst - 16][2];
	}
	WriteInstVol(ch);
}

static void SetDrumset(u8 set)
{
	if (set > 2)
		set = 0;
	WriteOPLLreg(0x16, (u8)capcalera.drum_frequencies_music[set][0]);
	WriteOPLLreg(0x26, (u8)(capcalera.drum_frequencies_music[set][0] >> 8));
	WriteOPLLreg(0x17, (u8)capcalera.drum_frequencies_music[set][1]);
	WriteOPLLreg(0x27, (u8)(capcalera.drum_frequencies_music[set][1] >> 8));
	WriteOPLLreg(0x18, (u8)capcalera.drum_frequencies_music[set][2]);
	WriteOPLLreg(0x28, (u8)(capcalera.drum_frequencies_music[set][2] >> 8));
}

static void NextPosition(void)
{
	u8  pat;
	u16 off;

	if (g_pos == capcalera.song_length)
	{
		if (g_loop)
		{
			if (capcalera.loop_position != LOOP_OFF)
				g_pos = capcalera.loop_position;
			else
				g_pos = 0;
		}
		else
		{
			g_playing = 0;
			return;
		}
	}
	else
		g_pos++;

	pat = songFile[g_pos_off + g_pos];
	if (pat == 0)
		pat = 1;
	off = g_pat_off + (u16)(pat - 1) * 2;
	g_pat_ptr = (u16)songFile[off] | ((u16)songFile[off + 1] << 8);
}

static void DecrunchStep(void)
{
	u8 col = 0;

	g_step++;
	if (g_step > MAX_STEP)
	{
		g_step = 0;
		NextPosition();
		if (!g_playing)
			return;
	}

	while (col < STEP_COLS)
	{
		u8 b;
		if (g_pat_ptr >= (u16)sizeof(songFile))
		{
			g_playing = 0;
			return;
		}
		b = songFile[g_pat_ptr++];
		if (b >= 243)
		{
			u8 n = b - 242;
			while (n && (col < STEP_COLS))
			{
				g_step_buf[col++] = 0;
				n--;
			}
		}
		else
			g_step_buf[col++] = b;
	}

	for (col = 0; col < NR_OF_CHANNELS; col++)
	{
		if (g_step_buf[col] && (g_step_buf[col] <= NOTE_ON)
			&& (MusicActive(col) || AudioActive(col)))
			g_ch[col].mode = FREQ_NORMAL;
	}
}

static void HandlePitchMod(void)
{
	u8 i;

	for (i = 0; i < NR_OF_CHANNELS; i++)
	{
		if (g_ch[i].mode == FREQ_NORMAL)
			continue;
		if (g_ch[i].mode == FREQ_PITCH_BEND)
		{
			if (MusicActive(i))
				AddFreq(i, g_ch[i].pitch);
			if (AudioActive(i))
				AddAudioFreq(i, (i16)g_ch[i].pitch << 1);
		}
		else if (g_ch[i].mode >= FREQ_MODULATION)
		{
			u8 idx = g_ch[i].mode - FREQ_MODULATION;
			if (MusicActive(i))
				AddFreq(i, g_mod[idx]);
			if (AudioActive(i))
				AddAudioFreq(i, (i16)g_mod[idx] << 1);
			idx++;
			if (idx >= 10)
				idx = 0;
			g_ch[i].mode = (u8)(FREQ_MODULATION + idx);
		}
	}
}

static void NoteOn(u8 ch)
{
	u8 note = ApplyTranspose(g_step_buf[ch]);
	g_ch[ch].mode = FREQ_NORMAL;
	if (MusicActive(ch))
		FreqToRegs(ch, note, 1, 0);
	if (AudioActive(ch))
		AudioFreqToRegs(ch, note, 1);
}

static void NoteOff(u8 ch, u8 sus)
{
	u8 hi;
	g_ch[ch].mode = FREQ_NORMAL;
	if (MusicActive(ch))
	{
		hi = (u8)((g_ch[ch].hi & 0x2F) & ~0x10);
		if (sus)
			hi |= 0x20;
		else
			hi &= (u8)~0x20;
		g_ch[ch].hi = hi;
		WriteOPLLreg(0x10 + ch, g_ch[ch].lo);
		WriteOPLLreg(0x20 + ch, hi);
	}
	if (AudioActive(ch))
		AudioNoteOff(ch);
}

static void Brightness(u8 ch, i8 delta)
{
	u8 v;
	if (g_ch[ch].inst <= 15)
		return;
	v = (u8)((g_ch[ch].bright & 0x3F) + (u8)delta);
	v = (u8)((g_ch[ch].bright & 0xC0) | (v & 0x3F));
	g_ch[ch].bright = v;
	WriteOPLLreg(0x02, v);
}

static void PlayChannel(u8 ch)
{
	u8 b = g_step_buf[ch];
	u8 mus, aud;

	if (!b)
		return;
	mus = MusicActive(ch);
	aud = AudioActive(ch);
	if (!mus && !aud)
		return;

	if (b <= NOTE_ON)
		NoteOn(ch);
	else if (b == NOTE_OFF)
		NoteOff(ch, 0);
	else if (b < VOLUME_CHANGE)
	{
		g_ch[ch].mode = FREQ_NORMAL;
		if (mus)
			SetInstrument(ch, (u8)(b - NOTE_OFF));
		if (aud)
			SetAudioInstrument(ch, (u8)(b - NOTE_OFF));
	}
	else if (b < STEREO_SET)
	{
		if (mus)
		{
			g_ch[ch].vol = (u8)((b - VOLUME_CHANGE) >> 2);
			WriteInstVol(ch);
		}
		if (aud)
			SetAudioVolume(ch, (u8)(b - VOLUME_CHANGE));
	}
	else if (b < NOTE_LINK)
	{
		g_chip[ch] = (u8)(b - STEREO_SET + 1);
		if (!MusicActive(ch))
		{
			WriteOPLLreg(0x10 + ch, 0);
			WriteOPLLreg(0x20 + ch, 0);
		}
		if (!AudioActive(ch) && g_audio)
		{
			WriteAUDIOreg((char)(0xA0 + ch), 0);
			WriteAUDIOreg((char)(0xB0 + ch), 0);
		}
	}
	else if (b < PITCH)
	{
		i8 link = (i8)(b - 189);
		u8 note = (u8)(g_ch[ch].note + link);
		if ((note < 1) || (note > NOTE_ON))
			note = g_ch[ch].note;
		g_ch[ch].mode = FREQ_NORMAL;
		if (mus)
			FreqToRegs(ch, note, 1, 0);
		if (aud)
			AudioFreqToRegs(ch, note, 1);
	}
	else if (b < BRIGHTNESS_NEGATIVE)
	{
		g_ch[ch].mode = FREQ_PITCH_BEND;
		g_ch[ch].pitch = (i8)(b - 208);
	}
	else if (b < REVERB)
	{
		if (mus)
			Brightness(ch, (i8)(b - 224));
		if (aud)
			AudioBrightness(ch, (i8)(b - 224));
	}
	else if (b < BRIGHTNESS_POSITIVE)
		g_ch[ch].detune = (i8)(b - 227);
	else if (b < SUSTAIN)
	{
		if (mus)
			Brightness(ch, (i8)(b - 230));
		if (aud)
			AudioBrightness(ch, (i8)(b - 230));
	}
	else if (b == SUSTAIN)
		NoteOff(ch, 1);
	else if (b == MODULATION)
		g_ch[ch].mode = FREQ_MODULATION;
}

static void PlayDrums(void)
{
	u8 drum = g_step_buf[DRUM_COL] & 0x0F;
	u8 bits;

	if (!g_rhythm || !drum)
		return;
	bits = capcalera.drum_setup_music_psg[drum - 1] & 0x1F;
	WriteOPLLreg(0x0E, bits);
	WriteOPLLreg(0x0E, (u8)(bits | 0x20));
}

static void PlayCommand(void)
{
	u8 b = g_step_buf[CMD_COL];
	if (!b)
		return;
	if (b <= COMMAND_TEMPO)
	{
		g_speed = (u8)(25 - b);
		if (g_speed < 2)
			g_speed = 2;
	}
	else if (b == COMMAND_PATTERN_END)
		g_step = MAX_STEP;
	else if (b < COMMAND_STATUS_BYTE)
		SetDrumset((u8)(b - COMMAND_DRUM_SET_MUSIC));
	else if (b >= COMMAND_TRANSPOSE)
		g_transpose = (u8)(b - (55 - 48));
}

static void PlayStep(void)
{
	u8 i;
	for (i = 0; i < NR_OF_CHANNELS; i++)
		PlayChannel(i);
	PlayDrums();
	PlayCommand();
}

static void Silence(void)
{
	u8 i;
	for (i = 0; i < 9; i++)
	{
		WriteOPLLreg(0x10 + i, 0);
		WriteOPLLreg(0x20 + i, 0);
	}
	WriteOPLLreg(0x0E, 0);
	if (g_audio)
	{
		for (i = 0; i < 9; i++)
		{
			WriteAUDIOreg((char)(0xA0 + i), 0);
			WriteAUDIOreg((char)(0xB0 + i), 0);
		}
	}
}

u8 MBM_LoadFile(const c8* name)
{
	u8 h;

	g_mbm_bytes = 0;
	h = DOS_OpenHandle(name, O_RDONLY);
	if (h == HANDLE_INVALID)
		return 0;
	Mem_Set(0, songFile, sizeof(songFile));
	g_mbm_bytes = DOS_ReadHandle(h, songFile, (u16)sizeof(songFile));
	DOS_CloseHandle(h);
	return (g_mbm_bytes >= 0x178) ? 1 : 0;
}

void MBM_SetLoop(u8 enable)
{
	g_loop = enable ? 1 : 0;
}

void MBM_SetAudio(u8 enable)
{
	g_audio = enable ? 1 : 0;
}

u8 MBM_HasAudio(void)
{
	return g_audio;
}

void MBM_Start(void)
{
	u8 i;
	u16 extra = 0;
	u8 *hdr = songFile;

	if (songFile[0] == 0xFF)
	{
		extra = 1;
		hdr = songFile + 1;
	}
	Mem_Copy(hdr, &capcalera, sizeof(capcalera));
	capcalera.track_name[40] = 0;

	g_pos_off = extra + 0x178;
	if (extra)
		g_pat_off = extra + 0x178 + 201;
	else
		g_pat_off = 0x178 + (u16)capcalera.song_length + 1;

	g_rhythm = (capcalera.sustain & 0x20) ? 1 : 0;
	g_melodic = g_rhythm ? 6 : 9;
	g_speed = capcalera.start_tempo;
	if (g_speed < 2)
		g_speed = 2;
	g_speed_cnt = 0;
	g_pos = 255;
	g_step = MAX_STEP;
	g_transpose = DEFAULT_TRANSPOSE;
	g_playing = 1;
	Mem_Set(0, g_step_buf, sizeof(g_step_buf));

	for (i = 0; i < STEREO_SETTING_SIZE; i++)
		g_chip[i] = capcalera.channel_chip_set[i];

	Silence();

	if (g_audio)
		WriteAUDIOreg(0xBD, (char)(capcalera.sustain & 0xC0));

	for (i = 0; i < NR_OF_CHANNELS; i++)
	{
		Mem_Set(0, &g_ch[i], sizeof(CHAN));
		g_ch[i].detune = (i8)capcalera.start_reverb[i];
		if ((i < g_melodic) && MusicActive(i))
			SetInstrument(i, capcalera.start_instruments_music[i]);
		if (AudioActive(i))
			SetAudioInstrument(i, capcalera.start_instruments_audio[i]);
	}

	WriteOPLLreg(0x0E, 0x00);
	if (g_rhythm)
	{
		WriteOPLLreg(0x36, capcalera.drum_volumes_music[0]);
		WriteOPLLreg(0x37, capcalera.drum_volumes_music[1]);
		WriteOPLLreg(0x38, capcalera.drum_volumes_music[2]);
		SetDrumset(0);
	}
}

void MBM_Tick(void)
{
	if (!g_playing)
		return;

	DisableInterrupt();
	HandlePitchMod();
	g_speed_cnt++;
	if (g_speed_cnt >= g_speed)
	{
		g_speed_cnt = 0;
		PlayStep();
	}
	else if (g_speed_cnt == (u8)(g_speed - 1))
		DecrunchStep();
	EnableInterrupt();
}

void MBM_Mute(void)
{
	DisableInterrupt();
	Silence();
	EnableInterrupt();
}

void MBM_Stop(void)
{
	g_playing = 0;
	MBM_Mute();
}

u8 MBM_IsPlaying(void)
{
	return g_playing;
}

// MoonBlaster 1.4 USER replay for YM2413 / MSX-MUSIC.
// Timing, frequency table and effects follow the official MBPLAY.SRC.
#include "msxgl.h"

typedef u8  uint8_t;
typedef u16 uint16_t;
typedef i8  int8_t;

#define NR_OF_AUDIO_INSTRUMENTS 16
#define NR_OF_MUSIC_INSTRUMENTS 16
#define AUDIO_INSTRUMENT_DATA_SIZE 9
#define STEREO_SETTING_SIZE 10
#define NR_OF_CHANNELS 9
#define NR_OF_ORIGINAL_INSTRUMENTS 6
#define MUSIC_INSTRUMENT_LENGTH 8

#define CHANNEL_MUSIC 0x02
#define CHANNEL_AUDIO 0x01

#define NOTE_ON 96
#define NOTE_OFF 97
#define VOLUME_CHANGE 114
#define STEREO_SET 177
#define NOTE_LINK 180
#define PITCH 199
#define BRIGHTNESS_NEGATIVE 218
#define REVERB 224
#define BRIGHTNESS_POSITIVE 231
#define SUSTAIN 237
#define MODULATION 238

#define COMMAND_TEMPO 23
#define COMMAND_PATTERN_END 24
#define COMMAND_DRUM_SET_MUSIC 25
#define COMMAND_STATUS_BYTE 28
#define COMMAND_TRANSPOSE 49

#define FREQ_NORMAL     0
#define FREQ_PITCH_BEND 1
#define FREQ_MODULATION 2

#define DEFAULT_TRANSPOSE 48
#define LOOP_OFF 255

typedef struct {
	uint8_t instrument;
	uint8_t volume;
} INSTRUMENT_DATA_MUSIC;

typedef struct {
	uint8_t song_length;
	uint8_t id[2];

	uint8_t voice_data_audio[NR_OF_AUDIO_INSTRUMENTS][AUDIO_INSTRUMENT_DATA_SIZE];

	uint8_t instrument_list_audio[NR_OF_AUDIO_INSTRUMENTS];
	INSTRUMENT_DATA_MUSIC instrument_list_music[NR_OF_MUSIC_INSTRUMENTS];

	uint8_t channel_chip_set[STEREO_SETTING_SIZE];
	uint8_t start_tempo;
	uint8_t sustain;
	char track_name[41];

	uint8_t start_instruments_audio[NR_OF_CHANNELS];
	uint8_t start_instruments_music[NR_OF_CHANNELS];
	uint8_t original_instrument_data[NR_OF_ORIGINAL_INSTRUMENTS][MUSIC_INSTRUMENT_LENGTH];
	uint8_t original_instrument_prog[NR_OF_ORIGINAL_INSTRUMENTS];

	char sample_kit_name[8];

	uint8_t drum_setup_music_psg[15];
	uint8_t drum_volumes_music[3];
	uint16_t drum_frequencies_music[3][3];
	uint8_t dummy[2];
	uint8_t start_reverb[NR_OF_CHANNELS];
	uint8_t loop_position;
} MBM_HEADER;

extern MBM_HEADER capcalera;
extern uint8_t songFile[80 * 16 * 13 + 80 * 2 + 200 + 0x178];
extern u16 g_mbm_bytes;

void WriteOPLLreg(char reg, char value) __naked;
void WriteAUDIOreg(char reg, char value) __naked;

u8   MBM_LoadFile(const c8* name);
void MBM_SetLoop(u8 enable);
void MBM_SetAudio(u8 enable);
u8   MBM_HasAudio(void);
void MBM_Start(void);
void MBM_Tick(void);
void MBM_Mute(void);
void MBM_Stop(void);
u8   MBM_IsPlaying(void);

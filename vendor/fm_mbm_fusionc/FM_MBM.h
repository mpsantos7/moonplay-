// Header file per separar el provaMBM.c perquè sigui un mòdul que es pugui
// cridar

#include <stdint.h>

#define NR_OF_AUDIO_INSTRUMENTS 16
#define NR_OF_MUSIC_INSTRUMENTS 16
#define AUDIO_INSTRUMENT_DATA_SIZE 9
#define STEREO_SETTING_SIZE 10
#define NR_OF_CHANNELS 9
#define NR_OF_ORIGINAL_INSTRUMENTS 6  // Són els instruments que es configuren per software. El moonblaster només deixa tenir-ne 6 configurats pel MSX-Music
#define MUSIC_INSTRUMENT_LENGTH 8  // Els 8 primers registres que s'han de configurar per un instrument original

#define NOTE_ON 96              /* 001 - 096 */
#define NOTE_OFF 97             /* 097       */
#define INSTRUMENT_CHANGE 98    /* 098 - 113 */
#define VOLUME_CHANGE 114       /* 114 - 176 */
#define STEREO_SET 177          /* 177 - 179 */
#define NOTE_LINK 180           /* 180 - 198 */
#define PITCH 199               /* 199 - 217 */
#define BRIGHTNESS_NEGATIVE 218 /* 218 - 223 */
#define REVERB 224              /* 224 - 230 */ // És el detune en el manual
#define BRIGHTNESS_POSITIVE 231 /* 231 - 236 */
#define SUSTAIN 237             /* 237       */
#define MODULATION 238          /* 238       */

#define COMMAND_TEMPO 23          /* 001 - 023 */
#define COMMAND_PATTERN_END 24    /* 024       */
#define COMMAND_DRUM_SET_MUSIC 25 /* 025 - 027 */
#define COMMAND_STATUS_BYTE 28    /* 028 - 039 */
#define COMMAND_TRANSPOSE 49      /* 049 - ... */

extern uint8_t TurboR;
extern uint8_t tempo;
extern uint8_t copsInterrupcio;
extern uint8_t filera;
typedef struct {
  uint8_t instrument;
  uint8_t volume;
} INSTRUMENT_DATA_MUSIC;

typedef struct {
  uint8_t song_length;
  uint16_t id;

  uint8_t voice_data_audio[NR_OF_AUDIO_INSTRUMENTS][AUDIO_INSTRUMENT_DATA_SIZE];

  uint8_t instrument_list_audio[NR_OF_AUDIO_INSTRUMENTS];
  INSTRUMENT_DATA_MUSIC instrument_list_music[NR_OF_MUSIC_INSTRUMENTS]; // Els 16 que es poden escollir al llistat. Quan indico una I a les notes, el número indica instrument de l'1 al 16. Si aquest número és més gran que 16 és que és un instrument sintètic i els seus valors de registre es troben a original_instrument_data. Però no sé si estan indexats per original_instrument_prog
  // De l'1 al 16 s'indiquen amb la I, indiquen quins dels 16 instruments estan sonant. Un cop saps quin dels 16 instruments tens

  uint8_t channel_chip_set[STEREO_SETTING_SIZE];
  uint8_t start_tempo;
  uint8_t sustain;
  char track_name[41];

  uint8_t start_instruments_audio[NR_OF_CHANNELS];
  uint8_t start_instruments_music[NR_OF_CHANNELS];
  uint8_t original_instrument_data[NR_OF_ORIGINAL_INSTRUMENTS]
                                  [MUSIC_INSTRUMENT_LENGTH];
  // Estan indexats fent l'instument del canvi I? - 16 i aquest instrument és el punter de NR_OF_ORIGINAL_INSTRUMENTS
  uint8_t original_instrument_prog[NR_OF_ORIGINAL_INSTRUMENTS]; // el roboplay no els fa servir

  char sample_kit_name[8];

  uint8_t drum_setup_music_psg[15];
  // Al full drum_setup.ods hi ha l'explicació detallada. Cada byte conté els 3 msb la combinació de l'efecte PSG a activar, ja que només pot funcionar 1, els altres 5 bits contenen l'activació del BD-SD-To-Cy_HH
  uint8_t drum_volumes_music[3]; // També he de mirar com estan guardats pel BD-SD-To-Cy-HH. Quan posa 15 en el moonblaster, es guarden com a 0 en el mbm, ja que el volum màxim és 0
  uint16_t drum_frequencies_music[3][3]; // El primer és el drumset i el segon són pels channels 7, 8 i 9
  // El drumset és el que es pot canviar a la columna de comanda
  // Hi ha 9 bits per la freqüència i 3 per l'octava,  els 9LSB és per la freqüència i després venen els 3 per l'octava. L'octava apareix un 1 en el moonblaster, però hi ha un 0 al fitxer, un 2 fa aparèixer un 1 en el mbm (el fitxer ja té el valor que ha d'anar al registre, ja no cal restar un 1)
  // Pel que he vist en el vgmrips cada cop que canvia la freqüència d'un canal posa el registre 0x0E el valor 0x20
  uint8_t dummy[2];
  uint8_t start_reverb[NR_OF_CHANNELS];
  uint8_t loop_position;
} MBM_HEADER;

extern MBM_HEADER capcalera;
extern uint8_t songFile[80*16*13 + 80*2 + 200 + 0x178]; // 80 patrons de tamany 16 fileres per 13 columnes com a màxim (sample i drum van al mateix byte) més la posició dels 80 patrons dins el fitxer que tenen tamany 2 bytes (d'aquí el 80*2) + 200 de les posicions dels patrons + la capçalera
// No em cal calcular el posPatrons, estan després de la longitud, són uint16_t. És la posició dins el fitxer file. Aleshores jo les hauré de restar dins de song
extern uint8_t ordrePatrons[200];
extern uint8_t posSong; // Patró que estic tocant dins la cançó. Aquest ja és el
                        // de la capçalera, no el necessitaria
extern uint16_t posPatro; // Notes que estic tocant dins el patró

extern const uint8_t notesFreq[12];

typedef struct {
  uint8_t octava;
  uint8_t nota;
  // Podria fer un byte amb els dos, però ja està bé així
  // Potser també necessitaré el volum del canal
  int8_t bending;
  uint8_t freq_bending; // Segurament es podria ajuntar amb nota per tenir només una freqüència
  uint8_t overflow_bending; // Per quan passa la freqüència de 8 bits
  // El roboplay té una estrucgtura de status g_mbm_status_table per cada canal que no sé el que hi té
} NOTA_TOCANT;

extern NOTA_TOCANT notaTocantCanal[6];
// Guardarà la informació de la nota de notesFreq que
// s'està tocant per aquell canal

extern uint8_t brightness_inst_orig;
// Quan carreguem els registres de l'instrument
// original recarregarem aquest registre i anirà
// variant en les comandes de Brightness

void ompleCapcaleraPatrons(); // La mateixa funció que readFileSong però sense
                              // el fcb_read

void WriteOPLLreg(char reg, char value);
void WriteOPLLreg_z80(char reg, char value);
void WriteOPLLreg_TR(char reg, char value);
void PlayNote(uint8_t columna, uint8_t octava, uint8_t freq, uint8_t highfreq);
void PlayBend(uint8_t columna);

void seguentPatro();

void canviDrumset(uint8_t set);
void InicialitzemCanalsReproductor();
void playPatro();
void apaguemOPLL();

//uint8_t debug_stop;
//extern uint8_t debug_byteLlegit;

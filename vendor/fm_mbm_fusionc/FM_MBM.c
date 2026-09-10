#include "FM_MBM.h"
#include "fusion-c/header/msx_fusion.h"
#include <stdint.h>
#include <string.h>

uint8_t tempo;
uint8_t TurboR;
uint8_t copsInterrupcio;
uint8_t filera;

MBM_HEADER capcalera;
uint8_t songFile[80 * 16 * 13 + 80 * 2 + 200 + 0x178];

uint8_t ordrePatrons[200];
uint8_t posSong;
uint16_t posPatro;
const uint8_t notesFreq[12] = {
    173, // 0b10101101, // 173 (AD), C,
    181, // 0b10110101, // 181 (B5), c#, freq 272.2
    192, // 0b11000000, // 192 (C0), D, freq 293.7
    204, // 0b11001100, // 204 (CC), D#, freq 311.1
    216, // 0b11011000, // 216 (D8), E, freq 329.6
    229, // 0b11100101, // 229 (E5), F, freq 349.2
    242, // 0b11110010, // 242 (F2), F#, freq 370
    1, // 0b00000001, // 257 (1+1), G, freq 392 A partir d'aquest porten 1 extra
    16, // 0b00010000, // 272 (1+10), G#, freq 415.3
    32, // 0b00100000, // 288 (1+20), A, freq 440
    39, // 0b00110001, // 305 (1+31), A#, freq 466.2
    57, // 0b01000011, // 323 (1+43), B, freq 493.9
};
NOTA_TOCANT notaTocantCanal[6];
uint8_t brightness_inst_orig;
uint8_t debug_byteLlegit;

void ompleCapcaleraPatrons() {
  memcpy(&capcalera, songFile, sizeof(capcalera));

  memcpy(ordrePatrons, &songFile[0x178], capcalera.song_length + 1);
}
void WriteOPLLreg(char reg, char value) {
  if (TurboR==1) {
    WriteOPLLreg_TR(reg,value);
  }  else {
    WriteOPLLreg_z80(reg, value);
  }
}

void WriteOPLLreg_z80(char reg, char value) __naked {
  // Escrivim els registres a l'OPLL tal i com vam fer a les proves de FM_scratch
  __asm
    ld iy,#2
    add iy,sp ;Bypass the return addess of the function. Totes les funcions en asm del llibre de fusion-c fan primer aquest pas

    ld D,(IY) ; reg
    ld E,1(IY)  ; value

    ex af, af'
    ld A,D
    out (#0x7C),A
    ex af, af'
    LD A,E
    out (#0x7D),A
    ex (SP),HL
    ex (SP),HL
    ret
  __endasm;
}

void WriteOPLLreg_TR(char reg, char value)__naked {
  // La mateixa funció que WriteOPLLReg però per Turbo-R, els temps són més ràpids. Basat en comentaris de https://msx.org/forum/msx-talk/development/msx-music-player-using-r800#comment-436319
  __asm
    ld iy,#2
    add iy,sp 

    ld D,(IY) ; reg
    ld E,1(IY)  ; value

    ex af, af'
    ld A,D
    out (#0x7C),A
    ex af, af'
    ex af, af'
    ex af, af'
    ex af, af'
    LD A,E
    out (#0x7D),A
    ex (SP),HL
    ex (SP),HL
    ex (SP),HL
    ex (SP),HL
    ex (SP),HL
    ex (SP),HL
    ex (SP),HL
    ex (SP),HL
    ret
  __endasm;
}


void PlayNote(uint8_t columna, uint8_t octava, uint8_t freq, uint8_t highfreq) {
  WriteOPLLreg(0x10 + columna, freq);
  char valor = (octava << 1) | highfreq;
  WriteOPLLreg(0x20 + columna, valor);
  // Al fer el canvi de posar el registre el key en off a l'escriptura, el MSX-Music sona
  // com el del moonblaster, amb un so més nítid
  valor= valor | 0x10;
  WriteOPLLreg(0x20 + columna, valor);
}
void PlayBend(uint8_t columna) {
  uint8_t novafreq =
      notaTocantCanal[columna].freq_bending + notaTocantCanal[columna].bending;
  if (notaTocantCanal[columna].bending > 0) {
    if (novafreq < notaTocantCanal[columna].freq_bending) {
      // Hi ha hagut overflow
      if (notaTocantCanal[columna].overflow_bending > 0) {
        novafreq = 0xFF;
      } else {
        notaTocantCanal[columna].overflow_bending = 1;
      }
    }
  } else {
    if (novafreq > notaTocantCanal[columna].freq_bending) {
      // Hi ha hagut overflow
      if (notaTocantCanal[columna].overflow_bending > 0) {
        notaTocantCanal[columna].overflow_bending = 0;
      } else {
        novafreq = 0x01; // Diria que el 0x00 no és cap nota
      }
    }
  }
  // Tocar nota que podria fer una funció. Mirar les parts comunes i a on
  // apareix l'estructura
  PlayNote(columna,notaTocantCanal[columna].octava, novafreq, notaTocantCanal[columna].overflow_bending);
  notaTocantCanal[columna].freq_bending = novafreq;
}

void seguentPatro(){
  posSong = posSong + 1;
  if (posSong > capcalera.song_length) {
    posSong = 0;
  }
  posPatro =
      songFile[0x178 + capcalera.song_length + 2 * ordrePatrons[posSong] - 1] +
      (songFile[0x178 + capcalera.song_length + 2 * ordrePatrons[posSong]]
       << 8);
  filera = 0;
}

void canviDrumset(uint8_t set) {
  WriteOPLLreg(0x16, (uint8_t)capcalera.drum_frequencies_music[set][0]);
  WriteOPLLreg(0x26, (uint8_t)(capcalera.drum_frequencies_music[set][0] >> 8));
  WriteOPLLreg(0x17, (uint8_t)capcalera.drum_frequencies_music[set][1]);
  WriteOPLLreg(0x27, (uint8_t)(capcalera.drum_frequencies_music[set][1] >> 8));
  WriteOPLLreg(0x18, (uint8_t)capcalera.drum_frequencies_music[set][2]);
  WriteOPLLreg(0x28, (uint8_t)(capcalera.drum_frequencies_music[set][2] >> 8));
}

void InicialitzemCanalsReproductor() {
  copsInterrupcio = 0;
  filera =0;
  posSong = 0;
  posPatro =
      songFile[0x178 + capcalera.song_length + 2 * ordrePatrons[posSong] - 1] +
      (songFile[0x178 + capcalera.song_length + 2 * ordrePatrons[posSong]]
       << 8);

  // Hem d'inicialitzar amb els instruments definits al mbm. No sempre el violí
  // Fent proves amb el Moonblaster, sembla que el primer que agafa és la configuració de l'instrument 1 en Select Voices (F4). No hi ha forma de canviar el volum inicial, per tant és sempre al màxim. Tots els canals tenen el mateix instrument
  debug_byteLlegit=155;
  for( int k=0;k<6;k++) {
    // Si és instrument propi, el moonblaster el configura abans de configurar el canal
    uint8_t num_instrument =
        capcalera.instrument_list_music[capcalera.start_instruments_music[k] - 1].instrument;
    if (num_instrument > 0x0F) {
      // Configurem l'owner instrument
      debug_byteLlegit = 15;
      for(int kk=0;kk<8;kk++){
        WriteOPLLreg(kk, capcalera.original_instrument_data[num_instrument-0x10][kk]);
      }
      WriteOPLLreg(
          0x30 + k,
              capcalera.instrument_list_music[capcalera.start_instruments_music[k] - 1].volume);
    }
    else {
      WriteOPLLreg(
        0x30 + k,
        capcalera.instrument_list_music[capcalera.start_instruments_music[k]-1].instrument << 4 |
        capcalera.instrument_list_music[capcalera.start_instruments_music[k]-1].volume);
    }
  }

  // Inicialitzem la percussió
  WriteOPLLreg(0xE0, 0x00);
  WriteOPLLreg(0x36, capcalera.drum_volumes_music[0]);
  WriteOPLLreg(0x37, capcalera.drum_volumes_music[1]);
  WriteOPLLreg(0x38, capcalera.drum_volumes_music[2]);

  canviDrumset(0);

  // Inicialitzem el notaTocantCanal. Amb el bending a 0 n'hi ha prou. Així no es toquen les notes extres
  for(int k=0; k<6; k++) {
    notaTocantCanal[k].bending = 0;
  }
}

void playPatro() {
  DisableInterrupt();
    if (copsInterrupcio>=tempo) {
      uint8_t columna = 0;
      while (columna < 13) {
        uint8_t byteLlegit = songFile[posPatro];
        if (byteLlegit < 243) {
          // Faig només MSX-Music, només utilitzo 6 dels 9 canals que indica
          if(columna<6){
            if (byteLlegit <= NOTE_ON) {
              uint8_t octava = byteLlegit / 12;
              uint8_t nota = byteLlegit - octava * 12;
              uint8_t notaFreq =  notesFreq[nota - 1];
              if (nota < 8) {
                PlayNote(columna, octava, notaFreq, 0);
              } else {
                PlayNote(columna, octava, notaFreq, 1);
              }
              notaTocantCanal[columna].nota = nota - 1;
              notaTocantCanal[columna].octava = octava;
              notaTocantCanal[columna].bending = 0;
              notaTocantCanal[columna].freq_bending = notaFreq;
            } else if (byteLlegit == NOTE_OFF) {
              // Feia un 0x00 del registre 0x20+columna però no es recuperava, posaré la freqüència a 0 ja que he vist que en el VGM no posen mai cap registre 0x2? a 0x0
              // WriteOPLLreg(0x20+columna, 0x0);
              // He observat que el moonblaster escriu el registre de les notes però no activa el key, el deixa en off
              uint8_t highFreqOff =0;
              if (notaTocantCanal[columna].nota+1>=8) {
                highFreqOff = 1;
              }
              WriteOPLLreg(0x10+columna, notaTocantCanal[columna].freq_bending);
              WriteOPLLreg(0x20+columna, (notaTocantCanal[columna].octava<<1) | highFreqOff);
              notaTocantCanal[columna].bending = 0;
            } else if (byteLlegit < VOLUME_CHANGE) {
              // Canvi d'instrument
              // He de tenir guardat un estat actual del volum i instrument pel canal, per quan s'hagi de canviar un, pugui mantenir l'altre, ja que tant l'instrument com al canal estan al mateix byte
              uint8_t num_instrument = byteLlegit - NOTE_OFF -1 ;
              debug_byteLlegit = byteLlegit;
              // debug_stop = 1;
              // Hem de tenir en compte que si és instrument configureat (owner instrument) s'ha de configurar
              // Retorna el 5 que és l'instrument a que s'ha de canviar. No indiquen si és owner o no. S'ha d'anar a la llista d'instruments per descodificar-ho
              uint8_t nou_instrument = capcalera.instrument_list_music[num_instrument].instrument;
              if (nou_instrument > 0x0F) {
                // Configurem l'owner instrument
                for (int kk = 0; kk < 8; kk++) {
                  WriteOPLLreg(
                      kk,
                      capcalera.original_instrument_data
                          [nou_instrument - 0x10][kk]);
                }
                WriteOPLLreg(0x30 + columna,
                             capcalera.instrument_list_music[capcalera.start_instruments_music[columna] - 1]
                                 .volume);
              } else {
                WriteOPLLreg(
                    0x30 + columna,
                    nou_instrument << 4 |
                        capcalera.instrument_list_music[columna].volume);
              }
            }
            else if (byteLlegit < STEREO_SET) {
              // Canviem el volum
              // El volum té fins a 64 però l'MSX-Music només té fins a 15 i
              // en ordre invertit. És a dir en el moonblaster v64 és el màxim
              // i pel Music és 0. Hem de dividir entre 4 i negar
              capcalera.instrument_list_music[columna].volume =
                    !(byteLlegit >> 4);
              WriteOPLLreg(
                    0x30 + columna,
                    capcalera.instrument_list_music[columna].instrument << 4 |
                        capcalera.instrument_list_music[columna].volume);
            }
            else if (byteLlegit < NOTE_LINK) {
                // Do nothing. STereo only with moonsound
            }
            else if (byteLlegit < PITCH) {
              // Do note link
              // Hauré d'anar guardant per cada canal la nota que s'està
              // tocant. Depèn com també hauré de pujar l'octava Roboplay
              // resta 189 per obtenir -9 a +9 He de saber la nota que estic
              // tocant i canviar-la LA canço punkadidd del disk de
              // moonblaster té en el patró 10 un Link
                int8_t link = byteLlegit - 189;
                int8_t novaNota = notaTocantCanal[columna].nota + link;
                if (novaNota < 0) {
                  notaTocantCanal[columna].nota =
                      12 + novaNota; // Les 12 notes de l'escala
                  notaTocantCanal[columna].octava =
                      notaTocantCanal[columna].octava - 1;
                } else if (novaNota >= 12) {
                  notaTocantCanal[columna].nota = 12 - novaNota;
                  notaTocantCanal[columna].octava =
                      notaTocantCanal[columna].octava + 1;
                } else {
                  notaTocantCanal[columna].nota = novaNota;
                }
                // He d'escriure la nova nota al registre perquè sigui tocada
                if (notaTocantCanal[columna].nota < 7) {
                  PlayNote(columna, notaTocantCanal[columna].octava,
                           notaTocantCanal[columna].nota, 0);
                } else {
                  PlayNote(columna, notaTocantCanal[columna].octava,
                           notaTocantCanal[columna].nota, 1);
                }
              }
              else if (byteLlegit < BRIGHTNESS_NEGATIVE) {
                // He de fer exemples amb el pitchbend i estudiar-lo, és una
                // entrada de P. El roboplay resta 208 que és el valor per tenir
                // [-9,9] El pitchbend va sumant a cada iteració la freqüència
                // indicada amb la P. Suposo que fins al 0x1FF o fins el 0x0
                // S'atura a la següent nota, OFF, SUS or MOD event. I també si
                // hi ha P0 Ho hauré de controlar amb la nota i a cada columna
                // revisar si hi ha bending pendent d'executar. Abans de fer el
                // columna++ Si a la mateixa columna trobo una altra P continua
                // augmentant al nou ritme +3,+4,+5... la freqüència que ja
                // tenia
                int8_t pitch = byteLlegit - 208;
                if (pitch == 0) {
                  notaTocantCanal[columna].bending =
                      0; // Amb aquesta informació ja en tinc prou. La resta de
                         // camps ja els canviaré quan hi hagi un pitch que
                         // s'inicialitza a aquí.
                } else {
                  notaTocantCanal[columna].bending = pitch;
                  notaTocantCanal[columna].freq_bending =
                      notesFreq[notaTocantCanal[columna].nota];
                  if (notaTocantCanal[columna].nota >= 7) {
                    notaTocantCanal[columna].overflow_bending = 1;
                  } else {
                    notaTocantCanal[columna].overflow_bending = 0;
                  }
                  PlayBend(columna);
                }
              }
              else if (byteLlegit < REVERB) {
                // El brightness aplica només a l'instrument original, no al de
                // fàbrica i modifica el carrier, el registre 2
                uint8_t valor =
                    217 - byteLlegit; // El 217 és per tenir valors negatius ja
                                      // que byteLlegit 218-223
                brightness_inst_orig = brightness_inst_orig + valor;
                WriteOPLLreg(0x02, brightness_inst_orig);
              }
              else if (byteLlegit < BRIGHTNESS_POSITIVE) {
                // El detune agafa la freqüència (nota) i li suma+3 o -3 a la
                // freqüència, pel que he pogut comprovar amb el VGM
                // El roboplay li diu REVERB
                // A les instruccions (apartat 3.3 secció Detune switch) comenta
                // que no s'acumula, per tant no he de tornar a guardar la
                // freqüència dins de notaTocant
                uint8_t valor = (byteLlegit - 227) * 3;
                uint8_t octava = notaTocantCanal[columna].octava;
                uint8_t nota = notaTocantCanal[columna].nota;
                uint8_t freq = notesFreq[notaTocantCanal[columna].nota] + valor;
                if (nota < 7) {
                  PlayNote(columna, octava, freq, 0);
                } else {
                  PlayNote(columna, octava, freq, 1);
                }
              }
              else if (byteLlegit < SUSTAIN) {
                uint8_t valor = byteLlegit - 230;
                brightness_inst_orig = brightness_inst_orig + valor;
                WriteOPLLreg(0x02, brightness_inst_orig);
              }
              else if (byteLlegit == SUSTAIN) {
                uint8_t octava = notaTocantCanal[columna].octava;
                uint8_t nota = notaTocantCanal[columna].nota;
                if (nota < 7) {
                  WriteOPLLreg(0x20 + columna, (octava << 1) | 0x30);
                } else {
                  WriteOPLLreg(0x20 + columna, (octava << 1) | 0x31);
                }
              }
              else if (byteLlegit == MODULATION) {
                // L'he d'estudiar
                // Les instruccions diu que pugen la freqüència i la baixen.
                // Però quina quantitat? El que sí que he de parar és el bending
                notaTocantCanal[columna].bending = 0;
                // He fet proves posant un C3 i MOD i no he notat cap efecte
              }
          } else if (columna == 11) {
            // Processar sample i drum command
            // Les columnes van juntes. Entenc que el byte baix és el DRM
            uint8_t  drum = byteLlegit & 0x1F; // Comencen indicant el bloc 1 fins al 15, però per indexar vaig del 0 al 14
            WriteOPLLreg(0x0E, capcalera.drum_setup_music_psg[drum-1] & 0x1F);
            WriteOPLLreg(0x0E,
                         (capcalera.drum_setup_music_psg[drum-1] & 0x1F) | 0x20);
          } else if (columna == 12) {
            // PRocessar CMD
            if(byteLlegit<24) {
              // Canvi del tempo
              tempo = tempo - byteLlegit;
            } else if (byteLlegit == 24 ) {
              // End of Pattern
              // He de fer el mateix que si la filera == 16. Ho posaré en una funció
              seguentPatro();
            } else if (byteLlegit<28) {
              // Change drumset
              uint8_t nouDrumset = byteLlegit - 25;
            }
            // Hi ha més comandes especificades en el mbm file format, però no entenc què fan i com s'introdueixen
            // El mbm tampoc processa el status byte
            // El transpose command potser si utilitzes un transpose editant queda registrat al mbm. S'ha de comprovar. He fet un fitxer que s'haurà de provar de carregar
          }
          // Abans d'acabar la columna he de mirar si hi ha un pitchbend corrent, ja que hi ha comandes que no afecten al pitchbend
          if (notaTocantCanal[columna].bending != 0 && columna<6) {
            PlayBend(columna);
          }
          columna++;
        } else {
          // Aquestes columnes que no hi ha cap comanda, he de mirar si tenen un pitchbend
          for(int k=0;(k+columna<byteLlegit - 242) && (k+columna<6);k++){
            // El k<6 ja que el moonblaster té 11 columnes de comandes però el msx-music només fa servir les 6 primeres
            if(notaTocantCanal[columna+k].bending!=0){
              PlayBend(columna+k);
            }
          }
          columna = columna + byteLlegit-242;
        }
        posPatro++;
      }
      copsInterrupcio=0;
      filera++;
    }
    if (filera == 16) {
      // Hauré de saltar al següent patró
      // Tanbé podria fer-ho sabent la posició del següent patró i controlant si estic a l'últim patró
      seguentPatro();
    }
    copsInterrupcio++;
    EnableInterrupt();
}

void apaguemOPLL() {
  for (int k=0; k<9; k++) {
    WriteOPLLreg(0x10 + k, 0x00);
    WriteOPLLreg(0x20 + k, 0x00);
  }
  WriteOPLLreg(0xE0, 0x00);
}

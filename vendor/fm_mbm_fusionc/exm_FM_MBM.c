#include "FM_MBM.h"
#include "fusion-c/header/msx_fusion.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HALT __asm halt __endasm // wait for the next interrupt

static FCB file;

void FT_SetName(FCB *p_fcb, const char *p_name) // Routine servant à vérifier le
                                                // format du nom de fichier
{
  char i, j;
  memset(p_fcb, 0, sizeof(FCB));
  for (i = 0; i < 11; i++) {
    p_fcb->name[i] = ' ';
  }
  for (i = 0; (i < 8) && (p_name[i] != 0) && (p_name[i] != '.'); i++) {
    p_fcb->name[i] = p_name[i];
  }
  if (p_name[i] == '.') {
    i++;
    for (j = 0; (j < 3) && (p_name[i + j] != 0) && (p_name[i + j] != '.');
         j++) {
      p_fcb->ext[j] = p_name[i + j];
    }
  }
}

void FT_errorHandler(char n, char *name) // Gère les erreurs
{
  Screen(0);
  SetColors(15, 6, 6);
  switch (n) {
  case 1:
    Print("\n\rFAILED: fcb_open(): ");
    Print(name);
    break;

  case 2:
    Print("\n\rFAILED: fcb_close():");
    Print(name);
    break;

  case 3:
    Print("\n\rStop Kidding, run me on MSX2 !");
    break;
  }
  Exit(0);
}

int FT_openFile(char *file_name)
{
  FT_SetName(&file, file_name);
  if (fcb_open(&file) != FCB_SUCCESS) {
    FT_errorHandler(1, file_name);
    return (0);
  }
}

void WAIT(int cicles) {
  int i;
  for (i = 0; i < cicles; i++)
    HALT;
  return;
}

// Potser defineixo songfile aquí

void main(){
  FT_openFile("pep.mbm");
  fcb_read(&file, songFile, sizeof(songFile));

  printf("%d\n", notesFreq[3]);
  printf("%d\n", notesFreq[0]);
  printf("%d\n", notesFreq[10]);
  // Sembla que el songFileno omple les variables que s'han d'omplir a la crida
  // d'ompleCapcaleraPatrons
  ompleCapcaleraPatrons();

  InicialitzemCanalsReproductor();
 
  tempo = capcalera.start_tempo;

  InitVDPInterruptHandler((int)playPatro);
  WAIT(900);
 
  EndVDPInterruptHandler();
  apaguemOPLL();

  FT_openFile("badend1.mbm");
  fcb_read(&file, songFile, sizeof(songFile));

  printf("Seguent canco");
  ompleCapcaleraPatrons();

  InicialitzemCanalsReproductor();

  tempo = capcalera.start_tempo;

  InitVDPInterruptHandler((int)playPatro);
  WAIT(900);
 
  EndVDPInterruptHandler();
  apaguemOPLL();

  //  printf("ByteLleg: %d\n", debug_byteLlegit);
  Exit(0);
}

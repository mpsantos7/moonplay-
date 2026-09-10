# FM_MBM_FusionC

Play the FM part of a MBM files (the ones created using Moonblaster, the pieces are played using the VDP Interrupt

# How to use it
FM_MBM_FusionC is made of two files, the header file, FM_MBM.h, and FM_MBM.rel which is the compiled version of FM_MBM.c to be used as a library.

You only need to put the header file in the include of your C file and when you are compiling your C file add FM_MBM.rel before your C file. For example, using Fusion-C the command to compile is:
```
sdcc -V --code-loc 0x106 --data-loc 0x0 --disable-warning 196 -mz80 --no-std-crt0 --opt-code-size fusion.lib -L ./fusion-c/source/lib/ ./fusion-c/include/crt0_msxdos.rel FM_MBM.rel your_file.c
```

In the main function you must open the mbm file that you want to play and read it in the buffer `songFile`. After songFile is pointing to the mbm variable, you must call the inicialization functions:
```
  ompleCapcaleraPatrons();

  InicialitzemCanalsReproductor();

  tempo = capcalera.start_tempo;

  TurboR = 1; // or 0 if it is running in z80 mode
  ```
  When this is done, you only need to call playPatro() in your VPDInterruptHandler function. Be alert that playPatro enables the interruptions at the end of its call. To mute the sound you have to call:
  ```
  apaguemOPLL();
  ```

  One example with this implementation can be found in exm_FM_MBM.c. 
  
  In the repository can also be found two mbm files, escala.mbm that is a simple song that I use for debugging and lucifer.mbm that is a song that came in the original disk of Moonblaster 1.4.



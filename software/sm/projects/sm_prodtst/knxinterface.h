#ifndef __KNXINTERFACE_H
#define __KNXINTERFACE_H

typedef struct {
  u8  bOKTimer;
  u8  Mode;
  u8  OnCh1;
  u8  OnCh2;
  u8  LightLevelCh1; // from KNX 0..255 to Dali 0..254
  u8  LightLevelCh2;
  u32 ulCounter;
} tKNX;

extern tKNX KNX;

void ProcessKNXmessage(void);

#endif

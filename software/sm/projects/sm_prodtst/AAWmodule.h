#ifndef __AAWMODULE_H
#define __AAWMODULE_H

typedef struct {
  u8  fSensoDimOK;
  u8  bStatus;

  u8  bSwitch;
  u8  bPirOn; 
  u16 wLux;
  u8  bDaliValueCh1; // V0.40
  u8  bDaliValueCh2; // V0.40
  u8  bOKTimer;
  u32 ulCounter;
} tAAWmodule;

extern tAAWmodule AAW;

void ProcessAAWmodulemessage(void);


#endif







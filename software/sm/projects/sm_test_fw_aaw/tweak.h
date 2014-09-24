#ifndef __TWEAK_H
#define __TWEAK_H

#define KEY_SAMPLETIME_20MSEC           // ifndef then 50msec

typedef struct {
  u8 bOnStatus;
  u8 bLogScale;
  u8 fDALIout1;            // DALI channel 1 - 0=off, 1=on
  u8 fDALIout2;            // DALI channel 2 - 0=off, 1=on
  
  u8 bMinDalilevel;
  u8 bDaliSetLevelDirect;
  u8 bDaliSetLevelIndirect;
  u8 bDaliIncDec;
  u8 bBlockKeyTime;
  u8 bBlockIndirectKeyTimer;
  u8 bBlockDirectKeyTimer;
  u8 bDaliOutputTime;
  u8 bDaliOutputTimer;
} tTweak;




void TweakProcess(void);

void InitTweak(void);


#endif







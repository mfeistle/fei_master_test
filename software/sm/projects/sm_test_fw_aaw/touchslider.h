#ifndef __TOUCHSLIDER_H
#define __TOUCHSLIDER_H

typedef struct {
  u8  bKeys;
  u8  bKeyToggleStates;     // MAN: toggle state of bKeys
  u8  bSlider1;
  u8  bSlider2;
  u16 bAllKeyCounter;
  u8  bKeyCounter1;         
  u8  bKeyCounter2;         
  u8  bKeyCounter3;         
  u8  bSliderValue1;
  u8  bSliderValue2;
  u8  fNewKey;
  u8  fNewSlider1;
  u8  fNewSlider2;
  u8  fUdateDALIout;        // if true update DALI-outputs
  u8  bOKTimer;
  u8  fSetupRequest;
  u8  bSetupParameterSelection;
  u8  bSetupParameterIndex;
  u8  bSetupControl;
  u8  bSetupValue;
  u8  bR_Led;
  u8  bG_Led;
  u8  bB_Led;
  u32 ulCounter;
} tSlider;

extern tSlider SLIDER;


void ProcessTouchmessage(void);


#endif







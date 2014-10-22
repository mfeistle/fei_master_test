#ifndef __LAMPCONTROL_H
#define __LAMPCONTROL_H

#define STATUS_OFF              0
#define STATUS_SWITCHINGOFF     1
#define STATUS_SWITCHINGON      2
#define STATUS_ON               3
#define STATUS_GOTOSAVINGPOWER  4
#define STATUS_SAVINGPOWER      5
#define STATUS_FADING           6
#define STATUS_REMOTECONTROLLED 7


typedef struct {
  u8  bStatus;
  u8  fSensoDimOK;
  u8  bLuxControllerON;
  u8  fOn;
  u8  fManualOff;

  u8  bSoftOnTimer;
  u8  bSoftOffTimer;
  u8  bForceOutput;
  u8  bRelaisOfftimer;

  u16 wLux;
  u16 wLuxSpan;
  u16 wSetLux;
  u16 wSetLuxMin;
  u16 wSetLuxMax;
  u16 wLastSetLux;
  s16 iLuxError;
  u8  bSetValueChangeTimer;
  u16 wPowerSavingLux;
  u8  bLuxCtrlTimer;
  u8  bLuxCtrlTime;
  u8  bLuxCtrlFastTimer;
  
  u16 wDimPromille;
  u16 wSetDimPromille;
  u16 wStartDimPromille;
  u16 wMinimumDimPromille;
  u16 wDimPromilleStep;
  u32 ulPowerSavingtimer;

} tLAMPcontrol;

typedef struct {
  u8  bCouplingModus;
  u8  fPowerSavingModeON;
  u8  bPowerSavingErrorPercent;
  u8  bAAW_ModusOn;
  
  s16 iAverageLux;
  s16 iAverageSetLux;
  
  u8  bSoftOnTime;
  u8  bSoftOffTime;

  u16 wPIR_Ontime;
  u16 wPIR_Disabletime;
  u16 wPowerSavingtime;
  u16 wSetLuxIncrement;
  
  
  
} tLAMPcontrol_common;

extern tLAMPcontrol  LAMP_Ch1,LAMP_Ch2;
extern tLAMPcontrol_common LAMP_Both;

void ControllerStatusUpdate(u8 On1,u8 On2);
void ControllerSetLuxUpdate(u16 Setlux);
u16 CalculateSetlux(u8 sliderval);


void StatusControllerCh1(void);
void StatusControllerCh2(void);
void InitLampController(void);


#endif







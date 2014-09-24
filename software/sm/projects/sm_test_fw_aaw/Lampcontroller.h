#ifndef __LAMPCONTROL_H
#define __LAMPCONTROL_H


// Zeiten für Tastenverhalten in 10msec Schritten
#define ON_OFF_TIME     100                 // Sperrzeit für Impuls
#define BOUNCE_TIME     (ON_OFF_TIME-2)     // Prellzeit für Impuls oder Adjust LUX Sollwert Unterscheidung 20msec = 2
#define ADJUST_LUXTIME  (ON_OFF_TIME-50)    // Pulslänge, bis LUX Sollwert verändert wird 500msec = 50
#define ADJUST_LUXOFFDELAY  20              // Wenn Lux Sollwertveränderung abgeschlossen, dann nochmals 200msec = 20 warten bis neue Kommandos erlaubt sind

#define STATUS_OFF  0
#define STATUS_SWITCHINGOFF  1
#define STATUS_SWITCHINGON   2
#define STATUS_ON  3
#define STATUS_GOTOSAVINGPOWER 4
#define STATUS_SAVINGPOWER  5


typedef struct {
  u8  bStatus;
  u8  fSensoDimOK;
  
  u8  ON_OFF_Timer;
  u8  PIR_FLAG;

  u8  bLuxControllerON;
  u8  fOn;
  u8  fManualOn;
  u8  fManualOff;
  u16 wPIR_Ontimer;
  u16 wPIR_Disabletimer;

  u8  bSoftOnTimer;
  u8  bSoftOffTimer;
  u8  bRelaisOfftimer;

  u16 wLux;
  u16 wLuxSpan;
  u16 wSetLux;
  u16 wSetLuxMin;
  u16 wSetLuxMax;
  u8  fChangeSetLuxDown;
  s16 iLuxError;
  u8  bLuxCtrlTimer;
  u8  bLuxCtrlTime;
  
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
  u8  bAAW_ModusOn;
  
  s16 iAverageLux;
  s16 iAverageSetLux;
  
  u8  bSoftOnTime;
  u8  bSoftOffTime;

  u16 wPIR_Ontime;
  u16 wPIR_Disabletime;
  u16 wPowerSavingtime;
  u16 wSetLuxIncrement;
  
  
  u16 wPowerSavingLux;
  
} tLAMPcontrol_common;

extern tLAMPcontrol  LAMP_Ch1,LAMP_Ch2;
extern tLAMPcontrol_common LAMP_Both;

void PirTimeController(void);
void SensoDimControllerCh1(void);
void SensoDimControllerCh2(void);
void StatusControllerCh1(void);
void StatusControllerCh2(void);
void InitLampController(void);


#endif







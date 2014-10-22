#ifndef __SENSODIMCONTROLLER_H
#define __SENSODIMCONTROLLER_H


// Timing for Key in 10msec Steps
#define ON_OFF_TIME     100                 // Initial Time after Detecting Switch
#define BOUNCE_TIME     (ON_OFF_TIME-2)     // Prellzeit für Impuls oder Adjust LUX Sollwert Unterscheidung 20msec = 2
#define ADJUST_LUXTIME  (ON_OFF_TIME-50)    // Pulslänge, bis LUX Sollwert verändert wird 500msec = 50
#define ADJUST_LUXOFFDELAY  20              // Wenn Lux Sollwertveränderung abgeschlossen, dann nochmals 200msec = 20 warten bis neue Kommandos erlaubt sind


typedef struct {
  u8  fConnected; 
  
  u8  ON_OFF_Timer;
  u8  PIR_FLAG;

  u8  bClickCount;
  u8  bKeySetFunction; // 0=Setlux 1=Setdali
  u8  bOn; // for both channels 1+2
  u16 wPIR_Ontimer;
  u16 wPIR_Disabletimer;
  u8  OffBlockTime;
  u8  fDoubleClick;

  u16 wLux;
  u16 wLuxSpan;
  u8  bDalisetdivider;
  u8  bDalisetdividervalue;
  u8  fChangeSetValueDown;
} tSensodim;

extern tSensodim   SensoDim,RemoteSensoDim;


void ControllerStatusUpdate(u8 On1,u8 On2);
void ControllerSetLuxUpdate(u16 Setlux);
u16 CalculateSetlux(u8 sliderval);


void PIR_TimeController(void);
void SensoDimController(void);
void RemoteSensoDimController(void);

void InitSensodim(void);


#endif







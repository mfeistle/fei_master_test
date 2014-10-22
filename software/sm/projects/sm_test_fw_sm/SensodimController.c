  /******************************************************************************
  * @file    SensodimController.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    06.2012
  * @brief   1 or 2 Channel Luxcontrol, On/Off Statemachine Control
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  * @clock HSE_VALUE = 12000000 / SYSCLK_FREQ_24MHz
  ******************************************************************************
  


*/ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"

#include <stdio.h>
#include <stdlib.h>
#include "MAIN.h"
#include "rs.h"
#include "AAWmodule.h"
#include "modbus_master.h"
#include "SensodimController.h"
#include "Lampcontroller.h"
#include "OutputController.h"
#include "KNXinterface.h"
#include "flashsetup.h"


tSensodim   SensoDim,RemoteSensoDim;


static unsigned char ChangedCh1,ChangedCh2;

//-------------------------------------------------------------------------------
//
void ControllerSetLuxUpdate(u16 Setlux)
{
  LAMP_Ch1.wSetLux=Setlux;
}

//-------------------------------------------------------------------------------
//
u16 CalculateSetlux(u8 sliderval)
{
  u32 u32tmp;
  
  u32tmp=LAMP_Ch1.wSetLuxMin+(sliderval*(LAMP_Ch1.wSetLuxMax-LAMP_Ch1.wSetLuxMin)/127);
  
  return(u32tmp);
}

//-------------------------------------------------------------------------------
// called every 1sec 
//
void PIR_TimeController(void)
{
  if (Flash.Setup.PIR_MODE != PIR_MODE_OFF) {
    if(SensoDim.wPIR_Disabletimer!=0) {
      SensoDim.wPIR_Disabletimer--;
      if(SensoDim.wPIR_Disabletimer==0) {
        LAMP_Ch1.fManualOff=0;
      }
    }
    if(SensoDim.wPIR_Ontimer>1) {
      SensoDim.wPIR_Ontimer--;
    }
    
    if(RemoteSensoDim.wPIR_Disabletimer!=0) {
      RemoteSensoDim.wPIR_Disabletimer--;
      if(RemoteSensoDim.wPIR_Disabletimer==0) {
        LAMP_Ch2.fManualOff=0;
      }
    }
    if(RemoteSensoDim.wPIR_Ontimer>1) {
      RemoteSensoDim.wPIR_Ontimer--;
    }
  }
}

//------------------------------------------------------------------------------
// called every 10msec 
//
// Taste wird nach Ablauf der Prellzeit von 20msec bis 500msec überprüft, ab
// dann wird Ein/Ausschaltfunktion betätigt, sobald Taste losgelassen wird.
// Wenn Taste immer noch betätigt ist, wird der Luxstellwert verändert.
// 
// Key is tested after debounce time (20msec) to 500msec, On/Off request is
// performed if key is released. If still activ after 500msec, Lux set value
// is changed.
// SensoDim.ON_OFF_Timer is a Countdowntimer.
//
void SensoDimController(void)
{
  if(SensoDim.bOn) {
    if(SensoDim.ON_OFF_Timer>ADJUST_LUXTIME && SensoDim.ON_OFF_Timer<=BOUNCE_TIME && !SensoDim.OffBlockTime) {
      if(SensoSwitch1Off() && Button230VOff()) {
        if(!SensoDim.bClickCount) SensoDim.bClickCount=1;
      } else {
        if(SensoDim.bClickCount==1) SensoDim.bClickCount=2;
      }
    }
    if(SensoDim.ON_OFF_Timer<=ADJUST_LUXTIME) {
      if(SensoSwitch1On() || Button230VOn()) {
        SensoDim.ON_OFF_Timer=ADJUST_LUXOFFDELAY;
        ChangedCh1=1;
        if(!SensoDim.bKeySetFunction) { // prepare to also control Output directly
          if(!SensoDim.fChangeSetValueDown) { // increase brightness
            if(LAMP_Ch1.wSetLux<LAMP_Ch1.wSetLuxMax) 
              LAMP_Ch1.wSetLux+=LAMP_Both.wSetLuxIncrement;
          } else { // decrease brightness
            if(LAMP_Ch1.wSetLux>LAMP_Ch1.wSetLuxMin) 
              LAMP_Ch1.wSetLux-=LAMP_Both.wSetLuxIncrement;
          }
        } else {
          if(SensoDim.bDalisetdivider) SensoDim.bDalisetdivider--;
          else {
            SensoDim.bDalisetdivider=SensoDim.bDalisetdividervalue;
            if(!SensoDim.fChangeSetValueDown) { // increase brightness
              if(OUTPUT.bDaliSetLevelDirect<OUTPUT.bMaxDalilevel) 
                OUTPUT.bDaliSetLevelDirect++; 
            } else { // decrease brightness
              if(OUTPUT.bDaliSetLevelDirect>OUTPUT.bMinDalilevel) 
                OUTPUT.bDaliSetLevelDirect--;
            }
          }
        }
      } else {
        if(ChangedCh1) {
          if(SensoDim.ON_OFF_Timer==1) {
            if(SensoDim.fChangeSetValueDown) SensoDim.fChangeSetValueDown=0;
            else SensoDim.fChangeSetValueDown=1;
            ChangedCh1=0;
          }
        } else if(SensoDim.bClickCount) {
          if(SensoDim.bClickCount==1) SensoDim.bOn=0;
          else {
            if(SensoDim.bClickCount==2) SensoDim.fDoubleClick++;
          }
          SensoDim.bClickCount=0;
        }
      }
    }
  } else { // Lamp was off -> switch on immediately if timer is 100 ie new
           // switch activation was detected
    if(SensoDim.ON_OFF_Timer==ON_OFF_TIME) {
      SensoDim.OffBlockTime=ADJUST_LUXTIME;
      SensoDim.bOn=3;
      SensoDim.bClickCount=0;
      SensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime;
    }
  }
  
  if(SensoDim.ON_OFF_Timer) {
    SensoDim.ON_OFF_Timer--; // Discard Inputs for 1sec (100x10msec)
  }
  if(SensoDim.OffBlockTime) SensoDim.OffBlockTime--;

  if(SensoDim.PIR_FLAG) {
    SensoDim.PIR_FLAG=0;
    switch (Flash.Setup.PIR_MODE)
    {
      case PIR_MODE_OFF: // Ignore PIR sensor signals
        break;
      case PIR_MODE_MAN_ON:
        if(SensoDim.bOn) {
          SensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
        }
        break;
      case PIR_MODE_AUTO:
      default: // Unknown mode defaults to 'automatic' mode
        if(!SensoDim.bOn) {
          if(LAMP_Ch1.fManualOff) 
            SensoDim.wPIR_Disabletimer=LAMP_Both.wPIR_Disabletime;
          if(!SensoDim.wPIR_Disabletimer) {
            SensoDim.bOn=3;
            SensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
          }
        } else {
          SensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
        }
        break;
    }
  }
}
  
//------------------------------------------------------------------------------
// called every 10msec 
//
// Taste wird nach Ablauf der Prellzeit von 20msec bis 500msec überprüft, ab
// dann wird Ein/Ausschaltfunktion betätigt, sobald Taste losgelassen wird.
// Wenn Taste immer noch betätigt ist, wird der Luxstellwert verändert.
// 
// Key is tested after debounce time (20msec) to 500msec, On/Off request is
// performed if key is released. If still activ after 500msec, Lux set value
// is changed.
// RemoteSensoDim.ON_OFF_Timer is a Countdowntimer.
//
void RemoteSensoDimController(void)
{
  if(RemoteSensoDim.bOn) {
    if(RemoteSensoDim.ON_OFF_Timer>ADJUST_LUXTIME && RemoteSensoDim.ON_OFF_Timer<=BOUNCE_TIME && !RemoteSensoDim.OffBlockTime) {
      if(SensoSwitch2Off()) {
        if(!RemoteSensoDim.bClickCount) RemoteSensoDim.bClickCount=1;
      } else {
        if(RemoteSensoDim.bClickCount==1) RemoteSensoDim.bClickCount=2;
      }
    }
    if(RemoteSensoDim.ON_OFF_Timer<=ADJUST_LUXTIME) {
      if(SensoSwitch2On()) {
        RemoteSensoDim.ON_OFF_Timer=ADJUST_LUXOFFDELAY;
        ChangedCh2=1;
        if(!RemoteSensoDim.bKeySetFunction) { // prepare to also control Output directly
          if(!RemoteSensoDim.fChangeSetValueDown) { // turn brighter
            if(LAMP_Ch2.wSetLux<LAMP_Ch2.wSetLuxMax) 
              LAMP_Ch2.wSetLux+=LAMP_Both.wSetLuxIncrement;
          } else {
            if(LAMP_Ch2.wSetLux>LAMP_Ch2.wSetLuxMin) 
              LAMP_Ch2.wSetLux-=LAMP_Both.wSetLuxIncrement;
          }
        } else {
          if(RemoteSensoDim.bDalisetdivider) SensoDim.bDalisetdivider--;
          else {
            RemoteSensoDim.bDalisetdivider=RemoteSensoDim.bDalisetdividervalue;
            if(!RemoteSensoDim.fChangeSetValueDown) { // heller machen
              if(OUTPUT.bDaliSetLevelIndirect<OUTPUT.bMaxDalilevel) 
                OUTPUT.bDaliSetLevelIndirect++; 
            } else {
              if(OUTPUT.bDaliSetLevelIndirect>OUTPUT.bMinDalilevel) 
                OUTPUT.bDaliSetLevelIndirect--;
            }
          }
        }
      } else {
        if(ChangedCh2) {
          if(RemoteSensoDim.ON_OFF_Timer==1) {
            if(RemoteSensoDim.fChangeSetValueDown) RemoteSensoDim.fChangeSetValueDown=0;
            else RemoteSensoDim.fChangeSetValueDown=1;
            ChangedCh2=0;
          }
        } else if(RemoteSensoDim.bClickCount) {
          if(RemoteSensoDim.bClickCount==1) RemoteSensoDim.bOn=0;
          else {
            if(RemoteSensoDim.bClickCount==2) RemoteSensoDim.fDoubleClick++;
          }
          RemoteSensoDim.bClickCount=0;
        }
      }
    }
  } else { // Lamp was off -> switch on immediately if Timer is 100 ie new Switch Activation was detected
    if(RemoteSensoDim.ON_OFF_Timer==ON_OFF_TIME) {
      RemoteSensoDim.OffBlockTime=ADJUST_LUXTIME;
      RemoteSensoDim.bOn=3;
      RemoteSensoDim.bClickCount=0;
      RemoteSensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime;
    }
  }
  
  if(RemoteSensoDim.ON_OFF_Timer) {
    RemoteSensoDim.ON_OFF_Timer--; // Discard Inputs for 1sec (100x10msec)
  }
  if(RemoteSensoDim.OffBlockTime) RemoteSensoDim.OffBlockTime--;

  if(RemoteSensoDim.PIR_FLAG)
  {
    RemoteSensoDim.PIR_FLAG=0;
    if(!RemoteSensoDim.bOn) {
      if(LAMP_Ch2.fManualOff) 
         RemoteSensoDim.wPIR_Disabletimer=LAMP_Both.wPIR_Disabletime;
      if(!RemoteSensoDim.wPIR_Disabletimer) {
        RemoteSensoDim.bOn=3;
        RemoteSensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
      }
    } else {
      RemoteSensoDim.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
    }
    
  } // end if Pirflag
}


//------------------------------------------------------------------------------
// called before main loop starts
//
void InitSensodim(void)
{
  SensoDim.fChangeSetValueDown=0;
  RemoteSensoDim.fChangeSetValueDown=0;
  // Dali Control, if LUX Ctrl not enabled
  if(Flash.Setup.LUX_CTRL_EN & 1) 
    SensoDim.bKeySetFunction=0;
  else
    SensoDim.bKeySetFunction=1;
  if(Flash.Setup.LUX_CTRL_EN & 2) 
    RemoteSensoDim.bKeySetFunction=0;
  else
    RemoteSensoDim.bKeySetFunction=1;
  SensoDim.bDalisetdividervalue=2;
  RemoteSensoDim.bDalisetdividervalue=2;
}

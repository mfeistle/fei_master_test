  /******************************************************************************
  * @file    Lampcontroller.c
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
#include "Lampcontroller.h"
#include "SensodimController.h"
#include "SliderController.h"
#include "KNXinterface.h"
#include "flashsetup.h"

tLAMPcontrol  LAMP_Ch1,LAMP_Ch2;
tLAMPcontrol_common LAMP_Both;

//-------------------------------------------------------------------------------
static u16 KNX_LightLevelToDimPromille(u32 lightLevel)
{
  if (RemoteCtrl.Active) {
    if (lightLevel > 254) { // For safety
      lightLevel = 254;
    }
  }
  else {
    if(lightLevel<=254) {
      lightLevel*=3938;
      lightLevel/=1000;
    }
    else {
      lightLevel = 1000; // For safety
    }
  }
  return (u16)lightLevel;
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
static void LuxControllerCh1(void)
{
  u32 ulx;
  
  if(LAMP_Ch1.bLuxControllerON) {
    if(KNX.Mode != 2) {
      // Powersaving ?
      if(LAMP_Both.fPowerSavingModeON) {
        ulx=LAMP_Ch1.wSetLux;
        ulx *= LAMP_Both.bPowerSavingErrorPercent;
        ulx /= 100;
        LAMP_Ch1.wPowerSavingLux=ulx; //  LAMP_Ch1.wSetLux/2;
        if(LAMP_Ch1.wLux > LAMP_Ch1.wPowerSavingLux + LAMP_Ch1.wSetLux) { // if > 50% more light than desired set Powersaving mode
          if(LAMP_Ch1.ulPowerSavingtimer>1)
            LAMP_Ch1.ulPowerSavingtimer--;
          else if(LAMP_Ch1.ulPowerSavingtimer==1) {
            LAMP_Ch1.ulPowerSavingtimer=0;
            LAMP_Ch1.bStatus=STATUS_GOTOSAVINGPOWER;
          }
        } else
          LAMP_Ch1.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
      }

      // simple Controller with 3 Controlspeeds
      LAMP_Ch1.iLuxError=(s16)LAMP_Ch1.wSetLux-(s16)LAMP_Ch1.wLux;
      
      if(LAMP_Ch1.wSetLux!=LAMP_Ch1.wLastSetLux) // faster reaction for 2.5seconds, when Setvalue has changed
        LAMP_Ch1.bLuxCtrlFastTimer=250;
      
      if(LAMP_Ch1.bLuxCtrlFastTimer) { LAMP_Ch1.bLuxCtrlFastTimer--; LAMP_Ch1.bLuxCtrlTime=1-1;}
      else if(abs(LAMP_Ch1.iLuxError)>150) LAMP_Ch1.bLuxCtrlTime=1-1; // alle 10msec Klammerkorrektur V0.17
      else if(abs(LAMP_Ch1.iLuxError)>50) LAMP_Ch1.bLuxCtrlTime=2-1; // alle 20msec Klammerkorrektur V0.17
      else LAMP_Ch1.bLuxCtrlTime=5-1; // alle 50msec
      
      if(LAMP_Ch1.bLuxCtrlTimer) LAMP_Ch1.bLuxCtrlTimer--; // Controller Time Divider
      else {
        LAMP_Ch1.bLuxCtrlTimer=LAMP_Ch1.bLuxCtrlTime;
        // only change if error > 5 Lux
        if(LAMP_Ch1.wSetLux > LAMP_Ch1.wLux+5)      {
          if(LAMP_Ch1.wDimPromille<LAMP_Ch1.wMinimumDimPromille) LAMP_Ch1.wDimPromille=LAMP_Ch1.wMinimumDimPromille;
          if(LAMP_Ch1.wDimPromille<1000) 
            LAMP_Ch1.wDimPromille++; 
        } else if(LAMP_Ch1.wLux > LAMP_Ch1.wSetLux+5) {
          if(LAMP_Ch1.wDimPromille>LAMP_Ch1.wMinimumDimPromille)      
            LAMP_Ch1.wDimPromille--; 
          else
            LAMP_Ch1.wDimPromille=0;
        }
      }
    } else {
      LAMP_Ch1.wDimPromille = KNX_LightLevelToDimPromille(KNX.LightLevelCh1);
    }
  } else {
    LAMP_Ch1.wDimPromille = KNX_LightLevelToDimPromille(KNX.LightLevelCh1);
  }
}

//-------------------------------------------------------------------------------
//
static void CheckSwitchOffCh1(void)
{
  if(KNX.Mode) {
    if(!KNX.OnCh1) {
      LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch1.fManualOff=1;
    }
  } else {
    if(!SensoDim.bOn) {
      LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
      SLIDER.bKeyToggleStates=0;
      LAMP_Ch1.fManualOff=1;
    } else if(SensoDim.wPIR_Ontimer==1) { // expired PIR Timer
      SensoDim.wPIR_Ontimer=0;
      SLIDER.bKeyToggleStates=0;
      LAMP_Ch1.fManualOff=0;
      LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
      SensoDim.bOn=0;
    } else if(!(SLIDER.bKeyToggleStates & 2)) {
      LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch1.fManualOff=1;
      SLIDER.bKeyToggleStates=0;
      SensoDim.bOn=0;
    } else if(!(SLIDER.bKeyToggleStates & 1)) {
      LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch1.fManualOff=1;
      if(!(SLIDER.bKeyToggleStates & 4)) SLIDER.bKeyToggleStates=0;
      else SLIDER.bKeyToggleStates &= ~1;
      if(!SLIDER.bKeyToggleStates) SensoDim.bOn=0;
      else SensoDim.bOn=2;
    }
  }
  if(LAMP_Ch1.bStatus==STATUS_SWITCHINGOFF) {
    //SensoDim.bOn=0;
    if(LAMP_Ch1.fManualOff) 
      SensoDim.wPIR_Disabletimer=LAMP_Both.wPIR_Disabletime;
    SYS.bLastDali1Output=SYS.bDali1Output; // V0.43
    LAMP_Ch1.bRelaisOfftimer=200; // 2 Sekunden bis Relais Aus
    LAMP_Ch1.bSoftOffTimer=LAMP_Both.bSoftOffTime;
    LAMP_Ch1.wDimPromilleStep=LAMP_Ch1.wDimPromille/LAMP_Both.bSoftOffTime;
  }
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
void StatusControllerCh1(void)
{                
  switch(LAMP_Ch1.bStatus) {
    case STATUS_OFF: 
    {
      if(KNX.Mode) {
        if(KNX.OnCh1) {
          LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
          SLIDER.bKeyToggleStates=1+2+4;
        }
      } else {
        if(SensoDim.bOn == 3) {
          LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
          SLIDER.bKeyToggleStates=1+2+4;
        } else if(SLIDER.bKeyToggleStates & 1) {
          LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
        }
      }

      if(LAMP_Ch1.bStatus==STATUS_SWITCHINGON) {
        LAMP_Ch1.bRelaisOfftimer=0;
        LAMP_Ch1.bForceOutput=10;
        SYS.fRelais1=1;
        SensoDim.bOn|=1;
        LAMP_Ch1.fOn=1;
      }
      break;
    } 
    case STATUS_SWITCHINGON:
    {
      if(LAMP_Ch1.bSoftOnTimer) { // wird bei dieser App nicht benötigt, ist auf 10msec gesetzt
        LAMP_Ch1.bSoftOnTimer--;
      } else {
        LAMP_Ch1.bStatus=STATUS_ON;
        LAMP_Ch1.wDimPromille=200;
        LAMP_Ch1.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
      }
      break;
    } 
    case STATUS_SWITCHINGOFF: 
    {
      if(LAMP_Ch1.bSoftOffTimer) {
        LAMP_Ch1.bSoftOffTimer--;
        if(LAMP_Ch1.wDimPromille>LAMP_Ch1.wDimPromilleStep)
          LAMP_Ch1.wDimPromille-=LAMP_Ch1.wDimPromilleStep;
        else
          LAMP_Ch1.wDimPromille=0;
      } 
      if(LAMP_Ch1.bRelaisOfftimer>1) LAMP_Ch1.bRelaisOfftimer--;
      else if(LAMP_Ch1.bRelaisOfftimer==1) {
        SYS.fRelais1=0;
        LAMP_Ch1.fOn=0;
        LAMP_Ch1.bRelaisOfftimer=0;
        LAMP_Ch1.bStatus=STATUS_OFF;
        LAMP_Ch1.wDimPromille=0;
        //LAMP_Ch1.wSetLux=LAMP_Ch1.wSetLuxMin; // V0.43
      }
      break;
    } 
    case STATUS_ON: 
    {
      if(LAMP_Ch2.bStatus!=STATUS_ON || !LAMP_Ch2.fSensoDimOK || LAMP_Both.bCouplingModus!=1)
        LuxControllerCh1(); 
      CheckSwitchOffCh1();
      break;
    } 
    case STATUS_GOTOSAVINGPOWER: 
    {
      SYS.fRelais1=0;
      if(!LAMP_Ch2.fSensoDimOK) {// ???
        if(LAMP_Ch2.bStatus==STATUS_ON)
          LAMP_Ch2.bStatus=STATUS_GOTOSAVINGPOWER;
      }

      LAMP_Ch1.bStatus=STATUS_SAVINGPOWER;
      break;
    } 
    case STATUS_SAVINGPOWER: 
    {
      if(LAMP_Ch2.bStatus<STATUS_GOTOSAVINGPOWER || LAMP_Both.bCouplingModus!=1) {
        if(LAMP_Ch1.wLux < LAMP_Ch1.wSetLux) {
          LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
          SYS.fRelais1=1;
          LAMP_Ch1.bForceOutput=10;
          if(!LAMP_Ch2.fSensoDimOK) {
            if(LAMP_Ch2.bStatus==STATUS_SAVINGPOWER) {
              SYS.fRelais2=1;
              LAMP_Ch2.bForceOutput=10;
              LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
            }
          }
        }
      }
      CheckSwitchOffCh1();
      break;
    } 

    default:
      break;
  }
}

//-------------------------------------------------------------------------------
//
static void CheckSwitchOffCh2(void)
{
  if(KNX.Mode) {
    if(!KNX.OnCh2) {
      LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch2.fManualOff=1;
    }
  } else {
    if(!(SensoDim.bOn)) {
      LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
      SLIDER.bKeyToggleStates=0;
      LAMP_Ch2.fManualOff=1;
    } else if(SensoDim.wPIR_Ontimer==1) { // expired PIR Timer
      SensoDim.wPIR_Ontimer=0;
      SLIDER.bKeyToggleStates=0;
      LAMP_Ch2.fManualOff=0;
      LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
    } else if(!(SLIDER.bKeyToggleStates & 2)) {
      LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch2.fManualOff=1;
      SLIDER.bKeyToggleStates=0;
      SensoDim.bOn=0;
    } else if(!(SLIDER.bKeyToggleStates & 4)) {
      LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch2.fManualOff=1;
      if(!(SLIDER.bKeyToggleStates & 1)) SLIDER.bKeyToggleStates=0;
      else SLIDER.bKeyToggleStates &= ~4;
      if(!SLIDER.bKeyToggleStates) SensoDim.bOn=0;
      else SensoDim.bOn=1;
    }
  }
  if(LAMP_Ch2.bStatus==STATUS_SWITCHINGOFF) {
    SYS.bLastDali2Output=SYS.bDali2Output; // V0.43
    LAMP_Ch2.bRelaisOfftimer=200; // 2 Sekunden bis Relais Aus
    LAMP_Ch2.bSoftOffTimer=LAMP_Both.bSoftOffTime;
    LAMP_Ch2.wDimPromilleStep=LAMP_Ch2.wDimPromille/LAMP_Both.bSoftOffTime;
  }
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
void StatusControllerCh2(void)
{                
  switch(LAMP_Ch2.bStatus) {
    case STATUS_OFF: 
    {
      if(KNX.Mode) {
        if(KNX.OnCh2) {
          LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
          SLIDER.bKeyToggleStates=1+2+4;
        }
      } else {
        if(SensoDim.bOn == 3) {
          LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
          SLIDER.bKeyToggleStates=1+2+4;
        } else if(SLIDER.bKeyToggleStates & 4) {
          LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
        }
      }

      if(LAMP_Ch2.bStatus==STATUS_SWITCHINGON) {
        LAMP_Ch2.bRelaisOfftimer=0;
        LAMP_Ch2.bForceOutput=10;
        SYS.fRelais2=1;
        SensoDim.bOn|=2;
        LAMP_Ch2.fOn=1;
      }
      break;
    } 
    case STATUS_SWITCHINGON: 
    {
      if(LAMP_Ch2.bSoftOnTimer) { // wird bei dieser App nicht benötigt, ist auf 10msec gesetzt
        LAMP_Ch2.bSoftOnTimer--;
      } else {
        LAMP_Ch2.bStatus=STATUS_ON;
        LAMP_Ch2.wDimPromille=200;
        LAMP_Ch2.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
      }
      break;
    } 
    case STATUS_SWITCHINGOFF:
    {
      if(LAMP_Ch2.bSoftOffTimer) {
        LAMP_Ch2.bSoftOffTimer--;
        if(LAMP_Ch2.wDimPromille>LAMP_Ch2.wDimPromilleStep)
          LAMP_Ch2.wDimPromille-=LAMP_Ch2.wDimPromilleStep;
        else
          LAMP_Ch2.wDimPromille=0;
      } 
      if(LAMP_Ch2.bRelaisOfftimer>1) LAMP_Ch2.bRelaisOfftimer--;
      else if(LAMP_Ch2.bRelaisOfftimer==1) {
        SYS.fRelais2=0;
        LAMP_Ch2.fOn=0;
        LAMP_Ch2.bRelaisOfftimer=0;
        LAMP_Ch2.bStatus=STATUS_OFF;
        LAMP_Ch2.wDimPromille=0;
        LAMP_Ch2.wSetLux=LAMP_Ch2.wSetLuxMin; // V0.43
      }
      break;
    } 
    case STATUS_ON: 
    {
      // if(LAMP_Ch2.bStatus!=STATUS_ON || !LAMP_Ch2.fSensoDimOK || LAMP_Both.bCouplingModus!=1)
      //  LuxControllerCh1(); 
      if(KNX.Mode==2) {
        LAMP_Ch2.wDimPromille = KNX_LightLevelToDimPromille(KNX.LightLevelCh2);
      }
      CheckSwitchOffCh2();
      break;
    } 
    case STATUS_GOTOSAVINGPOWER: 
    {
      SYS.fRelais2=0;

      LAMP_Ch2.bStatus=STATUS_SAVINGPOWER;
      break;
    } 
    case STATUS_SAVINGPOWER: 
    {
      if(LAMP_Ch2.fSensoDimOK) {
        if(LAMP_Ch1.bStatus<STATUS_GOTOSAVINGPOWER || LAMP_Both.bCouplingModus!=1) {
          if(LAMP_Ch2.wLux < LAMP_Ch2.wSetLux) {
            LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
            SYS.fRelais2=1;
            LAMP_Ch2.bForceOutput=10;
          }
        }
      }
      CheckSwitchOffCh2();
      break;
    } 

    default:
      break;
  }
  
}

//-------------------------------------------------------------------------------
// called before Mainloop starts
//
void InitLampController(void)
{

  LAMP_Both.bSoftOnTime=1;   // Rampe in n x 10msec nicht benutzt wenn keine Indirekte Beleuchtung da ist
  LAMP_Both.bSoftOffTime=100; // Rampe in n x 10msec
  LAMP_Both.wSetLuxIncrement=2; // Increment / Decrement pro 10msec 

  LAMP_Ch1.wSetDimPromille=800;
  LAMP_Ch1.wStartDimPromille=50; 
  LAMP_Ch1.wMinimumDimPromille=30; // entspricht 3% Dali minimum Wert EVG
  
  LAMP_Ch2.wSetDimPromille=800;
  LAMP_Ch2.wStartDimPromille=50; 
  LAMP_Ch1.wMinimumDimPromille=30; // entspricht 3% Dali minimum Wert EVG

  if(Flash.Setup.SETUP_VALID == SETUP_VALID_MARK) {
    LAMP_Both.wPIR_Ontime=Flash.Setup.PIR_OFF_DELAY;
    LAMP_Both.wPIR_Disabletime=Flash.Setup.PIR_ON_DELAY;
    if(Flash.Setup.LUX_CTRL_EN & 1) LAMP_Ch1.bLuxControllerON=1;
    if(Flash.Setup.LUX_CTRL_EN & 2) LAMP_Ch2.bLuxControllerON=1;

    LAMP_Ch1.wSetLuxMin=LAMP_Ch2.wSetLuxMin=Flash.Setup.LUX_NOM_MIN;
    LAMP_Ch1.wSetLuxMax=LAMP_Ch2.wSetLuxMax=Flash.Setup.LUX_NOM_MAX;
    if(Flash.Setup.PWR_FAIL_MODE==2)
      LAMP_Ch1.wSetLux=LAMP_Ch2.wSetLux=Flash.Setup.LUX_NOM_MIN;
    else 
      LAMP_Ch1.wSetLux=LAMP_Ch2.wSetLux=Flash.Setup.LUX_NOM_START;
    //if(Flash.Setup.PWR_FAIL_MODE==3) restored value of EEprom
    LAMP_Both.bPowerSavingErrorPercent=Flash.Setup.LUX_ERR_OFF; 
    LAMP_Both.wPowerSavingtime=Flash.Setup.LUX_ERR_OFF_DELAY;
  } else {
    LAMP_Both.wPIR_Ontime=10*60; // sekunden
    LAMP_Both.wPIR_Disabletime=10*60; // sekunden
    LAMP_Ch1.bLuxControllerON=0;
    LAMP_Ch2.bLuxControllerON=0;
    LAMP_Both.bPowerSavingErrorPercent=50;
    LAMP_Both.wPowerSavingtime=5*60; 
    if(SYS.bApplication==APPLICATION_ALONEATWORK) {
      LAMP_Both.bAAW_ModusOn=1; // 1 wenn AAW Funktion eingeschaltet werden soll
      
      //LAMP_Both.bCouplingModus=0; // unabhängig
      LAMP_Both.bCouplingModus=1; // gekoppelt

      LAMP_Ch1.wSetLuxMin=500+200;
      LAMP_Ch1.wSetLux=LAMP_Ch1.wSetLuxMin;
      LAMP_Ch1.wSetLuxMax=1500; // 750+200;

      LAMP_Ch2.wSetLuxMin=LAMP_Ch1.wSetLuxMin;
      LAMP_Ch2.wSetLux=LAMP_Ch1.wSetLux;
      LAMP_Ch2.wSetLuxMax=LAMP_Ch1.wSetLuxMax;

    } else {
      LAMP_Ch1.wSetLux=500;
      LAMP_Ch1.wSetLuxMin=200;
      LAMP_Ch1.wSetLuxMax=1000;

      LAMP_Ch2.wSetLux=500;
      LAMP_Ch2.wSetLuxMin=200;
      LAMP_Ch2.wSetLuxMax=1000;
    }
  }
  if(LAMP_Both.bPowerSavingErrorPercent) 
    LAMP_Both.fPowerSavingModeON=1; // Powersaving bei Überbeleuchtung während xx Sekunden
}

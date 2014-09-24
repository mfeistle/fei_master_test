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


tLAMPcontrol  LAMP_Ch1,LAMP_Ch2;
tLAMPcontrol_common LAMP_Both;

/*
// Zeiten in 10msec Schritten
#define ON_OFF_TIME     100
#define BOUNCE_TIME     (ON_OFF_TIME-2)
#define ADJUST_LUXTIME  (ON_OFF_TIME-50)
*/

static unsigned char ChangedCh1,ChangedCh2,OffBlockTimeCh1,OffBlockTimeCh2;

//-------------------------------------------------------------------------------
// called every 1sec 
//
void PirTimeController(void)
{
  if(SYS.bApplication!=APPLICATION_TOUCHPANEL) {
    if(LAMP_Ch1.wPIR_Disabletimer) LAMP_Ch1.wPIR_Disabletimer--;
    if(LAMP_Ch1.wPIR_Ontimer>1) LAMP_Ch1.wPIR_Ontimer--;
    if(LAMP_Ch2.wPIR_Disabletimer) LAMP_Ch2.wPIR_Disabletimer--;
    if(LAMP_Ch2.wPIR_Ontimer>1) LAMP_Ch2.wPIR_Ontimer--;
  }
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
void CalculateAverageLuxvalues(void)
{
  LAMP_Both.iAverageLux=LAMP_Ch1.wLux+LAMP_Ch2.wLux;
  LAMP_Both.iAverageLux/=2;
  LAMP_Both.iAverageSetLux=LAMP_Ch1.wSetLux+LAMP_Ch2.wSetLux;
  LAMP_Both.iAverageSetLux/=2;
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
void LuxControllerCh1(void)
{
  if(LAMP_Ch1.bLuxControllerON) {

    // Powersaving ?
    if(LAMP_Both.fPowerSavingModeON) {
      if(LAMP_Ch1.wLux > LAMP_Both.wPowerSavingLux + LAMP_Ch1.wSetLux) {
        if(LAMP_Ch1.ulPowerSavingtimer>1)
          LAMP_Ch1.ulPowerSavingtimer--;
        else if(LAMP_Ch1.ulPowerSavingtimer==1) {
          LAMP_Ch1.ulPowerSavingtimer=0;
          // Modus auf Powersaving setzen 
          LAMP_Ch1.bStatus=STATUS_GOTOSAVINGPOWER;
        }
      } else
        LAMP_Ch1.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
    }

    // Einfacher Regler mit 3 Regelgeschwindigkeiten
    LAMP_Ch1.iLuxError=(s16)LAMP_Ch1.wSetLux-(s16)LAMP_Ch1.wLux;
      
    if(abs(LAMP_Ch1.iLuxError)>150) LAMP_Ch1.bLuxCtrlTime=1-1; // alle 10msec Klammerkorrektur V0.17
    else if(abs(LAMP_Ch1.iLuxError)>50) LAMP_Ch1.bLuxCtrlTime=2-1; // alle 20msec Klammerkorrektur V0.17
    else LAMP_Ch1.bLuxCtrlTime=5-1; // alle 50msec
    if(LAMP_Ch1.bLuxCtrlTimer) LAMP_Ch1.bLuxCtrlTimer--; // Regler Divider
    else {
      LAMP_Ch1.bLuxCtrlTimer=LAMP_Ch1.bLuxCtrlTime;
      // Nur verändern wenn Fehler > 5 Lux
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
  }
  
}


//-------------------------------------------------------------------------------
// called every 10msec 
//
void LuxControllerCh2(void)
{
  // AAW LuxControl
  if(LAMP_Ch2.bLuxControllerON) {

    // Powersaving ?
    if(LAMP_Both.fPowerSavingModeON) {
      if(LAMP_Ch2.wLux > LAMP_Both.wPowerSavingLux + LAMP_Ch2.wSetLux) {
        if(LAMP_Ch2.ulPowerSavingtimer>1)
          LAMP_Ch2.ulPowerSavingtimer--;
        else if(LAMP_Ch2.ulPowerSavingtimer==1) {
          LAMP_Ch2.ulPowerSavingtimer=0;
          // Modus auf Powersaving setzen 
          LAMP_Ch2.bStatus=STATUS_GOTOSAVINGPOWER;
        }
      } else
        LAMP_Ch2.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
    }

    // Einfacher Regler mit 3 Regelgeschwindigkeiten
    LAMP_Ch2.iLuxError=(s16)LAMP_Ch2.wSetLux-(s16)LAMP_Ch2.wLux;
    if(abs(LAMP_Ch2.iLuxError)>150) LAMP_Ch2.bLuxCtrlTime=1-1; // alle 10msec Klammerkorrektur V0.17
    else if(abs(LAMP_Ch2.iLuxError)>50) LAMP_Ch2.bLuxCtrlTime=2-1; // alle 20msec Klammerkorrektur V0.17
    else LAMP_Ch2.bLuxCtrlTime=5-1; // alle 50msec
    if(LAMP_Ch2.bLuxCtrlTimer) LAMP_Ch2.bLuxCtrlTimer--; // Regler Divider
    else {
      LAMP_Ch2.bLuxCtrlTimer=LAMP_Ch2.bLuxCtrlTime;
      // Nur verändern wenn Fehler > 5 Lux
      if(LAMP_Ch2.wSetLux > LAMP_Ch2.wLux+5)      {
        if(LAMP_Ch2.wDimPromille<LAMP_Ch2.wMinimumDimPromille) LAMP_Ch2.wDimPromille=LAMP_Ch2.wMinimumDimPromille;
        if(LAMP_Ch2.wDimPromille<1000) 
          LAMP_Ch2.wDimPromille++; 
      } else if(LAMP_Ch2.wLux > LAMP_Ch2.wSetLux+5) {
        if(LAMP_Ch2.wDimPromille>LAMP_Ch2.wMinimumDimPromille)      
          LAMP_Ch2.wDimPromille--; 
        else
          LAMP_Ch2.wDimPromille=0;
      }
    }
  }
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
// nimmt Mittelwert von beiden Luxsensoren und Mittelwert von beiden Sollwerten
// benutzt Ch1 für Berechnung und koppelt beide Ausgabewerte 
// 
void LuxControllerCoupled(void)
{
  //s16 iAverageLux,iAverageSetLux;

  
  
  if(LAMP_Ch1.bLuxControllerON) {

    CalculateAverageLuxvalues();

    //if(LAMP_Ch1.wDimPromille<LAMP_Ch2.wDimPromille) // Anfangsbedingung
    //  LAMP_Ch1.wDimPromille=LAMP_Ch2.wDimPromille;

    // Powersaving ?
    if(LAMP_Both.fPowerSavingModeON) {
      if(LAMP_Both.iAverageLux > LAMP_Both.wPowerSavingLux + LAMP_Both.iAverageSetLux) {
        if(LAMP_Ch2.ulPowerSavingtimer>1)
          LAMP_Ch2.ulPowerSavingtimer--;
        else if(LAMP_Ch2.ulPowerSavingtimer==1) {
          LAMP_Ch2.ulPowerSavingtimer=0;
          // Modus auf Powersaving setzen 
          LAMP_Ch1.bStatus=LAMP_Ch2.bStatus=STATUS_GOTOSAVINGPOWER;
        }
      } else
        LAMP_Ch2.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
    }




    // Einfacher Regler mit 3 Regelgeschwindigkeiten benutze Variablen von Ch1 für gemeinsame Regelung
    LAMP_Ch1.iLuxError=LAMP_Both.iAverageSetLux-LAMP_Both.iAverageLux;

    if(abs(LAMP_Ch1.iLuxError)>150) LAMP_Ch1.bLuxCtrlTime=1-1; // alle 10msec Klammerkorrektur V0.17
    else if(abs(LAMP_Ch1.iLuxError)>50) LAMP_Ch1.bLuxCtrlTime=2-1; // alle 20msec Klammerkorrektur V0.17
    else LAMP_Ch1.bLuxCtrlTime=5-1; // alle 50msec
    if(LAMP_Ch1.bLuxCtrlTimer) LAMP_Ch1.bLuxCtrlTimer--; // Regler Divider
    else {
      LAMP_Ch1.bLuxCtrlTimer=LAMP_Ch1.bLuxCtrlTime;
      // Nur verändern wenn Fehler > 5 Lux
      if(LAMP_Both.iAverageSetLux > LAMP_Both.iAverageLux+5) {
        if(LAMP_Ch1.wDimPromille<LAMP_Ch1.wMinimumDimPromille) LAMP_Ch1.wDimPromille=LAMP_Ch1.wMinimumDimPromille;
        if(LAMP_Ch1.wDimPromille<1000) 
          LAMP_Ch1.wDimPromille++; 
      } else if(LAMP_Both.iAverageLux > LAMP_Both.iAverageSetLux+5) {
        if(LAMP_Ch1.wDimPromille>LAMP_Ch1.wMinimumDimPromille)      
          LAMP_Ch1.wDimPromille--; 
        else {
          LAMP_Ch1.wDimPromille=0;
          LAMP_Ch2.wDimPromille=0;
        }
      }
    }
    if(LAMP_Ch2.wDimPromille<LAMP_Ch2.wMinimumDimPromille) LAMP_Ch2.wDimPromille=LAMP_Ch2.wMinimumDimPromille;
    
    if(LAMP_Ch2.wDimPromille<LAMP_Ch1.wDimPromille) LAMP_Ch2.wDimPromille++;
    else if(LAMP_Ch2.wDimPromille>LAMP_Ch1.wDimPromille) LAMP_Ch2.wDimPromille--;
  }
}



//-------------------------------------------------------------------------------
// called every 10msec 
//
void StatusControllerCh1(void)
{                
  if(LAMP_Ch1.bStatus==STATUS_SWITCHINGON) {
    if(LAMP_Ch1.bSoftOnTimer) { // wird bei dieser App nicht benötigt, ist auf 10msec gesetzt
      LAMP_Ch1.bSoftOnTimer--;
    } else {
      LAMP_Ch1.bStatus=STATUS_ON;
      LAMP_Ch1.wDimPromille=200;
      LAMP_Ch1.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
    }
  } else if(LAMP_Ch1.bStatus==STATUS_SWITCHINGOFF) {
    if(LAMP_Ch1.bSoftOffTimer) {
      LAMP_Ch1.bSoftOffTimer--;
      if(LAMP_Ch1.wDimPromille>LAMP_Ch1.wDimPromilleStep)
        LAMP_Ch1.wDimPromille-=LAMP_Ch1.wDimPromilleStep;
      else
        LAMP_Ch1.wDimPromille=0;
    } else {
      LAMP_Ch1.bStatus=STATUS_OFF;
      LAMP_Ch1.wDimPromille=0;
    }
  } else if(LAMP_Ch1.bStatus==STATUS_ON) {
    if(LAMP_Ch2.bStatus!=STATUS_ON || !LAMP_Ch2.fSensoDimOK || LAMP_Both.bCouplingModus!=1)
      LuxControllerCh1(); 
  } else if(LAMP_Ch1.bStatus==STATUS_GOTOSAVINGPOWER) {
    SYS.fRelais1=0;
    if(!LAMP_Ch2.fSensoDimOK)
      SYS.fRelais2=0;

    LAMP_Ch1.bStatus=STATUS_SAVINGPOWER;
  } else if(LAMP_Ch1.bStatus==STATUS_SAVINGPOWER) {
    if(LAMP_Ch2.bStatus<STATUS_GOTOSAVINGPOWER || LAMP_Both.bCouplingModus!=1) {
      if(LAMP_Ch1.wLux < LAMP_Ch1.wSetLux) {
        LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
        SYS.fRelais1=1;
        if(!LAMP_Ch2.fSensoDimOK)
          SYS.fRelais2=1;
      }
    }
  } 
  
  // Verzögertes Ausschalten der Relais
  if(LAMP_Ch1.bRelaisOfftimer>1) LAMP_Ch1.bRelaisOfftimer--;
  else if(LAMP_Ch1.bRelaisOfftimer==1) {
    SYS.fRelais1=0;
    if(!LAMP_Ch2.fSensoDimOK)
      SYS.fRelais2=0;
    LAMP_Ch1.bRelaisOfftimer=0;
    LAMP_Ch1.bStatus=STATUS_OFF;
  }
  // 2. Kanal wird parallel gesteuert bei diesen Bedingungen
  if(SYS.bApplication==APPLICATION_ALONEATWORK && !LAMP_Ch2.fSensoDimOK)  
    LAMP_Ch2.wDimPromille=LAMP_Ch1.wDimPromille;
}

//-------------------------------------------------------------------------------
// called every 10msec 
//
void StatusControllerCh2(void)
{
  // AAW Kanal
  if(SYS.bApplication==APPLICATION_ALONEATWORK && LAMP_Ch2.fSensoDimOK) {
    if(LAMP_Ch2.bStatus==STATUS_SWITCHINGON) {
      if(LAMP_Ch2.bSoftOnTimer) {
        LAMP_Ch2.bSoftOnTimer--;
      } else {
        LAMP_Ch2.bStatus=STATUS_ON;
        LAMP_Ch2.wDimPromille=200;
        LAMP_Ch2.ulPowerSavingtimer=100*(u32)LAMP_Both.wPowerSavingtime;
      }
    } else if(LAMP_Ch2.bStatus==STATUS_SWITCHINGOFF) {
      if(LAMP_Ch2.bSoftOffTimer) {
        LAMP_Ch2.bSoftOffTimer--;
        if(LAMP_Ch2.wDimPromille>LAMP_Ch2.wDimPromilleStep)
          LAMP_Ch2.wDimPromille-=LAMP_Ch2.wDimPromilleStep;
        else
          LAMP_Ch2.wDimPromille=0;
      } else {
        LAMP_Ch2.bStatus=STATUS_OFF;
        LAMP_Ch2.wDimPromille=0;
      }
    } else if(LAMP_Ch2.bStatus==STATUS_ON) {
      if(LAMP_Ch1.bStatus!=STATUS_ON || LAMP_Both.bCouplingModus!=1)
        LuxControllerCh2();
      else
        LuxControllerCoupled();
    } else if(LAMP_Ch2.bStatus==STATUS_GOTOSAVINGPOWER) {
      SYS.fRelais2=0;
      LAMP_Ch2.bStatus=STATUS_SAVINGPOWER;
    } else if(LAMP_Ch2.bStatus==STATUS_SAVINGPOWER) {
      if(LAMP_Ch1.bStatus!=STATUS_SAVINGPOWER || LAMP_Both.bCouplingModus!=1) {
        if(LAMP_Ch2.wLux < LAMP_Ch2.wSetLux) {
          LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
          SYS.fRelais2=1;
        }
      } else {
        CalculateAverageLuxvalues();
        if(LAMP_Both.iAverageLux < LAMP_Both.iAverageSetLux) {
          LAMP_Ch1.bStatus=LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
          SYS.fRelais1=1;
          SYS.fRelais2=1;
        }
      }
    } 
    
    // Verzögertes Ausschalten der Relais
    if(LAMP_Ch2.bRelaisOfftimer>1) LAMP_Ch2.bRelaisOfftimer--;
    else if(LAMP_Ch2.bRelaisOfftimer==1) {
      SYS.fRelais2=0;
      LAMP_Ch2.bRelaisOfftimer=0;
      LAMP_Ch2.bStatus=STATUS_OFF;
    }
  } else {
    LAMP_Ch2.bStatus=LAMP_Ch1.bStatus;
    LAMP_Ch2.fOn=LAMP_Ch1.fOn;
  }

}                


//---------------------------------------------------------------------------------------------------------------------------------------
// called every 10msec 
//
// Taste wird nach Ablauf der Prellzeit von 20msec bis 500msec überprüft, ab dann wird Ein/Ausschaltfunktion betätigt, 
// sobald Taste losgelassen wird; wenn Taste immer noch betätigt ist, wird der Luxstellwert verändert
//
void SensoDimControllerCh1(void)
{
  if(SYS.bApplication!=APPLICATION_TOUCHPANEL && SYS.bApplication!=APPLICATION_DUALSLIDER) {

    if(LAMP_Ch1.fOn) {
      if(LAMP_Ch1.ON_OFF_Timer>ADJUST_LUXTIME && LAMP_Ch1.ON_OFF_Timer<=BOUNCE_TIME && SensoSwitch1Off() && !OffBlockTimeCh1) {
        LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
        LAMP_Ch1.fOn=0;
        LAMP_Ch1.fManualOff=1;
        LAMP_Ch1.fManualOn=0;
        LAMP_Ch1.wPIR_Disabletimer=LAMP_Both.wPIR_Disabletime;
        LAMP_Ch1.bRelaisOfftimer=200; // 2 Sekunden bis Relais Aus
        LAMP_Ch1.bSoftOffTimer=LAMP_Both.bSoftOffTime;
        LAMP_Ch1.wDimPromilleStep=LAMP_Ch1.wDimPromille/LAMP_Both.bSoftOffTime;
      } else if(LAMP_Ch1.ON_OFF_Timer<=ADJUST_LUXTIME) {
        if(SensoSwitch1On()) {
          LAMP_Ch1.ON_OFF_Timer=ADJUST_LUXOFFDELAY;
          ChangedCh1=1;
          if(!LAMP_Ch1.fChangeSetLuxDown) { // heller machen
            if(LAMP_Ch1.wSetLux<LAMP_Ch1.wSetLuxMax) 
              LAMP_Ch1.wSetLux+=LAMP_Both.wSetLuxIncrement;
          } else {
            if(LAMP_Ch1.wSetLux>LAMP_Ch1.wSetLuxMin) 
              LAMP_Ch1.wSetLux-=LAMP_Both.wSetLuxIncrement;
          }
        } else {
          if(LAMP_Ch1.ON_OFF_Timer==1) {
            if(ChangedCh1) {
              if(LAMP_Ch1.fChangeSetLuxDown) LAMP_Ch1.fChangeSetLuxDown=0;
              else LAMP_Ch1.fChangeSetLuxDown=1;
              ChangedCh1=0;
            }
          }
        }
      }
    } else { // Lampe war aus -> sofort einschalten wenn Timer auf 100 gesetzt ist.
      if(LAMP_Ch1.ON_OFF_Timer==ON_OFF_TIME) {
        LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
        OffBlockTimeCh1=ADJUST_LUXTIME;
        LAMP_Ch1.fOn=1;
        LAMP_Ch1.fManualOn=1;
        LAMP_Ch1.bRelaisOfftimer=0;
        SYS.fRelais1=1;
        if(!LAMP_Ch2.fSensoDimOK)
          SYS.fRelais2=1;
        LAMP_Ch1.bSoftOnTimer=LAMP_Both.bSoftOnTime;
        LAMP_Ch1.wDimPromilleStep=(LAMP_Ch1.wSetDimPromille-LAMP_Ch1.wStartDimPromille)/LAMP_Both.bSoftOnTime;
        LAMP_Ch1.wPIR_Ontimer=LAMP_Both.wPIR_Ontime;
      }
    }
    
    if(LAMP_Ch1.ON_OFF_Timer) {
        LAMP_Ch1.ON_OFF_Timer--; // Discard Inputs for 1sec (100x10msec)
    }
    if(OffBlockTimeCh1) OffBlockTimeCh1--;

    if(LAMP_Ch1.PIR_FLAG)
    {
      LAMP_Ch1.PIR_FLAG=0;
      if(!LAMP_Ch1.fOn) {
        if(!LAMP_Ch1.wPIR_Disabletimer) {
          LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
          LAMP_Ch1.fOn=1;
          LAMP_Ch1.fManualOn=0;
          LAMP_Ch1.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
          LAMP_Ch1.bRelaisOfftimer=0;
          SYS.fRelais1=1;
          if(!LAMP_Ch2.fSensoDimOK)
            SYS.fRelais2=1;
          LAMP_Ch1.bSoftOnTimer=LAMP_Both.bSoftOnTime;
          LAMP_Ch1.wDimPromilleStep=(LAMP_Ch1.wSetDimPromille-LAMP_Ch1.wStartDimPromille)/LAMP_Both.bSoftOnTime;
        }
      } else {
        LAMP_Ch1.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
      }
    } // end if Pirflag

    if(LAMP_Ch1.wPIR_Ontimer==1) { // expired PIR Timer
      LAMP_Ch1.wPIR_Ontimer=0;
      LAMP_Ch1.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch1.fOn=0;
      LAMP_Ch1.fManualOff=0;
      LAMP_Ch1.fManualOn=0;
      LAMP_Ch1.bRelaisOfftimer=200; // 2 Sekunden bis Relais Aus
      LAMP_Ch1.bSoftOffTimer=LAMP_Both.bSoftOffTime;
      LAMP_Ch1.wDimPromilleStep=LAMP_Ch1.wDimPromille/LAMP_Both.bSoftOffTime;
    }
  } else { // end if(SYS.bApplication!=APPLICATION_TOUCHPANEL != APPLICATION_DUALSLIDER)
    if(!LAMP_Ch1.fOn) {
      LAMP_Ch1.bStatus=STATUS_SWITCHINGON;
      LAMP_Ch1.fOn=1;
      LAMP_Ch1.fManualOn=0;
      LAMP_Ch1.bRelaisOfftimer=0;
      SYS.fRelais1=1;
      if(SYS.bApplication!=APPLICATION_ALONEATWORK || (!LAMP_Ch2.fSensoDimOK))
        SYS.fRelais2=1;
    }
  }
}
  
//---------------------------------------------------------------------------------------------------------------------------------------
// called every 10msec 
//
// Taste wird nach Ablauf der Prellzeit von 20msec bis 500msec überprüft, ab dann wird Ein/Ausschaltfunktion betätigt, 
// sobald Taste losgelassen wird; wenn Taste immer noch betätigt ist, wird der Luxstellwert verändert
//
void SensoDimControllerCh2(void)
{
  // AAW Channel 2
  if(SYS.bApplication==APPLICATION_ALONEATWORK) {
    if(LAMP_Ch2.fOn) {
      if(LAMP_Ch2.ON_OFF_Timer>ADJUST_LUXTIME && LAMP_Ch2.ON_OFF_Timer<=BOUNCE_TIME && SensoSwitch2Off() && !OffBlockTimeCh2) {
        LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
        LAMP_Ch2.fOn=0;
        LAMP_Ch2.fManualOff=1;
        LAMP_Ch2.fManualOn=0;
        LAMP_Ch2.wPIR_Disabletimer=LAMP_Both.wPIR_Disabletime;
        LAMP_Ch2.bRelaisOfftimer=200; // 2 Sekunden bis Relais Aus
        LAMP_Ch2.bSoftOffTimer=LAMP_Both.bSoftOffTime;
        LAMP_Ch2.wDimPromilleStep=LAMP_Ch2.wDimPromille/LAMP_Both.bSoftOffTime;
      } else if(LAMP_Ch2.ON_OFF_Timer<=ADJUST_LUXTIME) {
        if(SensoSwitch2On()) {
          LAMP_Ch2.ON_OFF_Timer=ADJUST_LUXOFFDELAY;
          ChangedCh2=1;
          if(!LAMP_Ch2.fChangeSetLuxDown) { // heller machen
            if(LAMP_Ch2.wSetLux<LAMP_Ch2.wSetLuxMax) 
              LAMP_Ch2.wSetLux+=LAMP_Both.wSetLuxIncrement;
          } else {
            if(LAMP_Ch2.wSetLux>LAMP_Ch2.wSetLuxMin) 
              LAMP_Ch2.wSetLux-=LAMP_Both.wSetLuxIncrement;
          }
        } else {
          if(LAMP_Ch2.ON_OFF_Timer==1) {
            if(ChangedCh2) {
              if(LAMP_Ch2.fChangeSetLuxDown) LAMP_Ch2.fChangeSetLuxDown=0;
              else LAMP_Ch2.fChangeSetLuxDown=1;
              ChangedCh2=0;
            }
          }
        }
      }
    } else { // Lampe war aus -> sofort einschalten wenn Timer auf 100 gesetzt ist.
      if(LAMP_Ch2.ON_OFF_Timer==ON_OFF_TIME) {
        LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
        OffBlockTimeCh2=ADJUST_LUXTIME;
        LAMP_Ch2.fOn=1;
        LAMP_Ch2.fManualOn=1;
        LAMP_Ch2.bRelaisOfftimer=0;
        SYS.fRelais2=1;
        LAMP_Ch2.bSoftOnTimer=LAMP_Both.bSoftOnTime;
        LAMP_Ch2.wDimPromilleStep=(LAMP_Ch2.wSetDimPromille-LAMP_Ch2.wStartDimPromille)/LAMP_Both.bSoftOnTime;
        LAMP_Ch2.wPIR_Ontimer=LAMP_Both.wPIR_Ontime;
      }
    }

    if(LAMP_Ch2.ON_OFF_Timer) LAMP_Ch2.ON_OFF_Timer--; // Discard Inputs for 1sec (100x10msec)
    if(OffBlockTimeCh2) OffBlockTimeCh2--;

    if(LAMP_Ch2.PIR_FLAG)
    {
      LAMP_Ch2.PIR_FLAG=0;
      if(!LAMP_Ch2.fOn) {
        if(!LAMP_Ch2.wPIR_Disabletimer) {
          LAMP_Ch2.bStatus=STATUS_SWITCHINGON;
          LAMP_Ch2.fOn=1;
          LAMP_Ch2.fManualOn=0;
          LAMP_Ch2.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
          LAMP_Ch2.bRelaisOfftimer=0;
          SYS.fRelais2=1;
          LAMP_Ch2.bSoftOnTimer=LAMP_Both.bSoftOnTime;
        }
      } else {
        LAMP_Ch2.wPIR_Ontimer=LAMP_Both.wPIR_Ontime; 
      }
    } // end if Pirflag

    if(LAMP_Ch2.wPIR_Ontimer==1) { // expired PIR Timer
      LAMP_Ch2.wPIR_Ontimer=0;
      LAMP_Ch2.bStatus=STATUS_SWITCHINGOFF;
      LAMP_Ch2.fOn=0;
      LAMP_Ch2.fManualOff=0;
      LAMP_Ch2.fManualOn=0;
      LAMP_Ch2.bRelaisOfftimer=200; // 2 Sekunden bis Relais Aus
      LAMP_Ch2.bSoftOffTimer=LAMP_Both.bSoftOffTime;
      LAMP_Ch2.wDimPromilleStep=LAMP_Ch2.wDimPromille/LAMP_Both.bSoftOffTime;
      
    }
  } 
  
}


//-------------------------------------------------------------------------------
// called before Mainloop starts
//
void InitLampController(void)
{
  LAMP_Both.bAAW_ModusOn=1; // 1 wenn AAW Funktion eingeschaltet werden soll
  
  //LAMP_Both.bCouplingModus=0; // unabhängig
  LAMP_Both.bCouplingModus=1; // gekoppelt

  LAMP_Both.bSoftOnTime=1;   // Rampe in n x 10msec nicht benutzt wenn keine Indirekte Beleuchtung da ist
  LAMP_Both.bSoftOffTime=100; // Rampe in n x 10msec
  LAMP_Both.wSetLuxIncrement=2; // Increment / Decrement pro 10msec 

  LAMP_Ch1.wSetDimPromille=800;
  LAMP_Ch1.wStartDimPromille=50; 
  LAMP_Ch1.wMinimumDimPromille=492; // entspricht 3% Dali minimum Wert EVG

  LAMP_Ch1.wSetLux=600;
  LAMP_Ch1.wSetLuxMin=200;
  LAMP_Ch1.wSetLuxMax=1000;
  LAMP_Ch1.fChangeSetLuxDown=0;
  
  
  LAMP_Both.wPIR_Ontime=600; // sekunden
  LAMP_Both.wPIR_Disabletime=600; //sekunden

  LAMP_Both.fPowerSavingModeON=1; // Powersaving bei Überbeleuchtung während xx Sekunden
  LAMP_Both.wPowerSavingtime=300; 
  LAMP_Both.wPowerSavingLux=300;
  
  LAMP_Ch2.wSetDimPromille=800;
  LAMP_Ch2.wStartDimPromille=50; 
  LAMP_Ch1.wMinimumDimPromille=492; // entspricht 3% Dali minimum Wert EVG

  LAMP_Ch2.wSetLux=600;
  LAMP_Ch2.wSetLuxMin=200;
  LAMP_Ch2.wSetLuxMax=1000;
  LAMP_Ch2.fChangeSetLuxDown=0;
  
  
}

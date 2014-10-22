  /******************************************************************************
  * @file    AAWmodule.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    06.2012
  * @brief   RS485 Communication channel with AAW Module
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  * @clock HSE_VALUE = 12000000 / SYSCLK_FREQ_24MHz
  ******************************************************************************
  


*/ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include "MAIN.h"
#include "rs.h"
#include "AAWmodule.h"
#include "modbus_master.h"
#include "Lampcontroller.h"
#include "SensodimController.h"

tAAWmodule AAW;


void ProcessAAWmodulemessage(void)
{
  AAW.ulCounter++;
  AAW.bOKTimer=10;
  
  AAW.wLux=RS_RX_Buffer[3];
  AAW.wLux*=256;
  AAW.wLux |= RS_RX_Buffer[4];
  
  LAMP_Ch2.wLux = AAW.wLux;
  
  AAW.bSwitch=RS_RX_Buffer[6];
  if(AAW.bSwitch != 255) {
    AAW.fSensoDimOK=1;
    LAMP_Ch2.fSensoDimOK=1;
  }
  AAW.bPirOn=RS_RX_Buffer[5];
  
  if(AAW.bSwitch==1) {
    if(! RemoteSensoDim.ON_OFF_Timer)
      RemoteSensoDim.ON_OFF_Timer=ON_OFF_TIME;
  } 
  if(AAW.bPirOn)  
    RemoteSensoDim.PIR_FLAG=1;

  AAW.bDaliValueCh1=RS_RX_Buffer[7]; // V0.40
  AAW.bDaliValueCh2=RS_RX_Buffer[8]; // V0.40
}



void InitAAWmodule(void)
{
  
  
}

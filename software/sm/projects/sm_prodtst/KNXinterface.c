  /******************************************************************************
  * @file    KNXinterface.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    06.2012
  * @brief   RS485 Communication channel with KNXinterface
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
#include "KNXinterface.h"
#include "modbus_master.h"


tKNX KNX;

void ProcessKNXmessage(void)
{
  u8 i;
  
  KNX.ulCounter++;
  KNX.bOKTimer=10;

  // High Byte Mode: RS_RX_Buffer[3] not defined

  i=RS_RX_Buffer[4];
  KNX.Mode=i & 3;
  if(i & 4) KNX.OnCh1=1;
  else KNX.OnCh1=0;
  if(i & 8) KNX.OnCh2=1;
  else KNX.OnCh2=0;
  
  KNX.LightLevelCh1=RS_RX_Buffer[5];
  KNX.LightLevelCh2=RS_RX_Buffer[6];
}

 /******************************************************************************
  * @file    flashsetup.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    12.2012
  * @brief   Flashpages for definitions
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  * @clock HSE_VALUE = 12000000 / SYSCLK_FREQ_24MHz
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include "MAIN.h"
#include "stm32f10x_it.h"
#include "flashsetup.h"

FlashUnion_t Flash;

void WriteCompleteFlashSetup(void)
{
  int i;
   
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
  FLASH_ErasePage(FLASHSETUP_START_ADDRESS);
  
  for(i=0;i<256;i++)
  {
    FLASH_ProgramWord(FLASHSETUP_START_ADDRESS + i*sizeof(uint32_t), Flash.Var[i]);
  }
  FLASH_Lock();
}

void WriteFlashSetup(u32 varindex)
{
  s32 var;
  
  var=(*(__IO int32_t*)(FLASHSETUP_START_ADDRESS + varindex*sizeof(uint32_t)));
  if(var==-1) {
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    FLASH_ProgramWord(FLASHSETUP_START_ADDRESS + varindex*sizeof(uint32_t), Flash.Var[varindex]);  
    FLASH_Lock();
  } else
    WriteCompleteFlashSetup();
}


void EraseFlashSetup(void)
{
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
  FLASH_ErasePage(FLASHSETUP_START_ADDRESS);
  FLASH_Lock();
}

void InitFlashSetup(void)
{
  int i;

  for(i=0;i<256;i++)
  {
    Flash.Var[i]=(*(__IO uint32_t*)(FLASHSETUP_START_ADDRESS + i*sizeof(uint32_t)));
  }
  if(Flash.Setup.SETUP_VALID == -1) {
    Flash.Setup.SETUP_VALID=SETUP_VALID_MARK;
    Flash.Setup.VERSION=1;
    
    Flash.Setup.APPLICATION=APPLICATION_DUALSLIDER;
    Flash.Setup.DMX_RECEIVER=0 + 256*200; // Receive channel 0 100msec Selftesttime
    Flash.Setup.OUT_MODE=1;         // 0=off 1=Dali 2=1..10V 3=DMX
    Flash.Setup.OUT_MATRIX=0;     // 0=2Ch 1=Parallel
    Flash.Setup.OUT_BALANCE_CTRL=0; // Balance Mode

    Flash.Setup.OUT_LEV_MIN=5;
    Flash.Setup.OUT_LEV_MAX=100;
    Flash.Setup.OUT_SCALER=1;

    Flash.Setup.LUX_CTRL_EN=1; // 
    
    Flash.Setup.LUX_NOM_MIN=200;
    Flash.Setup.LUX_NOM_MAX=1000;
    Flash.Setup.LUX_NOM_START=500;
    Flash.Setup.LUX_ERR_OFF=50; // auf Null setzen, dann ist Powersaving Funktion ausgeschaltet
    Flash.Setup.LUX_ERR_OFF_DELAY=10*60;
    
    Flash.Setup.LUX_OV_OFF=2000;
    Flash.Setup.LUX_OV_OFF_DELAY=10*60;
    Flash.Setup.LUX_OFF_LEVEL=0; 

    Flash.Setup.PIR_OFF_DELAY= 10*60; // 10min
    Flash.Setup.PIR_ON_DELAY = 10*60; // 10min 

// todo Currently unused; 19-Apr-2013/ais
    Flash.Setup.PIR_FADE_OFF =  2*60; //  2min

    Flash.Setup.PIR_MODE = PIR_MODE_AUTO;

    Flash.Setup.PWR_FAIL_MODE=3; // // 0=Off 1=On 2=Restlicht 3=letzter Zustand
    
    Flash.Setup.SLIDER_SPEED=3;
    
    // Test Suva  7 = all 3 Touchkeys or 2 = Middle Key
    Flash.Setup.INPUT_SINGLE_CH=0;
  }
}

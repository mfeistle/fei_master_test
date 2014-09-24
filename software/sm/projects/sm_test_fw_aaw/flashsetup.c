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

FlashUnion_t FlashSetup;

void WriteCompleteFlashSetup(void)
{
  int i;
   
  FLASH_Unlock();
  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
  FLASH_ErasePage(FLASHSETUP_START_ADDRESS);
  
  for(i=0;i<256;i++)
  {
    FLASH_ProgramWord(FLASHSETUP_START_ADDRESS + i*sizeof(uint32_t), FlashSetup.Var[i]);
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
    FLASH_ProgramWord(FLASHSETUP_START_ADDRESS + varindex*sizeof(uint32_t), FlashSetup.Var[varindex]);  
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
    FlashSetup.Var[i]=(*(__IO uint32_t*)(FLASHSETUP_START_ADDRESS + i*sizeof(uint32_t)));
  }
}

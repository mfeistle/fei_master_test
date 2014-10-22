	/******************************************************************************
  * @file    RS_control.c.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    02.2012
  * @brief   RS485 mit DMA Control
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  * @clock HSE_VALUE = 12000000 / SYSCLK_FREQ_24MHz
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include "MAIN.h"
#include "RS.h"



void RS_TX_Send(vu8 *TX_Buffer,vu32 Length)
{

  DMA_InitTypeDef DMA_InitStructure;

  /* Wait until TXE=1 */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
  {
  }   

  /* Enable USART1 DMA TX request */
  USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);

  DMA_DeInit(DMA1_Channel7);  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART2->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)RS_TX_Buffer; // %%wm !! TX_Buffer ??
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = Length;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel7, &DMA_InitStructure);
  
  /* Enable DMA1 Channel7 transmit complete interrupt */
  DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
  
  /* Set high pin DE (PA6) */
  GPIOA->BSRR = GPIO_Pin_6; // enable  TEN (DE)
  GPIOA->BSRR = GPIO_Pin_7; // disable REN (RE)
  
  /* Enable DMA1 Channel7 */
  DMA_Cmd(DMA1_Channel7, ENABLE);

}


void RS_InterruptHandler(void)
{
  if(USART_GetITStatus(USART2, USART_IT_TC) != RESET) // TC Flag 
  {

     /* Set DE pin PA6 to low level */
     GPIOA->BRR = GPIO_Pin_6; // disable TEN
     GPIOA->BRR = GPIO_Pin_7; // enable REN (RE)
        
     /* Disable the USART2 Transmit Complete interrupt */
     USART_ITConfig(USART2, USART_IT_TC, DISABLE);
     
     /* Clear USART2 TC pending bit */
     USART_ClearFlag(USART2, USART_FLAG_TC);
  }
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
  {
    uint8_t b;

    /* Read one byte from the receive data register */
    b = USART_ReceiveData(USART2);

    if (RS_RX_Complete == 0 && RS_RX_Counter < RS_RX_BufferSize)
    {
      RS_RX_Buffer[RS_RX_Counter] = b; 
      RS_RX_Counter++;
      if (RS_RX_Counter >= RS_RX_BufferSize)
        RS_RX_Complete = 1;   
    }

    /* Clear the USART Receive interrupt */
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
  }


}


 /******************************************************************************
  * @file    dmx_control.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    02.2012
  * @brief   DMX 512 Control
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
#include "dmx_control.h"

vu8  DMX_data[MAX_CHANNEL_COUNT]; //DMX512 TX Buffer

vu8 b_IntCheck = 0;
vu8 b_PacketSent = 0;
u16 u16_BytesCount = BYTE_COUNT_ONE; // bis 512 bytes
u32 u32_EffPacketTime, u32_BreakGap;



u8   DMX_RX_Buff[MAX_CHANNEL_COUNT];
u8   DMX_UserBuffer[MAX_RX_CHANNEL_COUNT];
vu32 DMX_StatusRead = 0;
vu16 DMXChannelCount,DMX_LastChannel;
vu8 fDMX_RX_Start_OK = 0;
vu8 fDMX_DataCorruption = 0;
vu8 fDMX_RXDataValid = 0;

void DMX_SendInterruptHandler(void)
{      
  if(u16_BytesCount <= NO_OF_SEND_BYTES)
  {
    if(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // without ; it does not send DMX! -> testprocedures 'H' does not work
    {
      // Send Dimming Data 
      USART_SendData(USART1, DMX_data[u16_BytesCount]);
      u16_BytesCount++;
    }
  }
  else
  {
    u16_BytesCount = ONE;
    b_PacketSent = TRUE;
    // TIM4 enable counter 
    TIM_Cmd(TIM4, DISABLE);
  }
}      

void DMX_ResetSequenceInterrupt(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  if(TIM2->SR & TIM_IT_Update)
  {
    /* TIM2 disable counter */
    TIM2->CR1 &= CR1_CEN_Reset;
    TIM2->CNT = RESET_VALUE ;
  
    /*Send Start Code*/
    USART_SendData(USART1, DMX_data[RESET_VALUE]);
  
    /* TIM4 enable counter */
    TIM_Cmd(TIM4, ENABLE);
  }
  /* Break Time Complete*/
  else
  {
    /* GPIOA Configuration: PA8 in input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
  }
}


/*******************************************************************************
* Function Name  :  Delay
* Description    : Nominal delay
* Input          : delay parameter
* Return         : None
*******************************************************************************/
void Delay(u32 u32_RequiredDelay, u16 u16_RequiredDelay1)
{
  u16 u16_Counter1 = RESET_VALUE;
  u32 u32_Counter2 = RESET_VALUE;
  for(u16_Counter1 = RESET_VALUE; u16_Counter1 < u16_RequiredDelay1; u16_Counter1++)
  {
    for(u32_Counter2 = RESET_VALUE; u32_Counter2 < u32_RequiredDelay; u32_Counter2++);
  }
}

/*******************************************************************************
* Function Name  : Send_ResetSequence()
* Description    : Send break and MAB
* Input          : None
* Return         : None
*******************************************************************************/
void Send_ResetSequence(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* GPIOA Configuration: PA8 in Output PP */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Set the GPIOA port pin 8 */
  GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);

  /* TIM2 enable counter */
  TIM_Cmd(TIM2, ENABLE);
}

void Send_DMX(void)
{
  if(b_PacketSent == TRUE)
  {
    if(u32_EffPacketTime < BREAK_2_BREAK_TIME)
    {
      /* delay appr 25us */
      Delay(u32_BreakGap, 67);
    }
    Send_ResetSequence();

    b_PacketSent = FALSE;
  }
}



void DMX_RX_InterruptHandler(void)
{
  u32 i;


  u8 ReceivedData;
  /* Read and backup the status */
  DMX_StatusRead = USART2->SR;

  if((DMX_StatusRead & FRAMING_ERROR_FLAG) != RESET)
  {
    /* Dummy read to clear flag */
    USART_ReceiveData(USART2);
    fDMX_RX_Start_OK = 1;
    
    DMXChannelCount = 0;  

  }
  /* If no framming error but real data received */
  else if(((DMX_StatusRead & RXNE_FLAG) != RESET) && fDMX_RX_Start_OK )
  {
    if(DMXChannelCount == 0)
    {
      ReceivedData = USART_ReceiveData(USART2);
      if(ReceivedData != 0) // first byte must be zero
      {
          // Error detected
          fDMX_RX_Start_OK = 0;
          fDMX_DataCorruption = 1;     
       }
      else // First byte detected
      {
        DMX_RX_Buff[DMXChannelCount] = ReceivedData;
        DMXChannelCount++;
        fDMX_DataCorruption = 0;
      }
    }
    else if (!fDMX_DataCorruption)
    {
      if(DMXChannelCount < MAX_RX_CHANNEL_COUNT)
      {
        DMX_RX_Buff[DMXChannelCount] = USART_ReceiveData(USART2);
        DMXChannelCount++;
      }
      else
      {
        DMX_RX_Buff[DMXChannelCount] = USART_ReceiveData(USART2);
        DMXChannelCount = 0;
        fDMX_RX_Start_OK = 0;
        if(!fDMX_RXDataValid) {
          fDMX_RXDataValid = 1;
          for(i=0;i<MAX_RX_CHANNEL_COUNT;i++)
            DMX_UserBuffer[i]=DMX_RX_Buff[i];
        }
      }
    }
  }
  if(DMX_LastChannel<DMXChannelCount) DMX_LastChannel=DMXChannelCount;

  USART_ClearITPendingBit(USART2, USART_IT_FE);  // Framing Error Flag
  USART_ReceiveData(USART2); // ??
  USART_ClearITPendingBit(USART2, USART_IT_RXNE);// Register not empty Flag
}


void InitDMX(void)
{
  /* Calculation of Effective Packet time */
  u32_EffPacketTime = BREAK_TIME + MAB_TIME + (BYTE_TIME * (NO_OF_SEND_BYTES + 1))\
  + (MARK_TIME_SLOT * (NO_OF_SEND_BYTES));
  
  /* Calculation of Remaining Break time required for getting required break to
  break time */
  u32_BreakGap = ((BREAK_2_BREAK_TIME - u32_EffPacketTime)/BREAK_2_BREAK_TIME_RESOLUTION)\
  + 1;
}

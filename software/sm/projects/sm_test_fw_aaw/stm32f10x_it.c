/**
  ******************************************************************************
  * @file    stm32f10x_it.c 
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    02.2012
  * @brief   Main Interrupt Service Routines.
  ******************************************************************************


*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "MAIN.h"
#include "DALI.h"
#include "dmx_control.h"
#include "RS.h"
#include "watch.h"

//#define CR1_CEN_Reset               ((uint16_t)0x03FE)
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define FE_Status      (USART_GetITStatus(USART1, USART_IT_FE))


vu8  RxBuffer[RxBufferSize];  // DEBUG UART RX Buffer
vu16 RxCounter = 0;

uint8_t  RS_RX_Buffer[RS_RX_BufferSize]; // RS485 RX Buffer
vu16 RS_RX_Counter = 0;
vu8  RS_RX_Complete = 0;

vu8  RS_TX_Request;
 
vu8 Counter_100us = 0;
vu8 uTimer;
vu8 Ticker_1ms = 0;

vu8 Counter_50ms =0;
vu8 Ticker_50ms = 0;

vu32 Counter_1s = 0;
vu8 Ticker_1s = 0;

vu8 PIR_ENABLE,PIR_FLAG = 0;
//vu8 ON_OFF_Timer = 0;
vu8 SETUP_Timer = 0;
vu8 DeviceMode;

vu8 bSliderTimer;


static u16 ACphaseTimer;
static u8  ACinput;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{}

/**
  * @brief  This function handles 100us SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  u8 x;

  if(uTimer<255) uTimer++;

  /* _MAN_: synchronised relais switching behaviour
                 _         _         _
                  \       / \       /
  Line:          ..\...../...\...../..
                    \   /     \   /
                     \_/       \_/

                   <--------> = 20msec
  ACinput:       __-------___-------__
  ACphaseTimer:  ....<199><0>..<199><0>.

  */

  if (ACphaseTimer<1000) ACphaseTimer++;   // 0..199 for full phase (20msec) of 50Hz line signal 1000 == DC Operation, or power glitch

  if (ACinput && !GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13)) {
    if(ACphaseTimer<250)
      SYS.ACperiod=ACphaseTimer;
    ACphaseTimer=0;   // reset timer on falling edge
  } else if(!ACinput && GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13)) {
    if(ACphaseTimer<150) {
      SYS.ACOffperiod=ACphaseTimer;
      SYS.ACZeroCrossing=REL_SWITCH_PHASE + ACphaseTimer/2 - SYS.ACperiod/4; // dynamische Kompensation
      if(SYS.ACZeroCrossing>200) SYS.ACZeroCrossing=0; // Vorzeichenfehler abfangen
    }
  }
  ACinput=GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13);

  if(SYS.bApplication!=APPLICATION_TEST) {
    if (ACphaseTimer==1000) {
      // unsynchronised
      if(SYS.fRelais1) REL1_ON(); else REL1_OFF();
      if(SYS.fRelais2) REL2_ON(); else REL2_OFF();
    } else {
      // turn-on phase (0-crossing)
      if(ACphaseTimer==SYS.ACZeroCrossing) {
        if(SYS.fRelais1) REL1_ON(); else REL1_OFF();
        if(SYS.fRelais2) REL2_ON(); else REL2_OFF();
      }
    }
  }

  
  switch(Counter_100us)
  {
    case 0:
        Ticker_1ms = 1;
    break;
    case 1:
        Counter_50ms++;
        if(Counter_50ms == 50)
        {
          Ticker_50ms = 1;
          Counter_50ms = 0;
          Counter_1s++;
          if(SETUP_Timer) SETUP_Timer--;
        #ifdef USE_RTC
          if(WatchTimeout_Keyscan) WatchTimeout_Keyscan--; // watch.c für RTC Test 1msec
        #endif
        }
        
        if(Counter_1s==20)
        {
          Ticker_1s=1;
          Counter_1s = 0;
        }
    break;
    case 2:
        // PIR Detektor
        x=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13);
        if(x) PIR_ENABLE=1;
        else if(PIR_ENABLE) {
          PIR_FLAG = 1;
          PIR_ENABLE=0;
        }
    break;
    case 3:
        // Tasteneingang
        /*
        if(!GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)) {
          if(!ON_OFF_Timer)
            ON_OFF_Timer = 100;
        } */
    break;
    case 4:
        //if(bSliderTimer>1) bSliderTimer--;
        //else if(bSliderTimer==1) {
        //  RS_RxCounter=0; // V0.19 Hack für Touchslider  
        //  bSliderTimer=0;
        //}
    break;
    case 5:
    break;
    case 6:
    break;
    case 7:
    break;
    case 8:
    break;
    case 9:
        Counter_100us = 255;
    break;
    default:
    break;
  }
  Counter_100us++;
  if(RS_TX_Request>1) RS_TX_Request--;
}

/*******************************************************************************
* Function Name  : USART2_IRQHandler (RS485 Sender)
* Description    : This function handles USART2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void USART2_IRQHandler(void)
{

  if(DeviceMode==MODE_GATEWAY)
  {
    DMX_RX_InterruptHandler();   // Alternative use for DMX reception
  }else
  {
    bSliderTimer=10;
    RS_InterruptHandler();
  }

  
}


/*******************************************************************************
* Function Name  : USART3_IRQHandler
* Description    : This function handles USART3 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USART3_IRQHandler(void)
{

    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
    
      /* Read one byte from the receive data register */
      RxBuffer[RxCounter] = USART_ReceiveData(USART3); 
    if(RxCounter < 6)
    {
       RxCounter++;
    }else
    {
      RxCounter=0;
    }
      /* Clear the USART1 Receive interrupt */
      USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}


/*******************************************************************************
* Function Name  : DMA1_Channel7_IRQHandler ( RS485 )
* Description    : This function handles DMA1 Channel7 USART2 TX interrupt request
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMA1_Channel7_IRQHandler(void)
{
  if(DMA1->ISR & (u32)0x02000000)
  {
    /* Enable USART2 Transmit complete interrupt */
    USART_ITConfig(USART2, USART_IT_TC, ENABLE); 
    
    /* Disable DMA1 Channel7 transmit complete interrupt */
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, DISABLE);
  
    /* Clear DMA TC pending bit */
    DMA_ClearITPendingBit(DMA1_IT_TC7);
   }
}


/*******************************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : This function handles TIM2 global interrupt request. Used for
*                  sending reset sequence.
* Input          : None
* Return         : None
*******************************************************************************/
void TIM2_IRQHandler(void)
{
  //GPIO_InitTypeDef GPIO_InitStructure;

  if(b_IntCheck == TRUE)
  {
    DMX_ResetSequenceInterrupt();
    /*
    if(TIM2->SR & TIM_IT_Update)
    {
      // TIM2 disable counter 
      TIM2->CR1 &= CR1_CEN_Reset;
      TIM2->CNT = RESET_VALUE ;

      // Send Start Code
      USART_SendData(USART1, DMX_data[RESET_VALUE]);

      // TIM4 enable counter 
      TIM_Cmd(TIM4, ENABLE);
    }
    // Break Time Complete
    else
    {
      // GPIOA Configuration: PA8 in input
      GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      GPIO_Init(GPIOA, &GPIO_InitStructure);
    }
    */
  }
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
}

/*******************************************************************************
* Function Name  : TIM3_IRQHandler
* Description    : This function handles TIM3 global interrupt request. Used for DALI bit timing
*                  
* Input          : None
* Return         : None
*******************************************************************************/
void TIM3_IRQHandler(void)
{

  if(b_IntCheck == TRUE)
  {
    DaliInterruptHandler();
  }
}

/*******************************************************************************
* Function Name  : TIM4_IRQHandler
* Description    : This function handles TIM4 global interrupt request. Used for
*                  sending MARK time between slots
* Input          : None
* Return         : None
*******************************************************************************/
void TIM4_IRQHandler(void)
{
  if(b_IntCheck == TRUE)
  {
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
      DMX_SendInterruptHandler();
      /*
      if(u8_BytesCount <= NO_OF_SEND_BYTES)
      {
        if(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        {
          // Send Dimming Data 
          USART_SendData(USART1, DMX_data[u16_BytesCount]);
          u16_BytesCount++;
        }
      }
      else
      {
        u8_BytesCount = ONE;
        b_PacketSent = TRUE;
        // TIM4 enable counter 
        TIM_Cmd(TIM4, DISABLE);
      }
      */
    }
  }

  TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}



/*******************************************************************************
* Function Name  : TIM7_IRQHandler
* Description    : This function handles TIM7 global interrupt request. Used for DALI Slave bit timing
*                  
* Input          : None
* Return         : None
*******************************************************************************/
void TIM7_IRQHandler(void)
{

  if(b_IntCheck == TRUE)
  {
    DaliSlaveInterruptHandler();
  }
}

/*******************************************************************************
* Function Name  : EXTI1_IRQHandler
* Description    : This function handles EXTI1_Line global interrupt request. Used for
*                  receiving DALI on PB1 
* Input          : None
* Return         : None
*******************************************************************************/

void EXTI1_IRQHandler(void)
{
 // PB1 DALI RX2
 if(EXTI_GetITStatus(EXTI_Line1) != RESET)
  {
    DaliRXInterruptHandler();
    /* Clear the EXTI line 1 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line1);
  }

}

/*******************************************************************************
* Function Name  : EXTI2_IRQHandler
* Description    : This function handles EXTI2_Line global interrupt request. Used for
*                  receiving DALI on PB2 DALIP 
* Input          : None
* Return         : None
*******************************************************************************/

void EXTI2_IRQHandler(void)
{
 // PB2 DALIP
 if(EXTI_GetITStatus(EXTI_Line2) != RESET)
  {
    Dali_RXP_SlaveInterruptHandler();
    /* Clear the EXTI line 2 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line2);

  }

}


/*******************************************************************************
* Function Name  : EXTI3_IRQHandler
* Description    : This function handles EXTI3_Line global interrupt request. Used for
*                  receiving DALI on PB3 DALIN 
* Input          : None
* Return         : None
*******************************************************************************/

void EXTI3_IRQHandler(void)
{
 // PB2 DALIN
 if(EXTI_GetITStatus(EXTI_Line3) != RESET)
  {
    Dali_RXN_SlaveInterruptHandler();
    /* Clear the EXTI line 3 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line3);
  }

}



/*******************************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : This function handles EXTI5-9 Line global interrupt request. Used for
*                  receiving DALI on PB7 IN1_DALI 
* Input          : None
* Return         : None
*******************************************************************************/

void EXTI9_5_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line7) != RESET)
  {
    Dali_IN_SlaveInterruptHandler();
    EXTI_ClearITPendingBit(EXTI_Line7);
  }
}
/*******************************************************************************
* Function Name  : EXTI15_10_IRQHandler
* Description    : This function handles EXTI15_10_Line global interrupt request. Used for PIR und ON_OFF
*                   
* Input          : None
* Return         : None
*******************************************************************************/

void EXTI15_10_IRQHandler(void)
{
 /*
 // PIR Sensor Low active
 if(EXTI_GetITStatus(EXTI_Line13) != RESET)
  {
    // Clear the EXTI line 13 pending bit 
    EXTI_ClearITPendingBit(EXTI_Line13); 
    PIR_FLAG = 1;

  }
// Taster ON_OFF Low Active
 if(EXTI_GetITStatus(EXTI_Line12) != RESET)
  {
    // Clear the EXTI line 12 pending bit 
    EXTI_ClearITPendingBit(EXTI_Line12);     
    if(!ON_OFF_Timer)
      ON_OFF_Timer = 100;
  }
  */
}
/**
  * @brief  This function handles RTC global interrupt request.
  * @param  None
  * @retval None
  */
void RTC_IRQHandler(void)
{
  if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
  {
    /* Clear the RTC Second interrupt */
    RTC_ClearITPendingBit(RTC_IT_SEC);

    /* Enable time update */
    #ifdef USE_RTC
      RTCTimeDisplay = 1;
    #endif
    
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    
  }
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/

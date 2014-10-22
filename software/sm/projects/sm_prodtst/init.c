/**
  ******************************************************************************
  * @file    TB12000/init.c 
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    02.2012
  * @brief   Initialisation
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include"stm32f10x.h"
#include "stm32f10x_it.h"
#include "main.h"
#include "init.h"
#include "dmx_control.h"
#include "RS.h"
#include "watch.h"


#define ADC1_DR_Address    ((uint32_t)0x4001244C)
#define USART2_DR_Address  ((uint32_t)0x40004404)


RCC_ClocksTypeDef RCC_ClocksStatus;

ADC_InitTypeDef ADC_InitStructure;


TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;
TIM_ICInitTypeDef TIM_ICInitStructure;
TIM_BDTRInitTypeDef TIM_BDTRInitStructure;

USART_InitTypeDef USART_InitStructure;
USART_ClockInitTypeDef USART_ClockInitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
ErrorStatus HSEStartUpStatus;


vu16 ADC_ConvertedValue[AD_BUFSIZE];//__IO uint16_t

uint16_t TimerPeriod = 0;
uint16_t Channel1Pulse = 0;


uint8_t  RS_TX_Buffer[RS_TX_BufferSize]; // RS485 TX Buffer
vu16 RS_TX_Counter;

void RCC_Configuration(void)
{

#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL)
  /* ADCCLK = PCLK2/2 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div2); 
#else
  /* ADCCLK = PCLK2/4 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div4); 
#endif
  /* Enable peripheral clocks ------------------------------------------------*/
  /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
/*  USART1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  /* TIM1,ADC, GPIOA,GPIOB,GPIOC and AFIO clocks enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB 
                        |RCC_APB2Periph_GPIOC |RCC_APB2Periph_AFIO, ENABLE);
  /*PWR,BACKUP,TIM2,TIM3,TIM4,TIM7,USART2, USART3, DAC clocks enable*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP | RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3
                        |RCC_APB1Periph_TIM4|RCC_APB1Periph_TIM7|RCC_APB1Periph_USART2|RCC_APB1Periph_USART3
                        |RCC_APB1Periph_DAC, ENABLE);
}

void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

#ifdef  VECT_TAB_RAM
  /* Set the Vector Table base location at 0x20000000 */
  NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
  /* Set the Vector Table base location at 0x08002000  because of the custom Bootloader*/
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);
#endif

  /* 0 bit for pre-emption priority, 4 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  /* Enable the TIM2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the TIM3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);


  /* Enable the TIM4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the TIM7 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the USART3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the DMAChannel7 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 5;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the DMAChannel7 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 6;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the EXTI1  Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  if(SYS.bHardware_Version==V1_0)
  {
    /* Enable the EXTI2  Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;  // PB2
  }else
  {
    /* Enable the EXTI9-5  Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;//PB7
  }
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 8;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the EXTI3  Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 9;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the RTC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	

  b_IntCheck = TRUE;
}

void ADC_Configuration(void)
{
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_DeInit(ADC1);
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = AD_BUFSIZE;
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5); //Data[0]
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_55Cycles5); //Data[1]
  ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 3, ADC_SampleTime_55Cycles5); //Data[2]
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16,4, ADC_SampleTime_239Cycles5); //Data[3]
  ADC_RegularChannelConfig(ADC1, ADC_Channel_17,5, ADC_SampleTime_239Cycles5); //Data[4]

  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
   /*Enable internal temp. Sensor*/
  ADC_TempSensorVrefintCmd(ENABLE);

  /* Enable ADC1 reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
     
  /* Start ADC1 Software Conversion */ 
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}


void DAC_Configuration(void)
{
  DAC_InitTypeDef            DAC_InitStructure;

  /* DAC channel1 Configuration */
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);

  /* DAC channel2 Configuration */
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);
}


/*******************************************************************************
* Function Name  : SysTick_Configuration()
* Description    : Initialize SysTick to 100us
* Input          : None
* Return         : None
******************************************* ************************************/
void SysTick_Configuration(void)
{
  //CLK = HCLK
  SysTick_Config(2400);    //Parameter ist reload value 24MHz/2400 (SystemCoreClock/2400);
}

/*******************************************************************************
* Function Name  : EXTI_Configuration()
* Description    : Initialize External Interrupts
* Input          : None
* Return         : None
*******************************************************************************/
void EXTI_Configuration(void)
{
  /* Configure EXTI Line1 to generate an interrupt on falling edge */  
  EXTI_InitStructure.EXTI_Line = EXTI_Line1;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = DISABLE;
  EXTI_Init(&EXTI_InitStructure);
  /* Connect EXTI Line1 to PB1 */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);
  
  if(SYS.bHardware_Version==V1_0)
  {
    /* Configure EXTI Line2 to generate an interrupt on falling edge IN1DALIP*/  
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    /* Connect EXTI Line2 to PB2 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource2);

    /* Configure EXTI Line3 to generate an interrupt on falling edge IN2DALIN*/  
    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    /* Connect EXTI Line1 to PB3 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource3);
  } else { // Hardware V1_1
    /* Configure EXTI Line7 to generate an interrupt on falling edge IN1DALI*/  
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    /* Connect EXTI Line7 to PB7 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);
  }

}



/*******************************************************************************
* Function Name  : Timer2_Configuration()
* Description    : Initialize Timer2 for sending reset sequence
* Input          : None
* Return         : None
*******************************************************************************/
void Timer2_Configuration(void)
{
  /* timer2 configuration */

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = (BREAK_TIME + MAB_TIME);  // 100 + 20
  TIM_TimeBaseStructure.TIM_Prescaler = TIMER2_CLOCK_PRESCALER; // clk/24 = 1MHz
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);


  /* PWM1 Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
  TIM_OCInitStructure.TIM_Pulse = BREAK_TIME;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

  TIM_OC1Init(TIM2, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

  TIM_ARRPreloadConfig(TIM2, ENABLE);

  TIM_ITConfig(TIM2, TIM_IT_Update,ENABLE);
  TIM_ITConfig(TIM2, TIM_IT_CC1,ENABLE);
}

/*******************************************************************************
* Function Name  : Timer3_Configuration()
* Description    : Initialize Timer3 for DAL Master Timebase
* Input          : None
* Return         : None
*******************************************************************************/
void Timer3_Configuration(void)
{
  /* Time base configuration */
  TIM_DeInit(TIM3);
  TIM_TimeBaseStructure.TIM_Period = 9999;  
  TIM_TimeBaseStructure.TIM_Prescaler = 0; 
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);


  TIM_ARRPreloadConfig(TIM3, DISABLE);
  TIM_ITConfig(TIM3, TIM_IT_Update,ENABLE);


}
/*******************************************************************************
* Function Name  : Timer4_Configuration()
* Description    : Initialize Timer4 for sending MARK time between slots
* Input          : None
* Return         : None
*******************************************************************************/
void Timer4_Configuration(void)
{
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = TIMER4_PERIOD;
  TIM_TimeBaseStructure.TIM_Prescaler = RESET_VALUE;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* PWM1 Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_Pulse = TIMER4_PULSE;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

  TIM_OC1Init(TIM4, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

  TIM_ARRPreloadConfig(TIM4, ENABLE);

  TIM_ITConfig(TIM4, TIM_IT_Update,ENABLE);
}



/*******************************************************************************
* Function Name  : Timer7_Configuration()
* Description    : Initialize Timer7 for DAL SLAVE Timebase
* Input          : None
* Return         : None
*******************************************************************************/
void Timer7_Configuration(void)
{
  /* Time base configuration */
  TIM_DeInit(TIM7);
  TIM_TimeBaseStructure.TIM_Period = 14999;  
  TIM_TimeBaseStructure.TIM_Prescaler = 0; 
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);


  TIM_ARRPreloadConfig(TIM7, DISABLE);
  TIM_ITConfig(TIM7, TIM_IT_Update,ENABLE);
}

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;


  /* Configure PA0 (ADC Channel 0)und PA1 (ADC Channel 1) as analog input*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

/* DAC Outputs PA4 and PA5 as Analog inputs - for power saving  - DAC switches automaticaly to output*/

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);


/* PA6,PA7,PA11,PA12,PA15 as General Outputs Push-Pull*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /*PA3 as temporary general input*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Pin =GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);


  GPIOA->BSRR = GPIO_Pin_6; //PA6 HI-> Enable Transmit (disable Receipment on NEW Hardware )
  GPIOA->BRR = GPIO_Pin_7; // PA7 LO-> Enable Receipment only on the V1.0 Hardware
  GPIOA->BRR = GPIO_Pin_2; //PA2 LO-> Set Input LOW 

  // before evaluating HW1 condition, insert some other operations (as small delay)
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE); // JTAG Pins PB4 und PA15 remappen
  /* GPIOA Configuration Break Output: PA8 as input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);


  if( GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_3))// PA3 Read Output State
  {
    SYS.bHardware_Version=V1_1; // REN was of no effect, therefore HW=V1_1
  }else
  {
    SYS.bHardware_Version=V1_0; // REN enabled read back of Sensorbus (LOW) therefore HW=V1_0
  }
  GPIOA->BSRR = GPIO_Pin_2; //PA2 HI again

  /* Configure USART Tx PA2 and PA9 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure USART Rx PA3 and PA10 as input with Pullup */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_10;
  GPIO_Init(GPIOA, &GPIO_InitStructure);


  /*PORT B*/

  /* Configure USART3 Tx PB10 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Configure USART3 RX PB11 as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // wm Pullup statt floating
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Configure PB0 (ADC Channel 8) as analog input*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

/*
  Hardware Version V1.1-> swapped DALI Slave Inputs_Output!!

*/
  if(SYS.bHardware_Version==V1_0)
  {
    /*PB2 as general input*/
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin =GPIO_Pin_2;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    /* PB7 as output */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET); // TX Dali = 1 V0.42
  }else
  {
    /* PB2 as output*/
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin =GPIO_Pin_2;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOB,GPIO_Pin_2,Bit_SET); // TX Dali = 1 V0.42

    /*PB7 as general input*/
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin =GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

  }


  /*PB1,PB3,PB12,PB13 as general inputs*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_12|GPIO_Pin_13;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* GPIOB Configuration: PB4,PB5,PB6,PB8,PB9,PB14,PB15*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 |GPIO_Pin_5 |GPIO_Pin_6 |GPIO_Pin_8 | GPIO_Pin_9|GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // only for Value Line Evaluation Board activ
  #ifndef TB_12K
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
  #endif

}

void USART2_Setup(u32 BaudRate)
{
  if(BaudRate==250000) DMX_ReceiveMode=1;
  else DMX_ReceiveMode=0;
  USART_DeInit(USART2);
  USART_InitStructure.USART_BaudRate = BaudRate; 
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_2; // USART_StopBits_1
  USART_InitStructure.USART_Parity = USART_Parity_No; // USART_Parity_Even
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;

  /* USART2 configuration */
  USART_Init(USART2,&USART_InitStructure);
  /* Enable USART2 */
  USART_Cmd(USART2, ENABLE);
  /*Enable USART2 RX Interrrupt*/
  USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
}

void USART_Configuration(void)
{

  // USART1 for DMX512 250kB/8bit/2Stop bits / only TX
  // USART_DeInit(USART1);

  USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
  USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
  USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
  USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
  USART_ClockInit(USART1,&USART_ClockInitStructure);

  USART_InitStructure.USART_BaudRate = 250000;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_2 ;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx;

  /* USART1 configuration */
  USART_Init(USART1,&USART_InitStructure);
  /* Enable USART1 */
  USART_Cmd(USART1, ENABLE);


  // USART2 for RS485 MODBUS Protocoll, 115200, bidirektional 2x Stopbit
  if(!SYS.wDMX_Starttimer)
    USART2_Setup(115200);
  else
    USART2_Setup(250000); // Test for DMX reception at startup
//   USART_DeInit(USART2);
//   USART_InitStructure.USART_BaudRate = 115200; 
//   USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//   USART_InitStructure.USART_StopBits = USART_StopBits_2; // USART_StopBits_1
//   USART_InitStructure.USART_Parity = USART_Parity_No; // USART_Parity_Even
//   USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//   USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;

//   /* USART2 configuration */
//   USART_Init(USART2,&USART_InitStructure);
//   /* Enable USART2 */
//   USART_Cmd(USART2, ENABLE);
//   /*Enable USART2 RX Interrrupt*/
//   USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);


  //USART3 für Debuging
  USART_DeInit(USART3);
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;

  /* USART3 configuration */
  USART_Init(USART3,&USART_InitStructure);
  /* Enable USART3 */
  USART_Cmd(USART3, ENABLE);


  USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
}

void DMA_Configuration(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  /* DMA1 channel1 for ADC values - configuration --------------------*/
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)(&ADC_ConvertedValue[0]);
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = AD_BUFSIZE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* Enable DMA1 channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);



}


void CPU_Init(void)
{
  RCC_Configuration();
  //RCC_GetClocksFreq(&RCC_ClocksStatus);
  GPIO_Configuration();
  ADC_Configuration();
  DMA_Configuration();
  NVIC_Configuration();
  DAC_Configuration();
  EXTI_Configuration();
  SysTick_Configuration();
  USART_Configuration();
  Timer2_Configuration();
  Timer3_Configuration();
  Timer4_Configuration();
  Timer7_Configuration();
}

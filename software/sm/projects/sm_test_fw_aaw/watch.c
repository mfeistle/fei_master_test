/**
******************************************************************************
* @file    TB12000/watch.c 
* @author  Ing. Büro W.Meier
* @lib     version V3.5.0
* @date    11.2012
* @brief   RTC Clock/Watch
******************************************************************************
*/ 

/* Includes ------------------------------------------------------------------*/
#include"stm32f10x.h"
#include <stdio.h>
#include "MAIN.h"


#ifdef USE_RTC

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
vu8 RTCTimeDisplay = 0;
vu16 WatchTimeout_Keyscan;

/* Private function prototypes -----------------------------------------------*/
uint32_t Time_Regulate(void);
void Time_Adjust(void);
void Time_Show(void);
void Time_Display(uint32_t TimeVar);
void RTC_Configuration(void);

uint8_t USART_Scanf(uint32_t value);


/**

  * @brief  Management of the RTC Clock Startup.
  * @param  None
  * @retval None

*/
void Start_RTC(void)
{
  if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
  {
    /* Backup data register value is not correct or not yet programmed (when
       the first time the program is executed) 
       Backup data register could be used for another purpose as well*/

    printf("\r\n RTC not yet configured....");

    /* RTC Configuration */
    RTC_Configuration();

    printf("\r\n RTC configured,please adjust time with z!!");

    /* INITIAL - FIRST SETUP Adjust time by values entered by the user on the hyperterminal */
  // 		Time_Adjust();

    BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
  }else{
    

      printf("\r\nRTC active....");
      /* Wait for RTC registers synchronization */
      RTC_WaitForSynchro();

      /* Enable the RTC Second -> Interrupt */
      RTC_ITConfig(RTC_IT_SEC, ENABLE);
      /* Wait until last write operation on RTC registers has finished */
      RTC_WaitForLastTask();
  }
}




/**

  * @brief  Returns the time entered by user, using Hyperterminal.
  * @param  None
  * @retval Current time RTC counter value

*/

uint32_t Time_Regulate(void)
{
  uint32_t Tmp_HH = 0xFF, Tmp_MM = 0xFF, Tmp_SS = 0xFF;

  printf("\r\n==Time Settings==");
  printf("\r\n  Set Hours");
  
  while ((Tmp_HH == 0xFF) && WatchTimeout_Keyscan )
  {
    Tmp_HH = USART_Scanf(23);
  }
  printf(":  %d", Tmp_HH);
  printf("\r\n  Set Minutes");
  while ((Tmp_MM == 0xFF)&& WatchTimeout_Keyscan)
  {
    Tmp_MM = USART_Scanf(59);
  }
  printf(":  %d", Tmp_MM);
  printf("\r\n Set Seconds");
  while ((Tmp_SS == 0xFF)&& WatchTimeout_Keyscan)
  {
    Tmp_SS = USART_Scanf(59);
  }
  printf(":  %d", Tmp_SS);
  if(WatchTimeout_Keyscan > 0)	// Timeout hasn't occured, Data valid ?
  {
  /* Return the value to store in RTC counter register */
    return(Tmp_HH*3600 + Tmp_MM*60 + Tmp_SS);
  }else{
    return 0;
  }
}
/**
  * @brief  Displays the current time.
  * @param  TimeVar: RTC counter value.
  * @retval None
  */
void Time_Display(uint32_t TimeVar)
{
  uint32_t THH = 0, TMM = 0, TSS = 0;
  
  /* Reset RTC Counter when Time is 23:59:59 */
  if (RTC_GetCounter() == 0x0001517F)
  {
     RTC_SetCounter(0x0);
     /* Wait until last write operation on RTC registers has finished */
     RTC_WaitForLastTask();
  }
  
  /* Compute  hours */
  THH = TimeVar / 3600;
  THH %= 24;
  /* Compute minutes */
  TMM = (TimeVar % 3600) / 60;
  /* Compute seconds */
  TSS = (TimeVar % 3600) % 60;

  printf("\r\nTime: %0.2d:%0.2d:%0.2d", THH, TMM, TSS);
}

/**
  * @brief  Shows the current time (HH:MM:SS) on the Hyperterminal.
  * @param  None
  * @retval None
  */   
void Time_Show(void)
{
  /* Display current time */
  Time_Display(RTC_GetCounter());
}
/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint8_t USART_Scanf(uint32_t value)
{
  uint32_t index = 0;
  uint32_t tmp[2] = {0, 0};

  while ((index < 2) && WatchTimeout_Keyscan )
  {
    /* Loop until RXNE = 1 */
    while ((USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET) && WatchTimeout_Keyscan)
    {}
    tmp[index++] = (USART_ReceiveData(USART3));
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39))
    {
      printf("\r\nEnter valid number between 0 and 9");
      index--;
    }
  }
  if(!WatchTimeout_Keyscan)
  {
    printf("\r\nTimeout, please try again !\r\n");
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    return 0xFF;
  }else
  {
    /* Calculate the Corresponding value */
    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
    /* Checks */
    if (index > value)
    {
      printf("\r\nEnter valid number between 0 and %d", value);
      return 0xFF;
    }
  }

  return index;
}
void RTC_Configuration(void)
{

  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Reset Backup Domain */
  BKP_DeInit();

  /* Enable LSE */
  RCC_LSEConfig(RCC_LSE_ON);
  /* Wait till LSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)	
  {}

  /* Select LSE as RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

  /* Enable RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC registers synchronization */
  RTC_WaitForSynchro();

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Enable the RTC Second */
  RTC_ITConfig(RTC_IT_SEC, ENABLE);

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Set RTC prescaler: set RTC period to 1sec */
  RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

/**
  * @brief  Adjusts time.
  * @param  None
  * @retval None
  */
void Time_Adjust(void)
{
  uint32_t temp;
  /* Wait until last write operation on RTC registers has finished */
  WatchTimeout_Keyscan = 60*20;  // 60sec Timeout for Timesetting

  USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);	// RX Interrupt disable
    /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Disable the RTC Second */
  RTC_ITConfig(RTC_IT_SEC, DISABLE);

  RTC_WaitForLastTask();
  /* Change the current time */
  temp = Time_Regulate();
  if(temp)
  {
    RTC_SetCounter(temp);
  }

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Enable the RTC Second */
  RTC_ITConfig(RTC_IT_SEC, ENABLE);

  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);	// RX Interrupt enable
}

#endif

/******************************************************************************
  * @file    testprocedures.c
  * @author  Ing. Buero W.Meier
  * @lib version V3.5.0
  * @date    07.2012
  * @brief   Functions for Testing
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
#include "init.h"
#include "Lampcontroller.h"
#include "DALI.h"
#include "dmx_control.h"
#include "eeprom.h"
#include "RS.h"
#include "testprocedures.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// -> changes search for "MAN:"



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/
tTest TST;


/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

//---------------------------------------------------------------------------------------
// 
//
void TestProcess(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  u16    i;
  u8    oc2_freq=0;
  u8    oc4_freq=0;
  u8    oc2_res=0;
  u8    oc4_res=0;
  u8    count=0;
  u8    status_cnt = 0;
  u8    dali_flag = 0;
  u8    Ticker_1ms_flag=0;
  u8    Ticker_50ms_flag=0;
  u8    testcommand = 0;
  u32   u32Var;

  TST.MaxTestTime = 120*50; // 60sek (*50 msec)
  TST.PredelayTime=1;
  TST.TestState = 0;
  TST.ProCheck = 0;
  TST.SensorCheck = 0;
  RxCounter = 0;
  
  printf("\r\nTest V1.05 %usek max\r\n",TST.MaxTestTime/50);
 
  while(TST.MaxTestTime)
  {
    if(RxCounter)
    {
      testcommand = RxBuffer[0];
      RxCounter = 0;
      TST.MaxTestTime = 120*50;
      switch(testcommand) { 
        case 'X':
          TST.MaxTestTime = 0; // exit Testprocedures
          break;
        case 'Y':
          TST.DebugInfo=1;
          break;
        case 'Z':
          TST.DebugInfo=0;
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          TST.PredelayTime=testcommand-'0';
          break;
        default:
          TST.PredelayTimer=TST.PredelayTime;
          break;
      }
      status_cnt = 0;
      TST.ProCheck = 0;
      TST.SensorCheck = 0;
      dali_flag = 0;
    }
    switch(testcommand)
    {
      case 0:
      break;
//---------------------------------------------------------------------------------------
// Testcommand A
      case 'A': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: status_cnt++;
                    break;
                    case 1: TST.CurrentResults[0] = TST.Current;
                            if(TST.DebugInfo) printf("Idle ");
                            printf("%d\r\n",TST.CurrentResults[0]);
                            LED_ROT_EIN();
                            status_cnt++;
                    break;
                    case 2: status_cnt++;
                    break;
                    case 3: TST.CurrentResults[1] = TST.Current-TST.CurrentResults[0];
                            //printf("\r\nLED1: %d",TST.CurrentResults[1]);
                            if(TST.DebugInfo) printf("LED1 ");
                            printf("%d\r\n",TST.CurrentResults[1]);
                            LED_ROT_AUS();
                            LED_GRUEN_EIN();
                            status_cnt++;
                    break;
                    case 4: status_cnt++;
                    break;
                    case 5: TST.CurrentResults[2] = TST.Current-TST.CurrentResults[0];
                            //printf("\r\nLED2: %d",TST.CurrentResults[2]);
                            if(TST.DebugInfo) printf("LED2 ");
                            printf("%d\r\n",TST.CurrentResults[2]);
                            LED_GRUEN_AUS();
                            MaxCurrent = 0;
                            V2_DALI_TXSLAVE_ON();
                            status_cnt++;
                    break;
                    case 6: status_cnt++;
                    break;
                    case 7: TST.CurrentResults[3] = MaxCurrent-TST.CurrentResults[0];
                            //printf("\r\nDALItx: %d",TST.CurrentResults[3]);
                            if(TST.DebugInfo) printf("DALItx ");
                            printf("%d\r\n",TST.CurrentResults[3]);
                            V2_DALI_TXSLAVE_OFF();
                            DALI_ON();
                            status_cnt++;
                    break;
                    case 8: status_cnt++;
                    break;
                    case 9: TST.CurrentResults[4] = TST.Current-TST.CurrentResults[0];
                            TST.RefCurrent=TST.Current; // new
                            //printf("\r\nDALIoff: %d",TST.CurrentResults[4]);
                            if(TST.DebugInfo) printf("DALIoff ");
                            printf("%d\r\n",TST.CurrentResults[4]);
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_7V); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            status_cnt++;
                    break;
                    case 10: status_cnt++;
                    break;
                    case 11: TST.CurrentResults[5] = TST.Current-TST.RefCurrent;
                            //printf("\r\nDAC1: %d",TST.CurrentResults[5]);
                            if(TST.DebugInfo) printf("DAC1 ");
                            printf("%d\r\n",TST.CurrentResults[5]);
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_7V); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            status_cnt++;
                    break;
                    case 12: status_cnt++;
                    break;
                    case 13: TST.CurrentResults[6] = TST.Current-TST.RefCurrent;
                            //printf("\r\nDAC2: %d",TST.CurrentResults[6]);
                            if(TST.DebugInfo) printf("DAC2 ");
                            printf("%d\r\n",TST.CurrentResults[6]);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_10V); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            DALI_OFF();
                            status_cnt++;
                    break;
                    case 14: status_cnt++;
                    break;
                    case 15: TST.CurrentResults[7] = TST.Current;
                            //printf("\r\nIdle: %d",TST.CurrentResults[7]);
                            if(TST.DebugInfo) printf("Idle ");
                            printf("%d\r\n",TST.CurrentResults[7]);
                            REL1_ON();
                            status_cnt++;
                    break;
                    case 16: status_cnt++;
                    break;
                    case 17: TST.CurrentResults[8] = TST.Current-TST.CurrentResults[7];
                            //printf("\r\nRELAIS1: %d",TST.CurrentResults[8]);
                            if(TST.DebugInfo) printf("Rel1 ");
                            printf("%d\r\n",TST.CurrentResults[8]);
                            REL1_OFF();
                            REL2_ON();
                            status_cnt++;
                    break;
                    case 18: status_cnt++;
                    break;
                    case 19:  TST.CurrentResults[9] = TST.Current-TST.CurrentResults[7];
                              //printf("\r\nRELAIS2: %d",TST.CurrentResults[9]);
                              if(TST.DebugInfo) printf("Rel2 ");
                              printf("%d\r\n",TST.CurrentResults[9]);
                              REL2_OFF();
                              MaxCurrent = 0;
                              RS_TX_Send(RS_TX_Buffer,RS_TX_BufferSize);
                              status_cnt++;                     
                    break;
                    case 20:  MaxCurrent = 0;
                              RS_TX_Send(RS_TX_Buffer,RS_TX_BufferSize);
                              status_cnt++;
                    break;
                    case 21:  TST.CurrentResults[10] = MaxCurrent-TST.CurrentResults[7];
                              //printf("\r\nRS485 send: %d",TST.CurrentResults[10]);
                              if(TST.DebugInfo) printf("RS485 ");
                              printf("%d\r\n",TST.CurrentResults[10]);
                              CTRL1_OFF();
                              DMX_ON();
                              DMX_TEN_ON();
                              DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_5V); 
                              DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                              MaxCurrent = 0;
                              Send_DMX();	 // DMX512 testen
                              status_cnt++;
                    break;
                    case 22: status_cnt++;
                    break;
                    case 23:  TST.CurrentResults[11] = MaxCurrent-TST.CurrentResults[7];
                              //printf("\r\nDMX send: %d",TST.CurrentResults[11]);
                              if(TST.DebugInfo) printf("DMX ");
                              printf("%d\r\n",TST.CurrentResults[11]);
                              //CTRL1_ON();
                              DMX_OFF();
                              DMX_TEN_OFF();
                              DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
                              DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                              status_cnt++;
                    break;
                    case 24:  testcommand = 0;
                              InitHW();
                    break;
                    default:
                    break;
                  }
                }
      break;
//---------------------------------------------------------------------------------------
// Testcommand B
      case 'B': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: DALI_ON();
                            CTRL1_ON();
                            DMX_OFF();
                            DMX_TEN_OFF();
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            status_cnt++;
                    break;
                    case 1: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)==0)
                            {
                              dali_flag = 1;
                            }
                            CTRL1_OFF();
                            status_cnt++;
                    break;
                    case 2: status_cnt++;
                    break;
                    case 3: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7) && dali_flag)
                            {
                              printf("1\r\n");
                            }
                            else
                            {
                              printf("0\r\n");
                            }
                            status_cnt++;
                    break;
                    case 4: testcommand = 0;
                    break;
                    default:
                    break;
                  }
                }
      break;
//---------------------------------------------------------------------------------------
// Testcommand C
      case 'C': if(Ticker_1ms_flag)
                {
                  Ticker_1ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: DALI_ON();
                            DMX_OFF();
                            DMX_TEN_OFF();
                            CTRL1_ON();
                            CTRL2_ON();
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            DaliSlave_RX_Status(DALI_ENABLE);
                            V2_DALI_TXSLAVE_ON();
                            status_cnt++;
                    break;
                    case 1: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1) == 0)
                            {
                              printf("1\r\n");
                            }
                            else
                            {
                              printf("0\r\n");
                            }
                            V2_DALI_TXSLAVE_OFF();
                            status_cnt++;
                    break;
                    case 4: testcommand = 0;
                    break;
                    default:
                    break;
                  }
                }
      break;
//---------------------------------------------------------------------------------------
// Testcommand D                              
      case 'D': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: DALI_ON();
                            CTRL2_ON();
                            DMX_OFF();
                            DMX_TEN_OFF();
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            status_cnt++;
                    break;
                    case 1: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)==0)
                            {
                              dali_flag = 1;
                            }
                            CTRL2_OFF();
                            status_cnt++;
                    break;
                    case 2: status_cnt++;
                    break;
                    case 3: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7) && dali_flag)
                            {
                              printf("1\r\n");
                            }
                            else
                            {
                              printf("0\r\n");
                            }
                            status_cnt++;
                    break;
                    case 4: testcommand = 0;
                    break;
                    default:
                    break;
                  }
                }
      break;
//---------------------------------------------------------------------------------------
// Testcommand E
      case 'E': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: REL1_ON();
                            DALI_ON();
                            CTRL1_ON();
                            CTRL2_ON();
                            //DaliSlave_RX_Status(DALI_ENABLE);
                            status_cnt++;
                    break;
                    case 1: if(oc2_res > 7 && oc2_res < 13)
                            //if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)==0)
                            {
                              TST.OC_Results[0]=oc2_res;
                              TST.ProCheck |= 0x01;
                            }
                            REL1_OFF();
                            REL2_ON();
                            status_cnt++;
                    break;
                    case 2: status_cnt++;
                            count=0;
                            oc2_res=0;
                    break;
                    case 3: status_cnt++;
                    break;
                    case 4: if(oc2_res > 7 && oc2_res < 13)
                            {
                              TST.OC_Results[1]=oc2_res;
                              TST.ProCheck |= 0x02;
                            }
                            REL2_OFF();
                            DALI_OFF();
                            CTRL1_OFF();
                            CTRL2_OFF();
                            //DaliSlave_RX_Status(DALI_DISABLE);
                            status_cnt++;
                    break;
                    case 5: LED_ROT_EIN();
                            LED_GRUEN_EIN();
                            status_cnt++;
                    break;
                    case 6: if(SYS.wLuxADCmean>50)
                            {
                              TST.ProCheck |= 0x08;
                            }
                            //if(oc4_res > 7 && oc4_res < 13)
                            if(oc4_res > 5 && oc4_res < 13)         // Wegen grosser Streuung des Optokopplers OC4: Entschärfung Detektion Negative Netz-Halbwelle
                            {
                              TST.OC_Results[2]=oc2_res;
                              TST.ProCheck |= 0x04;
                            }
                            LED_ROT_AUS();
                            LED_GRUEN_AUS();
                            status_cnt++;
                    break;
                    case 7: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12))
                            {
                              TST.ProCheck |= 0x10;
                            }
                            status_cnt++;
                    break;
                    case 8: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)==0)
                            {
                              TST.ProCheck |= 0x20;
                            }
                            
                            //printf("\r\nProcessor Check: %x",TST.ProCheck);
                            if(TST.DebugInfo) printf("Check ");
                            printf("%02X\r\n",TST.ProCheck);
                            status_cnt++;
                    break;
                    case 9: testcommand = 0;
                    break;
                    default:                               
                    break;
                  }
                }
      break;

//---------------------------------------------------------------------------------------
// Testcommand F
      case 'F': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: status_cnt++;
                    break;
                    case 1: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==0)
                            {
                              TST.SensorCheck |= 0x01;
                            }
                            status_cnt++;
                    break;
                    case 2: if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13))
                            {
                              TST.SensorCheck |= 0x02;
                            }
                            //printf("\r\nSensor Check: %x %d",TST.SensorCheck,SYS.wLuxADCmean);
                            if(TST.DebugInfo) printf("Sensor ");
                            printf("%02X %d\r\n",TST.SensorCheck,SYS.wLuxADCmean);
                            status_cnt++;
                    break;                            
                    case 3: testcommand = 0;
                    break;
                    default:
                    break;
                  }
                }

      break;

//---------------------------------------------------------------------------------------
// Testcommand G
      case 'G': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: CTRL1_ON();
                            CTRL2_ON();
                            DALI_OFF();
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_5V); // 5V
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_7V); // 7V
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            status_cnt++;
                    break;
                    case 1: status_cnt++;
                            printf("1\r\n");
                    break;
                    case 2: status_cnt++;
                    break;                            
                    case 3: status_cnt++;
                    break;
                    case 4: status_cnt++;
                    break;
                    case 5: testcommand = 0;
                    break;
                    default:
                    break;
                  }
                }               
      break;
//---------------------------------------------------------------------------------------
// Testcommand H
      case 'H': if(Ticker_50ms_flag)
                {
                  Ticker_50ms_flag = 0;
                  switch(status_cnt)
                  {
                    case 0: status_cnt++;
                    break;
                    case 1: CTRL1_OFF();
                            CTRL2_ON();
                            DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                            DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
                            DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                            DMX_TEN_ON();
                            DALI_OFF();
                            DMX_ON();
                            USARTReinit();
                            status_cnt++;
                    break;
                    case 2: for(i = 0; i < MAX_CHANNEL_COUNT; i++)
                            {
                              DMX_data[i]=0x55;
                            }
                            Send_DMX();
                            status_cnt++;
                    break;
                    case 3: DMX_TEN_OFF();
                            TST.Comm485Test='0';
                            if(RS_RX_Buffer[1] == 0x55)
                            {
                              TST.Comm485Test|=1;
                            } 
                            RS_RX_Counter = 0;

                            /* Configure USART Tx PA2 as push-pull -> on the fly */
                            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
                            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
                            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
                            GPIO_Init(GPIOA, &GPIO_InitStructure);
                          
                            GPIO_WriteBit(GPIOA,GPIO_Pin_2,Bit_SET);  // HI on the PA2
                            SNSTEN(); // Enable Sensor RS485 Driver on HW1.1  
                            status_cnt++;                                                 
                    break;
                    case 4: 
                            if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_10)) // Check previous HI
                            {
                              TST.Comm485Test|=2;
                            } 
                            GPIO_WriteBit(GPIOA,GPIO_Pin_2,Bit_RESET);  // LO on the PA2

                            status_cnt++;
                    break;
                    case 5: 
                            if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_10)==0) // Check previous LO
                            {
                              TST.Comm485Test|=2;
                            } else {
                              TST.Comm485Test&=~2;
                            }
                            printf("%c\r\n",TST.Comm485Test);
                            /* Reconfigure USART Tx PA2 as alternate push-pull -> on the fly */
                            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
                            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
                            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
                            GPIO_Init(GPIOA, &GPIO_InitStructure);
                            SNSREN();           // Enable RX again
                            DMX_OFF();
                            CTRL2_OFF();
                            testcommand = 0;
                    break;
                    default:
                    break;
                  }
                }               
      break;

      default: 
      break;
    }


    if(Ticker_50ms)
    {
      Ticker_50ms = 0;
      if(TST.PredelayTimer) TST.PredelayTimer--;
      else Ticker_50ms_flag = 1;
      TST.MaxTestTime--;
    }

    if(Ticker_1ms)
    {
      Ticker_1ms = 0;
      Ticker_1ms_flag = 1;
//---------------------------------------------------------------------------------------
// Check 50Hz OC2 & OC4
      count++;
      if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_3))
      {
        oc2_freq++;
      }
      if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13))
      {
        oc4_freq++;
      }
      if(count>19)
      {
        oc2_res = oc2_freq;
        oc4_res = oc4_freq;
        oc2_freq = 0;
        oc4_freq = 0;
        count = 0;
      }

//---------------------------------------------------------------------------------------
// ADC Current & LUX
      while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));
      DMA_ClearFlag(DMA1_FLAG_TC1);

      for(i=0;i<AD_BUFSIZE;i++) {
        ADC_Buffer[i]=ADC_ConvertedValue[i];
      }
      // Mittelwert
      ADCAverage[CURR_INPUT]+=ADC_Buffer[CURR_INPUT];
      ADCAverage[LUXAD_INPUT]+=ADC_Buffer[LUXAD_INPUT];
                                                            
      switch(msFast_Counter) // 0..9 = 10msec per state
      {
        case 0: // Mittelwert Initialisierung
                
                ADCAverage[CURR_INPUT]=ADC_Buffer[CURR_INPUT];
                ADCAverage[LUXAD_INPUT]=ADC_Buffer[LUXAD_INPUT];
        break;
        case 7: // 8 Werte für Mittelwert
                TST.Current=(ADCAverage[CURR_INPUT]*107)/100; // mA Eichung
                if(MaxCurrent<TST.Current) MaxCurrent=TST.Current;
                SYS.wLuxADCmean=ADCAverage[LUXAD_INPUT];
                u32Var=SYS.wLuxADCmean;
                u32Var*=1000;
                u32Var/=LAMP_Ch1.wLuxSpan;
                LAMP_Ch1.wLux=u32Var; // SYS.wLuxADCmean/16; // Lux Eichung hier einfügen
        break;
        case 9: msFast_Counter=255;
        break;

        default:
        break;
    
      } // end switch ms_Fast_counter
      msFast_Counter++;
    }  // end if Ticker_1ms
  
  }
  printf("\r\nTestend");
}



void USART1Deinit(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* Configure USART Rx PA3 and PA10 as input with Pullup */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_10;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void USARTReinit(void)
{
  u16 i;

  USART_DeInit(USART1);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_2 ;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;

  /* USART1 configuration */
  USART_Init(USART1,&USART_InitStructure);
  /* Enable USART1 */
  USART_Cmd(USART1, ENABLE);
  /*Enable USART1 RX Interrrupt*/
  USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);

  USART_DeInit(USART2);
  USART_InitStructure.USART_BaudRate = 115200;     // V0.19
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_2;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
  /* USART2 configuration */
  USART_Init(USART2,&USART_InitStructure);
  /* Enable USART2 */
  USART_Cmd(USART2, ENABLE);
  /*Enable USART2 RX Interrrupt*/
  USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);

  //RS485 Buffer defaults
  for(i = 0; i < RS_TX_BufferSize; i++)
  {
    RS_RX_Buffer[i]=0;
  }
  //DMX Buffer defaults
  for(i = 0; i < MAX_CHANNEL_COUNT; i++)
  {
    DMX_RX_Buff[i]=0;
  }
}

void InitHW(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;


  //b_IntCheck = 0;
  REL1_OFF();
  REL2_OFF();
  V2_DALI_TXSLAVE_OFF();
  //LEDs off
  LED_ROT_AUS();
  LED_GRUEN_AUS();
  //DAC1&2 4095
  DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL);  // 
  DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
  DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_10V); 
  DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
  //CTRLs off
  CTRL1_OFF();
  CTRL2_OFF();
  //DALI off
  DALI_OFF();
  DMX_OFF();
  DMX_TEN_ON();

  /* GPIOA Configuration: PA8 in input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
}

void InitTestProcess(void)
{
  u16 i;
  //Relais off
  SYS.bApplication=APPLICATION_TEST;
  
  InitHW();
  //RS485 Buffer defaults
  for(i = 0; i < RS_TX_BufferSize; i++)
  {
    RS_TX_Buffer[i]=0;
  }
  //DMX Buffer defaults
  for(i = 0; i < MAX_CHANNEL_COUNT; i++)
  {
    DMX_data[i]=0;
  }

}


/**

 *****END OF FILE****/

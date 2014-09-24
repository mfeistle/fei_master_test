/******************************************************************************
  * @file    modbus.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    03.2012
  * @brief   modbus
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  ******************************************************************************

*/ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modbus.h"
#include "modbus_master.h"
#include "main.h"
#include "RS.h"
#include "Touchslider.h"
#include "AAWmodule.h"
#include "Lampcontroller.h"
#include "flashsetup.h"


/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  sNONE,
  sREADY,
  sSLIDERGET,
  sSLIDERGET_WAITFORRESPONSE,
  sSLIDERSET,
  sSLIDERSET_WAITFORRESPONSE,
  sSLIDERSET_FWCOMMAND,
  sSLIDERSET_FWCOMMANDWAITFORRESPONSE,
  sAWGET,
  sAWGET_WAITFORRESPONSE,
  sAWSET,
  sAWSET_WAITFORRESPONSE,
  sAWGETSET,
  sAWGETSET_WAITFORRESPONSE,
  sPCONLINE,
  sPCONLINE_WAITFORRESPONSE,
  sPC_RETURNFLASHVAR,
  sPC_RETURNFLASHVAR_WAITFORRESPONSE,
  sPC_GETFLASHVAR,
  sPC_GETFLASHVAR_WAITFORRESPONSE
} t_mbStates;


/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
//static 
uint16_t timerMS,timeoutMS;
//static 
uint16_t TimeOutCounter,WaitTimer;
#define TIMEOUT 10

static t_mbStates mbState;
static t_mbModus mbModus;

static int skipTick = 0;

static volatile uint8_t sliderValue1;
static volatile uint8_t sliderValue2;
static volatile uint8_t sliderKeys;
static uint8_t sliderFunctionSelector;
static uint8_t sliderR;
static uint8_t sliderG;
static uint8_t sliderB;
static uint8_t bAAW_Testrequest;

static uint8_t LocalDaliCh1;
static uint8_t LocalDaliCh2;

uint8_t abPCcommand[2];
union {
  u8  abVar[4];
  u32 ulVar;
} Setup;
uint16_t indexFlashVar;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

const uint16_t crcInitValue = 0xffff;
static const uint16_t crcTable[] = 
{
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

void SetRGBLeds(uint8_t array[])
{
  sliderFunctionSelector=array[0];
  sliderR=array[1];
  sliderG=array[2];
  sliderB=array[3];
}

void SetMB_Modus(t_mbModus Mode)
{
  mbModus=Mode;
}

void SetLocalDaliValues(uint8_t Ch1,uint8_t Ch2)
{
  LocalDaliCh1=Ch1;
  LocalDaliCh2=Ch2;
}


static uint16_t CalcCRC16(const uint8_t array[], uint16_t count)
{
  uint16_t crc = crcInitValue;
  uint16_t i;
  
  for (i = 0; i < count; i++)
    crc = (uint16_t)(crcTable[(crc ^ ((uint16_t)(array[i]))) & 0x00ff] ^ (crc >> 8));
  
  return crc;
}

static uint8_t CheckCRC16(const uint8_t data[], uint16_t length)
{
  uint16_t crc;



  crc =  data[length-1] << 8;
  crc += data[length-2];
  return (CalcCRC16(data, length-2) == crc);
}

void MB_Process1ms(void)
{

  timerMS++;
  if (timerMS >= 1000)
    timerMS = 0;
  timeoutMS=timerMS+1;
  
  do
  {
    switch (mbState)
    {
      case sNONE:
      {
        if (timerMS >= 500)
        {
          mbState = sREADY;
          timerMS = 0;
          timeoutMS=timerMS+1;
        }
        break;
      }
      case sREADY:
      {
        if (timerMS % 20 == 0) // Taktrate für Sliderget 20=20ms
        {
          switch(mbModus) {
            case mNONE:
              break;
            case mSLIDER:
              mbState = sSLIDERGET;
              skipTick = 1;
            
              if(SLIDER.bOKTimer) 
                SLIDER.bOKTimer--;
              break;
            case mAAW:
              skipTick = 1;
              if(bAAW_Testrequest==1) {
                bAAW_Testrequest=2;
                abPCcommand[0]=0xfe;
                abPCcommand[1]=100;
                mbState=sAWSET;
              } else if(bAAW_Testrequest==3) {
                //bAAW_Testrequest=0;
                abPCcommand[0]=0xfe;
                abPCcommand[1]=101;
                mbState=sAWSET;
              } else if(bAAW_Testrequest==2) {
                if(WaitTimer) {
                  WaitTimer--;
                  skipTick = 0;
                } else {
                  WaitTimer=10-1;
                  mbState=sAWGET;
                }
              } else {
                mbState = sAWGETSET;
              }
            
              if(AAW.bOKTimer>1)
                AAW.bOKTimer--;
              else if(AAW.bOKTimer==1) {
                AAW.bOKTimer=0;
                LAMP_Ch2.wLux=0;
              }
              break;
          }
        }
        else if (timerMS % 990 == 0)
        {
          mbState = sPCONLINE;
          skipTick = 1;
        }
        break;
      }
      
      // Slider Application
      case sSLIDERGET:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Get slider values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dSLIDER;
        RS_TX_Buffer[RS_TX_Counter++] = fcREADHOLDINGREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = SLIDER_A_Sliders >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = SLIDER_A_Sliders & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 2 >> 8;   // Anzahl zu lesende 16bit Register high Byte
        RS_TX_Buffer[RS_TX_Counter++] = 2 & 0xff; // Anzahl zu lesende 16bit Register low Byte
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sSLIDERGET_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      case sSLIDERGET_WAITFORRESPONSE:
      {

        if ((RS_RX_Counter != 9) ||
            (RS_RX_Buffer[0] != dSLIDER) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcREADHOLDINGREGISTER) ||
            (RS_RX_Buffer[2] != 2*2)  ||
            (CheckCRC16(RS_RX_Buffer, 9) != 1))

        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          {                                   
            TimeOutCounter++;
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
  
        // Get value
        sliderValue1 = RS_RX_Buffer[3];
        sliderValue2 = RS_RX_Buffer[4];
        sliderKeys   = RS_RX_Buffer[6];
        
        ProcessTouchmessage();
        
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        mbState = sSLIDERSET;
        skipTick = 1;
  
        break;
      }
      
      case sSLIDERSET:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Set slider values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dSLIDER;
        RS_TX_Buffer[RS_TX_Counter++] = fcWRITEMULTIPLEREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = SLIDER_A_FunctionR >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = SLIDER_A_FunctionR & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 2 >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = 2 & 0xff; // Anzahl Register
        RS_TX_Buffer[RS_TX_Counter++] = 4; // byte Count
        RS_TX_Buffer[RS_TX_Counter++] = sliderFunctionSelector;
        RS_TX_Buffer[RS_TX_Counter++] = sliderR;
        RS_TX_Buffer[RS_TX_Counter++] = sliderG;
        RS_TX_Buffer[RS_TX_Counter++] = sliderB;
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sSLIDERSET_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      
      case sSLIDERSET_WAITFORRESPONSE:
      {
        if ((RS_RX_Counter != 8) ||
            (RS_RX_Buffer[0] != dSLIDER) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcWRITEMULTIPLEREGISTER) ||
            (RS_RX_Buffer[2] !=  SLIDER_A_FunctionR >> 8) ||
            (RS_RX_Buffer[3] != (SLIDER_A_FunctionR & 0xff)) ||
            (RS_RX_Buffer[4] != 2 >> 8) ||
            (RS_RX_Buffer[5] != 2 & 0xff) ||
            (CheckCRC16(RS_RX_Buffer, 8) != 1)
           )
        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          { 
            TimeOutCounter++;
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        mbState = sREADY;
        skipTick = 0;
  
        break;
      }
      
      case sSLIDERSET_FWCOMMAND:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Set slider values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dSLIDER;
        RS_TX_Buffer[RS_TX_Counter++] = fcWRITEMULTIPLEREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = SLIDER_A_SWUPDATEORTEST >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = SLIDER_A_SWUPDATEORTEST & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 1 >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = 1 & 0xff; // Anzahl Register
        RS_TX_Buffer[RS_TX_Counter++] = 2; // byte Count
        RS_TX_Buffer[RS_TX_Counter++] = 0;
        RS_TX_Buffer[RS_TX_Counter++] = 0;
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sSLIDERSET_FWCOMMANDWAITFORRESPONSE;
        skipTick = 0;
      }
      
      case sSLIDERSET_FWCOMMANDWAITFORRESPONSE:
      {
        if ((RS_RX_Counter != 8) ||
            (RS_RX_Buffer[0] != dSLIDER) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcWRITEMULTIPLEREGISTER) ||
            (RS_RX_Buffer[2] !=  SLIDER_A_SWUPDATEORTEST >> 8) ||
            (RS_RX_Buffer[3] != (SLIDER_A_SWUPDATEORTEST & 0xff)) ||
            (RS_RX_Buffer[4] != 2 >> 8) ||
            (RS_RX_Buffer[5] != 2 & 0xff) ||
            (CheckCRC16(RS_RX_Buffer, 8) != 1)
           )
        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          { 
            TimeOutCounter++;
            // Trotz fehlender Antwort in Bootloader gehen
            if(abPCcommand[0]==0xfe) {
              if(abPCcommand[1]==0) 
                GotoBootloader();
            }
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
        if(abPCcommand[0]==0xfe) {
          if(abPCcommand[1]==0) 
            GotoBootloader();
          // später kann hier else warten bis Slider neue FW erhalten hat eingesetzt werden
        }
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        mbState = sREADY;
        skipTick = 0;
  
        break;
      }
      
      // Alone at Work
      case sAWGET:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Get AAW values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dAW; // Device Adress = 2
        RS_TX_Buffer[RS_TX_Counter++] = fcREADHOLDINGREGISTER; // Function Code = 3
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_TestResult1 >> 8; // 0x1000>>8 = 16
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_TestResult1 & 0xff;  //0
        RS_TX_Buffer[RS_TX_Counter++] = 3 >> 8;   // Anzahl zu lesende 16bit Register high Byte
        RS_TX_Buffer[RS_TX_Counter++] = 3 & 0xff; // Anzahl zu lesende 16bit Register low Byte
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sAWGET_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      
      case sAWGET_WAITFORRESPONSE:
      {
        uint16_t wA;
        
        if ((RS_RX_Counter != 11) ||    
            (RS_RX_Buffer[0] != dAW) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcREADHOLDINGREGISTER) ||
            (RS_RX_Buffer[2] != 3*2) ||
            (CheckCRC16(RS_RX_Buffer, 11) != 1))

        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          {                                   
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
  
        wA =RS_RX_Buffer[3]; wA*=256;
        wA|=RS_RX_Buffer[4];
        
        
        printf("\r\nTestresult: %04X %2u %2u %2u %2u",wA,RS_RX_Buffer[5],RS_RX_Buffer[6],RS_RX_Buffer[7],RS_RX_Buffer[8]);
        
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        
        mbState = sAWSET;
        skipTick = 0;
  
        break;
      }
      
      case sAWSET:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Set slider values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dAW;
        RS_TX_Buffer[RS_TX_Counter++] = fcWRITEMULTIPLEREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_SWUPDATEORTEST >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_SWUPDATEORTEST & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 1 >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = 1 & 0xff; // Anzahl Register
        RS_TX_Buffer[RS_TX_Counter++] = 2;  // Byte Count
        switch(abPCcommand[1]) {
          case 0: 
            RS_TX_Buffer[RS_TX_Counter++] = 0; // SM erhält update, AAW in Warteschlaufe setzen
            break;
          case 2:
            RS_TX_Buffer[RS_TX_Counter++] = 1; // AAW erhält update, AAW in Bootmode (Y-Modem) setzen
            break;
          case 100:
            RS_TX_Buffer[RS_TX_Counter++] = 100; // In Testmodus setzen
            break;
          case 101:
            RS_TX_Buffer[RS_TX_Counter++] = 101; // Testmodus zurücksetzen
            break;
          default:
            RS_TX_Buffer[RS_TX_Counter++] = 255; // nichts machen
            break;
        }
        RS_TX_Buffer[RS_TX_Counter++] = 0; //
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sAWSET_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      
      case sAWSET_WAITFORRESPONSE:
      {
        if ((RS_RX_Counter != 8) ||
            (RS_RX_Buffer[0] != dAW) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcWRITEMULTIPLEREGISTER) ||
            (RS_RX_Buffer[2] != AW_A_SWUPDATEORTEST >> 8) ||
            (RS_RX_Buffer[3] != (AW_A_SWUPDATEORTEST & 0xff)) ||
            (RS_RX_Buffer[4] != 1 >> 8) ||
            (RS_RX_Buffer[5] != 1 & 0xff) ||
            (CheckCRC16(RS_RX_Buffer, 8) != 1))
        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          { 
            // Trotz fehlender Antwort in Bootloader gehen
            if(abPCcommand[0]==0xfe) {
              if(abPCcommand[1]==0) 
                GotoBootloader();
            }
            TimeOutCounter++;
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }

        switch(abPCcommand[1]) {
          case 0: 
            // SM erhält update, AAW ist in Warteschlaufe
            GotoBootloader();
            break;
          case 2:
            // AAW erhält update, SM muss warten
            printf("\r\nAAW update");
            LED_ROT_EIN();
            TimeOutCounter=60; // 1 Minute Timeout
            while(TimeOutCounter) {
              if(Ticker_1s) {
                Ticker_1s=0;

                if(TimeOutCounter & 1)
                  LED_GRUEN_EIN();
                else 
                  LED_GRUEN_AUS();

                TimeOutCounter--;
                if(TimeOutCounter==1) 
                  printf("\r\nReset");
              }
            }
            __disable_irq();
            *((unsigned int *)0x20001FF0)=0xffffffff;
            NVIC_SystemReset();
            break;
          case 101:
            bAAW_Testrequest=0;
            break;
          default:
            
            break;
        }
        
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        mbState = sREADY;
        skipTick = 0;
  
        break;
      }

      
      case sAWGETSET:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Get AAW values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dAW; // Device Adress = 2
        RS_TX_Buffer[RS_TX_Counter++] = fcREADWRITEMULTIPLEREGISTER; // Function Code = 3
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_Luxvalue >> 8; // 0x1000>>8 = 16
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_Luxvalue & 0xff;  //0
        RS_TX_Buffer[RS_TX_Counter++] = 3 >> 8;   // Anzahl zu lesende 16bit Register high Byte
        RS_TX_Buffer[RS_TX_Counter++] = 3 & 0xff; // Anzahl zu lesende 16bit Register low Byte
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_LocalDalivalues >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = AW_A_LocalDalivalues & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 1 >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = 1 & 0xff; // Anzahl Register
        RS_TX_Buffer[RS_TX_Counter++] = 2;  // Byte Count
        RS_TX_Buffer[RS_TX_Counter++] = LocalDaliCh1; // Fixwerte zum testen statt LocalDaliCh1; // #11 DaliLocal Ch1;
        RS_TX_Buffer[RS_TX_Counter++] = LocalDaliCh2; // LocalDaliCh2; // #12 DaliLocal Ch2;
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sAWGETSET_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      
      case sAWGETSET_WAITFORRESPONSE:
      {

        if ((RS_RX_Counter != 11) ||    
            (RS_RX_Buffer[0] != dAW) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcREADWRITEMULTIPLEREGISTER) ||
            (RS_RX_Buffer[2] != 3*2) 		||
            (CheckCRC16(RS_RX_Buffer, 11) != 1))

        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          {                                   
            TimeOutCounter++;
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
  
        ProcessAAWmodulemessage();
        
        
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        
        mbState = sREADY;
        skipTick = 0;
  
        break;
      }

      
      case sPC_GETFLASHVAR:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Get slider values
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dPC;
        RS_TX_Buffer[RS_TX_Counter++] = fcREADHOLDINGREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = PC_GETFLASHVARL >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = PC_GETFLASHVARL & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 2 >> 8;   // Anzahl zu lesende 16bit Register high Byte
        RS_TX_Buffer[RS_TX_Counter++] = 2 & 0xff; // Anzahl zu lesende 16bit Register low Byte
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sPC_GETFLASHVAR_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      case sPC_GETFLASHVAR_WAITFORRESPONSE:
      {

        if ((RS_RX_Counter != 9) ||
            (RS_RX_Buffer[0] != dPC) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcREADHOLDINGREGISTER) ||
            (RS_RX_Buffer[2] != 2*2)  ||
            (CheckCRC16(RS_RX_Buffer, 9) != 1))

        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          {                                   
            TimeOutCounter++;
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
  
        // Get value
        Setup.abVar[0] = RS_RX_Buffer[3];
        Setup.abVar[1] = RS_RX_Buffer[4];
        Setup.abVar[2] = RS_RX_Buffer[5];
        Setup.abVar[3] = RS_RX_Buffer[6];
        FlashSetup.Var[indexFlashVar]=Setup.ulVar;
        
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        mbState = sREADY;
        skipTick = 0;
  
        break;
      }
      
      
      case sPC_RETURNFLASHVAR:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dPC;
        RS_TX_Buffer[RS_TX_Counter++] = fcWRITEMULTIPLEREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = PC_RETURNFLASHVAR >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = PC_RETURNFLASHVAR & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 2 >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = 2 & 0xff; // Anzahl Register
        RS_TX_Buffer[RS_TX_Counter++] = 4; // byte Count
        RS_TX_Buffer[RS_TX_Counter++] = Setup.abVar[0]; // Testreturn 1234 %%wm richtige Variable einsetzen
        RS_TX_Buffer[RS_TX_Counter++] = Setup.abVar[1];
        RS_TX_Buffer[RS_TX_Counter++] = Setup.abVar[2];
        RS_TX_Buffer[RS_TX_Counter++] = Setup.abVar[3];
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sPC_RETURNFLASHVAR_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }

      case sPC_RETURNFLASHVAR_WAITFORRESPONSE:
      {
        if ((RS_RX_Counter != 8) ||
            (RS_RX_Buffer[0] != dPC) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcWRITEMULTIPLEREGISTER) ||
            (RS_RX_Buffer[2] !=  PC_RETURNFLASHVAR >> 8) ||
            (RS_RX_Buffer[3] != (PC_RETURNFLASHVAR & 0xff)) ||
            (RS_RX_Buffer[4] != 2 >> 8) ||
            (RS_RX_Buffer[5] != 2 & 0xff) ||
            (CheckCRC16(RS_RX_Buffer, 8) != 1)
           )
        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          { 
            TimeOutCounter++;
            /* Error: Missing response */
          }
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          mbState = sREADY;
          skipTick = 0;
          break;
        }
        // Reset rx buffer
        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        mbState = sREADY;
        skipTick = 0;
  
        break;
      }
      
      
      case sPCONLINE:
      {
        uint16_t crc;
  
        RS_RX_Counter = 0;
  
        // Check for PC
        RS_TX_Counter = 0;
        RS_TX_Buffer[RS_TX_Counter++] = dPC;
        RS_TX_Buffer[RS_TX_Counter++] = fcREADHOLDINGREGISTER;
        RS_TX_Buffer[RS_TX_Counter++] = PC_A_COMMAND >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = PC_A_COMMAND & 0xff;
        RS_TX_Buffer[RS_TX_Counter++] = 1 >> 8;
        RS_TX_Buffer[RS_TX_Counter++] = 1 & 0xff;
        crc = CalcCRC16(RS_TX_Buffer, RS_TX_Counter);
        RS_TX_Buffer[RS_TX_Counter++] = crc & 0xFF;
        RS_TX_Buffer[RS_TX_Counter++] = crc >> 8;
        RS_TX_Send(RS_TX_Buffer, RS_TX_Counter);
  
        mbState = sPCONLINE_WAITFORRESPONSE;
        skipTick = 0;
  
        break;
      }
      
      case sPCONLINE_WAITFORRESPONSE:
      {
        t_mbPCCommand pcCommand;
  
        if ((RS_RX_Counter != 7) ||
            (RS_RX_Buffer[0] != dPC) ||
            ((t_mbFunctionCode)RS_RX_Buffer[1] != fcREADHOLDINGREGISTER) ||
            (RS_RX_Buffer[2] != 2) ||
            (CheckCRC16(RS_RX_Buffer, 7) != 1))
        {
          if (timeoutMS % TIMEOUT)
            break;
          else
          {                                   
            SYS.wLED_Redtimer=50;
            /* Error: Missing response */
          }
          mbState = sREADY;
          RS_RX_Counter = 0;
          RS_RX_Complete = 0;
          skipTick = 0;
          break;
        }
        pcCommand = (t_mbPCCommand)(RS_RX_Buffer[3] << 8);
        pcCommand |= (t_mbPCCommand)RS_RX_Buffer[4];

        abPCcommand[0]=RS_RX_Buffer[3]; // Test
        abPCcommand[1]=RS_RX_Buffer[4];
        
        switch(abPCcommand[0]) {
          default:
          case 0xff: 
            mbState=sREADY;
            break;
          case 0xfe:
          {
            if(AAW.bOKTimer)
              mbState=sAWSET;
            else if(SLIDER.bOKTimer) {
              SYS.bBootloaderRequest=1;
              mbState=sSLIDERSET_FWCOMMAND;
            } else {
              switch(abPCcommand[1]) {
                case 0: 
                  // SM erhält update, AAW ist in Warteschlaufe
                  GotoBootloader();
                  break;
                default:
                  mbState = sREADY;
                  break;
              }
            }
            break;
          }
          case 0x40: 
          case 0x41: 
          case 0x42: 
          case 0x43:
            indexFlashVar=((abPCcommand[0] & 3) << 8)+abPCcommand[1];
            Setup.ulVar=FlashSetup.Var[indexFlashVar];
            mbState=sPC_RETURNFLASHVAR;
            break;
          case 0x80: 
          case 0x81: 
          case 0x82: 
          case 0x83: 
            indexFlashVar=((abPCcommand[0] & 3) << 8)+abPCcommand[1];
            mbState=sPC_GETFLASHVAR;
            break;
          case 0xc0:
            if(abPCcommand[1]==0) EraseFlashSetup();
            if(abPCcommand[1]==1) WriteCompleteFlashSetup();
            indexFlashVar=0;
            mbState=sPC_RETURNFLASHVAR;
            //mbState = sREADY; // ?????
            break;
        }

        RS_RX_Counter = 0;
        RS_RX_Complete = 0;
        //mbState = sREADY;

        skipTick = 0;

        break;
      }
    }
  } while (skipTick);
}

void SetAAW_Testmode(u8 Bit)
{
  bAAW_Testrequest=Bit;
}

void MB_Init(void)
{
  timerMS = 0;
  mbState = sNONE;
  //mbModus = mAAW;//mSLIDER;
}

/**

 *****END OF FILE****/

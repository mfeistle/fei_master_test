/******************************************************************************
  * @file    tweak.c
  * @author  Ing. Buero W.Meier
  * @lib version V3.5.0
  * @date    07.2012
  * @brief   Functions for Tweak
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
#include "DALI.h"
#include "dmx_control.h"
#include "eeprom.h"
#include "RS.h"
#include "SliderController.h"
#include "OutputController.h"
#include "Lampcontroller.h"
#include "flashsetup.h"


#include "Dalitable256.c"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// -> changes search for "MAN:"

// parameters APPLICATION_DUALSLIDER
// =================================
// #define MIN_DIMM_LEVEL           10
// #define MAX_DIMM_LEVEL           100


/* Private macro -------------------------------------------------------------*/


// MAN: arc power to percentage mapping
const u8 lookup_percent_to_dali[101] =
{
      0,  85, 111, 126, 136, 144, 151, 157, 161, 166, //0-9
    170, 173, 176, 179, 182, 185, 187, 189, 191, 193, //10-19
    195, 197, 199, 200, 202, 203, 205, 206, 207, 209, //20-29
    210, 211, 212, 213, 214, 216, 217, 218, 219, 220, //30-39
    220, 221, 222, 223, 224, 225, 226, 226, 227, 228, //40-49
    229, 229, 230, 231, 231, 232, 233, 233, 234, 235, //50-59
    235, 236, 236, 237, 238, 238, 239, 239, 240, 240, //60-69
    241, 241, 242, 242, 243, 243, 244, 244, 245, 245, //70-79
    246, 246, 247, 247, 248, 248, 248, 249, 249, 250, //80-89
    250, 251, 251, 251, 252, 252, 253, 253, 253, 254, //90-99
    254                                               //100
};


const u8 lookup_dali_to_percent[256] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  0-  9
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 10- 19
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 20- 29
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 30- 39
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 40- 49
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 50- 59
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 60- 69
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 70- 79
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 80- 89
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 90- 99
     1,  2,  2,  2,  2,  2,  2,  2,  2,  2, //100-109
     2,  2,  2,  2,  2,  2,  2,  2,  2,  3, //110-119
     3,  3,  3,  3,  3,  3,  3,  3,  3,  3, //120-129
     3,  3,  4,  4,  4,  4,  4,  4,  4,  4, //130-139
     4,  5,  5,  5,  5,  5,  5,  5,  6,  6, //140-149
     6,  6,  6,  6,  7,  7,  7,  7,  7,  7, //150-159
     8,  8,  8,  8,  9,  9,  9,  9, 10, 10, //160-169
    10, 10, 11, 11, 11, 12, 12, 12, 13, 13, //170-179
    13, 14, 14, 14, 15, 15, 16, 16, 16, 17, //180-189
    17, 18, 18, 19, 19, 20, 21, 21, 22, 22, //190-199
    23, 24, 24, 25, 26, 26, 27, 28, 28, 29, //200-209
    30, 31, 32, 33, 34, 34, 35, 36, 37, 38, //210-219
    40, 41, 42, 43, 44, 45, 47, 48, 49, 51, //220-229
    52, 53, 55, 56, 58, 60, 61, 63, 65, 66, //230-239
    68, 70, 72, 74, 76, 78, 80, 83, 85, 87, //240-249
    90, 92, 95, 97, 100, 100                //250-255
};

tOutputController OUTPUT;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void DaliOutputProcess(void)
{
  if(SYS.bDali1Output>254) SYS.bDali1Output=254;
  if(LAMP_Ch1.fOn) {
    if(!SYS.bDali1Output) SYS.bDali1Output=1;
  } else SYS.bDali1Output=0;
  // when switched on fresh, force Output every 50msec to prevent Fulloutput in EVG
  if(LAMP_Ch1.bForceOutput) {
    LAMP_Ch1.bForceOutput--;
    Dali_TX_data[1]=0;
  }

  if(!OUTPUT.bParallelModus)  {
    if(SYS.bDali2Output>254) SYS.bDali2Output=254;
    if(LAMP_Ch2.fOn) {
      if(!SYS.bDali2Output) SYS.bDali2Output=1;
    } else SYS.bDali2Output=0;
    if(LAMP_Ch2.bForceOutput) {
      LAMP_Ch2.bForceOutput--;
      Dali_TX_data2[1]=0;
    }
  } else SYS.bDali2Output=SYS.bDali1Output;

  // V0.40
  switch(SYS.bDaliExternOperation) {
    case 0: // beide Kanäle = Lokal
      if (SYS.bDali1Output != Dali_TX_data[1] || SYS.bDali2Output != Dali_TX_data2[1]) { // alle 50msec uebertragen, wenn etwas geaendert
        Dali_TX_data [1]=SYS.bDali1Output;
        Dali_TX_data2[1]=SYS.bDali2Output;
        SetDaliTX_Data(Dali_TX_data,Dali_TX_data2); // Data in forward frame packen
        DALI_Send();  // DALI Data senden
        SYS.bDalitimer=20; // 
        //SYS.wLED_Greentimer=10;
      } 
      break;
    case 1: // Ch1 von AAW vorgegeben
      if(Dali_TX_data [1] != SYS.bDaliExternDimvalueCh1 || Dali_TX_data2 [1] != SYS.bDali2Output) {
        Dali_TX_data [1]=SYS.bDaliExternDimvalueCh1;
        Dali_TX_data2[1]=SYS.bDali2Output;
        SetDaliTX_Data(Dali_TX_data,Dali_TX_data2); // Data in forward frame packen
        DALI_Send();  // DALI Data senden
        SYS.bDalitimer=20; // 
      }
      break;
    case 2: // Ch2 von AAW vorgegeben
      if(Dali_TX_data [1] != SYS.bDali1Output || Dali_TX_data2 [1] != SYS.bDaliExternDimvalueCh2) {
        Dali_TX_data [1]=SYS.bDali1Output;
        Dali_TX_data2[1]=SYS.bDaliExternDimvalueCh2;
        SetDaliTX_Data(Dali_TX_data,Dali_TX_data2); // Data in forward frame packen
        DALI_Send();  // DALI Data senden
        SYS.bDalitimer=20; // 
      }
      break;
    case 3: // beide Kanäle von AAW vorgegeben
      if(Dali_TX_data [1] != SYS.bDaliExternDimvalueCh1 || Dali_TX_data2 [1] != SYS.bDaliExternDimvalueCh2) {
        Dali_TX_data [1]=SYS.bDaliExternDimvalueCh1;
        Dali_TX_data2[1]=SYS.bDaliExternDimvalueCh2;
        SetDaliTX_Data(Dali_TX_data,Dali_TX_data2); // Data in forward frame packen
        DALI_Send();  // DALI Data senden
        SYS.bDalitimer=20; // 
      }
      break;
  }
  if(SYS.bDalitimer) SYS.bDalitimer--;
  else {
    SetDaliTX_Data(Dali_TX_data,Dali_TX_data2); // Data in forward frame packen
    DALI_Send();  // DALI Data senden
    SYS.bDalitimer=20-1; // Repetition 1sec -> 500msec (20x50msec) wenn keine Änderung
  }
}


u16 GetOutputValue(u16 wPromille) 
{
  u32 x;

  if(wPromille>1000) wPromille=1000;
  x=wPromille;
  
  if (RemoteCtrl.Active) {
    x = wPromille;
  }
  else {
    switch(SYS.bDeviceMode) {
      case MODE_DALI:
      case MODE_DUALSLIDE:
        x=dali_250[wPromille/4]; // 250 Step Dali Table
        // x*=1000;
        // x/=3937; // 0..1000 -> 0..254
        break;
      case MODE_DMX:
        x*=102;
        x/=400;  // 0..1000 -> 0..255
        break;
      case MODE_VOLT:
        // 1..10V
        x*=(DAC_10V-DAC_1V);
        x/=1000;
        x+=DAC_1V;
        break;
    }
  }
  return x;
}


u16 SetOutputLux(u8 bSliderValue)
{
  if (bSliderValue>127) bSliderValue=127; 
  return CalculateSetlux(bSliderValue);
}

u8 SetDimValues(u8 bSliderValue)
{
  u32   u32tmp;
  
  if (bSliderValue>127) bSliderValue=127; 
  if(OUTPUT.bLogScale) {
    u32tmp=OUTPUT.bMinDimmLevel+((bSliderValue)*(OUTPUT.bMaxDimmLevel-OUTPUT.bMinDimmLevel)/127);
    if(u32tmp<sizeof(lookup_percent_to_dali)) 
      return(lookup_percent_to_dali[(u8)u32tmp]);
    else 
      return(lookup_percent_to_dali[OUTPUT.bMaxDimmLevel]);
  } else {
    return(bSliderValue*2);
  }
}


void DaliRampControl(void)
{
  u8 n;

  // Rampcontrol
  n=OUTPUT.bDaliIncDec; // how many increments or decrements per 10msec 
  while(n--) {
    if(!LAMP_Ch1.bLuxControllerON) {
      if(LAMP_Ch1.bStatus != STATUS_ON) {
        if(SYS.bDali1Output>OUTPUT.bMinDalilevel) SYS.bDali1Output--;
        else SYS.bDali1Output=0;
      } else {
        if(SYS.bDali1Output<OUTPUT.bMinDalilevel) SYS.bDali1Output=OUTPUT.bMinDalilevel;
        else if(SYS.bDali1Output<OUTPUT.bDaliSetLevelDirect) SYS.bDali1Output++;
        else if(SYS.bDali1Output>OUTPUT.bDaliSetLevelDirect) SYS.bDali1Output--;
      }
    }

    if(!OUTPUT.bParallelModus)  {
      if(LAMP_Ch2.bStatus != STATUS_ON) {
        if(SYS.bDali2Output>OUTPUT.bMinDalilevel) SYS.bDali2Output--;
        else SYS.bDali2Output=0;
      } else {
        if(SYS.bDali2Output<OUTPUT.bMinDalilevel) SYS.bDali2Output=OUTPUT.bMinDalilevel;
        else if(SYS.bDali2Output<OUTPUT.bDaliSetLevelIndirect) SYS.bDali2Output++;
        else if(SYS.bDali2Output>OUTPUT.bDaliSetLevelIndirect) SYS.bDali2Output--;
      }
    } 
  }
}


void InitOutputController(void)
{
  if(Flash.Setup.SETUP_VALID == SETUP_VALID_MARK) {
    OUTPUT.bParallelModus=Flash.Setup.OUT_MATRIX;
    OUTPUT.bLogScale=Flash.Setup.OUT_SCALER;
    OUTPUT.bDaliIncDec=Flash.Setup.SLIDER_SPEED;
    OUTPUT.bMinDimmLevel=Flash.Setup.OUT_LEV_MIN;
    OUTPUT.bMaxDimmLevel=Flash.Setup.OUT_LEV_MAX;
    if(Flash.Setup.PWR_FAIL_MODE==2) {
      OUTPUT.bDaliSetLevelDirect  =lookup_percent_to_dali[OUTPUT.bMinDimmLevel];
      OUTPUT.bDaliSetLevelIndirect=lookup_percent_to_dali[OUTPUT.bMinDimmLevel];
    }
  } else {
    OUTPUT.bLogScale=1;
    OUTPUT.bDaliIncDec=3;
    OUTPUT.bMinDimmLevel=5;
    OUTPUT.bMaxDimmLevel=100;
    OUTPUT.bDaliSetLevelDirect  =lookup_percent_to_dali[OUTPUT.bMaxDimmLevel];
    OUTPUT.bDaliSetLevelIndirect=lookup_percent_to_dali[OUTPUT.bMaxDimmLevel];
  }
  OUTPUT.bMinDalilevel=lookup_percent_to_dali[OUTPUT.bMinDimmLevel];
  OUTPUT.bMaxDalilevel=lookup_percent_to_dali[OUTPUT.bMaxDimmLevel];
}




/**

 *****END OF FILE****/

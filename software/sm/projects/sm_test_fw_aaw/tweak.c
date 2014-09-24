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
#include "Touchslider.h"
#include "tweak.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// -> changes search for "MAN:"

// parameters APPLICATION_DUALSLIDER
// =================================
#define MIN_DIMM_LEVEL           10
#define MAX_DIMM_LEVEL           100


/* Private macro -------------------------------------------------------------*/


// MAN: arc power to percentage mapping
u8 lookup_percent_to_dali[101] =
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


u8 lookup_dali_to_percent[256] =
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

tTweak TWEAK;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/


void ProcessKeys(void)
{
  
  
  if (SLIDER.fNewKey) {

    // top key pressed
    if (SLIDER.bKeys & 0x04) {

      // invert state
      if (SLIDER.bKeyToggleStates & 0x04) {
        if(!TWEAK.bBlockIndirectKeyTimer) { // nur ausschalten wenn Slider passiv
          if(!(SLIDER.bKeyToggleStates & 1)) SLIDER.bKeyToggleStates=0; // wenn nur obere Einheit aktiv, dann alles ausschalten
          else SLIDER.bKeyToggleStates &= ~0x04;   // clear
        }
        else SLIDER.fNewKey=0;
      } else {
        SLIDER.bKeyToggleStates |=0x04+2;   // set
      }
    }

    // middle key pressed
    
    if (SLIDER.bKeys & 0x02) {
      // invert state
      if (SLIDER.bKeyToggleStates & 0x02) { 
        if(!TWEAK.bBlockDirectKeyTimer && !TWEAK.bBlockIndirectKeyTimer) // nur ausschalten wenn kein Slider aktiv
          SLIDER.bKeyToggleStates = 0;   // clear
        else SLIDER.fNewKey=0;
      } else {
        SLIDER.bKeyToggleStates=1+2+4;   // set   einschalten = immer ok alle einschalten
      }
    }

    // bottom key pressed
    if (SLIDER.bKeys & 0x01) {
      // invert state
      if (SLIDER.bKeyToggleStates & 0x01) {
        if(!TWEAK.bBlockDirectKeyTimer) // nur ausschalten wenn Slider passiv
          if(!(SLIDER.bKeyToggleStates & 4)) SLIDER.bKeyToggleStates=0; // wenn nur untere Einheit aktiv, dann alles ausschalten
          else SLIDER.bKeyToggleStates &= ~0x01;   // clear
        else SLIDER.fNewKey=0;
      } else {
        SLIDER.bKeyToggleStates|=0x01+2;   // set
      }
    }
  } // if SLIDER.fNewKey

}

u8 SetDimValues(u8 bSliderValue)
{
  u32   u32tmp;
  
  if (bSliderValue>127) bSliderValue=127; 
  if(TWEAK.bLogScale) {
    u32tmp=MIN_DIMM_LEVEL+((bSliderValue)*(MAX_DIMM_LEVEL-MIN_DIMM_LEVEL)/127);
    if(u32tmp<sizeof(lookup_percent_to_dali)) 
      return(lookup_percent_to_dali[(u8)u32tmp]);
    else 
      return(lookup_percent_to_dali[MAX_DIMM_LEVEL]);
  } else {
    return(bSliderValue*2);
  }
}


void ProcessSliders(void)
{

  if (SLIDER.fNewKey) {
    SLIDER.fNewSlider1=0;
    SLIDER.fNewSlider2=0;
  }

  // 
  if(!TWEAK.fDALIout1) SLIDER.fNewSlider1=0;
  if(!TWEAK.fDALIout2) SLIDER.fNewSlider2=0;
  
  // top slider detected
  if (SLIDER.fNewSlider2) {
    TWEAK.bDaliSetLevelIndirect=SetDimValues(SLIDER.bSliderValue2);
  }

  // bottom slider detected
  if (SLIDER.fNewSlider1) {
    TWEAK.bDaliSetLevelDirect=SetDimValues(SLIDER.bSliderValue1);
  }


}


void LightControl(void)
{
  u8 n;

  if (SLIDER.fNewKey && (SLIDER.bKeys & 0x04)) {    // top key pressed
    // inactiv
    if (SLIDER.bKeyToggleStates & 0x04) {
      TWEAK.bOnStatus |= 2;
      TWEAK.fDALIout2=1;
    } else {
      TWEAK.bOnStatus &= ~2;
      TWEAK.fDALIout2=0;
    }
  }

  if (SLIDER.fNewKey && (SLIDER.bKeys & 0x02)) {    // middle key pressed
    if(SLIDER.bKeyToggleStates & 2) { // ausschalten sobald etwas leuchtet
      TWEAK.bOnStatus=3;
      TWEAK.fDALIout1=1;
      TWEAK.fDALIout2=1;
    } else {
      TWEAK.bOnStatus=0;
      TWEAK.fDALIout1=0;
      TWEAK.fDALIout2=0;
    }
  }
  

  if (SLIDER.fNewKey && (SLIDER.bKeys & 0x01)) {    // bottom key pressed
    // inactiv
    if (SLIDER.bKeyToggleStates & 0x01) {
      TWEAK.bOnStatus |= 1;
      TWEAK.fDALIout1=1;
    } else {
      TWEAK.bOnStatus &= ~1;
      TWEAK.fDALIout1=0;
    }
  }


  // reset all activity flags
  SLIDER.fNewKey=0;
  SLIDER.fNewSlider1=0;
  SLIDER.fNewSlider2=0;

  // turn ballasts on/off
  if (TWEAK.fDALIout1) {
    SYS.fRelais1=1;
  } else {
    if(!SYS.bDali1Output)
      SYS.fRelais1=0;
  }
  if (TWEAK.fDALIout2) {
    SYS.fRelais2=1;
  } else {
    if(!SYS.bDali2Output)
      SYS.fRelais2=0;
  }

  // Rampensteuerung 
  n=TWEAK.bDaliIncDec; // 
  while(n--) {
    if(!TWEAK.fDALIout1) {
      if(SYS.bDali1Output>TWEAK.bMinDalilevel) SYS.bDali1Output--;
      else SYS.bDali1Output=0;
    } else {
      if(SYS.bDali1Output<TWEAK.bMinDalilevel) SYS.bDali1Output=TWEAK.bMinDalilevel;
      else if(SYS.bDali1Output<TWEAK.bDaliSetLevelDirect) SYS.bDali1Output++;
      else if(SYS.bDali1Output>TWEAK.bDaliSetLevelDirect) SYS.bDali1Output--;
    }
      
    if(!TWEAK.fDALIout2) {
      if(SYS.bDali2Output>TWEAK.bMinDalilevel) SYS.bDali2Output--;
      else SYS.bDali2Output=0;
    } else {
      if(SYS.bDali2Output<TWEAK.bMinDalilevel) SYS.bDali2Output=TWEAK.bMinDalilevel;
      else if(SYS.bDali2Output<TWEAK.bDaliSetLevelIndirect) SYS.bDali2Output++;
      else if(SYS.bDali2Output>TWEAK.bDaliSetLevelIndirect) SYS.bDali2Output--;
    }
  }
  
  if(TWEAK.bDaliOutputTimer) TWEAK.bDaliOutputTimer--;
  else {
    TWEAK.bDaliOutputTimer=TWEAK.bDaliOutputTime;
    DaliOutputProcess();
  }

}

//---------------------------------------------------------------------------------------
// called every 20msec
//
void TweakProcess(void)
{
  if(SLIDER.fNewSlider1) TWEAK.bBlockDirectKeyTimer=TWEAK.bBlockKeyTime;
  else if(TWEAK.bBlockDirectKeyTimer) TWEAK.bBlockDirectKeyTimer--;

  if(SLIDER.fNewSlider2) TWEAK.bBlockIndirectKeyTimer=TWEAK.bBlockKeyTime;
  else if(TWEAK.bBlockIndirectKeyTimer) TWEAK.bBlockIndirectKeyTimer--;
  
  ProcessKeys();
  ProcessSliders();
  LightControl();
}



void InitTweak(void)
{

  TWEAK.bMinDalilevel=lookup_percent_to_dali[MIN_DIMM_LEVEL];
  
  SLIDER.bSliderValue1=127;
  SLIDER.bSliderValue2=127;
  SLIDER.bKeys='0';
  TWEAK.bLogScale=1;
  TWEAK.bDaliIncDec=2;
  TWEAK.bBlockKeyTime=25; // bBlockKeyTime x 20 msec Blockade vom benachbarten Schalter nach Slider Event

  TWEAK.bDaliSetLevelDirect  =lookup_percent_to_dali[MAX_DIMM_LEVEL];
  TWEAK.bDaliSetLevelIndirect=lookup_percent_to_dali[MAX_DIMM_LEVEL];

  TWEAK.bDaliOutputTime=2-1;

}




/**

 *****END OF FILE****/

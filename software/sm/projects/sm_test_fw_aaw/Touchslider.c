  /******************************************************************************
  * @file    Touchslider.c
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    06.2012
  * @brief   RS485 Communication channel with Touchinterface
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  * @clock HSE_VALUE = 12000000 / SYSCLK_FREQ_24MHz
  ******************************************************************************
  
  Communication reception: first Bytes @..F used == Hex 0x40..0x46

  '@' = Request for Touchkey and Touchslider data is followed by 3x '0' for single reply, 3x '1' for automatic retransmission with 20msec
  
  SCKey = SingleChannelKey
  MCKey = MultiChannelKey

  'A' = SCKEY_DETECTTHRESHOLD_DEFAULT          (15)   (value from 1 to 127)
  'B' = SCKEY_ENDDETECTTHRESHOLD_DEFAULT        (7)   (value from 1 to 127)
  'C' = SCKEY_RECALIBRATIONTHRESHOLD_DEFAULT  (-10)   (value from -1 to -127)

  'D' = MCKEY_DETECTTHRESHOLD_DEFAULT          (35)   (value from 1 to 127)
  'E' = MCKEY_ENDDETECTTHRESHOLD_DEFAULT       (30)   (value from 1 to 127)
  'F' = MCKEY_RECALIBRATIONTHRESHOLD_DEFAULT  (-35)   (value from -1 to -127)

  '0' = apply to selected Parameter of all Keys
  '1' = apply to selected Parameter of Key 1
  '2' = apply to selected Parameter of Key 2
  '3' = apply to selected Parameter of Key 3 (if applicable)

  ':' = Set Parameter to new Value
  '+' = Increment Parameter
  '-' = Decrement Parameter
  ';' = Report Parameter

  'x' = New Value
  'z' = dummy Character, to fill the 4-Character-Command-String
  ( Command String must allways be 4 Characters long,(( excl. '$')))

  Example: A2:x  = Set Parameter 'A' of SingleChannelKey2 to 'x'
  Example: B0:x  = Set Parameter 'B' of all SingleChannelKeys to 'x'
  Example: C3+z  = Increment Parameter 'C' of SingleChannelKey3
  Example: D0+z  = Increment Parameter 'D' of all MultiChannelKeys

  In the Setting Commands, negative Values are treated as positives:
  --> use Command values from +1 ... +127 only
  --> '+' for a negative Parameter (Recalibration) means (-1)
  (so '+' does not mean a mathematic '+' , but a "functional" '+' )


*/ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include "MAIN.h"
#include "rs.h"
#include "Touchslider.h"
#include "tweak.h"
#include "modbus_master.h"

//#define DBG_SLIDER

tSlider SLIDER;

static u8 fDn;

void ProcessTouchmessage(void)
{
  u8 i;
  u8 SliderMessage[4];
  
  SLIDER.ulCounter++;
  SLIDER.bOKTimer=10;

  //i=RS_RX_Buffer[3];
  i=RS_RX_Buffer[4];
  if(SLIDER.bSlider1 != i) {
    SLIDER.bSlider1 = i;
    if(i != 0x80) {
      SLIDER.fNewSlider1=1;
      SLIDER.bSliderValue1 = 127-i;
      SLIDER.bG_Led=255;
      #ifdef DBG_SLIDER
      if (!DebugSendTimer) printf("\r\n");
      printf(".%3u",SLIDER.bSliderValue1);
      DebugSendTimer=50;
      #endif
    } else SLIDER.bG_Led=0;
  }
  //i=RS_RX_Buffer[4];
  i=RS_RX_Buffer[3];
  if(SLIDER.bSlider2 != i) {
    SLIDER.bSlider2 = i;
    if(i != 0x80) {
      SLIDER.fNewSlider2=1;
      SLIDER.bSliderValue2 = i;
      SLIDER.bG_Led=255;
      #ifdef DBG_SLIDER
      if (!DebugSendTimer) printf("\r\n");
      printf(":%3u",SLIDER.bSliderValue2);
      DebugSendTimer=50;
      #endif
    } else SLIDER.bG_Led=0;
  } 

  ;

  switch(RS_RX_Buffer[6]) {
    case '1': i='4'; break;
    case '4': i='1'; break;
    case '1'+2: i='4'+2; break;
    case '4'+2: i='1'+2; break;
    
    /*
    case '0':
    case '2':
    case '7':
    case '1'+4:
    */
    default:
      i=RS_RX_Buffer[6];
      break;
  }
  
  if(SLIDER.bKeys != i) {
    SLIDER.bKeys = i;
    if(SLIDER.bKeys & 7) {
      SLIDER.fNewKey=1;
      SLIDER.bR_Led=255;
    } else SLIDER.bR_Led=0;
  }
  // Bootloader Detection
  if((SLIDER.bKeys & 7) == 7) {
    if(SLIDER.bAllKeyCounter<5000/20) SLIDER.bAllKeyCounter++;
    else {
      if(SLIDER.bAllKeyCounter==5000/20) {
        SLIDER.bAllKeyCounter++;
        SYS.bBootloaderRequest=1;
      }
    }
  } else {
    SLIDER.bAllKeyCounter=0;
  }
  // key press time
  if (SLIDER.bKeys & 0x01) {
    if (SLIDER.bKeyCounter1<100) SLIDER.bKeyCounter1++;
  } else {
    SLIDER.bKeyCounter1=0;
  }
  if (SLIDER.bKeys & 0x02) {
    if (SLIDER.bKeyCounter2<100) SLIDER.bKeyCounter2++;
  } else {
    SLIDER.bKeyCounter2=0;
  }
  if (SLIDER.bKeys & 0x04) {
    if (SLIDER.bKeyCounter3<100) SLIDER.bKeyCounter3++;
  } else {
    SLIDER.bKeyCounter3=0;
  }
  SliderMessage[0]='@';
  
  SliderMessage[1]=SLIDER.bR_Led;
  SliderMessage[2]=SLIDER.bG_Led;

  if(!fDn) SLIDER.bB_Led+=5;
  else     SLIDER.bB_Led-=5;
  if(SLIDER.bB_Led==255)    fDn=1;
  else if(SLIDER.bB_Led<=5) fDn=0;
  if(SLIDER.bR_Led || SLIDER.bG_Led) SliderMessage[3]=0;
  else SliderMessage[3]=SLIDER.bB_Led;
  SetRGBLeds(SliderMessage);
  
  TweakProcess();
}

void InitTouchSlider(void)
{
  
}

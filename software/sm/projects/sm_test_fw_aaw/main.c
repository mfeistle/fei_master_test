/******************************************************************************
  * @file    main.c
  * @author  Ing. Buero W.Meier
  * @lib version V3.5.0
  * @date    07.2012
  * @brief   Main program body
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  * @clock HSE_VALUE = 12000000 / SYSCLK_FREQ_24MHz
  ******************************************************************************

  Spezial Version fuer LiteCon Testaufbau:

  Stehlampen mit LED-Leuchte direkt Licht mit Dali 1 auf fix zB 500 Lux geregelt
                           indirekt Licht mit Dali 2 auf fixen Dimmwert geschaltet

  Das LiteCon Geraet gibt im Normalfall Dali 2 auf das EVG weiter.
  Wenn Dali 2 den Wert 0 ausgibt, dann darf das LiteCon Geraet einen fixen Dimmwert an das EVG weitergeben.
  Der Ausgang vom LiteCon Geraet wird auf den Dali Slave Eingang gefuehrt, damit dieser Dimmwert der Steuerung bekannt ist.
  Wenn der Dimmwert = Null ist (Dali Slave Eingang = 0xfe 0x00), dann wird Relais2 ausgeschaltet, sonst eingeschaltet

*/ 

/*
Serielle DBG Eingabe ermoeglicht folgendes:
  't': Simuliert Tasteneingabe
  'p': Simuliert PIR-Detektor
  '0': Standard Applikation (DALI - VOLT - DMX mit Jumper waehlbar)
  '1': Vodafone Applikation (Jumper ohne Funktion)
  '2': Eiger Touchpanel Applikation (DMX Device 1 - 2 - 3 mit Jumper waehlbar)
  '3': Touch Slyder Applikatin
  '4': AAW Applikation
  'j': Abgleich auf 1000 Lux
       Idee: Jumper in Vodafone Applikation fuer 3 verschiedene Lichtniveaus auf direkt oder indirekter Beleuchtung.
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include <stdio.h>
#include <stdlib.h>
#include "MAIN.h"
#include "DALI.h"
#include "dmx_control.h"
#include "history.h"
#include "eeprom.h"
#include "RS.h"
#include "tweak.h"
#include "Touchslider.h"
#include "modbus_master.h"
#include "testprocedures.h"
#include "AAWmodule.h"
#include "Lampcontroller.h"
#include "watch.h"
#include "flashsetup.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

// _MAN_: main application select
// ==============================
//#define APPLICATION_SELECT  APPLICATION_STANDARD
//#define APPLICATION_SELECT  APPLICATION_VODAFONE
//#define APPLICATION_SELECT  APPLICATION_TOUCHPANEL
//#define APPLICATION_SELECT  APPLICATION_DUALSLIDER
#define APPLICATION_SELECT  APPLICATION_ALONEATWORK



/* Private macro -------------------------------------------------------------*/

typedef  void (*pFunction)(void);



#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
// siehe Retarget.c!!

pFunction Jump_To_Application;
uint32_t JumpAddress;

struct __FILE { int handle; };
FILE __stdout;

u8  bMeasuredTime100usec;

u8  State_Counter;
u8  msFast_Counter;
u8  Mode_Setting;
u8  Pin50HzResult;
u8  DebugSendTimer;
u8  fDebugSend;
u8  PowerLostCounter=50;
u32 PowerLostCount,DCounter,ADCAverage[5];
u16 PortBInputResult,DaliSlaveReceived,Current,Lux,MaxCurrent,IDvalue,IDmax,IDmin=4095;
u16 ADC_Buffer[AD_BUFSIZE];

tSYSTEM SYS;

u16 EE_ReadData,EE_Var1;

///* Virtual address defined by the user: 0xFFFF value is prohibited */
uint16_t VirtAddVarTab[NumbOfVar] = {1,2,3,4};

char sMODE[5][5] = {
  "NONE",
  "VOLT",
  "DALI",
  " DMX",
  "GWAY"
};

char sAPPL[5][5] = {
  " STD",
  "VODA",
  "TOCH",
  "SLD2",  // V0.19
  " AAW"
};


/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void GotoBootloader(void)
{
  
  REL1_OFF();
  REL2_OFF();
  //LEDs off
  LED_ROT_EIN();
  LED_GRUEN_EIN();
  //DAC1&2 4095
  DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
  DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
  DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
  DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
  //CTRLs off
  CTRL1_OFF();
  CTRL2_OFF();
  //DALI off
  DALI_OFF();
  DMX_OFF();
  uTimer=0;
  while(uTimer<100);

  __disable_irq();
  *((unsigned int *)0x20001FF0)=0x12345678;
  NVIC_SystemReset();
}


void SetMode(u8 Mode)
{
  switch(Mode) {
  case MODE_PASSIVE:
    DALI_OFF(); 
    DMX_OFF();
    DMX_TEN_OFF();
    // DAC1_ PA4
    DAC_SetChannel1Data  ( DAC_Align_12b_R,4095); 
    DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    // DAC2_ PA5
    DAC_SetChannel2Data  ( DAC_Align_12b_R,4095); 
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
    CTRL1_OFF();
    CTRL2_OFF();
    DaliSlave_RX_Status(DALI_DISABLE);
    break;
  case MODE_VOLT:
    DALI_OFF(); 
    DMX_OFF();
    DMX_TEN_OFF();
    // DAC1_ PA4
    DAC_SetChannel1Data  ( DAC_Align_12b_R,0); 
    DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    // DAC2_ PA5
    DAC_SetChannel2Data  ( DAC_Align_12b_R,0); 
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
    CTRL1_ON();
    CTRL2_ON();
    DaliSlave_RX_Status(DALI_DISABLE);
    break;
  case MODE_DALI:
    DALI_ON(); 
    DMX_OFF();
    DMX_TEN_OFF();
    // DAC1_ PA4
    DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
    DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    // DAC2_ PA5
    DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
    CTRL1_ON();
    CTRL2_ON();
    if(SYS.bApplication != APPLICATION_ALONEATWORK)
      DaliSlave_RX_Status(DALI_ENABLE); // bei APPLICATION_ALONEATWORK nicht nötig
    break;
  case MODE_DMX:
    DALI_OFF(); 
    DMX_ON();
    DMX_TEN_ON();
    // DAC1_ PA4
    DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_5V);  // aprox 5V
    DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    // DAC2_ PA5
    DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_5V); 
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
    CTRL1_OFF();
    CTRL2_ON();
    DaliSlave_RX_Status(DALI_DISABLE);
    break;
  case MODE_GATEWAY:
    DALI_ON(); 
    DMX_OFF();
    DMX_TEN_OFF();
    // DAC1_ PA4
    DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
    DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    // DAC2_ PA5
    DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
    CTRL1_ON();
    CTRL2_ON();
    // Disable Dali Slave reception  
    DaliSlave_RX_Status(DALI_DISABLE);
    // Disable transmission of the Dali backward message
    DaliEnableSlaveFeedback = 0;
    if(SYS.bHardware_Version==V1_0)
    {
      GPIOA->BRR = GPIO_Pin_6;  // disable SNSTEN
      GPIOA->BRR = GPIO_Pin_7; // enable SNSREN (RE)
    }else
    {
      GPIOA->BRR = GPIO_Pin_6;  // enable \REN
    }
    break;
  case MODE_DUALSLIDE:  // V0.19
    DALI_ON(); 
    DMX_OFF();
    DMX_TEN_OFF();
    // DAC1_ PA4
    DAC_SetChannel1Data  ( DAC_Align_12b_R,DAC_FULL); 
    DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    // DAC2_ PA5
    DAC_SetChannel2Data  ( DAC_Align_12b_R,DAC_FULL); 
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
    CTRL1_ON();
    CTRL2_ON();
    // Disable Dali Slave reception  
    DaliSlave_RX_Status(DALI_DISABLE);
    // Disable transmission of the Dali backward message
    DaliEnableSlaveFeedback = 0;
    InitTweak();
    if(SYS.bHardware_Version==V1_0)
    {
      GPIOA->BRR = GPIO_Pin_6;  // disable SNSTEN
      GPIOA->BRR = GPIO_Pin_7; // enable SNSREN (RE)
    }else
    {
      GPIOA->BRR = GPIO_Pin_6;  // Enable \REN
    }
    break;
  }
  DMX_REN_ON(); // Stromverbrauch ? -> ist vernachlässbar

  if(SYS.bHardware_Version==V1_0)
  {
    DALI_TXSLAVE_OFF();
  }else
  {
    V2_DALI_TXSLAVE_OFF();
  }
}

void CheckMode(void)
{
  if(!SYS.fSensorOK) {
    if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)) {
      SYS.fSensorOK=1;
      LAMP_Ch1.fSensoDimOK=1;
    }
  }
  
  SetMB_Modus(mNONE);

  if(SYS.bApplication==APPLICATION_VODAFONE) {
    Mode_Setting=MODE_DALI;
  } else if(SYS.bApplication == APPLICATION_TOUCHPANEL) {
    Mode_Setting=MODE_GATEWAY;
    if(ADC_Buffer[AD_ID_INPUT]>3600)       SYS.bDeviceID=2;
    else if((ADC_Buffer[AD_ID_INPUT]<400)) SYS.bDeviceID=0;
    else SYS.bDeviceID=1;
  } else if(SYS.bApplication == APPLICATION_DUALSLIDER) { // V0.19
    Mode_Setting=MODE_DUALSLIDE;
    SetMB_Modus(mSLIDER);
  } else if(SYS.bApplication == APPLICATION_ALONEATWORK) { // V0.19
    Mode_Setting=MODE_DALI;
    SetMB_Modus(mAAW);
  } else {
    if(ADC_Buffer[AD_ID_INPUT]>3600)       Mode_Setting=MODE_DMX;
    else if((ADC_Buffer[AD_ID_INPUT]<400)) Mode_Setting=MODE_DALI;
    else Mode_Setting=MODE_VOLT;
  }
  if(Mode_Setting != SYS.bDeviceMode) {
    SYS.bDeviceMode=Mode_Setting;
    DeviceMode=Mode_Setting; // used in Interrupt
    SetMode(SYS.bDeviceMode);
  }
}

void DaliOutputProcess(void)
{
  if(SYS.bDali1Output>254) SYS.bDali1Output=254;
  if(SYS.bDali2Output>254) SYS.bDali2Output=254;

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
    //SYS.wLED_Greentimer=30;
  }
  //if(uTimer > bMeasuredTime100usec) bMeasuredTime100usec=uTimer; // Zeitmessung in 100usec
}

u16 GetOutputValue(u16 wPromille) 
{
  u32 x;

  if(wPromille>1000) wPromille=1000;
  x=wPromille;
  
  switch(SYS.bDeviceMode) {
    case MODE_DALI:
    case MODE_DUALSLIDE:
      x*=1000;
      x/=3937; // 0..1000 -> 0..254
      //if(SYS.fOn) {  // V0.18
      //  if(x==0) x=1; // V0.17 Null für Regler nicht zulassen, wenn eingeschaltet
      //}
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
  return x;
}


int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */

  long  temp;
  u8    i;
  vs32 	vsT_Board=0;
  u32   u32Var;
  
  InitDMX();
  /* Unlock the Flash Program Erase controller */
  FLASH_Unlock();

  /* EEPROM Init */
  EE_Init();

  // 
  InitFlashSetup();

  CPU_Init();	// Initialisierung
  STOP_RX(); 	// UART3 Empfangsinterrupt ausschalten
  // _MAN_ 2012-11-22: printf(" \r\n%s %s %s   HW V1.%u",Project,Version,Date,SYS.bHardware_Version );
  printf(" \r\n%s %s %s   HW V1.%u",fw_project,fw_version,fw_build,SYS.bHardware_Version );

  START_RX(); // Nach Bedarf Debug RX Uart3 einschalten

  MB_Init();

  // _MAN_: Initialisierung der Applikation

  #ifndef APPLICATION_SELECT

    // Initialisierung der Applikation
    EE_ReadVariable( FLASH_VARIABLE1_VALID,&EE_ReadData);  // Inhalt der Adresse GUELTIG in ReadData
    if(EE_ReadData == VALID )
    {
      EE_ReadVariable( FLASH_VARIABLE1,&EE_ReadData);  // Inhalt der Adresse VARIABLE1 in ReadData
      if(EE_ReadData>=48 && EE_ReadData<=48+APPLICATION_LAST) { 
        printf("\r\nEEapp:%s",sAPPL[EE_ReadData-48]);
        SYS.bApplication=EE_ReadData-48;
      } else {
        //SYS.bApplication=APPLICATION_VODAFONE;
        SYS.bApplication=APPLICATION_ALONEATWORK;
      }
    } else {
      // ohne EEprom Selektion hier fixe Aktivierung
      //SYS.bApplication=APPLICATION_STANDARD;
      //SYS.bApplication=APPLICATION_VODAFONE;
      //SYS.bApplication=APPLICATION_TOUCHPANEL;
      //SYS.bApplication=APPLICATION_DUALSLIDER;
      SYS.bApplication=APPLICATION_ALONEATWORK;
    }

  #else
    SYS.bApplication=APPLICATION_SELECT;
  #endif
  
  
  EE_ReadVariable( FLASH_VARIABLE2_VALID,&EE_ReadData);  // Inhalt der Adresse GUELTIG in ReadData
  if(EE_ReadData == VALID )
  {
    EE_ReadVariable(FLASH_VARIABLE2,&EE_ReadData);  // Inhalt der Adresse VARIABLE1 in ReadData
    LAMP_Ch1.wLuxSpan=EE_ReadData;
  } else {
    LAMP_Ch1.wLuxSpan=16000;
  }
  printf("\r\nLUXvar:%u",LAMP_Ch1.wLuxSpan);


  if(SYS.bApplication==APPLICATION_VODAFONE) {
    LAMP_Ch1.bLuxControllerON=1;
    LAMP_Ch2.bLuxControllerON=0;
  } else if(SYS.bApplication==APPLICATION_ALONEATWORK) {
    LAMP_Ch1.bLuxControllerON=1;
    LAMP_Ch2.bLuxControllerON=1;
  } else if(SYS.bApplication==APPLICATION_TOUCHPANEL) {
    LAMP_Ch1.bLuxControllerON=0;
    LAMP_Ch2.bLuxControllerON=0;
  }
  
  InitLampController();
  

  while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));
  DMA_ClearFlag(DMA1_FLAG_TC1);
  CheckMode();

  // Startup Reset sequence for DMX 512 
  if(SYS.bDeviceMode==MODE_DMX)	
  	Send_ResetSequence();

  // Dummy Werte mit DMX512 senden
  DMX_data[1] = 0xAA;
  DMX_data[2] = 0xAA;
  DMX_data[3] = 0xAA;
  DMX_data[4] = 0xAA;
  DMX_data[5] = 0xAA;
  DMX_data[6] = 0xAA;
  DMX_data[7] = 0xAA;
  DMX_data[8] = 0xAA;

  Dali_TX_data [0] = 0xFE; // Broadcast ARCpower command
  Dali_TX_data [1] = 0;    // Powerlevel 0..254
  Dali_TX_data2[0] = 0xFE; // Broadcast ARCpower command
  Dali_TX_data2[1] = 0;    // Powerlevel 0..254

  
  GPIO_WriteBit(GPIOB,GPIO_Pin_4,Bit_SET); //DALI_TX Line HI ->> AFIO_MAPR_SWJ_CFG_JTAGDISABLE
  
  // Phasenlage für Relais Switching auf Nulldurchgang setzen
  SYS.ACZeroCrossing=REL_SWITCH_PHASE;
  
  uTimer=0;
  
  DebugSendTimer=50;
  while (1)
  {
    
    if(RxCounter) {
      if(SYS.bTestCommand) {
        if(RxCounter==2) { 
          if(RxBuffer[1]=='@') { // in Testmode gehen
            InitTestProcess();
            TestProcess();
            // nach Test Reset durchführen
            __disable_irq();
            *((unsigned int *)0x20001FF0)=0xffffffff;
            NVIC_SystemReset();
          }
        }
        if(!SETUP_Timer) {
          RxBuffer[0]=0;
          RxCounter = 0;
          SYS.bTestCommand=0;          
        }
        
      } else if(SYS.bSetupCommand) {
        if(RxCounter==5) { 
          SLIDER.bSetupParameterSelection=RxBuffer[1];
          SLIDER.bSetupParameterIndex=RxBuffer[2];
          SLIDER.bSetupControl=RxBuffer[3];
          SLIDER.bSetupValue=RxBuffer[4];
          SLIDER.fSetupRequest=1;
          SETUP_Timer=0;
        }
        if(!SETUP_Timer) {
          RxBuffer[0]=0;
          RxCounter = 0;
          SYS.bSetupCommand=0;          
        }
      } else {
        switch(RxBuffer[0]) {
          case '@': SYS.bTestCommand=1;  SETUP_Timer=2; break; //
          case '$': SYS.bSetupCommand=1; SETUP_Timer=60; break; // Setup während n x 50msec = 3sec aktiv lassen
          case 'd': fDebugSend=~fDebugSend; break;
          case 't': LAMP_Ch1.ON_OFF_Timer=ON_OFF_TIME; break;  // Simuliert externe Taste (ein / ausschalten)
          case 'p': PIR_FLAG = 1; break;      // Simuliert PIR Detector aktivität

          case 'a':
            if(SYS.fRelais1) SYS.fRelais1=0;
            else SYS.fRelais1=1;
            break;
          case 'b':
            if(SYS.fRelais2) SYS.fRelais2=0;
            else SYS.fRelais2=1;
            break;
          case 'm':
            SYS.fRelais1=1;
            SYS.fRelais2=0;
            printf("\r\nP:%u %u",SYS.ACperiod,SYS.ACOffperiod);
            break;
          case 'n':
            SYS.fRelais1=0;
            SYS.fRelais2=1;
            printf("\r\nP:%u %u",SYS.ACperiod,SYS.ACOffperiod);
            break;
          // V0.19 Set Application through EEprom 
          case '0': // Standard Application, defined by Jumper Setting
          case '1': // Vodafone
          case '2': // APPLICATION_TOUCHPANEL (STEC)
          case '3': // Touchslider
          case '4': // APPLICATION_ALONEATWORK
            EE_WriteVariable(FLASH_VARIABLE1, RxBuffer[0]);
            EE_WriteVariable(FLASH_VARIABLE1_VALID, VALID);
            EE_ReadVariable( FLASH_VARIABLE1,&EE_ReadData);
            printf("\r\nEEvar: %u",EE_ReadData);
            break;
          case 'j':
            LAMP_Ch1.wLuxSpan=SYS.wLuxADCmean;
            EE_WriteVariable(FLASH_VARIABLE2, LAMP_Ch1.wLuxSpan);
            EE_WriteVariable(FLASH_VARIABLE2_VALID, VALID);
            EE_ReadVariable( FLASH_VARIABLE2,&EE_ReadData);
            printf("\r\nLUXvar: %u",EE_ReadData);
            break;
          case 'f':
            InitFlashSetup();
            break;
          case 'F':
            WriteCompleteFlashSetup();
            break;
          case 'v':
            fDebugSend=0;
            LAMP_Ch1.ON_OFF_Timer=ON_OFF_TIME;
            SetAAW_Testmode(1);
            break;
          case 'w':
            fDebugSend=1;
            SetAAW_Testmode(3);
            break;
          case 'x': // Goto Bootloader
            GotoBootloader();
            break;
          case 'r': // Reset = New Start
            __disable_irq();
            *((unsigned int *)0x20001FF0)=0xffffffff;
            NVIC_SystemReset();
            break;
        #ifdef USE_RTC
          case 'y':
            Start_RTC();
            break;
          case 'Z':
            Time_Adjust();  // Set Time
            break;
          case 'z':
            Time_Show();  // Display actual time on the Hyperterminal
            break;
        #endif
          
        }
        if(!SYS.bSetupCommand && !SYS.bTestCommand) {
          RxBuffer[0]=0;
          RxCounter = 0;
        }
      }
    }

    if(Ticker_50ms)
    {
      Ticker_50ms = 0;
      if(!fDebugSend)
        DebugSendTimer=50;
      else
        if(DebugSendTimer) DebugSendTimer--;

      // Check Dali Slave Timeouts and Reset if failed
      Check_Dali_Receive_Timeouts();            
      
      switch(SYS.bDeviceMode) {
        case MODE_DALI:

          if(SYS.bApplication==APPLICATION_ALONEATWORK) {
            // V0.40
            if(LAMP_Both.bAAW_ModusOn) {
              SYS.bDaliExternDimvalueCh1=AAW.bDaliValueCh1;
              SYS.bDaliExternDimvalueCh2=AAW.bDaliValueCh2;
            } else {
              SYS.bDaliExternDimvalueCh1=SYS.bDaliExternDimvalueCh2=0; 
            }
            // V0.40
            SYS.bDaliExternOperation=0;
            if(LAMP_Ch1.bStatus==STATUS_OFF) {
              if(SYS.bDaliExternDimvalueCh1) {
                SYS.fRelais1=1;
                SYS.bDaliExternOperation|=1;
              } else {
                SYS.fRelais1=0;  // _MAN_ 2012.11.11
              }
            }
            // V0.40
            if(LAMP_Ch2.bStatus==STATUS_OFF) {
              if(SYS.bDaliExternDimvalueCh2) {
                SYS.fRelais2=1;
                SYS.bDaliExternOperation|=2;
              } else {
                SYS.fRelais2=0;  // _MAN_ 2012.11.11
              }
            }
          } 
          
          DaliOutputProcess();

          break;

        case MODE_GATEWAY:

          if(fDMX_RXDataValid)
          {
            fDMX_RXDataValid = 0;
            DCounter++;

            i=1+SYS.bDeviceID*2;
            SYS.bDali1Output=DMX_RX_Buff[i++];
            SYS.bDali2Output=DMX_RX_Buff[i];
            DaliOutputProcess();
          } else
          {
            // optional ErrorCounter
          }
          break;

          
        case MODE_DUALSLIDE:
          
          TweakProcess();
          break;

        default:
          break;

      } // end switch

      
      switch(State_Counter) // 0..19 = 1sec pro State
      {
        case 0:
              PirTimeController();
          
              temp= 3300*ADC_Buffer[REF_INPUT]; 
              temp /=4096;
              if(!DebugSendTimer) {
                switch(SYS.bApplication) {
                  case APPLICATION_STANDARD:
                    printf("\r\n%s REF: %dmV ",sMODE[SYS.bDeviceMode],temp);
                    break;
                  case APPLICATION_VODAFONE:
                    printf("\r\n%s T: %3u D:%04X",sAPPL[SYS.bApplication],LAMP_Ch1.wPIR_Ontimer,DaliSlaveReceived);
                    DaliSlaveReceived=0;
                    break;
                  case APPLICATION_TOUCHPANEL:
                    printf("\r\n%s D1:%3u D2:%3u",sAPPL[SYS.bApplication],SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  case APPLICATION_DUALSLIDER:
                    printf("\r\n%s D1:%3u D2:%3u",sAPPL[SYS.bApplication],SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  case APPLICATION_ALONEATWORK:
                    printf("\r\n%s T:%3u %3u D:%3u %3u",sAPPL[SYS.bApplication],LAMP_Ch1.wPIR_Ontimer,LAMP_Ch2.wPIR_Ontimer,SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  default:
                    break;
                }
              }

              if(SYS.bDeviceMode==MODE_VOLT) {
                // DAC1_ PA4
                DAC_SetChannel1Data  ( DAC_Align_12b_R,1000 ); 
                DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                // DAC2_ PA5
                DAC_SetChannel2Data  ( DAC_Align_12b_R,1000 ); 
                DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
              }
              break;
        case 1:
              // CPU Internal Sensor - Temperature (in °C) = {(V25 - VSENSE) / Avg_Slope} + 25
              // V25=1.42V(1762),Vref = 1.2V , AVG Slope = 4.35mV/°C (ca. 5)
              vsT_Board = 1762-ADC_Buffer[CPU_T_INPUT];
              vsT_Board /= 5;
              vsT_Board += 25;
              
              if(!DebugSendTimer) {
                if(SYS.bApplication==APPLICATION_STANDARD) 
                  printf(" %d°C S:%4u LUX:%4u ",vsT_Board,LAMP_Ch1.wDimPromille,LAMP_Ch1.wLux); // ADC_ConvertedValue[LUXAD_INPUT]
                else if(SYS.bApplication==APPLICATION_VODAFONE) {
                  printf(" %d°C S:%4u LUX:%4u ",vsT_Board,LAMP_Ch1.wDimPromille,LAMP_Ch1.wLux); // ADC_ConvertedValue[LUXAD_INPUT]
                } else if(SYS.bApplication==APPLICATION_TOUCHPANEL) {
                  printf(" %d°C LUX:%4u ",vsT_Board,LAMP_Ch1.wLux); // ADC_ConvertedValue[LUXAD_INPUT]
                }
                switch(SYS.bApplication) {
                  case APPLICATION_STANDARD:
                    printf(" %d°C S:%4u LUX:%4u ",vsT_Board,LAMP_Ch1.wDimPromille,LAMP_Ch1.wLux);
                    break;
                  case APPLICATION_VODAFONE:
                    printf(" %d°C S:%4u LUX:%4u ",vsT_Board,LAMP_Ch1.wDimPromille,LAMP_Ch1.wLux); // ADC_ConvertedValue[LUXAD_INPUT]
                    break;
                  case APPLICATION_TOUCHPANEL:
                  printf(" %d°C LUX:%4u ",vsT_Board,LAMP_Ch1.wLux); // ADC_ConvertedValue[LUXAD_INPUT]
                    break;
                  case APPLICATION_DUALSLIDER:
                    printf(" K:%c 1:%3u 2:%3u ",SLIDER.bKeys,SLIDER.bSliderValue1,SLIDER.bSliderValue2);
                    break;
                  case APPLICATION_ALONEATWORK:
                    printf(" %d°C LUX:%4u %4u ",vsT_Board,LAMP_Ch1.wLux,LAMP_Ch2.wLux);
                    break;
                  default:
                    break;
                }
              }
              
              break;
        case 2:
              if(!DebugSendTimer) 
                printf("Curr: %3u ",Current);
        break;
        case 3:
        break;
        case 4:         
        break;
        case 5:
                if(SYS.bDeviceMode==MODE_VOLT) {
                  // DAC1_ PA4
                  DAC_SetChannel1Data  ( DAC_Align_12b_R,10 ); 
                  DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
                  // DAC2_ PA5
                  DAC_SetChannel2Data  ( DAC_Align_12b_R,10 ); 
                  DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
                }
                //printf(" D2: %u",DaliSlave_answer);  //Backward message received through the Master
        break;
        case 6:
        break;
        case 7:
        break;
        case 8:
        break;
        case 9:
              if(SYS.bApplication!=APPLICATION_TOUCHPANEL && SYS.bApplication!=APPLICATION_DUALSLIDER)
                if(!SYS.fSensorOK) SYS.wLED_Redtimer=ON_OFF_TIME; // Rot bei fehlendem Sensor
        break;
        case 18:
              if(!DebugSendTimer) 
                printf("%3umA  I:%04X",MaxCurrent,PortBInputResult);
        break;
        case 19:
              if(SYS.bApplication==APPLICATION_DUALSLIDER) {
                if(SLIDER.bOKTimer) SYS.wLED_Greentimer=10;
                else SYS.wLED_Redtimer=50;
              } else if(SYS.bApplication==APPLICATION_ALONEATWORK) {
                if(AAW.bOKTimer) SYS.wLED_Greentimer=10;
                else SYS.wLED_Redtimer=50;
              }
              if(!DebugSendTimer) 
                printf(" R:%u%u P%u",SYS.fRelais2,SYS.fRelais1,SYS.fPowerOkFlag);
              MaxCurrent=0;
              State_Counter = 255;
        break;
        default:
        break;
        
      }
      State_Counter++;
      

    } // end if Ticker_50ms
    
    if(Ticker_1ms)
    {
      Ticker_1ms=0;
      
      if(PIR_FLAG) {
        LAMP_Ch1.PIR_FLAG=1;
        PIR_FLAG=0;
      }
      
      if(SensoSwitch1On()) {
        if(!LAMP_Ch1.ON_OFF_Timer)
          LAMP_Ch1.ON_OFF_Timer = 100;
      }
      

      MB_Process1ms();

      // Synchronisation mit DMA ADC Conversion Prozess
      while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));
      DMA_ClearFlag(DMA1_FLAG_TC1);

      for(i=0;i<AD_BUFSIZE;i++) {
        ADC_Buffer[i]=ADC_ConvertedValue[i];
      }
      // Mittelwert
      ADCAverage[CURR_INPUT]+=ADC_Buffer[CURR_INPUT];
      ADCAverage[LUXAD_INPUT]+=ADC_Buffer[LUXAD_INPUT];

      //IDvalue=ADC_ConvertedValue[AD_ID_INPUT];

      if(SYS.wLED_Redtimer>1)       { SYS.wLED_Redtimer--; if(SYS.bHardware_Version==V1_0){LED_ROT_EIN();}else{LED_GRUEN_EIN();} }
      else if(SYS.wLED_Redtimer==1) { SYS.wLED_Redtimer=0; if(SYS.bHardware_Version==V1_0){LED_ROT_AUS();}else{LED_GRUEN_AUS();}}

      if(SYS.wLED_Greentimer>1)       { SYS.wLED_Greentimer--;  if(SYS.bHardware_Version==V1_0){LED_GRUEN_EIN();}else{LED_ROT_EIN();}}
      else if(SYS.wLED_Greentimer==1) { SYS.wLED_Greentimer=0; if(SYS.bHardware_Version==V1_0){LED_GRUEN_AUS();}else{LED_ROT_AUS();} }

      if(PortBInputResult!=(GPIO_ReadInputData(GPIOB) & 0x300e))
        PortBInputResult=GPIO_ReadInputData(GPIOB) & 0x300e;

      if(Pin50HzResult != GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13)) { //(GPIO_ReadInputData(GPIOC) & 0x2000)) {
        Pin50HzResult = GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13);
        PowerLostCounter=30;
        SYS.fPowerOkFlag=1;
      } else {
        if(PowerLostCounter>1) PowerLostCounter--;
        else if(PowerLostCounter==1) {
          SYS.fPowerOkFlag=0;
          PowerLostCount++;
          PowerLostCounter=0;
        } //else SYS.fPowerOkFlag=0;
      }

      switch(msFast_Counter) // 0..9 = 10msec per state
      {
        case 0:
                // Bedingung für Ausführung wird in Funktionen durchgeführt
                SensoDimControllerCh1();
                SensoDimControllerCh2();

                // Mittelwert Initialisierung
                ADCAverage[CURR_INPUT]=ADC_Buffer[CURR_INPUT];
                ADCAverage[LUXAD_INPUT]=ADC_Buffer[LUXAD_INPUT];
        break;
        case 1: 
                switch(SYS.bDeviceMode) {
                  case MODE_DALI:
                    SYS.bDali1Output=GetOutputValue(LAMP_Ch1.wDimPromille);
                    if(LAMP_Ch1.fOn && SYS.bDali1Output==0) SYS.bDali1Output=1;
                  
                    SYS.bDali2Output=GetOutputValue(LAMP_Ch2.wDimPromille);
                    if(LAMP_Ch2.fOn && SYS.bDali2Output==0) SYS.bDali2Output=1;
                  
                    SetLocalDaliValues(SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  case MODE_DMX:
                    DMX_data[1]=GetOutputValue(LAMP_Ch1.wDimPromille);
                    DMX_data[2]=GetOutputValue(LAMP_Ch2.wDimPromille);
                    break;
                  case MODE_VOLT:
                    break;
                }
                // Bedingung für Ausführung wird in Funktionen durchgeführt
                StatusControllerCh1();
                StatusControllerCh2();

        break;
        case 2:
                CheckMode();
        break;
        case 3:
//              if(SYS.bDeviceMode==MODE_DMX) {   // Test mit DMX Demux MK2
//                DMX_data[1]=u8LightIntensity;
//                DMX_data[8]=251-u8LightIntensity;
//                Send_DMX();	 // DMX512 testen
//              }
        break;
        case 4: // Polled Version for Slider
                //if(SYS.bApplication==APPLICATION_DUALSLIDER) {
                //  TouchSliderCommunication();
                //}
        break;
        case 5: //DALI_TXSLAVE_ON();   tested
        break;
        case 6: //DALI_TXSLAVE_OFF();  tested
        break;
        case 7: // 8 Werte für Mittelwert
                Current=ADCAverage[CURR_INPUT]/93; // mA Eichung
                if(MaxCurrent<Current) MaxCurrent=Current;
                SYS.wLuxADCmean=ADCAverage[LUXAD_INPUT];
                u32Var=SYS.wLuxADCmean;
                u32Var*=1000;
                u32Var/=LAMP_Ch1.wLuxSpan;
                LAMP_Ch1.wLux=u32Var; // SYS.wLuxADCmean/16; // Lux Eichung hier einfügen
        break;
        case 8:
                 
        break;
        case 9:
                msFast_Counter=255;
        break;
        default:
        break;
    
      } // end switch ms_Fast_counter
      msFast_Counter++;
    } 
    
    
  }//while

}//main

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */


PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART3 */
  USART_SendData(USART3, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
  {}

  return ch;
}
/**
  * @brief  Configure the TIM1 Pins.
  * @param  None
  * @retval None
  */

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while (1)
  {}
}
#endif





/**

 *****END OF FILE****/

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
#include "init.h"
#include "DALI.h"
#include "dmx_control.h"
#include "history.h"
#include "eeprom.h"
#include "RS.h"
#include "OutputController.h"
#include "SliderController.h"
#include "modbus_master.h"
#include "testprocedures.h"
#include "AAWmodule.h"
#include "Lampcontroller.h"
#include "SensodimController.h"
#include "KNXinterface.h"
#include "watch.h"
#include "flashsetup.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static u8  DebugSendTimer;
static u8  fDebugSend;
static u8  Pin50HzResult;
static u8  PowerLostCounter=50;
static u8  State_Counter;

static u16 Current;
static u16 DaliSlaveReceived;
static u16 EE_ReadData;
static u16 PortBInputResult;

static u32 DCounter;
static u32 PowerLostCount;

static const char sMODE[5][5] = {
  "NONE",
  "VOLT",
  "DALI",
  " DMX",
  "GWAY"
};

static const char sAPPL[5][5] = {
  " STD",
  "VODA",
  "TOCH",
  "SLD2",  // V0.19
  " AAW"
};

/* Public variables ----------------------------------------------------------*/
struct __FILE { int handle; };
FILE __stdout;

u32 ADCAverage[5];
u16 ADC_Buffer[AD_BUFSIZE];
u16 MaxCurrent;
u8  msFast_Counter;
tSYSTEM SYS;

tRemoteCtrl RemoteCtrl;

/* Private function prototypes -----------------------------------------------*/
static void SetMode(u8 Mode);

/* Private functions ---------------------------------------------------------*/
static void CheckMode(void)
{
  u8  Mode;
  if(!SYS.fSensorOK) {
    if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)) {
      SYS.fSensorOK=1;
      LAMP_Ch1.fSensoDimOK=1;
      SensoDim.fConnected=1;
    }
  }
  
  SetMB_Modus(mNONE);

  if(SYS.bApplication==APPLICATION_VODAFONE) {
    Mode=MODE_DALI;
  } else if(SYS.bApplication == APPLICATION_TOUCHPANEL) {
    Mode=MODE_GATEWAY;
    if(ADC_Buffer[AD_ID_INPUT]>3600)       SYS.bDeviceID=2;
    else if((ADC_Buffer[AD_ID_INPUT]<400)) SYS.bDeviceID=0;
    else SYS.bDeviceID=1;
  } else if(SYS.bApplication == APPLICATION_DUALSLIDER) { // V0.19
    Mode=MODE_DUALSLIDE;
    SetMB_Modus(mSLIDER);
  } else if(SYS.bApplication == APPLICATION_ALONEATWORK) { // V0.19
    Mode=MODE_DALI;
    SetMB_Modus(mAAW);
  } else {
    if(ADC_Buffer[AD_ID_INPUT]>3600)       Mode=MODE_DMX;
    else if((ADC_Buffer[AD_ID_INPUT]<400)) Mode=MODE_VOLT;
    else Mode=MODE_DALI;
  }
  if(Mode != SYS.bDeviceMode) {
    SYS.bDeviceMode=Mode;
    SetMode(SYS.bDeviceMode);
  }
}

static void GetStoredValues(void)
{
  // Restore Setlux after Powerdown
  if(LAMP_Ch1.bLuxControllerON) {
    EE_ReadVariable( FLASH_VARIABLE1_VALID,&EE_ReadData);  // Inhalt der Adresse GUELTIG in ReadData
    if(EE_ReadData == VALID )
    {
      EE_ReadVariable(FLASH_VARIABLE1,&EE_ReadData);  // Inhalt der Adresse VARIABLE5 in ReadData
      LAMP_Ch1.wSetLux=EE_ReadData;
    } else {
      EE_WriteVariable(FLASH_VARIABLE1, LAMP_Ch1.wSetLux);
      EE_WriteVariable(FLASH_VARIABLE1_VALID, VALID);
    }
    LAMP_Ch1.wLastSetLux=LAMP_Ch1.wSetLux;
  } else { 
    EE_ReadVariable( FLASH_VARIABLE3_VALID,&EE_ReadData);  // Inhalt der Adresse GUELTIG in ReadData
    if(EE_ReadData == VALID )
    {
      EE_ReadVariable(FLASH_VARIABLE3,&EE_ReadData);  // Inhalt der Adresse VARIABLE5 in ReadData
      OUTPUT.bDaliSetLevelDirect=EE_ReadData;
    } else {
      EE_WriteVariable(FLASH_VARIABLE3, OUTPUT.bDaliSetLevelDirect);
      EE_WriteVariable(FLASH_VARIABLE3_VALID, VALID);
    }
    OUTPUT.bLastDaliSetLevelDirect=OUTPUT.bDaliSetLevelDirect;
  }
  if(LAMP_Ch2.bLuxControllerON) {
    EE_ReadVariable(FLASH_VARIABLE2_VALID,&EE_ReadData);  // Inhalt der Adresse GUELTIG in ReadData
    if(EE_ReadData == VALID )
    {
      EE_ReadVariable(FLASH_VARIABLE2,&EE_ReadData);  // Inhalt der Adresse VARIABLE5 in ReadData
      LAMP_Ch2.wSetLux=EE_ReadData;
    } else {
      EE_WriteVariable(FLASH_VARIABLE2, LAMP_Ch2.wSetLux);
      EE_WriteVariable(FLASH_VARIABLE2_VALID, VALID);
    }
    LAMP_Ch2.wLastSetLux=LAMP_Ch2.wSetLux;
  } else { 
    EE_ReadVariable( FLASH_VARIABLE4_VALID,&EE_ReadData);  // Inhalt der Adresse GUELTIG in ReadData
    if(EE_ReadData == VALID )
    {
      EE_ReadVariable(FLASH_VARIABLE4,&EE_ReadData);  // Inhalt der Adresse VARIABLE5 in ReadData
      OUTPUT.bDaliSetLevelIndirect=EE_ReadData;
    } else {
      EE_WriteVariable(FLASH_VARIABLE4, OUTPUT.bDaliSetLevelIndirect);
      EE_WriteVariable(FLASH_VARIABLE4_VALID, VALID);
    }
    OUTPUT.bLastDaliSetLevelIndirect=OUTPUT.bDaliSetLevelIndirect;
  }
}

static void SetMode(u8 Mode)
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
    if(SYS.bApplication == APPLICATION_VODAFONE)
      DaliSlave_RX_Status(DALI_ENABLE); // 0.44 nur bei alter Vodafone app aktiv
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
    //InitTweak();
    InitTouchSlider();
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

/* Public functions ----------------------------------------------------------*/
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
  vs32  vsT_Board=0;
  u32   u32Var;
  
  InitDMX();
  /* Unlock the Flash Program Erase controller */
  FLASH_Unlock();

  /* EEPROM Init */
  EE_Init();

  InitFlashSetup();

  SYS.bApplication=Flash.Setup.APPLICATION;
  SYS.wDMX_Starttimer=(Flash.Setup.DMX_RECEIVER>>8) & 0xfff; // 0x00006400 100msec Selftesttime, Receive channel 0
  SYS.bDeviceID=Flash.Setup.DMX_RECEIVER & 0xff;

  CPU_Init();   // Initialisierung
  STOP_RX();    // UART3 Empfangsinterrupt ausschalten
  // _MAN_ 2012-11-22: printf(" \r\n%s %s %s   HW V1.%u",Project,Version,Date,SYS.bHardware_Version );
  printf(" \r\n%s %s %s   HW V1.%u",fw_project,fw_version,fw_build,SYS.bHardware_Version );

  START_RX(); // Nach Bedarf Debug RX Uart3 einschalten

  MB_Init();
  
  LAMP_Ch1.wLuxSpan=16000;
  //printf("\r\nLUXvar:%u",LAMP_Ch1.wLuxSpan);

  if(SYS.bApplication==APPLICATION_ALONEATWORK) {
    LAMP_Ch1.bLuxControllerON=1;
    LAMP_Ch2.bLuxControllerON=1;
  } else if(SYS.bApplication==APPLICATION_TOUCHPANEL) {
    LAMP_Ch1.bLuxControllerON=0;
    LAMP_Ch2.bLuxControllerON=0;
  } else if(SYS.bApplication==APPLICATION_STANDARD) {
    LAMP_Ch1.bLuxControllerON=1;
    LAMP_Ch2.bLuxControllerON=0;
  }
  
  InitLampController();
  InitOutputController();
  InitSensodim();

  while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));
  DMA_ClearFlag(DMA1_FLAG_TC1);
  CheckMode();

  // Startup Reset sequence for DMX 512 
  if(SYS.bDeviceMode==MODE_DMX)
    Send_ResetSequence();

  // Dummy Values for sending DMX512
  DMX_data[1] = 0xAA;
  DMX_data[2] = 0xAA;

  Dali_TX_data [0] = 0xFE; // Broadcast ARCpower command
  Dali_TX_data [1] = 0;    // Powerlevel 0..254
  Dali_TX_data2[0] = 0xFE; // Broadcast ARCpower command
  Dali_TX_data2[1] = 0;    // Powerlevel 0..254
  
  GPIO_WriteBit(GPIOB,GPIO_Pin_4,Bit_SET); //DALI_TX Line HI ->> AFIO_MAPR_SWJ_CFG_JTAGDISABLE
  
  // Phasenlage für Relais Switching auf Nulldurchgang setzen
  SYS.ACZeroCrossing=REL_SWITCH_PHASE;
  
  uTimer=0;
  DebugSendTimer=50;
  if(Flash.Setup.PWR_FAIL_MODE==1 || Flash.Setup.PWR_FAIL_MODE==2) {
    SensoDim.bOn = 3; // Force Switch On
    
  } else if(Flash.Setup.PWR_FAIL_MODE==3) {
    GetStoredValues();
  }
  
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
        // todo Unused; remove block if it stays unused; 23-Apr-2013/ais
        // Was foreseen to 'setup the slider' (whatever this means).
//         if(RxCounter==5) { 
//           SLIDER.bSetupParameterSelection=RxBuffer[1];
//           SLIDER.bSetupParameterIndex=RxBuffer[2];
//           SLIDER.bSetupControl=RxBuffer[3];
//           SLIDER.bSetupValue=RxBuffer[4];
//           SLIDER.fSetupRequest=1;
//           SETUP_Timer=0;
//         }
        if(RxCounter>=9) { // All characters received?
          // No data validation at all
          // Expected format: $<D1><R1><D2><R2>, all numbers decimal
          // Channel 1
          i  = 100 * (RxBuffer[1] - 48);
          i +=  10 * (RxBuffer[2] - 48);
          i +=   1 * (RxBuffer[3] - 48);
          KNX.LightLevelCh1 = i;
          i =         RxBuffer[4] - 48;
          KNX.OnCh1 = i ? 1 : 0;

          // Channel 2
          i  = 100 * (RxBuffer[5] - 48);
          i +=  10 * (RxBuffer[6] - 48);
          i +=   1 * (RxBuffer[7] - 48);
          KNX.LightLevelCh2 = i;
          i =         RxBuffer[8] - 48;
          KNX.OnCh2 = i ? 1 : 0;

          KNX.Mode = 2; // Switch the TB12000 in remote control mode
          RemoteCtrl.Active = 1;
          SETUP_Timer = 0; // The complete sequence has been received
        }
        if(!SETUP_Timer) {
          RxBuffer[0]=0;
          RxCounter = 0;
          SYS.bSetupCommand=0;
        }
      } else {
        switch(RxBuffer[0]) {
          case '@': SYS.bTestCommand=1;  SETUP_Timer=2; break;  // Setup während 2 x 50ms = 100ms aktiv lassen
          case '$': SYS.bSetupCommand=1; SETUP_Timer=20; break; // Setup während 20 x 50ms = 1s aktiv lassen
          case 'd': fDebugSend=~fDebugSend; break;
          case 't': SensoDim.ON_OFF_Timer=ON_OFF_TIME; break;  // Simuliert externe Taste am Steuermodul (ein / ausschalten)
          case 'T': RemoteSensoDim.ON_OFF_Timer=ON_OFF_TIME; break;  // _MAN_: Simuliert externe Taste am AAW (ein / ausschalten)
          case 'p': PIR_FLAG = 1; break;      // Simuliert PIR Detector aktivität
          case 'P': PIR_FLAG2 = 1; break;     // Simuliert PIR Detector aktivität am AAW

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
          case 'e':
            FLASH_Unlock();
            FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
            FLASH_ErasePage(EEPROM_START_ADDRESS);
            FLASH_Lock();
            printf("\r\nErase EEprom");
            break;
          case 'f':
            InitFlashSetup();
            break;
          case 'v':
            fDebugSend=0;
            SensoDim.ON_OFF_Timer=ON_OFF_TIME;
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

          case 'o': // _MAN_: Reset PIR-On-timer 1
            SensoDim.wPIR_Ontimer=2;
            break;

          case 'O': // _MAN_: Reset PIR-On-timer 2
            RemoteSensoDim.wPIR_Ontimer=2;
            break;

          case 'q': // _MAN_: Reset PIR-Disable-timer 1
            SensoDim.wPIR_Disabletimer=2;
            break;

          case 'Q': // _MAN_: Reset PIR-Disable-timer 2
            RemoteSensoDim.wPIR_Disabletimer=2;
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
      
      // for KNX testing
      // KNX.bOKTimer=10; // %%wm
      if(KNX.bOKTimer) fDebugSend=1;
      
      if(!fDebugSend)
        DebugSendTimer=50;
      else
        if(DebugSendTimer) DebugSendTimer--;

      // Check Dali Slave Timeouts and Reset if failed
      Check_Dali_Receive_Timeouts();            
      
      switch(SYS.bDeviceMode) {
        case MODE_DALI:
          // neu: interpretieren von LAMP_Ch1/2 .fManualOff
          // AAW.bDaliValueCh1 ist der Passiv Mode Wert bestimmt durch AAW
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
              if(SYS.bDaliExternDimvalueCh1 && !LAMP_Ch1.fManualOff) { // V0.43
                SYS.fRelais1=1;
                // _MAN_: channel 2 equal channel 1 (sensor NOT present at AAW
                if(SYS.bApplication==APPLICATION_ALONEATWORK && !LAMP_Ch2.fSensoDimOK) SYS.fRelais2=1; // V0.44
                SYS.bDaliExternOperation|=1;
              } else {
                SYS.fRelais1=0;
                // _MAN_: channel 2 equal channel 1 (sensor NOT present at AAW
                if(SYS.bApplication==APPLICATION_ALONEATWORK && !LAMP_Ch2.fSensoDimOK) SYS.fRelais2=0; // V0.44
              }
            }
            // V0.40
            if(LAMP_Ch2.bStatus==STATUS_OFF) {
              if(SYS.bDaliExternDimvalueCh2 && !LAMP_Ch2.fManualOff) { // V0.43
                // _MAN_: enabled for dual channel mode (sensor present at AAW)
                if(SYS.bApplication==APPLICATION_ALONEATWORK && LAMP_Ch2.fSensoDimOK) SYS.fRelais2=1; // V0.44
                SYS.bDaliExternOperation|=2;
              } else {
                // _MAN_: enabled for dual channel mode (sensor present at AAW)
                if(SYS.bApplication==APPLICATION_ALONEATWORK && LAMP_Ch2.fSensoDimOK) SYS.fRelais2=0;  // V0.44
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
          DaliOutputProcess();
          break;

        default:
          break;

      } // end switch

      
      switch(State_Counter) // 0..19 = 1sec pro State
      {
        case 0:
              if(!SYS.bDMX_Testmode) {
                PIR_TimeController();
              }
          
              temp= 3300*ADC_Buffer[REF_INPUT]; 
              temp /=4096;
              if(!DebugSendTimer) {
                switch(SYS.bApplication) {
                  case APPLICATION_STANDARD:
                    printf("\r\n%s REF: %ldmV ",sMODE[SYS.bDeviceMode],temp);
                    break;
                  case APPLICATION_VODAFONE:
                    printf("\r\n%s T: %3u D:%04X",sAPPL[SYS.bApplication],SensoDim.wPIR_Ontimer,DaliSlaveReceived);
                    DaliSlaveReceived=0;
                    break;
                  case APPLICATION_TOUCHPANEL:
                    printf("\r\n%s D1:%3u D2:%3u",sAPPL[SYS.bApplication],SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  case APPLICATION_DUALSLIDER:
                    printf("\r\n%s D1:%3u D2:%3u",sAPPL[SYS.bApplication],SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  case APPLICATION_ALONEATWORK:
                    // _MAN_
                    //printf("\r\n%s T1:%3u T2:%3u D1:%3u D2:%3u",sAPPL[SYS.bApplication],LAMP_Ch1.wPIR_Ontimer,LAMP_Ch2.wPIR_Ontimer,SYS.bDali1Output,SYS.bDali2Output);
                    //printf("\r\n%s Toff1/2: %3u/%3u Tdis1/2: %3u/%3u D1/2: %3u/%3u",sAPPL[SYS.bApplication],LAMP_Ch1.wPIR_Ontimer,LAMP_Ch2.wPIR_Ontimer,LAMP_Ch1.wPIR_Disabletimer,LAMP_Ch2.wPIR_Disabletimer,SYS.bDali1Output,SYS.bDali2Output);
                    //printf("\r\n%s Toff1/2: %3u/%3u Tdis1/2: %3u/%3u D1/2: %3u/%3u Daaw1/2: %3u/%3u",sAPPL[SYS.bApplication],LAMP_Ch1.wPIR_Ontimer,LAMP_Ch2.wPIR_Ontimer,LAMP_Ch1.wPIR_Disabletimer,LAMP_Ch2.wPIR_Disabletimer,SYS.bDali1Output,SYS.bDali2Output,SYS.bDaliExternDimvalueCh1,SYS.bDaliExternDimvalueCh2);

                    printf("\r\n%s",sAPPL[SYS.bApplication]);
                    printf(" Toff1/2: %3u/%3u",SensoDim.wPIR_Ontimer,RemoteSensoDim.wPIR_Ontimer);
                    printf(" Tdis1/2: %3u/%3u",SensoDim.wPIR_Disabletimer,RemoteSensoDim.wPIR_Disabletimer);
                    printf(" D1/2: %3u/%3u",SYS.bDali1Output,SYS.bDali2Output);
                    printf(" Daawi1/2: %3u/%3u",SYS.bDaliExternDimvalueCh1,SYS.bDaliExternDimvalueCh2);
                    printf(" Daawo1/2: %3u/%3u",SYS.bDaliExternDimvalueCh1,SYS.bDaliExternDimvalueCh2);

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
                switch(SYS.bApplication) {
                  case APPLICATION_STANDARD:
                    if(!KNX.bOKTimer)
                      //MAN_v0.47: printf(" %d°C S:%4u LUX:%4u ",vsT_Board,LAMP_Ch1.wDimPromille,LAMP_Ch1.wLux);
                      printf(" LUX1:%4u/%4u ",LAMP_Ch1.wSetLux,LAMP_Ch1.wLux);
                    else
                      printf(" KNX:%u On:%u%u Out:%3u %3u LUX:%4u ",KNX.Mode+1,KNX.OnCh1,KNX.OnCh2,KNX.LightLevelCh1,KNX.LightLevelCh2,LAMP_Ch1.wLux);
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
              // Check if Setlux changes, after 5sec stability store value into EEprom
              if(LAMP_Ch1.bStatus==STATUS_ON) { // V0.56
                if(LAMP_Ch1.bLuxControllerON) {
                  if(LAMP_Ch1.wSetLux!=LAMP_Ch1.wLastSetLux) {
                    LAMP_Ch1.wLastSetLux=LAMP_Ch1.wSetLux;
                    LAMP_Ch1.bSetValueChangeTimer=5;
                  } else if(LAMP_Ch1.bSetValueChangeTimer>1) {
                    LAMP_Ch1.bSetValueChangeTimer--;
                  } else if (LAMP_Ch1.bSetValueChangeTimer==1) {
                    // update new lux-soll when stable for 5sec
                    LAMP_Ch1.bSetValueChangeTimer=0;

                    EE_ReadVariable( FLASH_VARIABLE1_VALID,&EE_ReadData);
                    if(EE_ReadData == VALID ) {
                      EE_ReadVariable(FLASH_VARIABLE1,&EE_ReadData);
                      if (LAMP_Ch1.wSetLux!=EE_ReadData) {
                        // update new value
                        EE_WriteVariable(FLASH_VARIABLE1, LAMP_Ch1.wSetLux);
                        EE_WriteVariable(FLASH_VARIABLE1_VALID, VALID);
                        if(!DebugSendTimer) printf("\r\nSetLux updated\r\n");
                      } 
                    } else {
                      EE_WriteVariable(FLASH_VARIABLE1, LAMP_Ch1.wSetLux);
                      EE_WriteVariable(FLASH_VARIABLE1_VALID, VALID);
                      if(!DebugSendTimer) printf("\r\nSetLux init\r\n");
                    }
                  }
                } else {
                  if(OUTPUT.bLastDaliSetLevelDirect!=OUTPUT.bDaliSetLevelDirect) {
                    OUTPUT.bLastDaliSetLevelDirect=OUTPUT.bDaliSetLevelDirect;
                    LAMP_Ch1.bSetValueChangeTimer=5;
                  } else if(LAMP_Ch1.bSetValueChangeTimer>1) {
                    LAMP_Ch1.bSetValueChangeTimer--;
                  } else if (LAMP_Ch1.bSetValueChangeTimer==1) {
                    // update new setvalue when stable for 5sec
                    LAMP_Ch1.bSetValueChangeTimer=0;

                    EE_ReadVariable( FLASH_VARIABLE3_VALID,&EE_ReadData);
                    if(EE_ReadData == VALID ) {
                      EE_ReadVariable(FLASH_VARIABLE3,&EE_ReadData);
                      if (OUTPUT.bDaliSetLevelDirect!=EE_ReadData) {
                        // update new value
                        EE_WriteVariable(FLASH_VARIABLE3, OUTPUT.bDaliSetLevelDirect);
                        EE_WriteVariable(FLASH_VARIABLE3_VALID, VALID);
                        if(!DebugSendTimer) printf("\r\nSetdali updated\r\n");
                      } 
                    } else {
                      EE_WriteVariable(FLASH_VARIABLE3, OUTPUT.bDaliSetLevelDirect);
                      EE_WriteVariable(FLASH_VARIABLE3_VALID, VALID);
                      if(!DebugSendTimer) printf("\r\nSetdali init\r\n");
                    }
                  }
                }
              } else LAMP_Ch1.bSetValueChangeTimer=0;
              break;

        case 4:         
              // Check if Setlux changes, after 5sec stability store value into EEprom
              if(LAMP_Ch2.bStatus==STATUS_ON) { // V0.56
                if(LAMP_Ch2.bLuxControllerON) {

                  if(LAMP_Ch2.wSetLux!=LAMP_Ch2.wLastSetLux) {
                    LAMP_Ch2.wLastSetLux=LAMP_Ch2.wSetLux;
                    LAMP_Ch2.bSetValueChangeTimer=5;
                  } else if(LAMP_Ch2.bSetValueChangeTimer>1) {
                    LAMP_Ch2.bSetValueChangeTimer--;
                  } else if (LAMP_Ch2.bSetValueChangeTimer==1) {
                    // update new lux-soll when stable for 5sec
                    LAMP_Ch2.bSetValueChangeTimer=0;

                    EE_ReadVariable( FLASH_VARIABLE2_VALID,&EE_ReadData);
                    if(EE_ReadData == VALID ) {
                      EE_ReadVariable(FLASH_VARIABLE2,&EE_ReadData);
                      if (LAMP_Ch2.wSetLux!=EE_ReadData) {
                        // update new value
                        EE_WriteVariable(FLASH_VARIABLE2, LAMP_Ch2.wSetLux);
                        EE_WriteVariable(FLASH_VARIABLE2_VALID, VALID);
                        if(!DebugSendTimer) printf("\r\nSetLux2 updated\r\n");
                      } 
                    } else {
                      EE_WriteVariable(FLASH_VARIABLE2, LAMP_Ch2.wSetLux);
                      EE_WriteVariable(FLASH_VARIABLE2_VALID, VALID);
                      if(!DebugSendTimer) printf("\r\nSetLux2 init\r\n");
                    }
                  }
                } else {
                  if(OUTPUT.bLastDaliSetLevelIndirect!=OUTPUT.bDaliSetLevelIndirect) {
                    OUTPUT.bLastDaliSetLevelIndirect=OUTPUT.bDaliSetLevelIndirect;
                    LAMP_Ch2.bSetValueChangeTimer=5;
                  } else if(LAMP_Ch2.bSetValueChangeTimer>1) {
                    LAMP_Ch2.bSetValueChangeTimer--;
                  } else if (LAMP_Ch2.bSetValueChangeTimer==1) {
                    // update new setvalue when stable for 5sec
                    LAMP_Ch2.bSetValueChangeTimer=0;

                    EE_ReadVariable( FLASH_VARIABLE4_VALID,&EE_ReadData);
                    if(EE_ReadData == VALID ) {
                      EE_ReadVariable(FLASH_VARIABLE4,&EE_ReadData);
                      if (OUTPUT.bDaliSetLevelIndirect!=EE_ReadData) {
                        // update new value
                        EE_WriteVariable(FLASH_VARIABLE4, OUTPUT.bDaliSetLevelIndirect);
                        EE_WriteVariable(FLASH_VARIABLE4_VALID, VALID);
                        if(!DebugSendTimer) printf("\r\nSetdali indirect updated\r\n");
                      } 
                    } else {
                      EE_WriteVariable(FLASH_VARIABLE4, OUTPUT.bDaliSetLevelIndirect);
                      EE_WriteVariable(FLASH_VARIABLE4_VALID, VALID);
                      if(!DebugSendTimer) printf("\r\nSetdali init\r\n");
                    }
                  }
                }
              } else LAMP_Ch2.bSetValueChangeTimer=0;
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
                if(!SYS.bDMX_Testmode) {
                  if(!LAMP_Ch1.fSensoDimOK) {
                    if(LAMP_Ch1.bLuxControllerON) {
                      LAMP_Ch1.bLuxControllerON=0;
                      GetStoredValues();
                    } 
                  }
                  if(SLIDER.bOKTimer) {
                    SYS.wLED_Greentimer=10;
                    if(OUTPUT.bParallelModus==1) {
                      if(Flash.Setup.SETUP_VALID == SETUP_VALID_MARK)
                        OUTPUT.bParallelModus=Flash.Setup.OUT_MATRIX;
                    }
                  } else {
                    SYS.wLED_Redtimer=50;
                    if(OUTPUT.bParallelModus==0) // switch to parallel modus
                      OUTPUT.bParallelModus=1;
                  }
                } SYS.wLED_Greentimer=5;
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
      
      // If in remote control mode send sensor data
      if (RemoteCtrl.Active && !(State_Counter%2)) { // Send data every 2nd 50ms cycle
        printf("%u %u %u %u %u %u %u\r\n",
          RemoteCtrl.Presence,
          LAMP_Ch1.wLux,
          RemoteCtrl.Key1,
          RemoteCtrl.Key2,
          RemoteCtrl.Key3,
          RemoteCtrl.Slider1,
          RemoteCtrl.Slider2);
        RemoteCtrl.Presence=0;
        RemoteCtrl.Key1 = 0;
        RemoteCtrl.Key2 = 0;
        RemoteCtrl.Key3 = 0;
        RemoteCtrl.Slider1 = 0;
        RemoteCtrl.Slider2 = 0;
      }
    } // end if Ticker_50ms
    
    if(Ticker_1ms)
    {
      Ticker_1ms=0;
      
      if(PIR_FLAG) {
        SensoDim.PIR_FLAG=1;
        PIR_FLAG=0;
        RemoteCtrl.Presence=1;
      }

      // _MAN_
      if(PIR_FLAG2) {
        RemoteSensoDim.PIR_FLAG=1;
        PIR_FLAG2=0;
      }

      if(SensoSwitch1On() || Button230VOn()) {
        if(!SensoDim.ON_OFF_Timer)
          SensoDim.ON_OFF_Timer = 100;
      }
      
      if(!DMX_ReceiveMode) {
        MB_Process1ms();
        
        if (SLIDER.bKeyCounter1) {
          RemoteCtrl.Key1 = 1;
        }
        if (SLIDER.bKeyCounter2) {
          RemoteCtrl.Key2 = 1;
        }
        if (SLIDER.bKeyCounter3) {
          RemoteCtrl.Key3 = 1;
        }
        // Note: the slider values are set in SliderController.c
      }
      else { // DMX Receive Mode
        //
        if(SYS.wDMX_Starttimer>1) {
          SYS.wDMX_Starttimer--;
          if(fDMX_RXDataValid)
          {
            fDMX_RXDataValid = 0;
            DCounter++;
            
            SYS.wDMX_Starttimer=100;
            if(!SYS.bDMX_Testmode) {
              SYS.bDMX_Testmode=1;
              SLIDER.bKeyToggleStates=0;
              LAMP_Ch1.bLuxControllerON=0;
            }
            //i=1+SYS.bDeviceID*(MAX_RX_CHANNEL_COUNT-1);
            OUTPUT.bDaliSetLevelDirect=SYS.bDali1Output=DMX_UserBuffer[1];
            OUTPUT.bDaliSetLevelIndirect=SYS.bDali2Output=DMX_UserBuffer[2];
            
            if(DMX_UserBuffer[3]) SLIDER.bKeyToggleStates |=  1;
            else                    SLIDER.bKeyToggleStates &= ~1;
            if(DMX_UserBuffer[4]) SLIDER.bKeyToggleStates |=  4;
            else                    SLIDER.bKeyToggleStates &= ~4;
            if(SLIDER.bKeyToggleStates & 5) SLIDER.bKeyToggleStates |=  2;
          } else
          {
            // optional ErrorCounter
          }
        } else if(SYS.wDMX_Starttimer==1) {
          SYS.wDMX_Starttimer=0;
          if(!SYS.bDMX_Testmode)
            USART2_Setup(115200); // set to Modbus
          else {
            __disable_irq();
            *((unsigned int *)0x20001FF0)=0xffffffff;
            NVIC_SystemReset();
          }
        }
     }

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
                if(!SYS.bDMX_Testmode) {
                  SensoDimController();
                  RemoteSensoDimController();
                }

                // Mittelwert Initialisierung
                ADCAverage[CURR_INPUT]=ADC_Buffer[CURR_INPUT];
                ADCAverage[LUXAD_INPUT]=ADC_Buffer[LUXAD_INPUT];
        break;
        case 1: 
                StatusControllerCh1();
                StatusControllerCh2();
        
                switch(SYS.bDeviceMode) {
                  case MODE_DALI:
                    SYS.bDali1Output=GetOutputValue(LAMP_Ch1.wDimPromille);
                    // _MAN_: limit (1) minimal DALI value for to ballast 
                    if(LAMP_Ch1.fOn && SYS.bDali1Output==0) SYS.bDali1Output=1;
                  
                    SYS.bDali2Output=GetOutputValue(LAMP_Ch2.wDimPromille);
                    // _MAN_: limit (1) minimal DALI value for to ballast 
                    if(LAMP_Ch2.fOn && SYS.bDali2Output==0) SYS.bDali2Output=1;
                    
                    // V0.43
                    // DALI handshaking between AAW and SM (SetLocalDaliValues() = values send to AAW)
                    if(LAMP_Ch1.fManualOff && LAMP_Ch2.fManualOff)
                      SetLocalDaliValues(SYS.bLastDali1Output,SYS.bLastDali2Output);
                    else if(LAMP_Ch1.fManualOff) 
                      SetLocalDaliValues(SYS.bLastDali1Output,SYS.bDali2Output);
                    else if(LAMP_Ch2.fManualOff) 
                      SetLocalDaliValues(SYS.bDali1Output,SYS.bLastDali2Output);
                    else
                      SetLocalDaliValues(SYS.bDali1Output,SYS.bDali2Output);
                    break;
                  case MODE_DMX:
                    DMX_data[1]=GetOutputValue(LAMP_Ch1.wDimPromille);
                    DMX_data[2]=GetOutputValue(LAMP_Ch2.wDimPromille);
                    break;
                  case MODE_VOLT:
                    break;
                  case MODE_DUALSLIDE:
                    if(LAMP_Ch1.bLuxControllerON) {
                      if (LAMP_Ch1.fOn) {
                        SYS.bDali1Output=GetOutputValue(LAMP_Ch1.wDimPromille);
                        if (SYS.bDali1Output==0) {
                          SYS.bDali1Output=1;
                        }
                      }
                    }
                    if (LAMP_Ch1.bLuxControllerON || RemoteCtrl.Active) {
                      if (LAMP_Ch2.fOn) {
                        SYS.bDali2Output=GetOutputValue(LAMP_Ch2.wDimPromille);
                        if (SYS.bDali2Output==0) {
                          SYS.bDali2Output=1;
                        }
                      }
                    }
                    if (!RemoteCtrl.Active) {
                      DaliRampControl();
                    }
                    break;
                }
                // Bedingung für Ausführung wird in Funktionen durchgeführt

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
        case 7: // 8 Werte für Mittelwert
                Current=ADCAverage[CURR_INPUT]/93; // mA Eichung
                if(MaxCurrent<Current) MaxCurrent=Current;
                SYS.wLuxADCmean=ADCAverage[LUXAD_INPUT];
                u32Var=SYS.wLuxADCmean;
                u32Var*=1000;
                u32Var/=LAMP_Ch1.wLuxSpan;
                LAMP_Ch1.wLux=u32Var; // SYS.wLuxADCmean/16; // Lux Eichung hier einfügen
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


int fputc(int ch, FILE *f)
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

/******* END OF FILE ****/

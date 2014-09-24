#ifndef __MAIN_H
#define __MAIN_H

#define APPLICATION_STANDARD    0
#define APPLICATION_VODAFONE    1
#define APPLICATION_TOUCHPANEL  2
#define APPLICATION_DUALSLIDER  3
#define APPLICATION_ALONEATWORK  4
#define APPLICATION_LAST        4

#define APPLICATION_TEST      255

#define REL_SWITCH_PHASE 36  // 0-crossing, in 100usec

typedef struct {
  u8  bHardware_Version;
  u8  bApplication;
  u8  bBootloaderRequest;
  u8  bDeviceMode;
  u8  bDeviceID;
  u8  fSensorOK;
  u8  fPowerOkFlag;
  u8  bSetupCommand;
  u8  bTestCommand;
  u16 wLuxADCmean;
  u8  bDali1Output;
  u8  bDali2Output;
  u8  bDalitimer;
  u8  bDaliExternDimvalueCh1; // V0.40
  u8  bDaliExternDimvalueCh2; // V0.40
  u8  bDaliExternOperation; // Bit0 Ch1 Bit1 Ch2 wenn aktiviert via AAW 0=InternalOperation 1 2 3 Bit weise External Operation
  u8  fRelais1;
  u8  fRelais2;
  u8  bRelaisOfftimer;
  u16  ACZeroCrossing;
  u16  ACperiod;
  u16  ACOffperiod;
  u16 wLED_Redtimer;
  u16 wLED_Greentimer;
} tSYSTEM;


extern tSYSTEM SYS;


// Functional defines
// Mode
#define MODE_PASSIVE 0
#define MODE_VOLT    1   // output = 1-10V
#define MODE_DALI    2   // output = DALI
#define MODE_DMX     3   // output = DMX
#define MODE_GATEWAY 4   // input = DMX (SensorBus) , output = DALI
#define MODE_DUALSLIDE 5

//Analog Inputs
#define CURR_INPUT    0
#define AD_ID_INPUT   1
#define LUXAD_INPUT   2
#define CPU_T_INPUT   3
#define REF_INPUT     4


#define TRUE    1
#define FALSE   0

#define V1_0    0
#define V1_1    1

// Pin Function Definitions
#define DALI_TXSLAVE_ON()         GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_RESET)
#define DALI_TXSLAVE_OFF()        GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET)

#define V2_DALI_TXSLAVE_ON()         GPIO_WriteBit(GPIOB,GPIO_Pin_2,Bit_RESET)
#define V2_DALI_TXSLAVE_OFF()        GPIO_WriteBit(GPIOB,GPIO_Pin_2,Bit_SET)

#define DALI_OFF_PIN            GPIO_Pin_6
#define DALI_OFF()                GPIO_WriteBit(GPIOB,DALI_OFF_PIN,Bit_SET)
#define DALI_ON()                 GPIO_WriteBit(GPIOB,DALI_OFF_PIN,Bit_RESET)
#define DMX_TEN_PIN             GPIO_Pin_11
#define DMX_TEN_OFF()             GPIO_WriteBit(GPIOA,DMX_TEN_PIN,Bit_RESET)
#define DMX_TEN_ON()              GPIO_WriteBit(GPIOA,DMX_TEN_PIN,Bit_SET)
#define DMX_REN_PIN             GPIO_Pin_12
#define DMX_REN_OFF()             GPIO_WriteBit(GPIOA,DMX_REN_PIN,Bit_SET)
#define DMX_REN_ON()              GPIO_WriteBit(GPIOA,DMX_REN_PIN,Bit_RESET)
#define DMX_OFF_PIN             GPIO_Pin_15
#define DMX_OFF()                 GPIO_WriteBit(GPIOA,DMX_OFF_PIN,Bit_SET)
#define DMX_ON()                  GPIO_WriteBit(GPIOA,DMX_OFF_PIN,Bit_RESET)

#define REL1_ON()                 GPIO_WriteBit(GPIOB,GPIO_Pin_14,Bit_SET)
#define REL1_OFF()                GPIO_WriteBit(GPIOB,GPIO_Pin_14,Bit_RESET)

#define REL2_ON()                 GPIO_WriteBit(GPIOB,GPIO_Pin_15,Bit_SET)
#define REL2_OFF()                GPIO_WriteBit(GPIOB,GPIO_Pin_15,Bit_RESET)

#define CTRL1_ON()                GPIO_WriteBit(GPIOB,GPIO_Pin_4,Bit_SET)
#define CTRL1_OFF()               GPIO_WriteBit(GPIOB,GPIO_Pin_4,Bit_RESET)
#define CTRL2_ON()                GPIO_WriteBit(GPIOB,GPIO_Pin_5,Bit_SET)
#define CTRL2_OFF()               GPIO_WriteBit(GPIOB,GPIO_Pin_5,Bit_RESET)

#define SensoSwitch1On()          (!GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12))
#define SensoSwitch1Off()          (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12))

#define SensoSwitch2On()          AAW.bSwitch
#define SensoSwitch2Off()         !AAW.bSwitch

// TB12000
#ifdef TB_12K
  #define PORTLED GPIOB
  #define LED_GRUEN_PIN           GPIO_Pin_9
  #define LED_ROT_PIN             GPIO_Pin_8
#else
  #define PORTLED GPIOC
  #define LED_GRUEN_PIN           GPIO_Pin_8
  #define LED_ROT_PIN             GPIO_Pin_9
#endif

#define LED_ROT_EIN()             GPIO_WriteBit(PORTLED,LED_ROT_PIN,Bit_SET)
#define LED_ROT_AUS()             GPIO_WriteBit(PORTLED,LED_ROT_PIN,Bit_RESET)

#define LED_GRUEN_EIN()           GPIO_WriteBit(PORTLED,LED_GRUEN_PIN,Bit_SET)
#define LED_GRUEN_AUS()           GPIO_WriteBit(PORTLED,LED_GRUEN_PIN,Bit_RESET)
#define RxBufferSize              20

#define AD_BUFSIZE            5   // AD Wandler 3 Ch aktiv _ 2 Ch internal
#define HI                    1
#define LO                    0

#define DAC_1V        335
#define DAC_5V        1677
#define DAC_7V        2348
#define DAC_10V       3354
#define DAC_11V       3690
#define DAC_FULL      4095




#define STOP_RX()     {USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);USART_InitStructure.USART_Mode&=~USART_Mode_Rx;USART_Init(USART3, &USART_InitStructure);}
#define START_RX()    {while(!USART_GetFlagStatus(USART3, USART_FLAG_TC));USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);USART_InitStructure.USART_Mode|=USART_Mode_Rx;USART_Init(USART3, &USART_InitStructure);}


extern u8  State_Counter;
extern u8  msFast_Counter;
extern u8  Mode_Setting;
extern u8  Pin50HzResult;
extern u8  DebugSendTimer;
//u8  u8LightIntensity;
extern u8  PowerLostCounter;
extern u32 PowerLostCount,DCounter,ADCAverage[5];
extern u16 PortBInputResult,DaliSlaveReceived,Current,Lux,MaxCurrent,IDvalue,IDmax,IDmin;
extern u16 ADC_Buffer[];



/* DMX512 variables --------------------------------------------------------*/
extern vu8 b_IntCheck;
extern vu8 b_PacketSent;
extern u8 u8_BytesCount;
extern u32 u32_EffPacketTime;
extern u32 u32_BreakGap;


extern vu8  DMX_data[513]; //DMX512 TX Buffer

extern USART_InitTypeDef USART_InitStructure;
extern vu16 RxCounter;


extern vu8 RxBuffer[RxBufferSize];
extern vu16 ADC_ConvertedValue[AD_BUFSIZE];



extern void GotoBootloader(void);
extern void SysTick_Configuration(void);
extern void CPU_Init(void);
extern void Send_DMX(void);
extern void Send_ResetSequence(void);


void DaliOutputProcess(void);



#endif







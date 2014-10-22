/******************************************************************************
  * @file    modbus.h
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    08.2012
  * @brief   modbus
  * @device Medium-density value line STM32F100C8T6	(siehe target options_C/C++/Preprocessor_Define_STM32F10X_MD_VL)
  ******************************************************************************

*/ 

/* General modbus definitions */
typedef enum
{
  fcREADHOLDINGREGISTER = 0x03,
  fcWRITESINGLEREGISTER = 0x06,
  fcWRITEMULTIPLEREGISTER = 0x10,
  fcREADWRITEMULTIPLEREGISTER = 0x17
} t_mbFunctionCode;
typedef enum
{
  ecINVALIDFUNCTIONCODE = 1,
  ecINVALIDPARAMETERS = 2,
  ecINVALIDWRITE = 8
} t_mbErrorCode;


/* Known devices */
typedef enum
{
  dSLIDER = 0x01,
  dAW = 0x02,
  dKNX = 0x03,
  dPC = 0x11,
} t_mbDevices;

/* Slave Slider definitions */
typedef enum
{
  SLIDER_A_Sliders = 0x1000,        /* msb slider 1, lsb slider 2 */
  SLIDER_A_Keys = 0x1001,           /* lsb keys */ 
  SLIDER_A_FunctionR = 0x1100,      /* msb function selector, lsb R */
  SLIDER_A_GB = 0x1101,             /* msb G, lsb B */
  SLIDER_A_SWUPDATEORTEST = 0x2000  /* Passiv oder Goto Bootloader */
} t_mbSLIDERAddress;

/* Slave KNX definitions */
typedef enum
{
  KNX_A_Mode = 0x1000,        /* msb not defined, lsb Operation Mode / On Off */
  KNX_A_Output = 0x1001,      /* msb 0..100% Ch1, lsb 0..100% Ch2             */ 
  KNX_A_LuxSensor1 = 0x1100,  /* 0..2000 Lux of Sensor 1             */
  KNX_A_LuxSensor2 = 0x1101,  /* 0..2000 Lux of Sensor 2 (future Option) */
  KNX_A_SWUPDATEORTEST = 0x2000  /* Passiv or Goto Bootloader (future Option) */
} t_mbKNXAddress;


/* Slave AW definitions */
typedef enum
{
  AW_A_Luxvalue  = 0x1000,        /* Luxvalue -1 = no Sensor Attached */
  AW_A_SwitchPIR = 0x1001,        /* Switch PIR Info, activ passiv Info */ 
  AW_A_ExternalDalivalue = 0x1002,      /* highest Dali Value of Lamps next to me  */
  AW_A_TestResult1 = 0x1010,
  AW_A_TestResult2 = 0x1011,
  AW_A_TestResult3 = 0x1012,
  AW_A_LocalDalivalues = 0x1100,         /* msb Dali Ch1, lsb Dali Ch2 */
  AW_A_SWUPDATEORTEST       = 0x2000      /* Passiv oder Goto Bootloader */
} t_mbAWAddress;


/* Slave PC definitions */
typedef enum
{
  PC_A_COMMAND = 0x1000,
  PC_GETFLASHVARL = 0x1001,
  PC_GETFLASHVARH = 0x1002,
  PC_RETURNFLASHVAR = 0x1100
} t_mbPCAddress;

typedef enum
{
  PC_C_RESET,
} t_mbPCCommand;


/**

 *****END OF FILE****/

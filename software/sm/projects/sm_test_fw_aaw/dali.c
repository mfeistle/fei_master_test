/**
  ******************************************************************************
  * @file    TB12000/Dali.c 
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    02.2012
  * @brief   Dali TX_RX
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include"stm32f10x.h"
#include "MAIN.h"
#include "DALI.h"

#define TE             834/2                       // half bit time = 417 usec
#define MIN_TE         TE - 60                     // minimum half bit time
#define MAX_TE         TE + 60                     // maximum half bit time
#define MIN_2TE        2*TE - 60                   // minimum full bit time
#define MAX_2TE        2*TE + 60                   // maximum full bit time

#define INITIALISE     0xA500                      // command for starting initialization mode
#define RANDOMISE      0xA700                      // command for generating a random address

#define DALI_RESET_VALUE                     0
#define CR1_CEN_Reset               ((uint16_t)0x03FE)


//DALI Master (using CTRL1+2 ie PortB 4 and 5 to send Commmands
vu8 Dali_RX_Position;
vu8 Dali_Trigger_pending;
vu8 Dali_RX_Timeout;
vu8 DaliSlave_answer;
vu8 Dali_Polarity;


// Dual Dali Master Output
vu8 Dali_TX_position;                      // sending bit position

vu8 Dali_TX_bit_value,Dali_TX_bit_value2;  // used for dali send bit
vu16 DALI_Master_frame;                   // holds the received slave backward frame
vu8 f_repeat;                              // flag command shall be repeated
vu8 f_busy;                                // flag DALI transfer busy
vu16 Dali_forward  = 0;                    // DALI forward frame
vu16 Dali_forward2 = 0;                    // DALI forward frame Channel 2
vu8 f_dalitx = 0;
vu8 f_dalirx = 0;
u8  Dali_TX_data [2];    // Data[0] und Command[1]
u8  Dali_TX_data2[2];    // Data[0] und Command[1]



// Dali Slave
vu8 DaliSlave_Trigger_pending;
vu8 DaliSlave_RX_Timeout;
vu8 DaliSlave_Polarity;
 
vu8 DaliSlave_TX_position;                      // sending bit position
vu8 DaliSlave_RX_Position = 0;                  // reception bit position
vu8 DaliSlave_TX_bit_value;  // used for dali send bit
vu16 DALI_Slave_frame;                                // holds the received master forward frame
vu16 DaliSlave_backward  = 0;                    // DALI backward frame
vu8 Dali_Slave_f_dalitx = 0;                      // transmission active
vu8 Dali_Slave_f_dalirx = 0;                    // reception active
vu8 DaliSlaveInput = 0;
vu8 DaliReceivedForward = 0;
vu8 DaliSlaveRXDataValid = 0;
vu8 DaliMasterRXDataValid = 0;
vu16 DaliReceivedForwardFull = 0;
vu8 DaliEnableSlaveFeedback = 0;
vu8 Dali_IN_Timeout = 0;
vu8 Dali_RXN_Timeout = 0;
vu8 Dali_RXP_Timeout = 0;

void DaliInterruptHandler(void)
{ 
  EXTI_InitTypeDef EXTI_InitStructure;
     
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
  {
      if(f_dalitx)
      {        
        if(Dali_TX_bit_value)
        {
            GPIO_WriteBit(GPIOB,GPIO_Pin_4,Bit_SET);  // DALI output pin PB4 high (can be used for other TX Pins too)
        } else
        {
            GPIO_WriteBit(GPIOB,GPIO_Pin_4,Bit_RESET); // DALI output pin PB4 low
        }

        if(Dali_TX_bit_value2)
        {
            GPIO_WriteBit(GPIOB,GPIO_Pin_5,Bit_SET);  // DALI output pin PB5 high
        } else
        {
            GPIO_WriteBit(GPIOB,GPIO_Pin_5,Bit_RESET); // DALI output pin PB5 low
        }

        if (Dali_TX_position == 0)                     // second half of start bit = 1
        {
          Dali_TX_bit_value  = 1;
          Dali_TX_bit_value2 = 1;
        } else if (Dali_TX_position < 33)              // 1TE - 32TE, so address + command
        {
            Dali_TX_bit_value = (Dali_forward  >> ( (32 - Dali_TX_position)/2) ) & 1;
            Dali_TX_bit_value2= (Dali_forward2 >> ( (32 - Dali_TX_position)/2) ) & 1;
            if (Dali_TX_position & 1) {
              Dali_TX_bit_value = !Dali_TX_bit_value;  // invert if first half of data bit
              Dali_TX_bit_value2= !Dali_TX_bit_value2; // invert if first half of data bit
            }
        } else if (Dali_TX_position == 33)              // 33TE start of stop bit (4TE)
        {                                               // and start of minimum set time (7TE)
          Dali_TX_bit_value  = 1;
          Dali_TX_bit_value2 = 1;
        } else if (Dali_TX_position == 37)              // 34TE, end of stopbits and settling time
        {
        // Timeout for Slave backward frame = 9.15ms(22TE)-> Enable RX
        // First PRESET for EXTI Interrupt - Starting synchronised RX on PB1 DALIRX2
        	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
        	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
          EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger 
        	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        	EXTI_Init(&EXTI_InitStructure);
          Dali_RX_Position = 0;
          Dali_RX_Timeout = 27;

        }

        if(Dali_RX_Timeout > 1)
        {
          Dali_RX_Timeout--;
        }else if (Dali_RX_Timeout == 1)
        {
            Dali_RX_Timeout = 0;
            // TIM3 disable counter 
            TIM3->CR1 &= CR1_CEN_Reset;
            TIM3->CNT = DALI_RESET_VALUE ;
            f_dalirx = 0;
            f_dalitx = 0;
            f_busy = 0;
            // DALI Slave Error -> Answer missing
        }

        if(Dali_TX_position < 45)
        {
          Dali_TX_position++;
        }
      }

      if(f_dalirx)
      {

          if(Dali_RX_Position==1)
          {
            TIM3->CR1 &= CR1_CEN_Reset;  // Dali RX Trigger through TIM3
            TIM3->CNT = DALI_RESET_VALUE ;
            TIM3->ARR = 9999; // switch to 416us delay
            TIM_Cmd(TIM3, ENABLE);  // Dali RX Trigger through TIM3
          }

          if(Dali_RX_Position < 17)
          {
            Dali_Trigger_pending = 1; // EXT Trigger freigeben
            EXTI_InitStructure.EXTI_Line = EXTI_Line1;
            EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
            if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1))// Polarity is HI
            {
              Dali_Polarity = HI;
            	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set trigger 
            }else // or LOW
            {
              Dali_Polarity = LO;
              EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
            
            }
            EXTI_InitStructure.EXTI_LineCmd = ENABLE;
            EXTI_Init(&EXTI_InitStructure);
          }else if(Dali_RX_Position == 17)
          {
            // Transmitting done
            f_dalirx = 0;
            f_dalitx = 0;
            // TIM3 disable counter 
            TIM3->CR1 &= CR1_CEN_Reset;
            TIM3->CNT = DALI_RESET_VALUE ;
            TIM3->ARR = 9999; // 416us
            // exti DISABLE
          	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
          	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
          	EXTI_Init(&EXTI_InitStructure);
            Dali_RX_Position = 0; 
            DALI_Master_frame  = 0;                            // reset receive frame
            f_busy = 0;                            // end of transmission
            Dali_TX_bit_value = 0;
            Dali_TX_bit_value2 = 0;
            Dali_RX_Timeout = 0;
//            if (f_repeat)                          // repeat forward frame ?
//                f_dalitx = 1;                      // yes, set flag to signal application
          }
          Dali_RX_Position++;

      }

      TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
  }
  else
  {
  }
}




void DaliRXInterruptHandler(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    switch(Dali_RX_Position)
    {
      case 0:
              Dali_RX_Position = 1;
              if(Dali_Trigger_pending)
              {
                Dali_Trigger_pending = 0;
              }else
              {
              // DALI Error
              }
              // Next trigger 
            	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
            	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
              EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger 
            	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
            	EXTI_Init(&EXTI_InitStructure);



      break;
      case 1:
              TIM3->CR1 &= CR1_CEN_Reset;  // Dali RX Trigger through TIM3
              TIM3->CNT = DALI_RESET_VALUE ;
              TIM3->ARR = 12479; // switch to 520us delay
              TIM_Cmd(TIM3, ENABLE);  // Dali RX Trigger through TIM3
              f_dalirx = 1;
              //START BIT
              if(Dali_Trigger_pending)
              {
                Dali_Trigger_pending = 0;
              }else
              {
              // DALI Error
              }

      break;
      case 2:
      case 4:
      case 6:
      case 8:
      case 10:
      case 12:
      case 14:
      case 16:
              if(Dali_Trigger_pending)
              {
                Dali_Trigger_pending = 0;
              }else
              {
              }
              if(Dali_Polarity == HI) // Fallende Flanke
              {
                DALI_Master_frame = (DALI_Master_frame << 1);
              }else   // Steigende Flanke
              {
                DALI_Master_frame = (DALI_Master_frame << 1);
                DALI_Master_frame |= 1;                              // shift bit
              }
              if(Dali_RX_Position == 16)
              {
                DaliSlave_answer = (u8)DALI_Master_frame;
                DaliMasterRXDataValid = 1;
              }
      break;
      default:
      break;
    }
}


void DaliSlaveInterruptHandler(void)
{ 
  EXTI_InitTypeDef EXTI_InitStructure;
     
  if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
  {

      // Dali Slave Transmit
      if(Dali_Slave_f_dalitx)
      {
        if(DaliEnableSlaveFeedback)
        {
          if(DaliSlave_TX_bit_value)
          {
              if(SYS.bHardware_Version==V1_0)
              {
                GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);  // DALI output pin PB7 high
              }else
              {
                GPIO_WriteBit(GPIOB,GPIO_Pin_2,Bit_SET);  // DALI output pin PB2 high
              }
          } else 
          {
              if(SYS.bHardware_Version==V1_0)
              {
                GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_RESET); // DALI output pin PB7 low
              }else
              {
                GPIO_WriteBit(GPIOB,GPIO_Pin_2,Bit_RESET); // DALI output pin PB2 low
              }

          }
        }

        if (DaliSlave_TX_position == 0)                     // second half of start bit = 1
        {
          DaliSlave_TX_bit_value  = 1;
        } else if (DaliSlave_TX_position < 17)              // 1TE - 16TE, so address + command
        {
            DaliSlave_TX_bit_value = (DaliSlave_backward  >> ( (16 - DaliSlave_TX_position)/2) ) & 1;
            if (DaliSlave_TX_position & 1) {
              DaliSlave_TX_bit_value = !DaliSlave_TX_bit_value;  // invert if first half of data bit
            }
        } else if (DaliSlave_TX_position == 17)              // 17TE start of stop bit (4TE)
        {                                               
          DaliSlave_TX_bit_value  = 1;
        } else if (DaliSlave_TX_position == 21)              // 21TE, end of stopbits 
        {
              Dali_Slave_f_dalitx = 0;
              Dali_Slave_f_dalirx = 0;
              DaliSlave_RX_Position = 0;
              DaliSlave_TX_bit_value = 0;                // first half of start bit = 0
              TIM7->CR1 &= CR1_CEN_Reset;  // Dali RX Trigger through TIM7
              TIM7->ARR = 14999; // switch to 624us delay
              TIM7->CNT = DALI_RESET_VALUE ;
              if(SYS.bHardware_Version==V1_0)
              {

  // EXTI2 Start position
                EXTI_InitStructure.EXTI_Line = EXTI_Line2;
                EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger
              	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
              	EXTI_Init(&EXTI_InitStructure);
                
  // EXTI3 Start position
                EXTI_InitStructure.EXTI_Line = EXTI_Line3;
                EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger
              	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
              	EXTI_Init(&EXTI_InitStructure);
              }else
              {
  // EXTI7 Start position
                EXTI_InitStructure.EXTI_Line = EXTI_Line7;
                EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger
              	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
              	EXTI_Init(&EXTI_InitStructure);
              
              }
        }


        if(DaliSlave_TX_position < 37)
        {
          DaliSlave_TX_position++;
        }
      }



// Dali Slave Receive
      if(Dali_Slave_f_dalirx)
      {
          if(DaliSlave_TX_position)
          {
            DaliSlave_TX_position = 0;
          }
          if(Dali_Slave_f_dalitx)
          {
            Dali_Slave_f_dalitx = 0;
            DaliSlave_TX_bit_value = 0;
          }

          if(DaliSlave_RX_Position==1)
          {
            TIM7->CR1 &= CR1_CEN_Reset;  // Dali RX Trigger through TIM7
            TIM7->CNT = DALI_RESET_VALUE ;
            TIM7->ARR = 9999; // switch to 416us delay
            TIM_Cmd(TIM7, ENABLE);  // Dali RX Trigger through TIM7
          }

          if(DaliSlave_RX_Position < 42)
          {
//            if( DaliSlave_Trigger_pending &&( (DaliSlave_RX_Position % 2)==0 ) )
//            {
//              // DALI Slave Reset - pending bit is not cleared by EXTI 
//              DaliResetSlave();
//              return;
//            }
            switch(DaliSlaveInput)
            {
              case IN1DALIP:
                    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
                    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                    if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_2))// Polarity is HI
                    {
                      DaliSlave_Polarity = HI;
                    	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set trigger
                      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                      EXTI_Init(&EXTI_InitStructure);
 
                    }else // or LOW
                    {
                      DaliSlave_Polarity = LO;
                      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
                      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                      EXTI_Init(&EXTI_InitStructure);
                    }
              break;
              case IN1DALIN:
                    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
                    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                    if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_3))// Polarity is HI
                    {
                      DaliSlave_Polarity = HI;
                    	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set trigger
                      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                      EXTI_Init(&EXTI_InitStructure);
 
                    }else // or LOW
                    {
                      DaliSlave_Polarity = LO;
                      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
                      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                      EXTI_Init(&EXTI_InitStructure);
                    }

              break;
              case IN1DALI: // New Hardware V1_1
                    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
                    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                    if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7))// Polarity is HI
                    {
                      DaliSlave_Polarity = HI;
                    	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set trigger
                      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                      EXTI_Init(&EXTI_InitStructure);
 
                    }else // or LOW
                    {
                      DaliSlave_Polarity = LO;
                      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
                      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
                      EXTI_Init(&EXTI_InitStructure);
                    }

              break;

              default:
              break;
            }

          }else if(DaliSlave_RX_Position == 42)
          {
            if(SYS.bHardware_Version==V1_0)
            {

              // EXTI DISABLE
            	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
            	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
            	EXTI_Init(&EXTI_InitStructure);
  
            	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
            	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
            	EXTI_Init(&EXTI_InitStructure);
            }else
            {
            	EXTI_InitStructure.EXTI_Line = EXTI_Line7;
            	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
            	EXTI_Init(&EXTI_InitStructure);
            }

            // reception done
            Dali_Slave_f_dalirx = 0;
            Dali_Slave_f_dalitx = 1;
            DaliSlave_RX_Position = 255; 
            DALI_Slave_frame  = 0;                            // reset receive frame
            DaliSlave_TX_bit_value = 0;

          }
          DaliSlave_RX_Position++;

          if( (DaliSlave_RX_Position % 2)==0 )  // 2,4,6...32
          {
              DaliSlave_Trigger_pending = 1; // next EXT Trigger Enabled
          
          }
      }

      TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
  }
}



void Dali_RXP_SlaveInterruptHandler(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    if(SYS.bHardware_Version==V1_0)
    {


      switch(DaliSlave_RX_Position)
      {
        case 0:
                DaliSlave_RX_Position = 1;
                // Next trigger 
              	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
              	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger 
              	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
              	EXTI_Init(&EXTI_InitStructure);
                Dali_RXP_Timeout = 5; // 200ms timeout
  
        break;
        case 1:
                Dali_RXP_Timeout = 0;
                TIM_Cmd(TIM7, ENABLE);  // Dali RX Trigger through TIM7
                Dali_Slave_f_dalirx = 1;
                //START BIT
                DaliSlaveInput = 1; // PB2 active
        break;
        case 2:
        case 4:
        case 6:
        case 8:
        case 10:
        case 12:
        case 14:
        case 16:
        case 18:
        case 20:
        case 22:
        case 24:
        case 26:
        case 28:
        case 30:
        case 32:
                if(DaliSlave_Trigger_pending)
                {
                  DaliSlave_Trigger_pending = 0;
                }else
                {
                // DALI Error, Data Corruption
                  ResetDaliReceivers();
                }
                if(DaliSlave_Polarity == LO) // Rising edge ( inverted !)
                {
                  DALI_Slave_frame = (DALI_Slave_frame << 1);
  
                }else   // Falling edge
                {
                  DALI_Slave_frame = (DALI_Slave_frame << 1);
                  DALI_Slave_frame|=1;
   
                }
                if(DaliSlave_RX_Position == 32)
                {
                  DaliReceivedForwardFull = DALI_Slave_frame;
                  DaliReceivedForward = (u8)DALI_Slave_frame;
                  DaliSlaveRXDataValid = 1;
                }
        break;
        default:
        break;
      }

      

    }

}

void Dali_RXN_SlaveInterruptHandler(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    if(SYS.bHardware_Version==V1_0)
    {

          

      switch(DaliSlave_RX_Position)
      {
        case 0:
                DaliSlave_RX_Position = 1;
                // Next trigger 
              	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
              	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger 
              	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
              	EXTI_Init(&EXTI_InitStructure);
                Dali_RXN_Timeout = 5; // 200ms timeout 
  
        break;
        case 1:
                Dali_RXN_Timeout = 0;
                TIM_Cmd(TIM7, ENABLE);  // Dali RX Trigger through TIM7
                Dali_Slave_f_dalirx = 1;
                //START BIT
                DaliSlaveInput = 2; // PB3 active
        break;
        case 2:
        case 4:
        case 6:
        case 8:
        case 10:
        case 12:
        case 14:
        case 16:
        case 18:
        case 20:
        case 22:
        case 24:
        case 26:
        case 28:
        case 30:
        case 32:
                if(DaliSlave_Trigger_pending)
                {
                  DaliSlave_Trigger_pending = 0;
                }else
                {
                // DALI Error,Data Corruption, Reset DALI Slave
                  ResetDaliReceivers();
                  return;
                }
                if(DaliSlave_Polarity == LO) // Rising edge ( inverted !!)
                {
                  DALI_Slave_frame = (DALI_Slave_frame << 1);
  
                }else   // Falling edge
                {
                  DALI_Slave_frame = (DALI_Slave_frame << 1);
                  DALI_Slave_frame|=1;
   
                }
                if(DaliSlave_RX_Position == 32)
                {
                  
                  DaliReceivedForwardFull = DALI_Slave_frame;
                  DaliReceivedForward = (u8)DALI_Slave_frame;
                  DaliSlaveRXDataValid = 1;
                }
                
        break;
        default:
        break;
      }
    }
    

}


void Dali_IN_SlaveInterruptHandler(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    if(SYS.bHardware_Version==V1_0)
    {
    }else
    {
      switch(DaliSlave_RX_Position)
      {
        case 0:
                DaliSlave_RX_Position = 1;
                // Next trigger 
              	EXTI_InitStructure.EXTI_Line = EXTI_Line7;
              	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
                EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger 
              	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
              	EXTI_Init(&EXTI_InitStructure);
                Dali_IN_Timeout = 5;  // 200ms Timeout for reset
  
        break;
        case 1:
                Dali_IN_Timeout = 0;
                TIM_Cmd(TIM7, ENABLE);  // Dali RX Trigger through TIM7
                Dali_Slave_f_dalirx = 1;
                //START BIT
                DaliSlaveInput = 3; // PB7 active
        break;
        case 2:
        case 4:
        case 6:
        case 8:
        case 10:
        case 12:
        case 14:
        case 16:
        case 18:
        case 20:
        case 22:
        case 24:
        case 26:
        case 28:
        case 30:
        case 32:
                      
                if(DaliSlave_Trigger_pending)
                {
                  DaliSlave_Trigger_pending = 0;
                }else
                {
                // DALI Error,Data Corruption, Reset Slave
                  ResetDaliReceivers();
                }
                if(DaliSlave_Polarity == LO) // Rising edge ( inverted !!)
                {
                  DALI_Slave_frame = (DALI_Slave_frame << 1);
  
                }else   // Falling edge
                {
                  DALI_Slave_frame = (DALI_Slave_frame << 1);
                  DALI_Slave_frame|=1;
   
                }
                if(DaliSlave_RX_Position == 32)
                {
                  DaliReceivedForwardFull = DALI_Slave_frame;
                  DaliReceivedForward = (u8)DALI_Slave_frame;
                  DaliSlaveRXDataValid = 1;
                }
                
        break;
        default:
        break;
      }
    }

}

void SetDaliTX_Data(u8 *tx_data,u8 *tx_data2)                  // DALI Daten in Forward frame setzen
{
    Dali_forward = (tx_data [0] << 8) | tx_data [1];
    Dali_forward2= (tx_data2[0] << 8) | tx_data2[1];
    f_dalitx = 1;                                  // set DALI send ready flag
}



void DALI_Send(void)
{
  if (f_repeat)             // repeat last command ?
  {
      f_repeat = 0;
  }
  
  while (f_busy) ;          // Wait until dali port is idle
  
  DALI_Master_frame    = 0;
  Dali_TX_bit_value = Dali_TX_bit_value2 = 0;  // first half of start bit = 0
  Dali_TX_position = 0;
  Dali_RX_Timeout = 0;
  f_busy   = 1;             // Activate the timer module to transfer
  // Reset and Start Timer 3
  TIM3->CNT = DALI_RESET_VALUE ; // Define im DMX Teil !!
  TIM_Cmd(TIM3, ENABLE);  // 416us Dali TX Trigger mit TIM3
}

void DaliSlave_RX_Status(vu8 status)
{
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  if(SYS.bHardware_Version==V1_0)
  {
  	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger
    if(status)
    { 
    	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    }else
    {
      	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    }
  	EXTI_Init(&EXTI_InitStructure);
  
  
  	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger 
    if(status)
    { 
    	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    }else
    {
      	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    }
  	EXTI_Init(&EXTI_InitStructure);
  
  }else
  {
  	EXTI_InitStructure.EXTI_Line = EXTI_Line7;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Set falling trigger 
    if(status)
    { 
    	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    }else
    {
      	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    }
  	EXTI_Init(&EXTI_InitStructure);
  }

  NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
  if(status)
  {
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  }else
  {
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
  }
  NVIC_Init(&NVIC_InitStructure);

}

void ResetDaliReceivers(void)
{
  EXTI_InitTypeDef EXTI_InitStructure;


  Dali_Slave_f_dalitx = 0;
  Dali_Slave_f_dalirx = 0;
  DaliSlave_RX_Position = 0;
  DaliSlave_TX_position = 0;
  DaliSlave_TX_bit_value = 0;   // first half of start bit = 0
  TIM7->CR1 &= CR1_CEN_Reset;  // Dali RX Trigger through TIM7
  TIM7->ARR = 14999; // switch to 624us delay
  TIM7->CNT = DALI_RESET_VALUE ;

  if(SYS.bHardware_Version==V1_0)
  {
  // EXTI2 Start position
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);
    
  // EXTI3 Start position
    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);
  }else
  {
  // EXTI7 Start position
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // Set rising trigger
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);
  
  }
  DaliReceivedForwardFull = 0;
  DaliReceivedForward = 0; 
  DALI_Slave_frame = 0;
  DaliSlaveRXDataValid = 0;


}

void Check_Dali_Receive_Timeouts(void)
{
// Dali Slave RX Timeouts
  if(Dali_IN_Timeout)  Dali_IN_Timeout--;
  if(Dali_RXN_Timeout) Dali_RXN_Timeout--;
  if(Dali_RXP_Timeout) Dali_RXP_Timeout--;

  if( (Dali_IN_Timeout==1)||(Dali_RXN_Timeout==1) || (Dali_RXP_Timeout==1) )
  {
    Dali_IN_Timeout = 0; 
    Dali_RXN_Timeout = 0;
    Dali_RXP_Timeout = 0;
    ResetDaliReceivers();            
  } 




}

/*END OF FILE*/

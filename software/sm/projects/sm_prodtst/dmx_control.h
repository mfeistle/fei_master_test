 /******************************************************************************
  * @file    dmx_control.h
  * @author  Ing. Büro W.Meier
  ******************************************************************************
  */ 

#ifndef __DMX_CONTROL_H
#define __DMX_CONTROL_H


#define MAX_RX_CHANNEL_COUNT            6
#define MAX_CHANNEL_COUNT               513
#define NO_OF_SEND_BYTES                8 // Anzahl gesendete Bytes
#define BYTE_COUNT_ONE                  1
#define RESET_VALUE                     0
#define BYTE_TIME                       44
#define MARK_TIME_SLOT                  56
#define BREAK_2_BREAK_TIME_RESOLUTION   25
#define FOUR_BIT_RIGHT_SHIFT            4
#define BREAK_TIME                      100
#define MAB_TIME                        20
#define BREAK_2_BREAK_TIME              1204
#define BAUD_250_KBPS                   250000
#define TIMER2_CLOCK_PRESCALER          23 // Durch 24 teilen
#define TIMER4_PERIOD                   2399  //10kHz_0.1ms
#define TIMER4_PULSE                    1104//
#define ONE                             1


#define FRAMING_ERROR_FLAG      0x00000002
#define RXNE_FLAG                0x00000020

extern vu8 b_IntCheck;
extern vu8 b_PacketSent;

extern vu8  DMX_data[MAX_CHANNEL_COUNT];
extern u8   DMX_RX_Buff[MAX_CHANNEL_COUNT];
extern u8   DMX_UserBuffer[MAX_RX_CHANNEL_COUNT];
extern vu32 DMX_StatusRead;
extern vu16 DMXChannelCount;
extern vu8 fDMX_RX_Start_OK;
extern vu8 fDMX_DataCorruption;
extern vu8 fDMX_RXDataValid;

void Send_ResetSequence(void);
void DMX_ResetSequenceInterrupt(void);
void DMX_SendInterruptHandler(void);
void DMX_RX_InterruptHandler(void);
void Send_DMX(void);
void InitDMX(void);

#endif

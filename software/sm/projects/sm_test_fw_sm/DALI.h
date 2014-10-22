#ifndef __DALI_H
#define __DALI_H

#define IN1DALIP                     1
#define IN1DALIN                     2
#define IN1DALI                      3

#define DALI_ENABLE                   1
#define DALI_DISABLE                  0

extern vu8 Dali_TX_position;
extern vu8 Dali_TX_bit_value,Dali_TX_bit_value2;
extern vu16 DALI_Master_frame;                                 
extern vu8 f_repeat;                              
extern vu8 f_busy;                               
extern vu16 Dali_forward,Dali_forward2;
extern vu8 f_dalitx;
extern vu8 f_dalirx;

extern vu8 Dali_RX_Position;
extern vu8 Dali_Trigger_pending;
extern vu8 Dali_RX_Timeout;
extern vu8 DaliSlave_answer;
extern vu8 Dali_Polarity;
extern vu8 DaliReceivedForward;
extern vu8 DaliEnableSlaveFeedback;


// Dali Slave 
extern vu8 DaliSlave_TX_position; 
extern vu8 DaliSlave_TX_bit_value; 
extern vu16 DALI_Slave_frame; 
extern vu8 DALI_Slave_f_busy;
extern vu16 DaliSlave_forward; 
extern vu8 Dali_Slave_f_dalitx;
extern vu8 Dali_Slave_f_dalirx;
extern vu16 DaliSlave_backward;
extern vu8 DaliSlaveRXDataValid;
extern vu8 DaliMasterRXDataValid;
extern vu16 DaliReceivedForwardFull;

extern u8  Dali_TX_data[2],Dali_TX_data2[2];

void DaliInterruptHandler(void);
void DaliRXInterruptHandler(void);
void DaliSlaveInterruptHandler(void);
void Dali_IN_SlaveInterruptHandler(void);
void Dali_RXN_SlaveInterruptHandler(void);
void Dali_RXP_SlaveInterruptHandler(void);
void SetDaliTX_Data(u8 *tx_data,u8 *tx_data2);  
void DALI_Send(void);
void DaliSlave_RX_Status(vu8 status);
void ResetDaliReceivers(void);
void Check_Dali_Receive_Timeouts(void);

#endif







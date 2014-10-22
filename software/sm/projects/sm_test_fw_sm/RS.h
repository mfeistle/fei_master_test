#ifndef __RS_H
#define __RS_H


#define RS_RX_BufferSize             16    // V0.19
#define RS_TX_BufferSize             16    // V0.19

#define SNSTEN_PIN             GPIO_Pin_6
#define SNSREN()             GPIO_WriteBit(GPIOA,SNSTEN_PIN,Bit_RESET)
#define SNSTEN()             GPIO_WriteBit(GPIOA,SNSTEN_PIN,Bit_SET)

extern uint8_t  RS_TX_Buffer[RS_TX_BufferSize]; // RS485 TX Buffer
extern vu16 RS_TX_Counter;
extern void RS_TX_Send(vu8 *TX_Buffer,vu32 Length);

extern uint8_t  RS_RX_Buffer[RS_RX_BufferSize]; // RS485 RX Buffer
extern vu16 RS_RX_Counter;
extern vu8  RS_RX_Complete;
extern void RS_InterruptHandler(void);





#endif



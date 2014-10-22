#ifndef __INIT_H
#define __INIT_H




extern USART_InitTypeDef USART_InitStructure;
extern vu16 RxCounter;


extern vu8 RxBuffer[RxBufferSize];
extern vu16 ADC_ConvertedValue[AD_BUFSIZE];



void CPU_Init(void);
void USART2_Setup(u32 BaudRate);
#endif







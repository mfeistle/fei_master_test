#ifndef __TEST_H
#define __TEST_H

typedef struct {
  u16 MaxTestTime;
  u8  TestState;
  u8  PredelayTimer;
  u8  PredelayTime;
  u8  DebugInfo;
  s32 Current;
  s32 RefCurrent;
  s32 CurrentResults[12];
  u8  OC_Results[3];
  u8  ProCheck;
  u8  SensorCheck;
  u8  Comm485Test;
} tTest;




void TestProcess(void);

void USARTReinit(void);

void USART1Deinit(void);

void InitHW(void);

void InitTestProcess(void);


#endif







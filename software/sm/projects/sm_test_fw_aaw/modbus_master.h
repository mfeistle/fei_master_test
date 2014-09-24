#ifndef __MODBUS_H
#define __MODBUS_H

typedef enum
{
  mNONE,
  mSLIDER,
  mAAW,
} t_mbModus;
void SetMB_Modus(t_mbModus Mode);

void SetRGBLeds(uint8_t array[]);
void SetLocalDaliValues(uint8_t Ch1,uint8_t Ch2);


void MB_Process1ms(void);
void SetAAW_Testmode(u8 Bit);
void MB_Init(void);


#endif







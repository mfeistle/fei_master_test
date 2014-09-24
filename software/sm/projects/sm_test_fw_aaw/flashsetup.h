#ifndef __FLASHSETUP_H
#define __FLASHSETUP_H

#define FLASHSETUP_START_ADDRESS    ((uint32_t)0x0800e000) /* EEPROM emulation start address:
                                                  after 56KByte of used Flash memory */


typedef union {
  s32 Var[256];
  struct {
    s32 PirOnDelay;
    s32 PirOffDelay;
  } SetupVar;
} FlashUnion_t;

extern FlashUnion_t FlashSetup;

void WriteCompleteFlashSetup(void);
void EraseFlashSetup(void);
void InitFlashSetup(void);

#endif







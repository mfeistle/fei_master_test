#ifndef __FLASHSETUP_H
#define __FLASHSETUP_H

#define FLASHSETUP_START_ADDRESS    ((uint32_t)0x0800e000) /* EEPROM emulation start address:
                                                  after 56KByte of used Flash memory */
#define SETUP_VALID_MARK  0x7FFFFFFE

enum PIR_MODE_enum {
  PIR_MODE_OFF,     // PIR sensor is completely ignored
  PIR_MODE_MAN_ON,  // PIR sensor can't switch lamp on but keep lamp on
  PIR_MODE_AUTO,    // PIR sensor can switch lamp on and keep lamp on
};
  
typedef union {
  s32 Var[256];
  struct {
    s32 SETUP_VALID;    // -1 not valid 
    s32 VERSION;        // Setup Version
    s32 APPLICATION;    // prepare replacement of define and EEprom Function
    s32 DMX_RECEIVER;   // LOW to HIGH: 2nib Startaddress 3nib Startuptesttime (in msec)
    s32 OUT_MODE;       // 0=off 1=Dali 2=1..10V 3=DMX
    s32 OUT_MATRIX;     // 0=2Ch straigth 1=Parallel 2=swapped
    s32 OUT_BALANCE_CTRL; // Balance Mode
    s32 OUT_LEV_MIN;
    s32 OUT_LEV_MAX;
    s32 OUT_SCALER;     // lin log exp
    s32 OUT_ON_SRC_CH;  // ?? 1=SensodimTaste 2=Pir 4=Taster1 8=Taster2 0x10=Slyder Key oben 0x20=Slyder Key Mitte 0x40=Slyder Key Unten
    s32 OUT_LEV_SRC_CH; // ?? 1=SensodimTaste 2=SensodimTaste2 4=Taster1 8=Taster2 0x10=Slyder Oben 0x20=Slyder Unten
    s32 INPUT_SINGLE_CH;// 0=normal (individual keys),  7 = all keys, 2 = Middle Key
    s32 LUX_NOM_MIN;
    s32 LUX_NOM_MAX;
    s32 LUX_NOM_START;
    s32 LUX_ERR_OFF;
    s32 LUX_ERR_OFF_DELAY;
    s32 LUX_OV_OFF;
    s32 LUX_OV_OFF_DELAY;
    s32 LUX_OFF_LEVEL;
    s32 LUX_CTRL_EN;  // 0=off 1=Kanal1 2=Kanal2
    s32 LUX_ADJUST;
    s32 LUX_REFLECTION;
    s32 PIR_OFF_DELAY;
    s32 PIR_ON_DELAY;
    s32 PIR_FADE_OFF;
    s32 PIR_MODE;
    s32 PWR_FAIL_MODE;  // 0=Off 1=On 2=Restlicht 3=letzter Zustand
    s32 SLIDER_VAL_MIN;
    s32 SLIDER_VAL_MAX;
    s32 SLIDER_SPEED;
    s32 OUT_LEV_START;  // new parameters
    s32 LUX_NOM_OFF;
    s32 OUT_LEV_OFF;
  } Setup;
} FlashUnion_t;

extern FlashUnion_t Flash;

void WriteCompleteFlashSetup(void);
void EraseFlashSetup(void);
void InitFlashSetup(void);

#endif

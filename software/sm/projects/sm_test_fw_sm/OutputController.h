#ifndef __OUTPUTCONTROLLER_H
#define __OUTPUTCONTROLLER_H

#define KEY_SAMPLETIME_20MSEC           // ifndef then 50msec

extern const u8 lookup_percent_to_dali[101];


extern const u16 dali_permill[];
extern const u8 dali_250[];

typedef struct {
  u8 bLogScale;
  u8 bParallelModus;
  u8 bMinDalilevel;
  u8 bMaxDalilevel;
  u8 bDaliSetLevelDirect;
  u8 bLastDaliSetLevelDirect;
  u8 bDaliSetLevelIndirect;
  u8 bLastDaliSetLevelIndirect;
  u8 bMinDimmLevel; // Default 5% or 10%
  u8 bMaxDimmLevel; // Default 100%
  u8 bDaliIncDec;
  u16 wSetlux;

} tOutputController;

extern tOutputController OUTPUT;

void DaliOutputProcess(void);
u16 GetOutputValue(u16 wPromille);
u16 SetOutputLux(u8 bSliderValue);
u8 SetDimValues(u8 bSliderValue);
void DaliRampControl(void);

void InitOutputController(void);


#endif







#ifndef __watch_H
#define __watch_H

extern void Start_RTC(void);
extern void Time_Adjust(void); 
extern void Time_Show(void);

extern void Time_Display(uint32_t TimeVar);

extern  vu8 RTCTimeDisplay;
extern vu16 WatchTimeout_Keyscan;


#endif

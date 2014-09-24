/**
  ******************************************************************************
  * @file    TB12000/History.c 
  * @author  Ing. Büro W.Meier
  * @lib version V3.5.0
  * @date    02.2012
  * @brief   Dali TX_RX
  ******************************************************************************
  */ 

// on compilation dynamicly generated include file
#include "build.h"

// offset addr = 0x08002000 + 512
// max string size = 16 bytes
const char __attribute__((at(0x08002200))) fw_project[]= FWTYPE;
const char __attribute__((at(0x08002210))) fw_version[]= FWVER;
const char __attribute__((at(0x08002220))) fw_build[]=   FWBUILD;

// const char __attribute__((at(0x08002200))) Project[]= "TB12000";
// const char __attribute__((at(0x08002208))) Version[]= "V0.42";
// const char __attribute__((at(0x0800220e))) Date[] = "08.12.2012";

/*
02.03.2012 V0.10
  Start of Documented History

03.03.2012
*************************************************************************
 - Removed double Reset sequence from Send_DMX(); , this Reset Sequence was placed in the startup sequence 
04.03.2012 v0.12
***********************************************************************************************************
- Added Timer 7 for DALI Slave Timing
- EXTI Inputs PB2 PB3 for DALI Slave active
- DALI SLAVE  reception   on both DALIN und DALIP Channels - Channel with start bit detected is valid
- DALI backward on DALI TX implemented 
- Implemented EEPROM Emulation with read/write test routine

21.03.2012 Vodafone Applikation V0.13
*************************************************************************************************************
- Dali forward message received through the Dali Slave Pins PB2 or PB3 (DaliReceivedForward) implemented

24.03.2012 Vodafone Applikation V0.14 
*************************************************************************************************************
- Additional device mode -> MODE_GATEWAY with active Dali Master TX and Slave RX
- DALI Slave (backward message) transmition can be enabled/disabled with DaliEnableSlaveFeedback variable 
- DMX512 Message Reception through the UART2 ( RS485 ) in GATEWAY mode and translation to both DALI Master Channels . This Message is also received on the Dali slave
- DALI Timing changed to 416us+(416us/2) to sample received bits in the middle.
- DaliSlave_RX_Status(DALI_DISABLE)implemented to avoid activation of unused interrupts
- Implemented RS485 header RS.h
- USART2_IRQHandler is used alternatively for DMX512 reception with DMX_RX_InterruptHandler();    
 
27.03.2012 Unified Applikation V0.15
- diverse Erweiterungen / Bereinigungen
27.03.2012 Unified Applikation V0.16
- DMX App
27.03.2012 und 28.03.2012 V0.17
- Serielle DBG Eingabe ermöglicht folgendes:
't': Simuliert Tasteneingabe
'p': Simuliert PIR-Detektor

'0': Standard Applikation (DALI - VOLT - DMX mit Jumper wählbar)
'1': Vodafone Applikation (Jumper ohne Funktion)
'2': Eiger Touchpanel Applikation (DMX Device 1 - 2 - 3 mit Jumper wählbar)

'j': Abgleich auf 1000 Lux

02.04.2012 V0.18
  wenn eingeschaltet, keine 0-Werte zulassen für Regler, wenn ausgeschaltet dann ja

11.04.2012 V0.19
  Dualslider Applikation
  Achtung: für Applikation APPLICATION_TOUCHPANEL muss Bitrate auf 250000 geändert werden Zeile 545 in Init.c

20.06.2012 V0.20
  Slider Parameter werden synchronisiert übermittelt.

22.06.2012 V0.21
  mit Bootloader Funktionalität (Testversion)
25.06.2012 
  Hardware Support V1.1 implementiert ( Testversion ) 
26.06.2012 V0.22
  Sensorbus Kommunikation (im Moment Slider Binär) auf 115200 Baud
  Slider wird jetzt mit 20msec Interval gepollt.
19.07.2012 V0.23
  Validierungsversion für TWEAK
5.8.-14.09.2012 V0.24
  Modbus Erweiterungen
28.09.2012 V0.25
  Testprocedures Erweiterungen
01.10.2012 V0.26
  G / H in Testproceduren erweitert (G = 5 / 7V Ausgang)
04.10.2012 V0.27
  Slider Mode LED Behandlung für Test erweitert
  Testmode Erweiterungen
08.10.2012 V0.28
  testprocedures.c Zurücklesen nach DMX vom Sensorbus funktioniert nun auch.
12.10.2012 V0.29
  Versuche mit 10V DAC statt Full im Testmode, reduziert die Stromaufnahme (???)
  Relais Synchronisation eingeführt (zero-crossing)
15.10.2012 V0.30
  Relais Synchronisation / 50Hz Detektion Testbefehle eingeführt:
  case 'm':
    SYS.fRelais1=1;
    SYS.fRelais2=0;
    printf("\r\nP:%u %u",SYS.ACperiod,SYS.ACOffperiod);
    break;
  case 'n':
    SYS.fRelais1=0;
    SYS.fRelais2=1;
    printf("\r\nP:%u %u",SYS.ACperiod,SYS.ACOffperiod);
    break;
22.10.2012 V0.31
  Slider+Keys, Richtung vertauscht (Oben = Unten)

25.10.2012 V0.32
  Projektinformation an absolute Code Adressen platziert: 0x0x8002200 -> in *.bin dann an 0x200 (512)
29.10.2012 V0.33
  dynamische Kompensation der Relaisschaltzeiten
30.10.2012 V0.34
  Erste Vorvariante für Alone At Work mit 2x Lichtregelung
31.10.2012 V0.35
  On Off Behandlung vom AAW Module korrigiert
  AAW Gekoppelte Regelung: sobald beide Kanäle aktiv sind, wird auf den Mittelwert geregelt, beide Dali Ausgänge sind synchronisiert
01.11.2012 V0.36
  neue Lichtregelung / Sollwert mit Tastenbedienung für AAW
  02.11.2012 0-Wert Behandlung bei Remotebeleuchtung durch Nachbarlicht
06.11.2012 V0.37
  Powersaving eingeführt
  PIR Behandlung korrigiert (schaltete nicht mehr aus, wenn einmal ein bei Kanal 1, seit Einführung von Lampcontroller)
  Gekoppelte Behandlung Kanal 1+2 (Relais und Dali Ausgang) wenn Sensor bei AAW nicht angeschlossen ist.
08.11.2012 V0.38
  Sanftes Anfahren, wenn 2. Kanal zugeschaltet wird bei Regelung im 2 Kanal Mode
  Hinweis: Wenn 1. Kanal zugeschaltet wird, dann ist das Verhalten nicht genau gleich.. weiss noch nicht wie ich das genau lösen soll..
  AAW Funktion kann nun aktiviert werden beim InitLampcontroller
09.11.2012 V0.39
  Bugfix: Kanal 2 konnte Wert 0 erreichen, bei Doppelansteuerung
11.11.2012 V0.40
  2 Kanal Behandlung vom AAW (Modbus / Reduktionswert)
  
30.11.2012 V0.41
  MODBUS Erweiterungen / Korrekturen
  Im Projekt mit define "USE_RTC" aktiviert
  RTC mit BKP für Testzwecke aktiviert
  "y" startet die 
  Uhrzeit kann mit Kommando "Z" eingegeben werden! Eingabe erfolgt gem. Auforderung in zweistelligem Format!
  Zeitausgabe mit Kommando "z", aktuelle Zeit kann aus dem RTC counter register ausgelesen werden
  Einfache Scanfunktion für die Entkopplung vom restlichen Programm implementiert, Timeout_Keyscan verhindert "aufhängen" bei fehlende Eingabe
  BKP Register wurde als "Merker" beim Startup für Initialstart-Erkennung verwendet

05.12.2012 V0.42
  Hinweis:
  EEPROM start address in Flash 
  #define EEPROM_START_ADDRESS    ((uint32_t)0x0800f000) // EEPROM emulation start address
  Flashpage für Definitionen: ab 0x0800e000 bis 4x 1k Block 
  TX-Pulse von Dali Return auf Primärseite (PB2 auf HW V1.1 / PB7 auf HW V1.0) unterdrückt
  06. - 08.12.2012
  MODBUS Erweiterungen für PC Kommunikation
  09. - 17.12.2012
  Erweiterungen für AAW Test (Eingabe 'v' und 'w' ermöglichen Test von IR und US Sensoren
*/

/*END OF FILE*/

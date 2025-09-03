/******************************************************************************************
Supernatural 10 LED EMF Meter Replica

EMF ATTiny Code
Release: 2.0 (Label: EMF / 10r2)

Johnny Electronic
https://github.com/JohnnyElectronic
https://www.youtube.com/@Johnny_Electronic

Description **********************************************
This code uses an AVR device to control a DF Player audio device and drive a VU/LED meter. The EMF level will be determined by
the program/test (Prog) button. When the button is pressed quickly it will cycle through pre-programmed detector senarios. Holding the
button will cause full deflection of the meters. This meter is a prop only and does not have any real EMF detection abilities.

Copyright **********************************************
Copyright 2024 by John Bennett, aka Johnny Electronic.

  This program is free software: you can redistribute it and/or modify it under the terms 
  of the GNU General Public License as published by the Free Software Foundation, either 
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with this program. 
  If not, see <https://www.gnu.org/licenses/>.  

DFPlayer **********************************************
This code is configured to use the MP3-TF-16P (Blue LED) DF Player. Other DFPlayers may not provide the same response.

DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/product-1121.html>

 1.Connection and Diagram can be found here
 <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>

 2. The DF files used in this version are a custom creation.
*******************************************************************************************/

#include "DFPlayer.h"
#include "DFPlayer.h"

//#define DFP_DEBUG     /* Enables display of received data for query requests to Serial, Nano/ATTINY1604 only */

//#define NANO
#define ATTINY804
//#define ATTINY85

// Not all pins on the Mega and Mega 2560 boards support change interrupts, 
// so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69). 

// Nano Pinouts
//DFPlayer.h settings
#ifdef NANO
#define  DFP_RX_PIN    12     // Pin Assigned for serial RX, Connects to DF Player TX, set to -1 if not used - D12 Uno/Nano
#define  DFP_TX_PIN     8     // Pin Assigned for serial TX, Connects to DF Player RX,  - D8 Uno/Nano
#define  DFP_BUSY_PIN   2     // Pin Assigned for busy detection, set to 0 if not used
#define  METER_PIN     11     // PWM output to drive VU meter and LM3914
#define  PROG_PIN      10     // Program/Test Switch Input, Active LOW
#define  PROG_LED_PIN   3     // Indicates running a programmed sequence

// BCD Encoder Switch, Input from BCD encoder switch to select EMF Sequence Number, Active LOW
#define ENC_SW1_PIN   7     // D7, Pin 9, BCD 1
#define ENC_SW2_PIN   6     // D6, Pin 8, BCD 2
#define ENC_SW4_PIN   5     // D5, Pin 7, BCD 4
#define ENC_SW8_PIN   4     // D4, Pin 6, BCD 8
#endif

// Attny804/1604 Pinouts
// Use pins X and X to communicate with DFPlayer Mini
// ---------------------------------------------------------
// ATTny85: AVR_ATtiny85
// Attny804/1604
//                              _____
//                      VDD   1|*    |14  GND
//  (WO4)(nSS)(AIN4) PA4  0~  2|     |13  10~ PA3 (AIN3)(SCK)(EXTCLK)(WO3)
//       (WO5)(AIN5) PA5  1~  3|     |12  9   PA2 (AIN2)(MISO)
//       (DAC)(AIN6) PA6  2   4|     |11  8   PA1 (AIN1)(MOSI)
//            (AIN7) PA7  3   5|     |10  11  PA0 (AIN0)(nRESET/UPDI)
// (WO*)(RXD)(TOSC1) PB3  4   6|     |9   7~  PB0 (AIN11)(SCL)(WO0)
// (WO2)(TXD)(TOSC2) PB2  5~  7|_____|8   6~  PB1 (AIN10)(SDA)(WO1)
// 
// LED_BUILTIN    (PIN_PA7, 3)
// WO- Waveform Output (Timers, PWM ~)
// External interrupt on all general purpose pins
// 

#ifdef ATTINY804
#define DFP_BUSY_PIN  3     // Pin 5, Assigned for busy detection
#define DFP_TX_PIN    1     // Pin 3, Assigned for serial TX, Connects to DF Player RX
#define DFP_RX_PIN   -1     // Pin X, Assigned for serial RX, set to -1 if not used
#define PROG_PIN      8     // Pin 11, Assigned for Program/Test Switch Input 
#define METER_PIN     10    // Pin 13, Assigned for PWM output to drive VU meter
#define PROG_LED_PIN  9     // Pin 12, Indicates running a programmed sequence
// BCD Encoder Switch, Input from BCD encoder switch to select EMF Sequence Number, Active Low
#define ENC_SW1_PIN   7     // D7, Pin 9, BCD 1
#define ENC_SW2_PIN   5     // D5, Pin 7, BCD 2
#define ENC_SW4_PIN   6     // D6, Pin 8, BCD 4
#define ENC_SW8_PIN   4     // D4, Pin 6, BCD 8

//Serial Monitor Debug
#define SER_TX_PIN    8     // Pin 11, Assigned for serial TX, Connects to Serial Monitor RX
#define SER_RX_PIN    2     // Pin 4, Assigned for serial RX, set to -1 if not used
#endif

// ATtiny85 Pinouts
// Use pins 2 and 3 to communicate with DFPlayer Mini
// ---------------------------------------------------------
// ATTny85: AVR_ATtiny85
//                          _____
// (Reset)  (A0) PB5  5   1|*    |8  VCC
// (Xtal)   (A3) PB3  3   2|     |7  2   PB2 (A1)(INT0)(SCK)
// (Xtal)   (A2) PB4  4   3|     |6  1~  PB1     (MISO)
//                   GND  4|_____|5  0~  PB0     (MOSI)
// 
// ---------------------------------------------------------
/* DFP_BUSY_PIN     - D3, Pin 2
 * DFP_TX_PIN       - D4, Pin 3
 * progSwPin        - D2, Pin 7
 * meterPin         - D0, Pin 5
 * progLEDpin       - D1, pin 6
 */
#ifdef ATTINY85
#define DFP_BUSY_PIN  3     // ATTiny85 PB3, pin 2, pin Assigned for busy detection
#define DFP_TX_PIN    4     // Pin Assigned for serial TX, Connects to DF Player RX, ATTiny85 PB4, pin 3
#define DFP_RX_PIN   -1     // Pin Assigned for serial RX, set to -1 if not used
#define PROG_PIN      2     // D2, pin 7, Program/Test Switch Input 
#define METER_PIN     0     // D0, Pin 5, PWM output to drive VU meter
#define PROG_LED_PIN  1     // D1, Pin 6, Indicates running a programmed sequence
#endif

// Program (PROG/TEST) button
// 1 - Press and hold PROG/TEST button and the meter will deflect full while button is held
// 2 - Quickly press and release PROG/TEST button will cycle through the available programmed senarios.
//     Starts with program 1 and cycles though to the nth program and loops back to 1. The PROG_LED will
//     indicate when a selected program is running.
// 

/* Audio Tracks (mp3 folder on SD card):
1- 0001_emf start                       0.238s
2- 0002_emf low short                   0.282s
3- 0003_emf steady short (High Tone)    0.568s
4- 0004_emf steady long (High Tone)     1.435s
5- 0005_emf end                         0.282s
6- 0006_emf power up                    copy of emf start, 0.238s
*/
#define EMF_TONE_START   1    
#define EMF_TONE_LOW     2    
#define EMF_TONE_STEADY  3    
#define EMF_TONE_STEADYL 4    
#define EMF_TONE_END     5    
#define EMF_POWER_UP     6

/* EMF Levels (3v max scale) mapped to input voltage and LED indication
 LED   |  Voltage | Sensor Level | Meter Level
            0         0
  1         0.60     125              31
  2         1.2      251              63
  3         1.8      380              95
  4         2.4      504             126    
  5         3.2      632             158
*/

#define EMF_METER_0   0         /* Going to zero, end of sequence or low pause */
#define EMF_METER_1   31
#define EMF_METER_2   63
#define EMF_METER_3   95
#define EMF_METER_4   126 
#define EMF_METER_5   158 

/* Set values for 1.5v, 3v, or 5v scale range */
#define MAX_METER_LVL   158    /* Set to 80 for 1.5v, 158 for 3v, 255 for 5v scale */
#define MTR_TEST_LEVEL  118    /* Set to 50 for 1.5v, 118 for 3v, 215 for 5v scale */

const int dfpBusyPin = DFP_BUSY_PIN;
const int dfpTXpin = DFP_TX_PIN;
const int progPin = PROG_PIN;
const int meterPin = METER_PIN;
const int progLEDpin = PROG_LED_PIN;

const int encSw1Pin = ENC_SW1_PIN;
const int encSw2Pin = ENC_SW2_PIN;
const int encSw4Pin = ENC_SW4_PIN;
const int encSw8Pin = ENC_SW8_PIN;

#define EMF_AUDIO_LEVEL 15
int setEMFlevel = 0;              // Level for EMF meter output
unsigned long emfTimer = 0;       // Delay timer
int lastProgValue = 1;            // variable to store the value of the last program used

#ifdef DFP_DEBUG
int queryVal;
#endif

// EMF Meter programmed sequences
// Set EMF Level, Duration, and Sound file play or not.
const int maxProg = 6;            // Maximum number of programs used

struct emfProg {
  int emfLevel;
  int duration;
  bool sound;
};

// Program with meter level (0-158 | MAX_METER_LVL, 3v max), duration (ms), Sound Enable/Disable
// EMF_METER_0   - Plays end tone, 0.282s max
// EMF_METER_1   - Plays start tone, 0.238s max
// EMF_METER_2-3 - Plays low tone, 0.238s max
// EMF_METER_4+  - Plays high tone, 1.435s max
// 
// Make duration of last sound shorter than needed to prevent sound repeat.
// 
// Update maxProg value when adding or deleting sequences

// Program 1 (Single High/End)
// 3 steps
emfProg emfProg1 [] = {
  {EMF_METER_1, 200, 1},    // Start Tone
  {EMF_METER_5, 1400, 1},   // High Tone 
  {EMF_METER_0, 100, 1},    // End Tone, Make duration of last sound shorter than needed to prevent sound repeat.  
};

// Program 2 (Highs,Lows,End)
// 12 steps
emfProg emfProg2 [] = {
  {EMF_METER_1, 200, 1},    // Start Tone
  {EMF_METER_3, 200, 0},    // Display only
  {EMF_METER_5, 1000, 1},   // High Tone
  {EMF_METER_3, 200, 1},    // Low Tone
  {EMF_METER_2, 200, 0},    // Display only
  {EMF_METER_0, 150, 1},    // End Tone
  {EMF_METER_1, 150, 1},    // Start Tone
  {EMF_METER_3, 300, 1},    // Low Tone
  {EMF_METER_5, 1000, 1},   // High Tone
  {EMF_METER_3, 200, 1},    // Low Tone
  {EMF_METER_2, 200, 0},    // Display only
  {EMF_METER_0, 100, 1}     // End Tone
};

// Program 3 (High,Low,High,End)
// 10 steps
emfProg emfProg3 [] = {
  {EMF_METER_1, 150, 1},    // Start Tone 
  {EMF_METER_4, 1500, 1},   // High Tone  
  {EMF_METER_0, 150, 1},    // End Tone   
  {EMF_METER_1, 150, 1},    // Start Tone 
  {EMF_METER_2, 300, 1},    // Low Tone   
  {EMF_METER_3, 200, 0},    // Display only   
  {EMF_METER_4, 500, 1},    // High Tone  
  {EMF_METER_5, 2000, 1},   // High Tone 
  {EMF_METER_2, 200, 0},    // Display only
  {EMF_METER_0, 100, 1}     // End Tone   
};

// Program 4 (Single Mid/Low/End)
// 4 steps
emfProg emfProg4 [] = {
  {EMF_METER_1, 200, 1},    // Start Tone
  {EMF_METER_2, 200, 1},    // Low Tone 
  {EMF_METER_3, 200, 1},    // Mid Tone 
  {EMF_METER_0, 100, 1},    // End Tone  
};

// Program 5 (Dual Mid/Low/End)
// 9 steps
emfProg emfProg5 [] = {
  {EMF_METER_1, 150, 1},    // Start Tone
  {EMF_METER_2, 150, 1},    // Low Tone 
  {EMF_METER_3, 150, 1},    // Low Tone 
  {EMF_METER_0, 150, 1},    // End Tone  
  {EMF_METER_0, 500, 0},    // Pause  500 ms                            
  {EMF_METER_1, 150, 1},    // Start Tone
  {EMF_METER_2, 150, 1},    // Low Tone 
  {EMF_METER_3, 150, 1},    // Low Tone 
  {EMF_METER_0, 100, 1},    // End Tone  
};

// Program 6 (Start/Long Mid/Low/End)
// 6 steps
emfProg emfProg6 [] = {
  {EMF_METER_1, 150, 1},    // Start Tone
  {EMF_METER_2, 150, 1},    // Low Tone 
  {EMF_METER_3, 2000, 1},   // Long Low Tone 
  {EMF_METER_2, 150, 1},    // Low Tone
  {EMF_METER_1, 150, 1},    // Low Tone 
  {EMF_METER_0, 150, 1},    // End Tone  
};

SoftwareSerial dfpSerial (DFP_RX_PIN, dfpTXpin); // RX, TX

#if defined(DFP_DEBUG) && defined(ATTINY804)
// Set up for Serial Monitor
SoftwareSerial attinySerial (SER_RX_PIN, SER_TX_PIN); // RX, TX
#endif


/* Sets the state of the Program (PROG) LED to show when a program 
* is active.
*
* - state:  either a 1 or 0 to enable/disable the PROG LED
*/
void progLED (bool state)
{
  digitalWrite (progLEDpin, state);
} /* end progLED */

/* Based on the supplied meter level it will determine the mp3 file that should play with 
* that meter level.
*
* - meterLvl: a valid meter level based on EMF_METER_0-5
*
* Returns: mp3 index number to play
*/
int getMP3track (int meterLvl)
{
  if (meterLvl >= EMF_METER_4) {
      return (EMF_TONE_STEADYL);
  }
  else if (meterLvl >= EMF_METER_3) {
      return (EMF_TONE_LOW);
  }
  else if (meterLvl >= EMF_METER_2) {
      return (EMF_TONE_LOW);
  }
  else if (meterLvl >= EMF_METER_1) {
      return (EMF_TONE_START);
  }
  else {
      return (EMF_TONE_END);
  }
} /* end getMP3track */

/* Runs the selected program sequence and sets the PROG_LED state.
* - emfprog[]: program struture to execute
* - emfProgSz: number of program elements
*/
void runProgram (struct emfProg emfprog[], int emfProgSz)
{
  int mp3Track = 0;
  int currentMeterLvl = 0;
  int duration = 0;

  progLED (1);
  for (int i = 0; i < emfProgSz; i++) {
    currentMeterLvl = emfprog[i].emfLevel;
    duration = emfprog[i].duration;

    mp3Track = getMP3track(currentMeterLvl);

    analogWrite(meterPin,currentMeterLvl);
    emfTimer = millis();

    if (emfprog[i].sound) dfpPlayTrackMP3(mp3Track);

    while (millis() - emfTimer < duration) {
      if (emfprog[i].sound) {
          if (digitalRead(dfpBusyPin)) {
            dfpPlayTrackMP3(mp3Track);
            delay (30);
          }
      } 
    }
  }
  progLED (0);
  analogWrite(meterPin,0);
  pinMode(meterPin, INPUT);   // Shut down output 
} /* end runProgram */

void setup()
{
  pinMode(progPin, INPUT_PULLUP);         // Program switch input, active LOW 
  pinMode(progLEDpin, OUTPUT);            // Program LED output

#if defined(NANO) || defined(ATTINY804)
  pinMode (encSw1Pin, INPUT_PULLUP);      // Encoder Switch Input 
  pinMode (encSw2Pin, INPUT_PULLUP);
  pinMode (encSw4Pin, INPUT_PULLUP);
  pinMode (encSw8Pin, INPUT_PULLUP);
#endif

  // Set initial state of outputs
  digitalWrite (progLEDpin, 0);
  pinMode(meterPin, INPUT);

#ifdef DFP_DEBUG
    #ifdef NANO
      Serial.begin(115200);
      Serial.println();
      Serial.println(F("DFPlayer Test"));
      Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
    #endif

    #ifdef ATTINY804
      attinySerial.begin (115200);
      Serial.println();
      Serial.println(F("DFPlayer Test"));
      Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
    #endif
#endif

    dfpSerial.begin (9600);
    dfpSerial.listen();

// Initial setup for DFPlayer.
// Sets up DFP_BUSY_PIN if used to use the busy state/pin of the player. 
// Resets the device and provides a 1 sec delay.
    dfpSetup();
    dfpSetVolume(EMF_AUDIO_LEVEL);
    dfpSetEq(DFP_EQ_CLASSIC);

#ifdef DFP_DEBUG

    #ifdef NANO
        queryVal = dfpReadQuery(0);
        Serial.print(F("Params Rst:"));
        Serial.println(queryVal, HEX);
        
        queryVal = dfpGetVolume();
        Serial.print(F("Params Vol:"));
        Serial.println(queryVal);
    #endif
#endif

  // Power up sound and flick meter for start up visual
  dfpPlayTrackMP3(EMF_POWER_UP);
  analogWrite(meterPin, MTR_TEST_LEVEL);
  delay (500); 
  analogWrite(meterPin,0);
  pinMode(meterPin, INPUT);   // Shut down output 
}

void loop()
{
    // Program pin active low
    if (!digitalRead(progPin)) {
        // Enter program mode and run one of the programmed sequences or test mode
        delay (400);

        if (!digitalRead(progPin)) {
            // Button still pressed, test mode

            setEMFlevel = EMF_METER_1;
            analogWrite(meterPin,setEMFlevel);
            delay (200);
      
            /* High detection state
            Check busy line - if still playing skip or if not play again 
            dfpBusyPin is active low on busy state */
            dfpPlayTrackMP3(EMF_TONE_START);

            setEMFlevel = EMF_METER_2;
            analogWrite(meterPin,setEMFlevel);
            delay (200);

            setEMFlevel = EMF_METER_3;
            analogWrite(meterPin,setEMFlevel);
            delay (200);

            while (!digitalRead(progPin)) {
                if (digitalRead(dfpBusyPin)) {
                    dfpPlayTrackMP3(EMF_TONE_STEADYL);
                    setEMFlevel = EMF_METER_5;
                    analogWrite(meterPin,setEMFlevel);
                }
                delay (30);
            }

            dfpPlayTrackMP3(EMF_TONE_END);
            setEMFlevel = EMF_METER_2;
            analogWrite(meterPin,setEMFlevel);
            delay (200);

            setEMFlevel = 0;
            analogWrite(meterPin,setEMFlevel);
            pinMode(meterPin, INPUT);
        } else {
            // Select and run one of the programmed sequences
#if defined(NANO) || defined(ATTINY804)
            // Read value
            byte bcdValue1 = digitalRead(encSw1Pin);
            byte bcdValue2 = digitalRead(encSw2Pin);
            byte bcdValue4 = digitalRead(encSw4Pin);
            byte bcdValue8 = digitalRead(encSw8Pin);

    #ifdef DFP_DEBUG
        #ifdef NANO
            Serial.print (F("BCD Sw1: "));
            Serial.println (bcdValue1);
            Serial.print (F("BCD Sw2: "));
            Serial.println (bcdValue2);
            Serial.print (F("BCD Sw4: "));
            Serial.println (bcdValue4);
            Serial.print (F("BCD Sw8: "));
            Serial.println (bcdValue8);
        #endif

        #ifdef ATTINY804
            attinySerial.print (F("BCD Sw1: "));
            attinySerial.println (bcdValue1);
            attinySerial.print (F("BCD Sw2: "));
            attinySerial.println (bcdValue2);
            attinySerial.print (F("BCD Sw4: "));
            attinySerial.println (bcdValue4);
            attinySerial.print (F("BCD Sw8: "));
            attinySerial.println (bcdValue8);
        #endif
    #endif

            // Calculate value
            byte bcdValue = (bcdValue8 << 3) | (bcdValue4 << 2) | (bcdValue2 << 1) | bcdValue1;
            bcdValue = ((byte)~bcdValue & 0xF);

            if (bcdValue > 0) {
                lastProgValue = bcdValue;
            }

            if (lastProgValue > maxProg) {
                lastProgValue = 1;
            }

    #ifdef DFP_DEBUG
        #ifdef NANO
            Serial.print (F("BCD Value: "));
            Serial.println (bcdValue);

            Serial.print (F("Running Program: "));
            Serial.println (lastProgValue);
        #endif

        #ifdef ATTINY804
            attinySerial.print (F("BCD Value: "));
            attinySerial.println (bcdValue);

            attinySerial.print (F("Running Program: "));
            attinySerial.println (lastProgValue);
        #endif
    #endif
#endif   // endif Nano or ATTINY804/1604

            // Short press, run through program
            switch (lastProgValue) {
              case 1:
                  runProgram (emfProg1, 3);
                  break;
              case 2:
                  runProgram (emfProg2, 12);
                  break;
              case 3:
                  runProgram (emfProg3, 10);
                  break;
              case 4:
                  runProgram (emfProg4, 4);
                  break;
              case 5:
                  runProgram (emfProg5, 9);
                  break;
              case 6:
                  runProgram (emfProg6, 6);
                  break;
              default:
                  break;
            }

            lastProgValue++;
        } // end else
    } // if progPin
} // end loop


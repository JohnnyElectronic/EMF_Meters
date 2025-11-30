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
#include <EEPROM.h>

//#define DFP_DEBUG     /* Enables display of received data for query requests to Serial, Nano/ATTINY1604 only */

//#define NANO
#define ATTINY1604
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

#define RANDOM_AIN    A4    // Used to set a random seed
#endif

// Attny1604 Pinouts - 16MHz Clock
// Use pins 1 and 2 to communicate with DFPlayer Mini
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

#ifdef ATTINY1604
#define DFP_BUSY_PIN  3     // Pin 5, Assigned for busy detection
#define DFP_TX_PIN    1     // Pin 3, Assigned for serial TX, Connects to DF Player RX (Pin 2)
#define DFP_RX_PIN    2     // Pin 4, Assigned for serial RX, Connects to DF Player TX (Pin 3), set to -1 if not used
#define PROG_PIN      8     // Pin 11, Assigned for Program/Test Switch Input 
#define METER_PIN     10    // Pin 13, Assigned for PWM output to drive VU meter
#define PROG_LED_PIN  9     // Pin 12, Indicates running a programmed sequence
// BCD Encoder Switch, Input from BCD encoder switch to select EMF Sequence Number, Active Low
#define ENC_SW1_PIN   7     // D7, Pin 9, BCD 1
#define ENC_SW2_PIN   5     // D5, Pin 7, BCD 2
#define ENC_SW4_PIN   6     // D6, Pin 8, BCD 4
#define ENC_SW8_PIN   4     // D4, Pin 6, BCD 8

#define RANDOM_AIN    A4    // Pin 2, Used to set a random seed, used for Serial Monitor when active

//Serial Monitor Debug
#define SER_TX_PIN    0     // Pin 2, Assigned for serial TX, Connects to Serial Monitor RX
//#define SER_RX_PIN    2     // Pin 4, Assigned for serial RX, Connects to Serial Monitor TX
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
7- 0007_emf charge up                   1.0s
*/
#define EMF_TONE_START   1    
#define EMF_TONE_LOW     2    
#define EMF_TONE_STEADY  3    
#define EMF_TONE_STEADYL 4    
#define EMF_TONE_END     5    
#define EMF_POWER_UP     6
#define EMF_CHARGE_UP    7

#define EMF_TONE_STEADYL_DURATION  1300    /* Maximum time until replay */   

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

// Use a random selection for meter levels
#define EMF_RANDOM_1   -1       /* Selects a random range 10-30   */
#define EMF_RANDOM_2   -2       /* Selects a random range 31-62   */
#define EMF_RANDOM_3   -3       /* Selects a random range 63-94   */
#define EMF_RANDOM_4   -4       /* Selects a random range 95-125  */
#define EMF_RANDOM_5   -5       /* Selects a random range 126-157 */

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

#define AUDIO_LEVEL_ADDR     0          // Memory address of EMF audio level setting
#define EMF_AUDIO_LEVEL     20          // Typical 15-20 to start
#define EMF_MAX_AUDIO_LEVEL 30          // Highest Level
#define EMF_MIN_AUDIO_LEVEL  4          // Lowest Audible Level
byte emfVolumeLev = EMF_AUDIO_LEVEL;    // Current or initial DFP volume level for EMF
int setEMFlevel = 0;                    // Level for EMF meter output
unsigned long emfTimer = 0;             // Delay timer
unsigned long emfTimer2 = 0;            // Delay timer
int lastProgValue = 1;                  // variable to store the value of the last program used

#if defined(NANO) || defined(ATTINY1604)
int queryVal;
#endif

// EMF Meter programmed sequences
// Set EMF Level, Duration, and Sound file play or not.
const int maxProg = 9;            // Maximum number of programs used

struct emfProg {
  int emfLevel;
  int duration;
  bool sound;
};

// Program with meter level (0-158 | MAX_METER_LVL, 3v max), duration (ms), Sound Enable/Disable
// EMF_METER_0   - Plays end tone, 0.282s max
// EMF_METER_1   - Plays start tone, 0.287s max
// EMF_METER_2-3 - Plays low tone, 0.238s max
// EMF_METER_4+  - Plays high tone, 1.435s max
// 
// Make duration of last sound shorter than needed to prevent sound repeat.
// 
// Update maxProg value when adding or deleting sequences

// Program 1 (Single High/End, also used for intial meter adjustments)
// 4 steps
emfProg emfProg1 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_3, 100, 0},    // Display only
  {EMF_METER_5, 1400, 1},   // High Tone 
  {EMF_METER_0, 100, 1},    // End Tone, Make duration of last sound shorter than needed to prevent sound repeat.  
};

// Program 2 (Highs,Lows,End 2 cycles)
// 14 steps
emfProg emfProg2 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_3, 100, 0},    // Display only
  {EMF_RANDOM_4, 1000, 1},  // High Tone
  {EMF_METER_3, 210, 1},    // Low Tone
  {EMF_RANDOM_1, 100, 0},   // Display only, pause
  {EMF_METER_0, 150, 1},    // End Tone
  {EMF_METER_0, 800, 0},    // Display only, pause
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_RANDOM_3, 100, 0},   // Display only
  {EMF_RANDOM_5, 1000, 1},  // High Tone
  {EMF_METER_2, 210, 1},    // Low Tone
  {EMF_METER_1, 100, 0},    // Display only, pause
  {EMF_METER_0, 100, 1},    // End Tone
  {EMF_RANDOM_1, 300, 0}              // Display only
};

// Program 3 (High,Low,High,End)
// 12 steps
emfProg emfProg3 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone 
  {EMF_METER_3, 100, 0},    // Display only
  {EMF_METER_4, 1400, 1},   // High Tone  
  {EMF_METER_0, 150, 1},    // End Tone 
  {EMF_RANDOM_1, 500, 0},   // Display only, pause
  {EMF_METER_1, 200, 1},    // Start Tone 
  {EMF_RANDOM_2, 50, 0},    // Low Tone   
  {EMF_METER_3, 200, 1},    // Low Tone   
  {EMF_METER_5, 1400, 1},   // High Tone 
  {EMF_METER_2, 210, 1},    // Low Tone
  {EMF_RANDOM_1, 100, 0},   // Display only, pause
  {EMF_METER_0, 100, 1},    // End Tone
};

// Program 4 (Single Mid/Low/End)
// 4 steps
emfProg emfProg4 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_2, 100, 0},    // Display only 
  {EMF_RANDOM_3, 210, 1},   // Mid Tone 
  {EMF_METER_0, 100, 1},    // End Tone  
};

// Program 5 (Dual Chirps, Mid/Low/End)
// 9 steps
emfProg emfProg5 [] = {
  {EMF_METER_1, 260, 1},    // Start Tone
  {EMF_METER_2, 150, 1},    // Low Tone 
  {EMF_RANDOM_3, 150, 1},   // Low Tone 
  {EMF_METER_0, 200, 1},    // End Tone  
  {EMF_METER_0, 500, 0},    // Pause  500 ms                            
  {EMF_METER_1, 260, 1},    // Start Tone
  {EMF_METER_2, 150, 1},    // Low Tone 
  {EMF_RANDOM_3, 150, 1},   // Low Tone 
  {EMF_METER_0, 100, 1},    // End Tone  
};

// Program 6 (Start/Long Mid/Low/End)
// 6 steps
emfProg emfProg6 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_2, 200, 1},    // Low Tone 
  {EMF_RANDOM_3, 2000, 1},  // Long Low Tone 
  {EMF_METER_2, 200, 1},    // Low Tone
  {EMF_RANDOM_1, 200, 1},   // Low Tone 
  {EMF_METER_0, 100, 1},    // End Tone  
};

// Program 7. Inspired by Olivias S4 E2 sequence, ~16.5s
// 32 steps
emfProg emfProg7 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_3, 100, 0},    // Display only, pause
  {EMF_METER_5, 1300, 1},   // High Tone 
  {EMF_METER_0, 150 , 1},   // End Tone 
  {EMF_METER_0, 300, 0},    // Pause  500 ms  ---------------  
  {EMF_METER_1, 230, 1},    // Start                            
  {EMF_METER_0, 100, 1},    // End Tone, Make duration of last sound shorter than needed to prevent sound repeat.  
  {EMF_METER_0, 300, 0},    // Pause  300 ms  --------------
  {EMF_RANDOM_1, 230, 1},   // Start Tone
  {EMF_METER_3, 100, 0},    // Display only, pause
  {EMF_RANDOM_5, 1300, 1},  // High Tone 
  {EMF_METER_0, 150 , 1},   // End Tone 
  {EMF_METER_0, 800, 0},    // Pause    -------------------
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_2, 50, 0},     // Display only, pause
  {EMF_METER_0, 180, 1},    // End Tone 
  {EMF_METER_0, 500, 0},    // Pause    ------------------
  {EMF_METER_2, 200, 1},    // Mid                          
  {EMF_METER_0, 150, 1},    // End    
  {EMF_METER_0, 1000, 0},   // Pause     ------------------
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_2, 50, 0},     // Display only, pause
  {EMF_METER_3, 200, 1},    // Mid Tone 
  {EMF_RANDOM_5, 1300, 1},  // High Tone 
  {EMF_RANDOM_3, 100, 0},   // Pause
  {EMF_METER_0, 150, 1},    // End 
  {EMF_METER_0, 1000, 0},   // Pause    ------------------                            
  {EMF_METER_2, 150, 1},    // Mid                          
  {EMF_METER_0, 150, 1},    // End  
  {EMF_METER_0, 1000, 0},   // Pause     -----------------
  {EMF_RANDOM_2, 150, 1},   // Mid                          
  {EMF_METER_0, 100, 1},    // End Tone, Make duration of last sound shorter than needed to prevent sound repeat.                        
};

// Program 8. Inspired by Deans S1 E11 sequence, ~15.4s
// 34 steps
emfProg emfProg8 [] = {
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_0, 150 , 1},   // End Tone 
  {EMF_METER_0, 500, 0},    // Pause      -----------------                        
  {EMF_RANDOM_1, 230, 1},   // Start                            
  {EMF_METER_4, 200, 1},    // Short High Tone   
  {EMF_METER_0, 150, 1},    // End    
  {EMF_METER_0, 500, 0},    // Pause      -----------------
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_RANDOM_5, 350, 1},   // Short High Tone
  {EMF_METER_0, 150 , 1},   // End Tone 
  {EMF_METER_0, 1000, 0},   // Pause      -----------------
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_0, 150 , 1},   // End Tone 
  {EMF_METER_0, 700, 0},    // Pause      -----------------
  {EMF_RANDOM_2, 150 , 1},  // Mid Tone 
  {EMF_METER_0, 700, 0},    // Pause      -----------------
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_METER_2, 100, 0},    // Display 
  {EMF_METER_3, 200, 1},    // Mid Tone 
  {EMF_RANDOM_4, 100, 0},   // Display
  {EMF_METER_5, 1300, 1},   // High Tone 
  {EMF_METER_0, 100, 0},    // Pause      -----------------
  {EMF_METER_2, 200, 1},    // Low    
  {EMF_METER_0, 150, 1},    // End    
  {EMF_METER_0, 600, 0},    // Pause      -----------------
  {EMF_METER_1, 230, 1},    // Start Tone
  {EMF_RANDOM_4, 300, 1},   // Short High Tone 
  {EMF_METER_0, 150, 1},    // End 
  {EMF_METER_0, 700, 0},    // Pause      -----------------
  {EMF_METER_2, 200, 1},    // Low Tone 
  {EMF_METER_0, 150, 1},    // End 
  {EMF_METER_0, 500, 0},    // Pause            -
  {EMF_RANDOM_2, 200, 1},   // Low Tone 
  {EMF_METER_0, 100, 1},    // End                           
};

// Program 9. Inspired by S4 E13, Low tone / High tone, lots of meter movement 
// 16 steps
emfProg emfProg9 [] = {
  {EMF_METER_1, 200, 1},    // Start Tone
  {EMF_METER_3, 100, 0},    // Display only
  {EMF_METER_2, 100, 1},    // Low Tone
  {EMF_RANDOM_1, 100, 0},   // Display only
  {EMF_METER_2, 100, 0},    // Display only
  {EMF_METER_0, 150, 1},    // End Tone
  {EMF_METER_0, 400, 0},    // Pause
  {EMF_METER_1, 150, 1},    // Start Tone
  {EMF_RANDOM_3,100, 0},    // Display only
  {EMF_METER_2, 100, 0},    // Display only
  {EMF_RANDOM_3, 150, 1},   // Low Tone
  {EMF_METER_5, 1200, 1},   // High Tone
  {EMF_METER_2, 250, 0},    // Display only
  {EMF_RANDOM_4, 300, 0},   // Display only                         
  {EMF_METER_0, 100, 1},     // End Tone
  {EMF_METER_1, 200, 0}     // Display only
};


SoftwareSerial dfpSerial (DFP_RX_PIN, dfpTXpin); // RX, TX

#if defined(DFP_DEBUG) && defined(ATTINY1604)
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
  unsigned long duration = 0;

  progLED (1);
  for (int i = 0; i < emfProgSz; i++) {
    currentMeterLvl = emfprog[i].emfLevel;

    switch (currentMeterLvl) {  
       case -1:
          currentMeterLvl = random (10, 31);
        break;
       case -2: 
          currentMeterLvl = random (31, 63);
        break;
       case -3:
          currentMeterLvl = random (63, 95);
        break;
       case -4:
          currentMeterLvl = random (95, 126);
        break;
       case -5:
          currentMeterLvl = random (126, 158);
        break;
       default:
          if (currentMeterLvl < 0) {
            currentMeterLvl = EMF_METER_0;
          }
          break;
    }

    duration = emfprog[i].duration;

    mp3Track = getMP3track(currentMeterLvl);

    analogWrite(meterPin,currentMeterLvl);

    if (emfprog[i].sound) dfpPlayTrackMP3(mp3Track);
    emfTimer = millis();
    delay (10);

    while (millis() - emfTimer < duration) {
      if (emfprog[i].sound) {
          if (digitalRead(dfpBusyPin)) {
            dfpPlayTrackMP3(mp3Track);
            delay (10);
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
  // Set initial state of outputs
  pinMode(progPin, INPUT_PULLUP);         // Program switch input, active LOW 
  pinMode(progLEDpin, OUTPUT);            // Program LED output

#if defined(NANO) || defined(ATTINY1604)
  pinMode (encSw1Pin, INPUT_PULLUP);      // Encoder Switch Input 
  pinMode (encSw2Pin, INPUT_PULLUP);
  pinMode (encSw4Pin, INPUT_PULLUP);
  pinMode (encSw8Pin, INPUT_PULLUP);
#endif

  digitalWrite (progLEDpin, 0);
  pinMode(meterPin, INPUT);

#ifdef DFP_DEBUG
    #ifdef NANO
      Serial.begin(115200);
      Serial.println();
      Serial.println(F("DFPlayer Test"));
      Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

      randomSeed (RANDOM_AIN);
    #endif

    #ifdef ATTINY1604
      attinySerial.begin (115200);
      Serial.println();
      Serial.println(F("DFPlayer Test"));
      Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
    #endif
#else
    #ifdef ATTINY1604
      randomSeed (RANDOM_AIN);
    #endif
#endif

    dfpSerial.begin (9600);
    dfpSerial.listen();

    // Initial setup for DFPlayer.
    // Sets up DFP_BUSY_PIN if used to use the busy state/pin of the player. 
    // Resets the device and provides a 1 sec delay.
    dfpSetup(DFP_BUSY_PIN, 1000);

    emfVolumeLev = EEPROM.read(AUDIO_LEVEL_ADDR);

    if ((emfVolumeLev < EMF_MIN_AUDIO_LEVEL) || (emfVolumeLev > EMF_MAX_AUDIO_LEVEL)) {
        // Likely first run, set to a reasonable value. If audio too low, reset to default
        emfVolumeLev = EMF_AUDIO_LEVEL;   
    }

    dfpSetVolume(emfVolumeLev);
    dfpSetEq(DFP_EQ_CLASSIC);

#ifdef DFP_DEBUG
    #ifdef NANO
        Serial.print(F("Audio Level:"));
        Serial.println(emfVolumeLev);

        queryVal = dfpReadQuery(0);
        Serial.print(F("Params Rst:"));
        Serial.println(queryVal, HEX);
        
        queryVal = dfpGetVolume();
        Serial.print(F("Params Vol:"));
        Serial.println(queryVal);
    #endif
#endif

  // Power up sound and flick meter for start up visual
  dfpPlayTrackMP3(EMF_CHARGE_UP);
  analogWrite(meterPin, MTR_TEST_LEVEL);
  delay (1000);               // EMF_CHARGE_UP runs for 1 sec
  analogWrite(meterPin,0);
  pinMode(meterPin, INPUT);   // Shut down output 
}

void loop()
{
    // Program pin active low
    if (!digitalRead(progPin)) {
        // Enter program mode and run one of the programmed sequences or test mode
        delay (350);

        if (!digitalRead(progPin)) {
            // Button still pressed, test mode

            // EMF  start tone
            setEMFlevel = EMF_METER_1;
            analogWrite(meterPin,setEMFlevel);
            dfpPlayTrackMP3(EMF_TONE_START);
            delay (150);

            if (!digitalRead(progPin)) {
                // Button still pressed, Meter only
                setEMFlevel = EMF_METER_2;
                analogWrite(meterPin,setEMFlevel);
                delay (150);
            }
      
            if (!digitalRead(progPin)) {
                // Button still pressed, EMF low tone
                setEMFlevel = EMF_METER_3;
                analogWrite(meterPin,setEMFlevel);
                dfpPlayTrackMP3(EMF_TONE_LOW);
                delay (300);
            }

            emfTimer = millis();     // MP3 track time
            emfTimer2 = millis();    // Meter random set time
            // Prog/Test pin is active low
            while (!digitalRead(progPin)) {
                if (digitalRead(dfpBusyPin) || ((millis() - emfTimer) > EMF_TONE_STEADYL_DURATION)) {
                    // Replay just under sound file play length, slightly faster than using busy pin
                    setEMFlevel = EMF_METER_5;
                    analogWrite(meterPin,setEMFlevel);
                    dfpPlayTrackMP3(EMF_TONE_STEADYL);
                    emfTimer = millis();
                }
                delay (30);

                if ((millis() - emfTimer2) > 400) {
                    setEMFlevel = random (95, 158);
                    analogWrite(meterPin,setEMFlevel);
                    emfTimer2 = millis();
                }
            }

            // Wind down the meter
            dfpPlayTrackMP3(EMF_TONE_END);
            setEMFlevel = random (60, 90);
            analogWrite(meterPin,setEMFlevel);
            delay (100);

            setEMFlevel = random (30, 60);
            analogWrite(meterPin,setEMFlevel);
            delay (100);

            setEMFlevel = 0;
            analogWrite(meterPin,setEMFlevel);
            pinMode(meterPin, INPUT);   // Shut down PWM

            dfpSerialPurge ();    // Prep for volume read

            /* Check volume level */           
#if defined(NANO) || defined(ATTINY1604)
            queryVal = dfpGetVolume();
            if ((queryVal != emfVolumeLev) && (queryVal <= EMF_MAX_AUDIO_LEVEL)) {
                /* Update value */
                emfVolumeLev = queryVal;
                EEPROM.write(AUDIO_LEVEL_ADDR, emfVolumeLev);

#ifdef DFP_DEBUG
        #ifdef NANO
            Serial.print (F("New Audio Level: "));
            Serial.println (emfVolumeLev);
        #endif
#endif
            }
#endif
        } else {
            // Select and run one of the programmed sequences
#if defined(NANO) || defined(ATTINY1604)
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

        #ifdef ATTINY1604
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

        #ifdef ATTINY1604
            attinySerial.print (F("BCD Value: "));
            attinySerial.println (bcdValue);

            attinySerial.print (F("Running Program: "));
            attinySerial.println (lastProgValue);
        #endif
    #endif
#endif   // endif Nano or ATTINY1604

            // Short press, run through program
            switch (lastProgValue) {
              case 1:
                  runProgram (emfProg1, 4);
                  break;
              case 2:
                  runProgram (emfProg2, 14);
                  break;
              case 3:
                  runProgram (emfProg3, 12);
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
              case 7:
                  runProgram (emfProg7, 32);
                  break;
              case 8:
                  runProgram (emfProg8, 34);
                  break;
              case 9:
                  runProgram (emfProg9, 16);
                  break;
              default:
                  /* If none of the above, bad state, reset */
                  lastProgValue = 1;
                  runProgram (emfProg1, 4);
                  break;
            }

            lastProgValue++;
        } // end else
    } // if progPin
} // end loop


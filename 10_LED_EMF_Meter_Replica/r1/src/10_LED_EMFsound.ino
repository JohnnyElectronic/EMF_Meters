/******************************************************************************************
Supernatural 10 LED EMF Meter Replica

EMF ATTiny85 Code
Release: 1.0 (Label: EMF / 10r1)

Johnny Electronic
https://github.com/JohnnyElectronic
https://www.youtube.com/@Johnny_Electronic

Description **********************************************
This code uses an ATTiny 85 to control a DF Player audio device and drive a VU meter. The EMF level will be determined by
the program/test (Prog) button. When the button is pressed quickly it will cycle through pre-programmed detector senarios. Holding the
button will cause full deflection of the meter. This meter is a prop only and does not have any real EMF detection abilities.

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
This code is configured to use the MP3-TF-16P V3.0 DF Player. Delays have been added due to the slower device.

DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/product-1121.html>

 1.Connection and Diagram can be found here
 <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>

 2. The DF files used in this version are a custom creation.
*******************************************************************************************/

#include "DFPlayer.h"

//#define DFP_DEBUG     /* Enables display of received data for query requests to Serial, Nano only */

//#define NANO
#define ATTINY

// Not all pins on the Mega and Mega 2560 boards support change interrupts, 
// so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69). 

// Nano Pinouts
//DFPlayer.h settings
#ifdef NANO
#define  DFP_RX_PIN    12     // Pin Assigned for serial RX, Connects to DF Player TX, set to -1 if not used - D12 Uno/Nano
#define  DFP_TX_PIN     8     // Pin Assigned for serial TX, Connects to DF Player RX,  - D8 Uno/Nano
#define  DFP_BUSY_PIN   2     // Pin Assigned for busy detection, set to 0 if not used
#define  METER_PIN     11     // PWM output to drive VU meter and LM3914
#define  PROG_PIN      10     // Program/Test Switch Input
#define  PROG_LED_PIN   4     // Indicates running a programmed sequence
#endif

// ATtiny85 Pinouts
// Use pins 2 and 3 to communicate with DFPlayer Mini
// ---------------------------------------------------------
// ATTny85: AVR_ATtiny85
//                          _____
// (Reset)  (A0) PB5  5   1|*    |8  VCC
// (Xtal)   (A3) PB3  3   2|     |7  2   PB2 (A1)(SCK)
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
#ifdef ATTINY
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

SoftwareSerial dfpSerial (DFP_RX_PIN, DFP_TX_PIN); // RX, TX

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

#define EMF_AUDIO_LEVEL 15
int setEMFlevel = 0;              // Level for EMF meter output
unsigned long emfTimer = 0;       // Delay timer
int lastProgValue = 1;            // variable to store the value of the last program used

#ifdef DFP_DEBUG
int queryVal;
#endif

// EMF Meter programmed sequences
// Set EMF Level, Duration, and Sound file play or not.
const int maxProg = 5;            // Maximum number of programs used

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

// Program 5 (Lows/End)
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

/* Sets the state of the Program (PROG) LED to show when a program 
* is active.
*
* - state:  either a 1 or 0 to enable/disable the PROG LED
*/
void progLED (bool state)
{
  digitalWrite (PROG_LED_PIN, state);
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

    analogWrite(METER_PIN,currentMeterLvl);
    emfTimer = millis();

    if (emfprog[i].sound) dfpPlayTrackMP3(mp3Track);

    while (millis() - emfTimer < duration) {
      if (emfprog[i].sound) {
          if (digitalRead(DFP_BUSY_PIN)) {
            dfpPlayTrackMP3(mp3Track);
            delay (30);
          }
      } 
    }
  }
  progLED (0);
  analogWrite(METER_PIN,0);
  pinMode(METER_PIN, INPUT);   // Shut down output 
} /* end runProgram */

void setup()
{
  pinMode(PROG_PIN, INPUT);          // Program switch input 
  pinMode(PROG_LED_PIN, OUTPUT);     // Program LED output

  // Set initial state of outputs
  digitalWrite (PROG_LED_PIN, 0);
  pinMode(METER_PIN, INPUT);

#ifdef DFP_DEBUG
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("DFPlayer Test"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  Serial.print(F("DFP_BUSY_PIN:"));
  Serial.println(DFP_BUSY_PIN);
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
    queryVal = dfpReadQuery(0);
    Serial.print(F("Params Rst:"));
    Serial.println(queryVal, HEX);

    queryVal = dfpGetVolume();
    Serial.print(F("Params Vol:"));
    Serial.println(queryVal);
#endif

  // Power up sound and flick meter for start up visual
  dfpPlayTrackMP3(EMF_POWER_UP);
  analogWrite(METER_PIN, MTR_TEST_LEVEL);
  delay (500); 
  analogWrite(METER_PIN,0);
  pinMode(METER_PIN, INPUT);   // Shut down output 
}

void loop()
{
    // Program pin active low
    if (digitalRead(PROG_PIN)) {
        // Enter program mode and run one of the programmed sequences or test mode
        delay (400);

        if (digitalRead(PROG_PIN)) {
            // Button still pressed, test mode

            setEMFlevel = EMF_METER_1;
            analogWrite(METER_PIN,setEMFlevel);
            delay (200);
      
            /* High detection state
            Check busy line - if still playing skip or if not play again 
            DFP_BUSY_PIN is active low on busy state */
            dfpPlayTrackMP3(EMF_TONE_START);

            setEMFlevel = EMF_METER_2;
            analogWrite(METER_PIN,setEMFlevel);
            delay (200);

            setEMFlevel = EMF_METER_3;
            analogWrite(METER_PIN,setEMFlevel);
            delay (200);

            while (digitalRead(PROG_PIN)) {
                if (digitalRead(DFP_BUSY_PIN)) {
                    dfpPlayTrackMP3(EMF_TONE_STEADYL);
                    setEMFlevel = EMF_METER_5;
                    analogWrite(METER_PIN,setEMFlevel);
                }
                delay (30);
            }

            dfpPlayTrackMP3(EMF_TONE_END);
            setEMFlevel = EMF_METER_2;
            analogWrite(METER_PIN,setEMFlevel);
            delay (200);

            setEMFlevel = 0;
            analogWrite(METER_PIN,setEMFlevel);
            pinMode(METER_PIN, INPUT);
        } else {

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
              default:
                  break;
            }

            lastProgValue++;
            if (lastProgValue > maxProg) {
                lastProgValue = 1;
            }
        } // end else
    } // if progPin
} // end loop


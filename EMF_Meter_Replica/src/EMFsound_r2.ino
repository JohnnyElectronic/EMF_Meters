/******************************************************************************************
Supernatural EMF Meter Replica

EMF ATTiny85 Code
Release: 2.0 (Label: EMF / r2)


Compatible with PCB version r-1 (based on modifications) using EMF_V1_2 define
- 1.1 reduces the gap between high tone track play
- 1.2 requires jumper between ATTiny85 pin 6 to DFPlayer pin 3

Compatible with PCB version r-2 using EMF_V2_0 define
- 2.0 New ATTiny85 pin mapping for r2 PCB

Johnny Electronic
https://github.com/JohnnyElectronic
https://www.youtube.com/@Johnny_Electronic

Description **********************************************
This code uses an ATTiny 85 to monitor EMF levels from the core logic and control a 
DF Player audio device (MP3Player). The EMF level (or test button) will determine the sound file 
to be played and the voltage applied to a LED Display and VU meter element.

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
DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/product-1121.html>

 1.Connection and Diagrams can be found here
 <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>

 2. The DF files used in this version are a custom creation.
*******************************************************************************************/

#include "DFPlayer.h"
#include <EEPROM.h>

//#define DFP_DEBUG     /* Enables display of received data for query requests to Serial, Nano only */
//#define EMF_DEBUG     /* Enables display of EMF data for debug, Nano only */

//#define NANO
#define ATTINY

//#define EMF_V1_2        /* Enable version 1.2 code, stores changed volume level every 5s, reloads between power cycles, requires PCB r1 */
#define EMF_V2_0        /* Enable version 2.0 code, all features of 1.2 w/ new ATTiny 85 SOIC-8 pin mapping, requires PCB r2 */

// Not all pins on the Mega and Mega 2560 boards support change interrupts, 
// so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69). 

// Nano Pinouts
//DFPlayer.h settings
#ifdef NANO
#define  DFP_RX_PIN   12     // Pin Assigned for serial RX, set to -1 if not used - D12 Uno/Nano
#define  DFP_TX_PIN    8     // Pin Assigned for serial TX  - D8 Uno/Nano
#define  DFP_BUSY_PIN  2     // Pin Assigned for busy detection, set to -1 if not used
#define  SENSOR_PIN   A1     // EMF input level
#define  METER_PIN    11     // PWM output to drive VU meter
#endif

// ATtiny85 Pinouts
// ---------------------------------------------------------
// ATTny85: AVR_ATtiny85
//                          _____
// (Reset)  (A0) PB5  5   1|*    |8  VCC
// (Xtal)   (A3) PB3  3   2|     |7  2   PB2 (A1)(SCK)
// (Xtal)   (A2) PB4  4   3|     |6  1~  PB1     (MISO)
//                   GND  4|_____|5  0~  PB0     (MOSI)
// 
// ---------------------------------------------------------
#ifdef ATTINY

#ifdef EMF_V2_0
    #define DFP_BUSY_PIN 0     // ATTiny85 PB0 (Pin 5), pin Assigned for busy detection, set to -1 if not used
    #define DFP_TX_PIN   3     // ATTiny85 PB3 (Pin 2), pin Assigned for serial TX, Connects to module's RX pin 2
    #define DFP_RX_PIN   4     // ATTiny85 PB4 (Pin 3), pin Assigned for serial RX, Connects to module's TX pin 3, set to -1 if not used
    #define SENSOR_PIN   A1    // ATTiny85 A1  (Pin 7), EMF input level
    #define METER_PIN    1     // ATTiny85 PB1 (Pin 6), PWM output to drive VU/LED meter (Power up pulse and TBD usage)
#else
    #define DFP_BUSY_PIN  3     // ATTiny85 PB3 (Pin 2), pin Assigned for busy detection, set to -1 if not used
    #define DFP_TX_PIN    4     // ATTiny85 PB4 (Pin 3), pin Assigned for serial TX, Connects to module's RX pin 2
    #define DFP_RX_PIN    1     // ATTiny85 PB1 (Pin 6), pin Assigned for serial RX, Connects to module's TX pin 3, set to -1 if not used
    #define SENSOR_PIN   A1     // ATTiny85 A1  (Pin 7), EMF input level
    #define METER_PIN    0      // ATTiny85 PB0 (Pin 5), PWM output to drive VU meter
#endif

#endif

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

// EMF States
#define EMF_DETECT  0
#define EMF_FOUND   1

/* EMF Levels (1.3v max scale) mapped to input voltage and LED indication
 LED   |  Voltage | Sensor Level
            0         0
  1         0.260     53
            0.300     60
            0.500     103
  2         0.530     110
            0.602     125 
            0.700     146
  3         0.750     157
            1v        210
  4         1.125     235          
  5         1.250     261
            1.30      271
            2v        421
            2.5       525
            3.0       633
*/

/* EMF Levels (3v max scale) mapped to input voltage and LED indication
 LED   |  Voltage | Sensor Level
            0         0
  1         0.60     125
  2         1.2      251
  3         1.8      380
  4         2.4      504      
  5         3.2      632
*/

/* Set values for 1.5v, 3v, or 5v scale range */
#define MAX_METER_LVL   158    /* Set to 80 for 1.5v, 158 for 3v, 255 for 5v scale */
#define MTR_TEST_LEVEL  118    /* Set to 50 for 1.5v, 118 for 3v, 215 for 5v scale */
#define EMF_LEVEL_LOW   250    /* Set to 110 for 1.5v scale or 250 for 3v scale (Approximatley LED 2) */
#define EMF_LEVEL_HIGH  440    /* Set to 200 for 1.5v scale or 440 for 3v scale (Approximatley LED 3 1/2) */

#define EMF_TONE_STEADYL_DURATION  1200    /* Maximum time until replay */   

#define AUDIO_LEVEL_ADDR     0          // Memory address of EMF audio level setting
#define EMF_AUDIO_LEVEL     20          // Typical 15-20 to start
#define EMF_MAX_AUDIO_LEVEL 30          // Highest Level
#define EMF_MIN_AUDIO_LEVEL  4          // Lowest Audible Level
#define EMF_VOL_SAVE_DELAY   5000       // Check for save every 5s 
byte emfVolumeLev = EMF_AUDIO_LEVEL;    // Current or initial DFP volume level for EMF

const int sensorPin = SENSOR_PIN; // select the input pin for the VU level
const int meterPin = METER_PIN;   // Output to VU meter
const int emfDelay = 100;         // 100ms for default freq. Wait delay for checking EMF levels for audio only, 50-200 range, 200 decent but slows mid level sound.
const int meterMin =  20;         // Minimum level for meter to respond, shuts down PWM when below this level.

int emfDetected = 0;              // Current state of EMF sound to signal detection
int sensorValue = 0;              // variable to store the value coming from the sensor
int meterOutputValue = 0;         // variable to store the converted meter value
int lastSensorValue = 0;          // variable to store the last value coming from the sensor
unsigned long emfTimer = 0;       // Delay timer
unsigned long emfHighTimer = 0;   // Delay timer
bool emfHighZone = false;         // Tracks if max EMF levels

#if defined(DFP_DEBUG) || defined(EMF_V1_2) || defined(EMF_V2_0)
int queryVal;
#endif

#if defined(EMF_V1_2) || defined(EMF_V2_0)
unsigned long emfVolumeSaveTimer = 0;   // Delay timer
#endif

#ifdef EMF_DEBUG
int lastMP3value = 0;      // variable to store the value of the MP3 played, debug only
#endif

void setup()
{
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
    dfpSetup(DFP_BUSY_PIN, 1000);

#if defined(EMF_V1_2) || defined(EMF_V2_0)   /* Save off volume levels between power cycles */
    emfVolumeLev = EEPROM.read(AUDIO_LEVEL_ADDR);

    if ((emfVolumeLev < EMF_MIN_AUDIO_LEVEL) || (emfVolumeLev > EMF_MAX_AUDIO_LEVEL)) {
        // Likely first run, set to a reasonable value. If audio too low, reset to default
        emfVolumeLev = EMF_AUDIO_LEVEL;   
    }
#endif

    dfpSetVolume(emfVolumeLev);
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

#if defined(EMF_V1_2) || defined(EMF_V2_0)   /* Set save timer for volume levels between power cycles, deflect meter */
//  analogWrite(meterPin, map(emfVolumeLev, 0, 30, 0, MAX_METER_LVL));   /* Show volume level as a defletion of the VU meter */
  analogWrite(meterPin, MTR_TEST_LEVEL);
  emfVolumeSaveTimer = millis();
#else
  analogWrite(meterPin, MTR_TEST_LEVEL);
#endif

  delay (1500); 
  analogWrite(meterPin,0);
  pinMode(meterPin, INPUT);   // Shut down output 
  delay (400);
  emfTimer = millis();
}

void loop()
{
  sensorValue = analogRead(sensorPin);   // 0 - 1023

#ifdef EMF_V1_2 
  // Setup meter output. Scale the analog value, and write to the meter output pin based on the selected scale range (1.3v / 3v / 5v)
  meterOutputValue = sensorValue / 4;
  // meterOutputValue = map(sensorValue, 0, 632, 0, 255);  /* For 5v mapping */
  meterOutputValue = constrain(meterOutputValue, 0, MAX_METER_LVL);

  if (meterOutputValue < meterMin) {
    pinMode(meterPin, INPUT);   // Shut down output
  } else {
    analogWrite(meterPin,meterOutputValue);
  }
#endif

#ifdef EMF_DEBUG
  if (sensorValue != lastSensorValue) {
      Serial.print(F("Sen:"));
      Serial.println(sensorValue);

      Serial.print(F("Last Sen:"));
      Serial.println(lastSensorValue);

      Serial.print(F("EMF Detect:"));
      Serial.println(emfDetected);
  }
      Serial.print(F("MP3:"));
      Serial.println(lastMP3value);
#endif

    if (millis() - emfTimer > emfDelay) {

      /* Checking for valid signal detected, startup */
      if (emfDetected == EMF_DETECT) {
        if (sensorValue > EMF_LEVEL_LOW) {
            emfDetected = EMF_FOUND;
            dfpPlayTrackMP3(EMF_TONE_START);

    #ifdef EMF_DEBUG
    lastMP3value = EMF_TONE_START;
    #endif
        } /* end if EMF_DETECT */

        emfHighZone = false;
      } else if (emfDetected == EMF_FOUND) {
        // Low detection state
        if (sensorValue <= EMF_LEVEL_LOW) {
          // Going back down
          dfpPlayTrackMP3(EMF_TONE_END);

    #ifdef EMF_DEBUG
    lastMP3value = EMF_TONE_END;
    #endif

          emfDetected = EMF_DETECT;  /* Reset back to detect state */
          emfHighZone = false;
        } else if ((sensorValue > EMF_LEVEL_LOW) && (sensorValue < EMF_LEVEL_HIGH)) {
          // Low steady detection state
          if (sensorValue >= lastSensorValue) {
            lastSensorValue = sensorValue;
            delay (10);
            sensorValue = analogRead(sensorPin);
            if (sensorValue >= lastSensorValue) {
              // Still mid range or higher
              dfpPlayTrackMP3(EMF_TONE_LOW);
            }
          }
          // Else we should be coming down in levels so let the next cycle catch it.

    #ifdef EMF_DEBUG
    lastMP3value = EMF_TONE_LOW;
    #endif
    
          emfHighZone = false;
        } else {
          /* High detection state
            Check busy line - if still playing skip or if not play again 
            DFP_BUSY_PIN is active low on busy state */
            if (!emfHighZone) {
                emfHighTimer = millis();
                emfHighZone = true;
            }

            if (digitalRead(DFP_BUSY_PIN) || ((millis() - emfHighTimer) > EMF_TONE_STEADYL_DURATION))  {
                // Replay just under sound file play length, slightly faster than using busy pin
                dfpPlayTrackMP3(EMF_TONE_STEADYL);
                emfHighTimer = millis();
            } // end if DFP_BUSY_PIN / Timer Check
        } // end if High Level
      } // end if EMF_FOUND 
      
      emfTimer = millis();
  } // End if timer triggered

#if defined(EMF_V1_2) || defined(EMF_V2_0)
  if ((millis() - emfVolumeSaveTimer) > EMF_VOL_SAVE_DELAY) {

      // Check and save volume when idle
      if (emfDetected == EMF_DETECT) {
          /* Check volume level */           
          dfpSerialPurge ();    // Prep for volume read
          //  delay (100);
          queryVal = dfpGetVolume();

#ifdef DFP_DEBUG
          Serial.print(F("Current Vol:"));
          Serial.println(queryVal);
#endif
          if ((queryVal != emfVolumeLev) && (queryVal <= EMF_MAX_AUDIO_LEVEL) && (queryVal >= EMF_MIN_AUDIO_LEVEL)) {
              /* Update value */
              emfVolumeLev = queryVal;
              EEPROM.write(AUDIO_LEVEL_ADDR, emfVolumeLev);

#ifdef DFP_DEBUG
          Serial.println(F("Updating EPROM"));
#endif
          }
      }
      emfVolumeSaveTimer = millis();
  }
#endif

  lastSensorValue = sensorValue; 
}


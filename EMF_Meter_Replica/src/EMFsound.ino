/******************************************************************************************
Supernatural EMF Meter Replica

EMF ATTiny85 Code
Release: 1.0

Johnny Electronic
https://github.com/JohnnyElectronic
https://www.youtube.com/@Johnny_Electronic

Description **********************************************
This code uses an ATTiny 85 to control a DF Player audio device, monitor EMF levels from the core logic, 
and drive a VU meter. The EMF level will determine the sound file to be played and the 
voltage applied to a VU meter element.

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

 1.Connection and Diagram can be found here
 <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>
*******************************************************************************************/

#include "DFPlayer.h"

//#define DFP_DEBUG     /* Enables display of received data for query requests to Serial, Nano only */
//#define EMF_DEBUG     /* Enables display of EMF data for debug, Nano only */

//#define NANO
#define ATTINY

// Not all pins on the Mega and Mega 2560 boards support change interrupts, 
// so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69). 

// Nano Pinouts
//DFPlayer.h settings
#ifdef NANO
#define  DFP_RX_PIN   12     // Pin Assigned for serial RX, set to -1 if not used - D12 Uno/Nano
#define  DFP_TX_PIN    8     // Pin Assigned for serial TX  - D8 Uno/Nano
#define  DFP_BUSY_PIN  2     // Pin Assigned for busy detection, set to 0 if not used
#define  SENSOR_PIN   A1     // EMF input level
#define  METER_PIN    11     // PWM output to drive VU meter
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
 * sensorPin        - A1, Pin 7
 * meterPin         - D0, Pin 5
 */
#ifdef ATTINY
#define DFP_BUSY_PIN  3     // ATTiny85 PB3, pin 2, pin Assigned for busy detection
#define DFP_TX_PIN    4     // Pin Assigned for serial TX, Connects to module's RX, ATTiny85 PB4, pin 3
#define DFP_RX_PIN   -1     // Pin Assigned for serial RX, set to -1 if not used
#define SENSOR_PIN   A1     // A1, pin 7, EMF input level
#define METER_PIN    0      // D0, Pin 5, PWM output to drive VU meter
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

#define EMF_AUDIO_LEVEL 15
const int sensorPin = SENSOR_PIN; // select the input pin for the VU level
const int meterPin = METER_PIN;   // Output to VU meter
const int emfDelay = 100;         // 100 for default freq. Wait delay for checking EMF levels for audio only, 50-200 range, 200 decent but slows mid level sound.
const int meterMin =  20;         // Minimum level for meter to respond, shuts down PWM when below this level.

int emfDetected = 0;              // Current state of EMF sound to signal detection
int sensorValue = 0;              // variable to store the value coming from the sensor
int meterOutputValue = 0;         // variable to store the converted meter value
int lastSensorValue = 0;          // variable to store the last value coming from the sensor
unsigned long emfTimer = 0;       // Delay timer

#ifdef DFP_DEBUG
int queryVal;
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
  analogWrite(meterPin, MTR_TEST_LEVEL);
  delay (500); 
  analogWrite(meterPin,0);

  pinMode(meterPin, INPUT);   // Shut down output 

  emfTimer = millis();
}

void loop()
{
  sensorValue = analogRead(sensorPin);   // 0 - 1023

  // Setup meter output. Scale the analog value, and write to the meter output pin based on the selected scale range (1.3v / 3v / 5v)
  meterOutputValue = sensorValue / 4;
  // meterOutputValue = map(sensorValue, 0, 632, 0, 255);  /* For 5v mapping */
  meterOutputValue = constrain(meterOutputValue, 0, MAX_METER_LVL);

  if (meterOutputValue < meterMin) {
    pinMode(meterPin, INPUT);   // Shut down output
  } else {
    analogWrite(meterPin,meterOutputValue);
  }

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
      } else if (emfDetected == EMF_FOUND) {
        // Low detection state
        if (sensorValue <= EMF_LEVEL_LOW) {
          // Going back down
          dfpPlayTrackMP3(EMF_TONE_END);

    #ifdef EMF_DEBUG
    lastMP3value = EMF_TONE_END;
    #endif

          emfDetected = EMF_DETECT;  /* Reset back to detect state */
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

        } else {
          /* High detection state
            Check busy line - if still playing skip or if not play again 
            DFP_BUSY_PIN is active low on busy state */
          if (digitalRead(DFP_BUSY_PIN)) {
            dfpPlayTrackMP3(EMF_TONE_STEADYL);

    #ifdef EMF_DEBUG
    lastMP3value = EMF_TONE_STEADYL;
    #endif

          }
        }
      } /* end if EMF_FOUND */
      
      emfTimer = millis();
  } // End if timer triggered

  lastSensorValue = sensorValue; 
}


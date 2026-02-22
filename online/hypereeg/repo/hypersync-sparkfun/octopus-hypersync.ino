/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* This is the code written for a Sparkfun Pro Micro device.
 * It is basically for sending a byte to the device which in it parallelize the
 * 8-bits and sends from its 8-pins. These 8-pins are then sent to multiple
 * EEG amps at the same time to establish/guarantee data stream syncronization of
 * acquired data afterwards. The FreeCAD and KiCAD design files are separately
 * (and freely) available. */

// Pro Micro (ATmega32U4)

// Pins 2..9 used as 8-bit parallel trigger bus (bit0 -> D2, ..., bit7 -> D9)
const uint8_t TRIG_PINS[8]={2,3,4,5,6,7,8,9};

// Dedicated audio trigger pin (D10==A10)
const uint8_t AUDIO_TRIG_PIN=10;

const uint8_t LED_PIN=17;     // TX/RX LED on Pro Micro (active LOW)
const unsigned PULSE_US=2000; // 2 ms pulse

void setup() {
 for (uint8_t i=0;i<8;++i) {
  pinMode(TRIG_PINS[i],OUTPUT); digitalWrite(TRIG_PINS[i],LOW); // start LOW to avoid spurious highs
 }
 // Init audio trigger pin LOW
 pinMode(AUDIO_TRIG_PIN,OUTPUT); digitalWrite(AUDIO_TRIG_PIN,LOW);
 // LED Pin
 pinMode(LED_PIN,OUTPUT); digitalWrite(LED_PIN,HIGH); // start HIGH (active LOW)
 Serial.begin(115200);
 delay(200); // Let USB settle (200ms)
 Serial.println("OCTOPUS-HyperEEG role=SYNC unit=A proto=1 sig=OCTO-EEG-42");
}

// Drive bits simultaneously (sequential digitalWrite is fine at 2ms scale)
inline void writeParallel(uint8_t b) {
 for (uint8_t i=0;i<8;++i) digitalWrite(TRIG_PINS[i],(b>>i)&0x01);
}

inline void clearParallel() {
 for (uint8_t i=0;i<8;++i) digitalWrite(TRIG_PINS[i],LOW);
}

void loop() {
 if (!Serial.available()) return;

 int v=Serial.read(); if (v<0) return; // v can be -1 as errcode.. should not happen
 uint8_t inByte=static_cast<uint8_t>(v);

 // Emit pulse: raise audio trigger+bus,hold,clear all
 writeParallel(inByte); digitalWrite(AUDIO_TRIG_PIN,HIGH);
 delayMicroseconds(PULSE_US);
 clearParallel(); digitalWrite(AUDIO_TRIG_PIN,LOW);

 // LED Blink
 digitalWrite(LED_PIN,LOW); delayMicroseconds(200); digitalWrite(LED_PIN,HIGH);

 Serial.write(inByte); // Echo the same byte as ACK
}

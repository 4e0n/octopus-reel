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

unsigned int i; char inChar;

void setup() {
 Serial.begin(9600);
 pinMode(17,OUTPUT);
 inChar=0;
 for (i=0;i<8;i++) pinMode(2+i, OUTPUT);
 for (i=0;i<8;i++) digitalWrite(2+i, HIGH);
}

void loop() {
 while(Serial.available()) { inChar=(char)Serial.read();
  //for (i=0;i<8;i++) digitalWrite(9-i, (inChar>>i) & (0x01));
  for (i=0;i<8;i++) digitalWrite(2+i, (inChar>>i) & (0x01));
  inChar=0; delay(2); // 2ms trigger duration
  //for (i=0;i<8;i++) digitalWrite(9-i, LOW);
  for (i=0;i<8;i++) digitalWrite(2+i, LOW);
  //Serial.println("here");
  digitalWrite(17, LOW); delay(200); digitalWrite(17, HIGH);
 }
}

/*
void setup() {
  inChar=0;
  pinMode(17, OUTPUT); // pin 17 is RXLED
  Serial.begin(9600); //while(!Serial);
  //Serial1.begin(9600); //while(!Serial1);
  for (i=0;i<8;i++) pinMode(2+i, OUTPUT);
  for (i=0;i<8;i++) digitalWrite(2+i, LOW);
}

void serialEvent() {
 while (Serial.available()) inChar=(char)Serial.read();
}

//void serialEvent1() {
// while (Serial1.available()) inChar=(char)Serial1.read();
//}

void loop() {
 if (inChar!=0) { // Serial.print(inChar,HEX);
  Serial.println("here");
  digitalWrite(17, HIGH); delay(200); digitalWrite(17, LOW);
  //for (i=0;i<8;i++) digitalWrite(9-i, (inChar>>i) & (0x01));
  inChar=0; //delay(2); // 1ms trigger duration
  //for (i=0;i<8;i++) digitalWrite(9-i, LOW);
 }
}
*/

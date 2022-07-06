
/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* This is the arduino code for lab cabin lights. It is basically designed
   to be 4 separate LED arrays driven from a common supply. They can be individually
   activated/deactivated either by telneting to Arduino server (with a default
   IP of 10.0.10.7) or via DAQ card digital outputs (by default 4..7) */

#include <SPI.h>
#include <Ethernet.h>

byte mac[]={0xde,0xad,0xbe,0xef,0xfe,0xed};
IPAddress ip(10,0,10,7);
IPAddress myDns(10,0,10,10);
IPAddress gateway(10,0,10,1);
IPAddress subnet(255,255,255,0);

// telnet defaults to port 23
EthernetServer server(23);
const int MAX_CLIENT=8;
EthernetClient clients[MAX_CLIENT];

const int PIN_COUNT=4;
int relayPin[PIN_COUNT]={2,3,5,6};
int inputPin[PIN_COUNT]={8,9,10,11};

const int BUF_SIZE=80; byte buffer[BUF_SIZE]; int bufCount;
char inStrC[BUF_SIZE]; String inStr,cmdStr,paramStr;

String cmdQuitStr=String("quit");
String cmdSetRelayStr=String("setRelay");
String cmdResetRelayStr=String("resetRelay");

void setup() {
 // Relay setup
 for (int i=0;i<PIN_COUNT;i++) { pinMode(relayPin[i],OUTPUT); digitalWrite(relayPin[i],HIGH); }
 
 // You can use Ethernet.init(pin) to configure the CS pin
 Ethernet.init(10);  // Most Arduino shields
 Ethernet.begin(mac,ip,myDns,gateway,subnet);
 Serial.begin(9600);
 while (!Serial); // wait for serial port to connect. Needed for native USB port only

 if (Ethernet.hardwareStatus()==EthernetNoHardware) {
  Serial.println("Error: Ethernet shield not found.");
  while (true) delay(1);
 }
 if (Ethernet.linkStatus()==LinkOFF) Serial.println("Ethernet cable is not connected.");

 server.begin(); // Start listening for clients
 Serial.print("Server address:"); Serial.println(Ethernet.localIP());
}

void loop() {
 EthernetClient newClient=server.accept();
 if (newClient) {
  for (byte i=0;i<MAX_CLIENT;i++) {
   if (!clients[i]) {
    Serial.print("New client: #"); Serial.println(i);
    newClient.print("Welcome to Octopus Lab Environment Server (10.0.10.7).\n");
    newClient.print("Commands:\nquit: Quits.\nsetRelay <#>\nresetRelay <#>\n----\n");
    clients[i]=newClient;
    //clients[i].write(">");
    break;
   }
  }
 }
 //else {
 // for (byte i=0;i<PIN_COUNT;i++) {
 //  if (digitalRead(inputPin[i])) digitalWrite(relayPin[i],LOW); else digitalWrite(relayPin[i],HIGH);
 // }
 //}

 for (byte i=0;i<MAX_CLIENT;i++) {
  if (clients[i] && clients[i].available()>0) {
   bufCount=clients[i].read(buffer,BUF_SIZE);
   for (byte j=0;j<bufCount;j++) inStrC[j]=(char)buffer[j]; inStrC[bufCount-2]='\0';
   inStr=String(inStrC); inStr.trim();
   byte idx=inStr.indexOf(' ');
   if (idx==-1) cmdStr=inStr;
   else {
    cmdStr=inStr.substring(0,idx);
    paramStr=inStr.substring(idx+1,inStr.length());
   }

   // Parsed .. now interpret ..
   if (cmdStr.equals(cmdQuitStr)) {
    clients[i].stop();
    Serial.print("Disconnect client #"); Serial.println(i);
   } else if (cmdStr.equals(cmdSetRelayStr)) {
    byte relayNo=paramStr.toInt();
    digitalWrite(relayPin[relayNo],LOW);
    Serial.print("Relay on #"); Serial.println(relayNo);
   } else if (cmdStr.equals(cmdResetRelayStr)) {
    byte relayNo=paramStr.toInt();
    digitalWrite(relayPin[relayNo],HIGH);
    Serial.print("Relay off #"); Serial.println(relayNo);
   }
   clients[i].write(">");
  }
 }

 // stop any clients which disconnect
 for (byte i=0;i<MAX_CLIENT;i++) {
  if (clients[i] && !clients[i].connected()) {
   Serial.print("disconnect client #"); Serial.println(i);
   clients[i].stop();
  }
 }
}

//   Serial.print(cmdStr); Serial.println((byte)cmdStr.length());


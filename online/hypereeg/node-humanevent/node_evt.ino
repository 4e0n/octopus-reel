/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

/*
 * Octopus / HyperEEG
 * Ethernet-based Subject Event Network Relay Box
 * ----------------------------------------------
 * Dynamic network version (standard Octopus local net):
 * - Arduino gets its own (MAC-based) IP by DHCP from Leviathan
 * - node-acq hostname is resolved at startup via DNS
 * - resolved IP is cached and used later
 * - keep-alive: if reconnects keep failing, hostname is resolved again
 * ----------------------------------------------
 * Hardware:
 * - Arduino Uno w/Ethernet shield (W5100)
 * - 2 buttons x 2 subjects = 4 buttons (active-low)
 * - LED on pin 13 -> trigger feedback
 * ----------------------------------------------
 * Events -> (subNo<<10)|evtCode
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Dns.h>

// -----------------------------------------------------------------------------
// Network configuration
// -----------------------------------------------------------------------------

byte mac[]={0xDE,0xAD,0xBE,0xEF,0xFE,0xED};

// node-acq identity
const char serverHost[]="acq.octopus0";
const uint16_t serverPort=65000;

// Cached resolved IP of acq.octopus0
IPAddress serverIp;
bool serverIpValid=false;

EthernetClient client;
DNSClient dnsClient;

// -----------------------------------------------------------------------------
// Subject / trigger packing
// -----------------------------------------------------------------------------

const uint16_t subNo=1;   // hardcoded box / subject identifier for now

static inline uint16_t packSubEvt(uint16_t subjectNo,uint16_t trigCode) {
 return uint16_t((subjectNo<<10)|(trigCode&0x03ff));
}

// -----------------------------------------------------------------------------
// Input configuration
// -----------------------------------------------------------------------------

const uint8_t INPUT_COUNT=4;
const uint8_t inputPins[INPUT_COUNT]={2,3,6,7};
const uint16_t trigCodes[INPUT_COUNT]={1,2,3,4};

// Active-low with INPUT_PULLUP:
// idle    -> HIGH
// pressed -> LOW
const unsigned long debounceMs=40;

// -----------------------------------------------------------------------------
// LED feedback
// -----------------------------------------------------------------------------

const uint8_t LED_PIN=13;
unsigned long ledOffTimeMs=0;
const unsigned long ledBlinkMs=40;

// -----------------------------------------------------------------------------
// Reconnect / anti-hang timing
// -----------------------------------------------------------------------------

const unsigned long reconnectIntervalMs=2000; // retry connect every 2s
const unsigned long socketRefreshMs=30000;    // refresh socket every 30s
const unsigned long reResolveAfterMs=10000;   // after 10s of failure, resolve hostname again

unsigned long lastReconnectAttemptMs=0;
unsigned long lastSocketActivityMs=0;
unsigned long firstConnectFailMs=0;

// -----------------------------------------------------------------------------
// Per-button debounce state
// -----------------------------------------------------------------------------

bool stablePressed[INPUT_COUNT];
bool lastRawPressed[INPUT_COUNT];
unsigned long lastRawChangeMs[INPUT_COUNT];

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void blinkLED() {
 digitalWrite(LED_PIN,HIGH);
 ledOffTimeMs=millis()+ledBlinkMs;
}

void updateLED() {
 if (ledOffTimeMs!=0) {
  unsigned long now=millis();
  if ((long)(now-ledOffTimeMs)>=0) {
   digitalWrite(LED_PIN,LOW);
   ledOffTimeMs=0;
  }
 }
}

bool resolveServerHost() {
 IPAddress dnsServer=Ethernet.dnsServerIP();
 if (dnsServer==INADDR_NONE) {
  Serial.println(F("[DNS] No DNS server from DHCP."));
  return false;
 }
 dnsClient.begin(dnsServer);
 Serial.print(F("[DNS] Resolving ")); Serial.println(serverHost);
 if (dnsClient.getHostByName(serverHost,serverIp)==1) {
  serverIpValid=true;
  Serial.print(F("[DNS] Resolved to ")); Serial.println(serverIp);
  return true;
 }
 serverIpValid=false;
 Serial.println(F("[DNS] Resolution failed."));
 return false;
}

bool bringUpEthernetDHCP() {
 Ethernet.init(10); // standard W5100 CS pin
 Serial.println(F("[NET] Starting DHCP..."));
 if (Ethernet.begin(mac)==0) {
  Serial.println(F("[NET] DHCP failed."));
  return false;
 }
 Ethernet.setRetransmissionTimeout(2000);
 Ethernet.setRetransmissionCount(3);
 delay(1000);
 if (Ethernet.hardwareStatus()==EthernetNoHardware) {
  Serial.println(F("[ERR] Ethernet shield not found."));
  while (true) delay(1);
 }
 if (Ethernet.linkStatus()==LinkOFF) {
  Serial.println(F("[WARN] Ethernet cable not connected."));
 }
 Serial.print(F("[NET] Local IP: "));   Serial.println(Ethernet.localIP());
 Serial.print(F("[NET] DNS IP: "));     Serial.println(Ethernet.dnsServerIP());
 Serial.print(F("[NET] Gateway IP: ")); Serial.println(Ethernet.gatewayIP());

 return true;
}

bool ensureServerResolved() {
 if (serverIpValid) return true;
 return resolveServerHost();
}

bool ensureConnected() {
 if (client.connected()) return true;
 unsigned long now=millis();
 if ((now-lastReconnectAttemptMs)<reconnectIntervalMs) return false;
 lastReconnectAttemptMs=now;
 if (!serverIpValid) {
  if (!resolveServerHost()) return false;
 }
 client.stop();
 Serial.print(F("[NET] Connecting to ")); Serial.print(serverHost);
 Serial.print(F(" [")); Serial.print(serverIp); Serial.print(F("]:")); Serial.println(serverPort);
 if (client.connect(serverIp,serverPort)) {
  Serial.println(F("[NET] Connected."));
  lastSocketActivityMs=now;
  firstConnectFailMs=0;
  return true;
 }
 Serial.println(F("[NET] Connect failed."));
 if (firstConnectFailMs==0) firstConnectFailMs=now;
 if ((now-firstConnectFailMs)>=reResolveAfterMs) {
  Serial.println(F("[DNS] Re-resolving after repeated connect failures."));
  serverIpValid=false;
  resolveServerHost();
  firstConnectFailMs=now;
 }
 return false;
}

void refreshSocketIfNeeded() {
 if (!client.connected()) return;
 unsigned long now=millis();
 if ((now-lastSocketActivityMs)>=socketRefreshMs) {
  Serial.println(F("[NET] Refreshing socket."));
  client.stop();
  delay(50);
  ensureConnected();
  lastSocketActivityMs=millis();
 }
}

void drainIncomingReplies() {
 while (client.connected() && client.available()>0) {
  char c=client.read();
  Serial.write(c);
  lastSocketActivityMs=millis();
 }
}

bool sendSubEvt(uint16_t subjectNo,uint16_t trigCode) {
 if (!client.connected()) return false;
 const uint16_t packed = packSubEvt(subjectNo,trigCode);
 client.print(F("SUBEVT=")); client.print(packed); client.print('\n');
 lastSocketActivityMs=millis();
 Serial.print(F("[SUBEVT] subNo=")); Serial.print(subjectNo); Serial.print(F(" trigCode="));
 Serial.print(trigCode); Serial.print(F(" packed=")); Serial.println(packed);
 return true;
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup() {
 Serial.begin(9600);
 delay(300);

 Serial.println("HELLO!");
 
 pinMode(LED_PIN,OUTPUT);
 digitalWrite(LED_PIN,LOW);
 for (uint8_t i=0;i<INPUT_COUNT;++i) {
  pinMode(inputPins[i],INPUT_PULLUP);
  const bool pressed=(digitalRead(inputPins[i])==LOW);
  stablePressed[i]=pressed;
  lastRawPressed[i]=pressed;
  lastRawChangeMs[i]=millis();
 }
 if (!bringUpEthernetDHCP()) {
  Serial.println(F("[FATAL] Ethernet/DHCP init failed."));
  while (true) delay(1000);
 }
 resolveServerHost();
 ensureConnected();
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------

void loop() {
 updateLED();
 ensureConnected();
 drainIncomingReplies();
 refreshSocketIfNeeded();
 const unsigned long now = millis();
 for (uint8_t i=0;i<INPUT_COUNT;++i) {
  const bool rawPressed=(digitalRead(inputPins[i])==LOW);  // active-low
  if (rawPressed!=lastRawPressed[i]) {
   lastRawPressed[i]=rawPressed;
   lastRawChangeMs[i]=now;
  }
  if ((now-lastRawChangeMs[i])>=debounceMs && stablePressed[i]!=rawPressed) {
   stablePressed[i]=rawPressed;
   // send only on press, not on release
   if (stablePressed[i]) {
    if (sendSubEvt(subNo,trigCodes[i])) {
     blinkLED();
    } else {
     Serial.print(F("[WARN] Event not sent, disconnected. trigCode=")); Serial.println(trigCodes[i]);
    }
   }
  }
 }
}

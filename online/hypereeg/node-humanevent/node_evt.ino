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
//const char serverHost[]="vacq.octopus0";

const uint16_t serverPort=65000;

// Cached resolved IP of vacq.octopus0
IPAddress serverIp; bool serverIpValid=false;

EthernetClient client; DNSClient dnsClient;

// -----------------------------------------------------------------------------
// Subject / trigger packing
// -----------------------------------------------------------------------------

const uint16_t subNo=1; // hardcoded box / subject identifier for now

static inline uint16_t packSubEvt(uint16_t subjectNo,uint16_t trigCode) {
 return uint16_t((subjectNo<<10)|(trigCode&0x03ff));
}

// -----------------------------------------------------------------------------
// Input configuration
// -----------------------------------------------------------------------------

const uint8_t INPUT_COUNT=4;
const uint8_t inputPins[INPUT_COUNT]={2,3,6,7};
const uint16_t trigCodes[INPUT_COUNT]={1,2,3,4};

// Active-low with INPUT_PULLUP: idle->HIGH -- pressed->LOW
const unsigned long debounceMs=40;

// -----------------------------------------------------------------------------
// LED feedback
// -----------------------------------------------------------------------------

const uint8_t LED_PIN=8; // we should avoid pin 13 (SPI SCK)
unsigned long ledOffTimeMs=0;
const unsigned long ledBlinkMs=40;

// -----------------------------------------------------------------------------
// Non-blocking-ish service timing
// -----------------------------------------------------------------------------

const unsigned long dhcpRetryIntervalMs=10000;  // don't hammer DHCP
const unsigned long postDhcpGraceMs=2500;       // allow network to settle
const unsigned long reconnectIntervalMs=2000;   // TCP reconnect retry
const unsigned long reResolveAfterMs=10000;     // repeated TCP fail => resolve again
const unsigned long dnsRetryIntervalMs=5000;    // DNS retry interval
const unsigned long statusPrintIntervalMs=5000; // periodic status line
const unsigned long socketRefreshMs=0;          // 0 => disabled

unsigned long lastDhcpAttemptMs=0;
unsigned long dhcpAcquiredMs=0;
unsigned long lastReconnectAttemptMs=0;
unsigned long lastSocketActivityMs=0;
unsigned long firstConnectFailMs=0;
unsigned long lastDnsAttemptMs=0;
unsigned long lastStatusPrintMs=0;

// -----------------------------------------------------------------------------
// Network state
// -----------------------------------------------------------------------------

bool ethernetConfigured=false;

// -----------------------------------------------------------------------------
// Per-button debounce state
// -----------------------------------------------------------------------------

bool stablePressed[INPUT_COUNT];
bool lastRawPressed[INPUT_COUNT];
unsigned long lastRawChangeMs[INPUT_COUNT];

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void blinkLED() { digitalWrite(LED_PIN,HIGH); ledOffTimeMs=millis()+ledBlinkMs; }

void updateLED() {
 if (ledOffTimeMs!=0) {
  unsigned long now=millis();
  if ((long)(now-ledOffTimeMs)>=0) { digitalWrite(LED_PIN,LOW); ledOffTimeMs=0; }
 }
}

void printStatusPeriodically() {
 unsigned long now=millis();
 if ((now-lastStatusPrintMs)<statusPrintIntervalMs) return;
 lastStatusPrintMs=now;
 Serial.print(F("[STAT] eth=")); Serial.print(ethernetConfigured ? F("UP"):F("DOWN"));
 Serial.print(F(" dns=")); Serial.print(serverIpValid ? F("OK"):F("NO"));
 Serial.print(F(" tcp=")); Serial.print(client.connected() ? F("ON"):F("OFF"));
 Serial.print(F(" ip=")); Serial.print(Ethernet.localIP());
 if (serverIpValid) { Serial.print(F(" srv=")); Serial.print(serverIp); }
 Serial.println();
}

void stopClientCleanly() {
 if (client.connected()) { client.stop(); }
 else { client.stop(); }
}

bool networkGraceElapsed() {
 if (!ethernetConfigured) return false;
 unsigned long now=millis();
 return ((now-dhcpAcquiredMs)>=postDhcpGraceMs);
}

bool resolveServerHost() {
 if (!ethernetConfigured) return false;

 IPAddress dnsServer=Ethernet.dnsServerIP();
 if (dnsServer==IPAddress(0,0,0,0)) {
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

 serverIpValid=false; Serial.println(F("[DNS] Resolution failed."));
 return false;
}

void serviceDhcp() {
 if (ethernetConfigured) return;

 unsigned long now=millis();
 if ((now-lastDhcpAttemptMs)<dhcpRetryIntervalMs) return;
 lastDhcpAttemptMs=now;

 Serial.println(F("[NET] Starting DHCP..."));

 // Disable SD card chip select even if unused
 pinMode(4,OUTPUT); digitalWrite(4,HIGH);

 Ethernet.init(10); // standard W5100 CS pin

 // Do DHCP first
 if (Ethernet.begin(mac)==0) {
  Serial.println(F("[NET] DHCP failed."));
  ethernetConfigured=false; serverIpValid=false;
  stopClientCleanly();
  return;
 }

 // Now the chip state is much more reliable to query
 if (Ethernet.hardwareStatus()==EthernetNoHardware) {
  Serial.println(F("[ERR] Ethernet shield not found."));
  ethernetConfigured=false; serverIpValid=false;
  stopClientCleanly();
  return;
 }

 Ethernet.setRetransmissionTimeout(2000); Ethernet.setRetransmissionCount(3);

 ethernetConfigured=true; serverIpValid=false;
 dhcpAcquiredMs=millis(); lastSocketActivityMs=millis();
 firstConnectFailMs=0;

 Serial.print(F("[NET] Local IP: ")); Serial.println(Ethernet.localIP());
 Serial.print(F("[NET] DNS IP: ")); Serial.println(Ethernet.dnsServerIP());
 Serial.print(F("[NET] Gateway IP: ")); Serial.println(Ethernet.gatewayIP());

 EthernetLinkStatus ls=Ethernet.linkStatus();
 if (ls==LinkON) Serial.println(F("[NET] Link: ON"));
 else if (ls == LinkOFF) Serial.println(F("[NET] Link: OFF"));
 else Serial.println(F("[NET] Link: UNKNOWN"));
}

void serviceDns() {
 if (!ethernetConfigured) return;
 if (serverIpValid) return;
 if (!networkGraceElapsed()) return;

 unsigned long now=millis();
 if ((now-lastDnsAttemptMs)<dnsRetryIntervalMs) return;
 lastDnsAttemptMs=now;

 resolveServerHost();
}

void serviceTcpConnect() {
 if (!ethernetConfigured) return;
 if (!networkGraceElapsed()) return;
 if (!serverIpValid) return;
 if (client.connected()) return;

 unsigned long now=millis();
 if ((now-lastReconnectAttemptMs)<reconnectIntervalMs) return;
 lastReconnectAttemptMs=now;

 stopClientCleanly();

 Serial.print(F("[NET] Connecting to ")); Serial.print(serverHost);
 Serial.print(F(" [")); Serial.print(serverIp);
 Serial.print(F("]:")); Serial.println(serverPort);

 if (client.connect(serverIp,serverPort)) {
  Serial.println(F("[NET] Connected."));
  lastSocketActivityMs=now; firstConnectFailMs=0;
  return;
 }

 Serial.println(F("[NET] Connect failed."));

 if (firstConnectFailMs==0) firstConnectFailMs=now;

 if ((now-firstConnectFailMs)>=reResolveAfterMs) {
  Serial.println(F("[DNS] Re-resolving after repeated connect failures."));
  serverIpValid=false;
  firstConnectFailMs=now;
 }
}

void refreshSocketIfNeeded() {
 if (socketRefreshMs==0) return;
 if (!client.connected()) return;

 unsigned long now=millis();
 if ((now-lastSocketActivityMs)>=socketRefreshMs) {
  Serial.println(F("[NET] Refreshing socket."));
  stopClientCleanly();
  lastSocketActivityMs=millis();
 }
}

void drainIncomingReplies() {
 while (client.connected() && client.available()>0) {
  char c=client.read(); Serial.write(c);
  lastSocketActivityMs=millis();
 }
}

bool sendSubEvt(uint16_t subjectNo,uint16_t trigCode) {
 if (!client.connected()) return false;
 const uint16_t packed=packSubEvt(subjectNo,trigCode);

 client.print(F("SUBEVT=")); client.print(packed); client.print('\n');

 lastSocketActivityMs=millis();

 Serial.print(F("[SUBEVT] subNo=")); Serial.print(subjectNo);
 Serial.print(F(" trigCode=")); Serial.print(trigCode);
 Serial.print(F(" packed=")); Serial.println(packed);

 return true;
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup() {
 Serial.begin(9600); delay(300);
 Serial.println(F("[INFO] HUMEVT booting..."));
 pinMode(LED_PIN,OUTPUT); digitalWrite(LED_PIN,LOW);
 pinMode(4,OUTPUT); digitalWrite(4,HIGH); // Disable SD card CS
 for (uint8_t i=0;i<INPUT_COUNT;++i) {
  pinMode(inputPins[i],INPUT_PULLUP);
  const bool pressed=(digitalRead(inputPins[i])==LOW);
  stablePressed[i]=pressed; lastRawPressed[i]=pressed;
  lastRawChangeMs[i]=millis();
 }
 ethernetConfigured=false; serverIpValid=false;
 lastDhcpAttemptMs=0; dhcpAcquiredMs=0; lastReconnectAttemptMs=0;
 lastSocketActivityMs=0; firstConnectFailMs=0; lastDnsAttemptMs=0; lastStatusPrintMs=0;
 // Important: no blocking DHCP/DNS/TCP work here.
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------

void loop() {
 updateLED();

 // Always keep inputs responsive
 const unsigned long now=millis();

 for (uint8_t i=0;i<INPUT_COUNT;++i) {
  const bool rawPressed=(digitalRead(inputPins[i])==LOW); // active-low

  if (rawPressed!=lastRawPressed[i]) {
   lastRawPressed[i]=rawPressed; lastRawChangeMs[i]=now;
  }

  if ((now-lastRawChangeMs[i])>=debounceMs && stablePressed[i]!=rawPressed) {
   stablePressed[i]=rawPressed;
   // send only on press, not on release
   if (stablePressed[i]) {
    //Serial.println("[INFO] PRESSED ANY!");
    if (sendSubEvt(subNo,trigCodes[i])) {
     //Serial.println(F("[INFO] Event sent: trigCode="));
     blinkLED();
    } else {
     Serial.print(F("[WARN] Event not sent, disconnected. trigCode="));
     Serial.println(trigCodes[i]);
    }
   }
  }
 }

 // Meanwhile network services continue running
 serviceDhcp();
 serviceDns();
 serviceTcpConnect();
 drainIncomingReplies();
 refreshSocketIfNeeded();
 printStatusPeriodically();
}

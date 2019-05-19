/* This software, the ideas and concepts is Copyright (c) David Bird 2019 and beyond.
 *  All rights to this software are reserved. 
 *  It is prohibited to redistribute or reproduce of any part or all of the software contents in any form other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.
 5. You MUST include all of this copyright and permission notice ('as annotated') and this shall be included in all copies or substantial portions of the software 
    and where the software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT.
 FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #define SDA D3   // Choose your pins according to your boards SDA and SCL lines
 #define SCL D4   // Pins for the ESP8266 for direct plug-in, use e.g. dupont cables for other connections
#else
 #include <WiFi.h>
  #define SDA 5  // Choose your pins according to your boards SDA and SCL lines
  #define SCL 4 
#endif

#include "time.h" 
#include <Wire.h>
#include "SSD1306.h"               // See https://github.com/squix78/esp8266-oled-ssd1306 or via Sketch/Include Library/Manage Libraries
SSD1306 display(0x3c, SDA, SCL);   // OLED display object definition (address, SDA, SCL)

// Change to your WiFi credentials and select your time zone
const char* ssid     = "your_PASSWORD";
const char* password = "your_SSID";
const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";       // UK

//Example time zones
//const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";     // UK
//const char* Timezone = "MET-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";       // Central Europe
//const char* Timezone = "EST-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "EST5EDT,M3.2.0,M11.1.0";           // EST USA  
//const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
//const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
//const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
//const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
//const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia

String Date_str, Time_str, Time_format;

void setup() {
  Serial.begin(115200);
  #if ESP8266
  Wire.begin(SDA, SCL);               // (sda,scl,bus_speed) Start the Wire service for the OLED display using pin=D4 for SCL and Pin-D3 for SDA
  #else
  Wire.begin(SDA, SCL, 100000);       // (sda,scl,bus_speed) Start the Wire service for the OLED display using pin=D4 for SCL and Pin-D3 for SDA
  #endif  
  display.init();                     // Initialise the display
  display.flipScreenVertically();     // In my case flip the screen around by 180°
  display.setContrast(255);           // If you want turn the display contrast down, 255 is maxium and 0 in minimum, in practice about 128 is OK
  display.setFont(ArialMT_Plain_16);  // Set the Font size
  StartWiFi();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1);
  Time_format = "I"; // or "I" for Imperial 12:00 PM format and Date format MM-DD-CCYY e.g. 12:30PM 31-Mar-2019
}

void loop() {
  UpdateLocalTime(Time_format);
  display.clear();
  display.drawString(20, 10, Date_str);
  if (Time_format == "I")
    display.drawString(20, 35, Time_str); // Adjust for addition of AM/PM indicator
  else
    display.drawString(35, 35, Time_str);
  display.display();
  delay(500);
}

//#########################################################################################
void UpdateLocalTime(String Format){
  time_t now;
  time(&now);
  //See http://www.cplusplus.com/reference/ctime/strftime/
  char hour_output[30], day_output[30];
  if (Format == "M") {
    strftime(day_output, 30, "%a  %d-%m-%y", localtime(&now)); // Formats date as: Sat 24-Jun-17
    strftime(hour_output, 30, "%T", localtime(&now));    // Formats time as: 14:05:49
  }
  else {
    strftime(day_output, 30, "%a  %m-%d-%y", localtime(&now)); // Formats date as: Sat Jun-24-17
    strftime(hour_output, 30, "%r", localtime(&now));          // Formats time as: 2:05:49pm
  }
  Date_str = day_output;
  Time_str = hour_output;
}
//#########################################################################################
void StartWiFi(){
 /* Set the ESP to be a WiFi-client, otherwise by default, it acts as ss both a client and an access-point
  *  and can cause network-issues with other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  Serial.print(F("\r\nConnecting to: ")); Serial.println(String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) { delay(500); Serial.print(F(".")); }
  Serial.println("WiFi connected to address: "+String(WiFi.localIP()));
}

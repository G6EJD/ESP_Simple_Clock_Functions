#ifdef ESP8266
#include <ESP8266WiFi.h>
#define SDA D3
#define SCL D4
#else
#include <WiFi.h>
#define SDA 5
#define SCL 4
#endif

#include "time.h"
#include <Wire.h>
#include "SSD1306.h"               // See https://github.com/squix78/esp8266-oled-ssd1306 or via Sketch/Include Library/Manage Libraries
SSD1306 display(0x3c, SDA, SCL);   // OLED display object definition (address, SDA, SCL)

// Change to your WiFi credentials and select your time zone
const char* ssid     = "your_SSID";
const char* password = "your_PASSWORD";
const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";       // UK

//Example time zones see: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
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

String Date_str, Time_str;
volatile unsigned int local_Unix_time = 0, next_update_due = 0;
volatile unsigned int update_duration = 60*60; // Time duration in seconds, so synchronise every hour

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
  // Now configure time services
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1);
  delay(1000); // Wait for time services
  // Now setup a timer interrupt to occur every 1-second, to keep seconds accurate
  Setup_Interrupts_and_Initialise_Clock();
}

void loop() {
  UpdateLocalTime();
  // At this point the variables 'Date_str' and 'Time_str' have the current date-time values
  display.clear();
  display.drawString(10, 10, Date_str);
  display.drawString(30, 35, Time_str); // Adjust for addition of AM/PM indicator
  display.display();
  Serial.println(Date_str+" "+Time_str);
  delay(500);
}

//#########################################################################################
void UpdateLocalTime() {
  time_t now;
  if (local_Unix_time > next_update_due) { // only get a time synchronisation from the NTP server every duration for update set
    time(&now);
    Serial.println("Synchronising local time, time error was: "+String(now-local_Unix_time));
    // If this displays a negative result the interrupt clock is running fast or positive running slow
    local_Unix_time = now;
    next_update_due = local_Unix_time + update_duration;
  } else now = local_Unix_time;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  char hour_output[30], day_output[30];
  strftime(day_output, 30, "%a  %d-%m-%y", localtime(&now)); // Formats date as: Sat 24-Jun-17
  strftime(hour_output, 30, "%T", localtime(&now));    // Formats time as: 14:05:49
  Date_str = day_output;
  Time_str = hour_output;
}
//#########################################################################################
void StartWiFi() {
  /* Set the ESP to be a WiFi-client, otherwise by default, it acts as ss both a client and an access-point
      and can cause network-issues with other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  Serial.print(F("\r\nConnecting to: ")); Serial.println(String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println("WiFi connected at: " + WiFi.localIP().toString());
}

//#########################################################################################
// Interrupt setup and local Unix Time initialisation
#ifdef ESP8266
void Setup_Interrupts_and_Initialise_Clock(){
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(timer0_ISR);
  timer0_write(ESP.getCycleCount() + 80000000L); // 80MHz == 1sec, some devices might be slower or faster than 80MHz, adjust for absolute time accuracy if required.
  interrupts();
  //Now get current Unix time and assign the value to local Unix time counter and start the clock.
  time_t now;
  time(&now);
  local_Unix_time = now + 1; // The addition of 1 counters the NTP setup time delay
  next_update_due = local_Unix_time + update_duration;
}
//#########################################################################################
// Interrupt service routine
void timer0_ISR (void) { // Every second come here to increment the local Unix time counter
  local_Unix_time++;
  timer0_write(ESP.getCycleCount() + 80000000L); // 80MHz == 1sec
}
#else
volatile int interruptCounter;
int totalInterruptCounter;
 
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
 
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  local_Unix_time++;
  portEXIT_CRITICAL_ISR(&timerMux);
}
 
void Setup_Interrupts_and_Initialise_Clock(){
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
  //Now get current Unix time and assign the value to local Unix time counter and start the clock.
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println(F("Failed to obtain time"));
  }
  time_t now;
  time(&now);
  local_Unix_time = now + 1; // The addition of 1 counters the NTP setup time delay
  next_update_due = local_Unix_time + update_duration;
}
#endif

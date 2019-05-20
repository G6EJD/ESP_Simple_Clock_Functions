#include <ESP8266WiFi.h> // or #include <WiFI.h> for ESP32
#include <time.h>
void setup(){
 Serial.begin(115200);
 WiFi.begin("your_ssid","your_password");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  setenv("TZ", "GMT0BST,M3.5.0/01,M10.5.0/02", 1);
}

void loop(){
   Serial.println(get_time());
}

String get_time(){
  time_t now;
  time(&now);
  char time_output[30];
  // See http://www.cplusplus.com/reference/ctime/strftime/ for strftime functions
  strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now)); 
  return String(time_output); // returns Sat 20-Apr-19 12:31:45
}

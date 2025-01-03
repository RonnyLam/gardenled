#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "Dusk2Dawn.h"
#include <WebServer.h>

  // set wifi credentials
  const char* ssid       = "xxx";
  const char* password   = "xxx";
  unsigned long previousMillis = 0;
  unsigned long interval = 30000;

  // set NTP variables
  const char* ntpServer = "europe.pool.ntp.org";


  // init time variables
  int minsToday;
  struct tm timeinfo;
  int sunChecked;
  int GNMrise;
  int GNMset;
  bool DST = 1;
  const int pwmFull = 127;
  const int pwmHigh = 32;
  const int pwmLow = 1;
  const int morning = 7*60;
  
  // setting PWM properties
  const int ledPin = 2;  // GPIO2
  const int freq = 300;
  const int ledChannel = 0;
  const int resolution = 7;

Dusk2Dawn GNM(53, 7, 1);

// Set web server port number to 80
WebServer server(80);

// Auxiliar variables to store the current output state
bool ledState = LOW;
unsigned long ledTime = 0;
  
  void setup()
  {
    
//    Serial.begin(115200);
    
    // configure LED PWM functionalitites
    ledcAttach(ledPin, freq, resolution);
    
    //connect to WiFi
//    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
//    while (WiFi.status() != WL_CONNECTED) {
//        delay(500);
//        Serial.print(".");
//    }
//    Serial.println(" CONNECTED");
//    Serial.println(WiFi.localIP());

    
    //init and get the time
    configTime(3600, DST*3600, ntpServer);

    server.on("/xxx", handle_OnConnect);
    server.on("/xxx/ledfull", handle_ledFull);
    server.on("/xxx/lednormal", handle_ledNormal);
    server.on("/xxx/leddemo", handle_ledDemo);
    server.on("/xxx/time", handle_time);
    server.on("/xxx/bright", handle_bright);
    server.onNotFound(handle_NotFound);
  
    server.begin();
//    Serial.println("HTTP server started");

    blinkDemo(); //end of setup
  }
  
  void loop()
  {
    unsigned long currentMillis = millis();
    // if WiFi is down, try reconnecting
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
//      Serial.print(millis());
//      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
    }
    
    getLocalTime(&timeinfo);
    
    if (sunChecked != (timeinfo.tm_mon+1) * (timeinfo.tm_mday+1) && timeinfo.tm_hour > 3) {
          if (timeinfo.tm_mon+1 > 3 && timeinfo.tm_mon+1 < 10) {DST = 1;}
          else if (timeinfo.tm_mon+1 == 3) {
                 if (timeinfo.tm_mday >= 25 && timeinfo.tm_mday - (timeinfo.tm_wday + 25) >= 0){DST = 1;}
            else {DST = 0;}
          }
          else if (timeinfo.tm_mon+1 == 10) {
                 if (timeinfo.tm_mday >= 25 && timeinfo.tm_mday - (timeinfo.tm_wday + 25) >= 0){DST = 0;}
            else {DST = 1;}
          }
          else {DST = 0;}
          configTime(3600, DST*3600, ntpServer);
          GNMrise = GNM.sunrise(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, DST);
          GNMset = GNM.sunset(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, DST);
          sunChecked = (timeinfo.tm_mon+1) * (timeinfo.tm_mday+1);
    }

    minsToday = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    
    if (ledState == HIGH) {
      if (millis() - ledTime < 15*60000) {
        ledcWrite(ledPin, pwmFull);
      } else {
        ledState = LOW;
      }
    } else {
        if (minsToday < GNMrise) {
          ledcWrite(ledPin, pwmLow);
        }
        if (minsToday >= GNMrise && minsToday < GNMset && GNMrise < morning) {
          ledcWrite(ledPin, 0);
        }
        if (minsToday >= morning && minsToday < GNMrise && GNMrise >= morning) {
          ledcWrite(ledPin, pwmHigh);
        }
        if (minsToday >= GNMrise && minsToday < GNMset && GNMrise >= morning) {
          ledcWrite(ledPin, 0);
        }
        if (minsToday >= GNMset) {
          ledcWrite(ledPin, pwmHigh);
        }       
    }
    
    server.handleClient();
  

  delay(1000);
  }

void handle_OnConnect() {
//  Serial.println("Client connect to /");
  server.send(200, "text/html", SendHTML(ledState)); 
}

void handle_ledFull() {
//    Serial.println("LEDs tijdelijk vol aan");
    ledState = HIGH;
    ledTime = millis();
    ledcWrite(ledPin, pwmFull);
    server.send(200, "text/html", SendHTML(ledState)); 
}

void handle_ledNormal() {
//    Serial.println("LEDs normaal");
    ledState = LOW;
    server.send(200, "text/html", SendHTML(ledState)); 
}

void handle_ledDemo() {
//    Serial.println("LED Demo");
    server.send(200, "text/plain", "Running demo"); 
    blinkDemo();
}

void handle_time() {
    String timeString = String(minsToday);
    String riseString = String(GNMrise);
    String setString = String(GNMset);
    String allString = String(timeString + " " + riseString + " " + setString);
    server.send(200, "text/plain", allString); 
}

void handle_bright() {
    String brightString = String(ledcRead(ledPin));
    server.send(200, "text/plain", brightString); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void blinkDemo(){
    for (int i=0; i < 10; i++) {
      delay(100);
      ledcWrite(ledPin, pwmHigh);
      delay(100);
      ledcWrite(ledPin, pwmFull);
      delay(100);
      ledcWrite(ledPin, pwmHigh);
      delay(100);
      ledcWrite(ledPin, pwmLow);
    }
}

String SendHTML(uint8_t ledstat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Tuinverlichting</h1>\n";
  ptr +="<h3>om deze 15 minuten aan te zetten</h3>\n";
  
   if(ledState)
  {ptr +="<p>LED Status: Full</p><a class=\"button button-off\" href=\"/xxx/lednormal\">Go normal</a>\n";}
  else
  {ptr +="<p>LED Status: Normal</p><a class=\"button button-on\" href=\"/xxx/ledfull\">Go full</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

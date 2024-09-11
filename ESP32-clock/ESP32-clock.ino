/*

  "Smart" watch, that accurately displays current time and weather information

  Author: Carl Hjalmar Love Hult

*/

#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define MOSI 23 //mosi
#define SCK 18 
#define SS 5
#define DC 27
#define RST 14
#define SDA 21
#define SCL 22
#define BTN 12
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define CYAN            0x07FF
//define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

const char ntp_server[] = "ee.pool.ntp.org";
const int time_line = 30;
const uint8_t api_call_delay = 50;
const uint8_t icon_location = 70;

Adafruit_SSD1331 display = Adafruit_SSD1331(SS, DC, MOSI, SCK, RST);

RTC_DS3231 rtc;

unsigned long last_call = 0;
uint8_t old_sec = 0;
uint8_t old_min = 0;
uint8_t old_hour = 0;
uint16_t timeout = 0;
bool is_button_pressed = true;
bool screen_on = true;
unsigned long button_last_pressed = 7000;

// If one wants to try this for themselves, then they should go to https://openweathermap.org/ and generate their own API key
char server_path[] = ; // put API key here

// Bitmap images of weather icons
/*
// 'cloud16', 16x16px
const unsigned char weathercloud16 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x7f, 0xfe, 
  0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00
};*/
// 'cloud24', 24x24px
const unsigned char weathercloud24 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 
  0xff, 0x80, 0x03, 0xff, 0xc0, 0x03, 0xff, 0xc0, 0x03, 0xff, 0xc0, 0x03, 0xff, 0xc0, 0x01, 0xff, 
  0x80, 0x3e, 0x7e, 0x7c, 0x7e, 0x7e, 0x7e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xfc, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'sun24', 24x24px
const unsigned char weathersun24 [] PROGMEM = {
  0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00, 0x30, 0xc3, 0x0c, 0x30, 0xc3, 0x0c, 0x08, 0xff, 0x10, 0x07, 
  0xff, 0xe0, 0x07, 0xff, 0xe0, 0x07, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 
  0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x07, 0xff, 0xe0, 0x07, 0xff, 0xe0, 0x07, 0xff, 0xe0, 0x08, 0xff, 0x10, 0x30, 0xc3, 0x0c, 0x30, 
  0xc3, 0x0c, 0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00
};
// 'half-moon16', 16x16px
const unsigned char weatherhalf_moon16 [] PROGMEM = {
  0x03, 0xf0, 0x0f, 0xfc, 0x1f, 0xfe, 0x3f, 0xfe, 0x3f, 0x80, 0x7f, 0x00, 0x7e, 0x00, 0x7e, 0x00, 
  0x7e, 0x00, 0x7e, 0x00, 0x7f, 0x00, 0x3f, 0x80, 0x3f, 0xfe, 0x1f, 0xfe, 0x0f, 0xfc, 0x03, 0xf0
};
// 'half-moon24', 24x24px
const unsigned char weatherhalf_moon24 [] PROGMEM = {
  0x00, 0x7f, 0xc0, 0x00, 0xff, 0xe0, 0x03, 0xff, 0xf8, 0x07, 0xff, 0xfc, 0x0f, 0xff, 0xfc, 0x1f, 
  0xff, 0xfc, 0x1f, 0xf8, 0x00, 0x3f, 0xf8, 0x00, 0x3f, 0xe0, 0x00, 0x3f, 0xc0, 0x00, 0x3f, 0xc0, 
  0x00, 0x3f, 0xc0, 0x00, 0x3f, 0xc0, 0x00, 0x3f, 0xc0, 0x00, 0x3f, 0xc0, 0x00, 0x3f, 0xe0, 0x00, 
  0x3f, 0xf8, 0x00, 0x1f, 0xf8, 0x00, 0x1f, 0xff, 0xfc, 0x0f, 0xff, 0xfc, 0x07, 0xff, 0xfc, 0x03, 
  0xff, 0xf8, 0x00, 0xff, 0xe0, 0x00, 0x7f, 0xc0
};
// 'rain16', 16x16px
const unsigned char weatherrain16 [] PROGMEM = {
  0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x77, 0xee, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0x7f, 0xfe, 0x3f, 0xfc, 0x00, 0x00, 0x48, 0x91, 0x91, 0x22, 0x22, 0x44, 0x44, 0x88
};
// 'sun16', 16x16px
const unsigned char weathersun16 [] PROGMEM = {
  0x02, 0x40, 0x02, 0x40, 0x03, 0xc0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0xff, 0xff, 0x3f, 0xfc, 
  0x3f, 0xfc, 0xff, 0xff, 0x1f, 0xf8, 0x1f, 0xf8, 0x0f, 0xf0, 0x03, 0xc0, 0x02, 0x40, 0x02, 0x40
};
// 'rain24', 24x24px
const unsigned char weatherrain24 [] PROGMEM = {
  0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x03, 0xff, 0xc0, 0x03, 
  0xff, 0xc0, 0x03, 0xff, 0xc0, 0x3d, 0xff, 0xbc, 0x7d, 0xff, 0xbe, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x1f, 0xff, 0xf8, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x18, 0x63, 0x84, 0x20, 0x84, 0x84, 0x20, 0x84, 0x18, 
  0xc3, 0x18, 0x21, 0x04, 0x20, 0x21, 0x04, 0x20
};
// 'snowy', 24x24px
const unsigned char weathersnowy24 [] PROGMEM = {
  0x00, 0x7e, 0x00, 0x00, 0xff, 0x00, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x03, 0xff, 0xc0, 0x03, 
  0xff, 0xc0, 0x03, 0xff, 0xc0, 0x3d, 0xff, 0xbc, 0x7d, 0xff, 0xbe, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x1f, 0xff, 0xf8, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x3c, 0x1c, 0xfc, 0x7e, 0x3f, 0xfc, 0x7e, 0x3f, 0xfc, 
  0x7e, 0x3f, 0x78, 0x3c, 0x1e, 0x38, 0x3c, 0x1c
};

void updateRTC() {
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, ntp_server, 3600*3);
  timeClient.begin();
  timeClient.update();
  rtc.adjust(DateTime(2022, 5, timeClient.getDay(), timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
}

void senseButtonPressed() {
  if (!is_button_pressed) {
    is_button_pressed = true;
  }
}

void WiFiSetup() {
  BluetoothSerial SerialBT;
  SerialBT.begin("ESP_clock");
  delay(50);
  
  char ssid_buff[15];
  char pass_buff[20];

  display.begin();
  display.fillScreen(BLACK);
  display.setTextColor(BLUE);
  display.setCursor(0, 0);
  display.println("Awaiting WiFi");
  display.println("credentials...");
  display.println("SSID: ");
  display.println("PASS: ");
  
  while (true) {
    if (SerialBT.available()) {
      if (ssid_buff[0] == 0) {
        SerialBT.readBytesUntil('\n', ssid_buff, 15);
        //char ssid[ssid_buff.length()];
        Serial.println(ssid_buff);
        display.setCursor(30, 18);
        display.println(ssid_buff);
        //display.println("Listening for WiFi ssid over BT");
      } else if (pass_buff[0] == 0) {
        SerialBT.readBytesUntil('\n', pass_buff, 20);
        Serial.println(pass_buff);
        display.setCursor(30, 27);
        display.println(pass_buff);
        break;
      }
    }
  }

  // dealing with the c-strings was necessary...

  // debug messages to check that no extra hidden characters are left
  //Serial.println(strlen(ssid_buff));
  //Serial.println(strlen(pass_buff));
  ssid_buff[strcspn(ssid_buff, "\n")] = '\0';
  pass_buff[strcspn(pass_buff, "\n")] = '\0';
  ssid_buff[strcspn(ssid_buff, "\r")] = '\0';
  pass_buff[strcspn(pass_buff, "\r")] = '\0';
  //Serial.println(strlen(ssid_buff));
  //Serial.println(strlen(pass_buff));
  delay(2000);

  display.fillScreen(BLACK);
  
  WiFi.begin(ssid_buff, pass_buff);
  delay(50);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    timeout += 500;
    if (timeout > 10000) {
      display.fillScreen(BLACK);
      display.setCursor(0,5);
      display.println("Failed to connect");
      display.println("Setting default time");
      delay(2000);
      break;
    }
  }
  display.fillScreen(BLACK);
  delay(50);
}

void hard_reset() {
  display.fillScreen(BLACK);
  DateTime now = rtc.now();
  display.setTextSize(2);
  display.setCursor(0, time_line);
  display.setTextColor(BLUE);
  
  if (now.hour() < 10) {
    display.print(0);
    display.print(now.hour());
    display.print(":");
  } else {
    display.print(now.hour());
    display.print(":");
  }
  if (now.minute() < 10) {
    display.print(0);
    display.print(now.minute());
    display.print(":");  
  } else {
    display.print(now.minute());
    display.print(":");  
  }
  if (now.second() < 10) {
    display.print(0);
    display.print(now.second());
  } else {
    display.print(now.second());
  }
  old_hour = now.hour();
  old_min = now.minute();
  old_sec = now.second();
  delay(20);
}

void weather_update() {
  hard_reset();
  String json_buffer;
  double temp;
  if (millis() - last_call > api_call_delay) {
    if(WiFi.status() == WL_CONNECTED) {
      json_buffer = httpGETRequest(server_path);
      //debug info
      //Serial.println(json_buffer);
      last_call = millis();
      JSONVar json_obj = JSON.parse(json_buffer);
      if (JSON.typeof(json_obj) == "undefined") {
        //Debug json obj broken
        display.setTextColor(RED);
        display.setCursor(0,5);
        display.setTextSize(1);
        display.println("API problem");
        Serial.println("Parsing failed");
      } else {
        Serial.print("Temp: ");
        temp = double(json_obj["main"]["temp"]);
        Serial.println(temp);
        Serial.print("Weather: ");
        String weather = JSON.stringify(json_obj["weather"][0]["main"]);
        String weather_icon = JSON.stringify(json_obj["weather"][0]["icon"]);
        weather.remove(0,1);
        int last_index = weather.length() - 1;
        weather.remove(last_index);
        
        weather_icon.remove(0,1);
        Serial.println(weather);
        
        // show the right icon

        //clear
        if (weather_icon.startsWith("01")) {
          if (weather_icon[2] == 'd') {
            display.drawBitmap(icon_location,0, weathersun24, 24, 24, YELLOW);
          } else {
            display.drawBitmap(icon_location,0, weatherhalf_moon24, 24, 24, CYAN);
          }
        }

        //cloudy with sun
        if ((weather_icon.startsWith("02")) || (weather_icon.startsWith("03")) || (weather_icon.startsWith("50"))) {
          if (weather_icon[2] == 'd') {
            display.drawBitmap(icon_location + 10,-2, weathersun16, 16, 16, YELLOW);
            display.drawBitmap(icon_location,0, weathercloud24, 24, 24, WHITE);
          } else {
            display.drawBitmap(icon_location + 10,-2, weatherhalf_moon16, 16, 16, CYAN);
            display.drawBitmap(icon_location,0, weathercloud24, 24, 24, WHITE);
          }
        }

        // cloudy
        if (weather_icon.startsWith("04")) {
          display.drawBitmap(icon_location,0, weathercloud24, 24, 24, WHITE);
        }
          
        // rain
        if ((weather_icon.startsWith("09")) || (weather_icon.startsWith("10")) || (weather_icon.startsWith("11"))){
          display.drawBitmap(icon_location,0, weatherrain24, 24, 24, WHITE);
        }

        // snow
        if (weather_icon.startsWith("13")) {
          display.drawBitmap(icon_location,0, weathersnowy24, 24, 24, WHITE);
        }

        display.setCursor(0,5);
        display.setTextSize(1);
        display.println(weather);
        display.print(temp - 272.15);
        display.print("C");

      }
    } else {
        display.setTextColor(RED);
        display.setCursor(0,5);
        display.setTextSize(1);
        display.println("No WiFi to get weather");
    }
  }
}

String httpGETRequest(const char* server_name) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, server_name);

  int http_response = http.GET();

  String payload = "{}";

  if (http_response > 0) {
    Serial.print("Http response code: ");
    Serial.println(http_response);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(http_response);
  }
  http.end();

  return payload;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(10);

  pinMode(BTN,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN), senseButtonPressed, FALLING);
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  Serial.println("declared rtc object!");
  
  WiFiSetup();
  delay(50);

  updateRTC();

  weather_update();
  delay(50);
}

//text size 2 is 10x14 + 1 pixel each side

void loop() {

  DateTime now = rtc.now();

  if (screen_on) {
    if ((button_last_pressed + 1000 > millis()) && is_button_pressed) {
      button_last_pressed = millis();
      is_button_pressed = false;
      //Serial.println("hello");
      //make api call
      weather_update();
    
    } else if (button_last_pressed + 20000 < millis()) {
      screen_on = false;
      display.enableDisplay(screen_on);
      Serial.println("Turning off display");
      is_button_pressed = false;
    }
  } else {
    if (is_button_pressed) {
      button_last_pressed = millis();
      screen_on = true;
      is_button_pressed = false;
      Serial.println("enabling display");
      display.enableDisplay(screen_on);
    }
  }

  // This whole following block exists to only update the numbers that need updating on the clock
  // Basically, has the number changed? -> redraw old number in black -> draw new number in blue
  // Anything else leads to a "flashing" appearance of the screen
  if (now.hour() != old_hour) {
    display.setTextSize(2);
    display.setCursor(0, time_line);
    display.setTextColor(BLACK);
    if (old_hour < 10) {
      display.print(0);
      display.print(old_hour);
      display.print(":");
      display.print(0);
      display.print(old_min);
      display.print(":");
      display.print(0);
      display.print(old_sec);
    } else {
      display.print(old_hour);
      display.print(":");
      display.print(old_min);
      display.print(":");
      display.print(old_sec);
    }  
    
    display.setCursor(0, time_line);
    display.setTextColor(BLUE);

    if (now.hour() < 10) {
      display.print(0);
      display.print(now.hour());
      display.print(":");
      display.print(0);
      display.print(now.minute());
      display.print(":");
      display.print(0);
      display.print(now.second());
    } else {
      display.print(now.hour());
      display.print(":");
      display.print(0);
      display.print(now.minute());
      display.print(":");
      display.print(0);
      display.print(now.second());
    }
    
  } else if (now.minute() != old_min) {
    display.setTextSize(2);
    display.setCursor(36, time_line);
    display.setTextColor(BLACK);
    if (old_min < 10) {
      display.print(0);
      display.print(old_min);
      display.print(":");
      display.print(old_sec);
    } else {
      display.print(old_min);
      display.print(":");
      display.print(old_sec);
    }
    delay(5);
    display.setCursor(36, time_line);
    display.setTextColor(BLUE);

    if (now.minute() < 10) {
      display.print(0);
      display.print(now.minute());
      display.print(":");
      display.print(0);
      display.print(now.second());
    } else { 
      display.print(now.minute());
      display.print(":");
      display.print(0);
      display.print(now.second());
    }
    delay(5);
    
  } else if (now.second() != old_sec) {
    display.setTextSize(2);
    display.setCursor(72, time_line);
    display.setTextColor(BLACK); 
    if (old_sec < 10) {
      display.print(0);
      display.print(old_sec);
    } else {
      display.print(old_sec);
    }
    
    display.setCursor(72, time_line);
    display.setTextColor(BLUE);
    
    if (now.second() < 10) {
      display.print(0);
      display.print(now.second());  
    } else {
      display.print(now.second());  
    }
  }
  
  old_hour = now.hour();
  old_min = now.minute();
  old_sec = now.second();
  
  
  delay(100);
}

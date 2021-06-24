/*
 * Shashank Kulkarni (Github - sckulkarni246)
 * shanky.kulkarni@gmail.com
 * 
 * ESP32 - Pinouts
 * ===============
 * => MAX7219 LED Matrix - Clock (GPIO18), Chip Select (GPIO15), MOSI (GPIO23), 3.3V
 * => DHT11 - Data (GPIO19), 3.3V 
 * => Buzzer - Buzzer Drive (GPIO5), 3.3V (unused as of now)
 * => RGB LED - Red (GPIO26), Green (GPIO12), Blue (GPIO27)
 * => Joystick - X axis (GPIO36), Y axis (GPIO39), 5V
 * 
 */

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "DHT.h"
#include <WiFi.h>
#include "time.h"
#include <WiFiMulti.h>

#define DISPLAY_SCK       18
#define DISPLAY_CS        15
#define DISPLAY_MOSI      23
#define HARDWARE_TYPE     MD_MAX72XX::FC16_HW
#define MAX_DEVICES       4

#define DHT11_DATA        19
#define DHTTYPE           DHT11   

#define BUZZER_DRIVE      5 // unused as of now

#define LED_R             25
#define LED_G             12
#define LED_B             27

#define JOYSTICK_X        36
#define JOYSTICK_Y        39

#define WIFI_SSID         "" // hard-code your SSID name here!
#define WIFI_PSK          "" // hard-code your PSK here!

#define NTP_SERVER        "pool.ntp.org"
#define GMT_OFFSET_HOURS  (5.5)
#define GMT_OFFSET_SEC    (3600 * GMT_OFFSET_HOURS)

#define SCREEN_TIME       (1)
#define SCREEN_DATE       (2)
#define SCREEN_TEMP       (3)
#define SCREEN_HUMI       (4)
#define NUM_SCREENS       (4)

#define JS_THRESH_LEFT    (2600)
#define JS_THRESH_RIGHT   (2900)
#define JS_THRESH_UP      (500)
#define JS_THRESH_DOWN    (3500)

#define INTERVAL_POLL     (50)
#define COUNT_POLL_TIME   (1)


MD_Parola my_display = MD_Parola(HARDWARE_TYPE, DISPLAY_CS, MAX_DEVICES);
DHT dht(DHT11_DATA, DHTTYPE);
WiFiMulti WiFiMulti;
struct tm timeinfo;
char display_time[] = "hh:mm";
char display_date[] = "dd/mm";
char tempstr[6] = "Deg C";
char humistr[6] = "Val %";
uint8_t curr_hh = 0;
uint8_t curr_mm = 0;
uint8_t curr_dd = 0;
uint8_t curr_mon = 0;
uint8_t curr_screen = 1;
uint8_t pollctr = 0;
uint8_t ntpctr = 0;
uint8_t tempctr = 0;
uint8_t humictr = 0;
uint8_t connstatusctr = 0;
bool js_left = false;
bool js_right = false;
bool js_up = false;
bool js_down = false;
bool js_center = true;
bool screen_changed = false;
bool unable_to_connect = false;
bool i_am_disconnected = false;
bool no_wifi = false;
float curr_temp = 0.0;
float curr_humi = 0.0;
unsigned long app_curr_time;
unsigned long app_prev_time;

/*
 * Connect to an Wi-Fi AP - modify SSID and PSK in macros above
 */

void app_wifi_connect() {
  WiFiMulti.addAP(WIFI_SSID,WIFI_PSK);
  Serial.println("Waiting for WiFi connection...");

  my_display.print("WiFi...");
  digitalWrite(LED_R, HIGH);
  
  while(WiFiMulti.run() != WL_CONNECTED);

  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  my_display.print("WiFi OK");
  Serial.println("WiFi Connected...");
  delay(2000);
  
  configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);
  my_display.print("Time...");

}

void app_get_time() {
  
  ++ntpctr;
  
  if(ntpctr < 40) {
    
  }
  else {
    if(!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time...");
      unable_to_connect = true;
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_R, HIGH);
    }
    else {
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_R, LOW);
      unable_to_connect = false;
    }

    ntpctr = 0;
    
  }

  if(curr_screen == SCREEN_TIME) {
    if(screen_changed == false) {
      if((timeinfo.tm_hour != curr_hh) || (timeinfo.tm_min != curr_mm)) {
        curr_hh = timeinfo.tm_hour;  
        curr_mm = timeinfo.tm_min;
        display_time[0] = 48 + (curr_hh / 10);
        display_time[1] = 48 + (curr_hh % 10);
        display_time[3] = 48 + (curr_mm / 10);
        display_time[4] = 48 + (curr_mm % 10);
        if(curr_screen == SCREEN_TIME)
          my_display.print(display_time); 
      }  
    }
    else {
      my_display.print(display_time); 
    }  
  }

  if(curr_screen == SCREEN_DATE) {
    if(screen_changed == false) {
      if((timeinfo.tm_mday != curr_dd) || (timeinfo.tm_mon != curr_mon)) {
        curr_dd = timeinfo.tm_mday;  
        curr_mon = timeinfo.tm_mon + 1;
        display_date[0] = 48 + (curr_dd / 10);
        display_date[1] = 48 + (curr_dd % 10);
        display_date[3] = 48 + (curr_mon / 10);
        display_date[4] = 48 + (curr_mon % 10);
        if(curr_screen == SCREEN_DATE)
          my_display.print(display_date); 
      }  
    }
    else {
      my_display.print(display_date); 
    }  
  }
  
}

void app_get_temperature() {
  
  tempctr++;

  if(tempctr < 100) {
    
  }
  else {
    curr_temp = dht.readTemperature();
    sprintf(tempstr,"%.1fc",curr_temp);
    tempctr = 0;
  }
  if(curr_screen == SCREEN_TEMP) {
    my_display.print(tempstr);
  }

}

void app_get_humidity() {
  
  humictr++;

  if(humictr < 200) {
    
  }
  else {
    curr_humi = dht.readHumidity();
    sprintf(humistr,"%d%%",(int)curr_humi);
    humictr = 0;
  }
  if(curr_screen == SCREEN_HUMI) {
    my_display.print(humistr);
  }

}

void check_for_js_movement() {
  int val;

  js_left = js_right = false;
  js_center = true;
  
  val = analogRead(JOYSTICK_X);
  
  if(val == 0) {
    digitalWrite(LED_B, HIGH);
    delay(200);
    digitalWrite(LED_B, LOW);  
    js_left = true;
    js_right = false;
    js_center = false;   
  }
  else if(val == 4095) {
    digitalWrite(LED_B, HIGH);
    delay(200);
    digitalWrite(LED_B, LOW);   
    js_left = false;
    js_right = true;
    js_center = false;      
  }
  else {
    js_left = false;
    js_right = false;
    js_center = true;
  }
}

void setup() {
  // Start the serial console...we may need it for development
  Serial.begin(115200);
  delay(10);
  
  // Initialize the MAX7219 Dot Matrix display
  my_display.begin();
  my_display.setIntensity(2);
  my_display.displayClear();
  my_display.setTextAlignment(PA_CENTER);

#if 0 // Unimplemented - maybe later?
  // Initialize the Buzzer PWM as an input - we will attach a PWM enlosed by #if-#endif) to it on-demand
  // in a separate function. This is done so to avoid the continuous "buzz" from the buzzer after setup ;-).
  pinMode(BUZZER_DRIVE, INPUT);

  ledcSetup(0,2000,8);
  ledcAttachPin(BUZZER_DRIVE,0);
#endif
  // Initialize the DHT11 sensor
  dht.begin();

  // Initialize the RGB LED
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  app_wifi_connect();

  app_prev_time = app_curr_time = millis();

}

void loop() {
  delay(INTERVAL_POLL);
  ++pollctr;

  check_for_js_movement();

  if(js_right) {
    app_prev_time = millis();
    screen_changed = true;
    if(curr_screen == NUM_SCREENS)
      curr_screen = 1;
    else
      curr_screen = curr_screen + 1;   
  }
  else if(js_left) {
    app_prev_time = millis();
    screen_changed = true;
    if(curr_screen == 1)
      curr_screen = NUM_SCREENS;
    else
      curr_screen = curr_screen - 1;   
  }
  else {
    screen_changed = false;
    app_curr_time = millis();
    switch(curr_screen) {
      case SCREEN_TIME:
        if(app_curr_time - app_prev_time > 30000) {
          screen_changed = true;
          app_prev_time = app_curr_time;
          curr_screen = curr_screen + 1;
        }
        break;

     case SCREEN_DATE:
     case SCREEN_TEMP:
     case SCREEN_HUMI:
        if(app_curr_time - app_prev_time > 5000) {
          screen_changed = true;
          app_prev_time = app_curr_time;
          curr_screen = curr_screen + 1;
          if(curr_screen > NUM_SCREENS)
            curr_screen = SCREEN_TIME;
        }
        break;
     
    }
  }
    
  if(pollctr == COUNT_POLL_TIME) {
    app_get_time(); 
    app_get_temperature();
    app_get_humidity();
    pollctr = 0; 
  }
  


}

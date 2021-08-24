/*
Long Distance Touch Lamp!
* Edit Configuration before uploading
* Uploading over OTA? Go to Tools > Port > Select the correct ESP.
* (c) Olivia Chang & Andrew Toteda.
*/

/************************** Configuration ***********************************/

#define IO_USERNAME  "liver"
#define IO_KEY       ""
#define LAMP_NAME     "OliviaLamp" // or "AndrewLamp"
// Simon (Andrew's Lamp) = 1
// Lampo (Olivia's Lamp) = 2
#define LAMP_ID       2

/************************** Start Sketch ***********************************/

// Libraries
#include <Arduino.h>
#include <FastLED.h>
// wifi libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "AdafruitIO_WiFi.h"
// WifiManager is not included in here — our hack of the AdafruitIO library includes WifiManager

/******************************* WIFI **************************************/
/*
 * How to get MAC Address without connecting to serial monitor:
 * Turn on lamp and connect to AP
 * Open WifiManager > Info and get Station MAC Address (not AP MAC Address)
 * Station MAC Address is the MAC Address that is assigned to each ESP8266
 * Need to change the MAC Address? https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/
*/

// If a static IP address is required:
// https://randomnerdtutorials.com/esp8266-nodemcu-static-fixed-ip-address-arduino/
//IPAddress local_IP(192, 168, 1, 114);
//// Set your Gateway IP address
//IPAddress gateway(192, 168, 1, 1);
//
//IPAddress subnet(255, 255, 255, 0);
//IPAddress primaryDNS(8, 8, 8, 8);   //optional
//IPAddress secondaryDNS(8, 8, 4, 4); //optional

/************************ Setup *******************************/

// Adafruit.io
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, nullptr, nullptr);

// Lamp ID
int lampId = LAMP_ID;

 // set up the 'lampo' feed
AdafruitIO_Feed *lampo = io.feed("lampo");

/************************ END SETUP *******************************/

// FastLED settings
#define NUM_LEDS 22
#define INITIAL_BRIGHTNESS  120 // initial brightness
#define MAX_BRIGHTNESS  255 //  max brightness
#define MIN_BRIGHTNESS  10 //  min brightness
#define HUE_CHANGE_SPEED 3 // higher number = faster change
#define SAT_CHANGE_SPEED 15 // higher number = faster change
#define BRIGHTNESS_CHANGE_SPEED 15 // higher number = faster change

/* FastLED variables*/

/* CHSV explanation
 *  0 - 255: 0 is lowest, 255 is max
 *  H = hue
 *  S = saturation
 *  V = brightness
 *  https://github.com/FastLED/FastLED/wiki/Pixel-reference#chsv
*/


CRGB leds[NUM_LEDS];
CHSV colour( 0, 255, INITIAL_BRIGHTNESS);
int brightness = INITIAL_BRIGHTNESS;

// when this is HIGH, brightness is increasing
// LOW = brightness decreasing

boolean brightnessIncreasing = HIGH;
boolean brightnessIncreasing = HIGH;

/* LAMP MODE
 0 = off
 1 = selecting hue
 2 = color steady
 3 = selecting value
 4 = color steady

0, lamp is off
touch top bottom
1, while touching, turns on and cycles through hues
2, let go of touch—color turns steady
3, while touching, turns on and cycles through values
4, let go of touch—color turns steady
touch again, turns off
*/
int lampMode = 0;

/************************ PIN DEFINITIONS *******************************/
// D0 — top button
// D1 — bottom button
// D2 — led strip
#define TOUCH_SW_TOP_PIN D0
#define TOUCH_SW_BOTTOM_PIN D1
#define LED_PIN D2

// Button States
int lastStateTop = LOW;  // the previous state from top input pin
int lastStateBottom = LOW;  // the previous state from bottom input pin

// Handling 2 buttons pressed... TODO add more explanation
//unsigned long currentMillis;
// 0 means it is not set (ms)
//unsigned long topButtonTouchedMillis = 0; // point in time when top button is touched (ms)
//unsigned long bottomButtonTouchedMillis = 0; // point in time when bottom button is touched (ms)
// // how long to wait after a single button is pressed until acting as if only ONE button is pressed (ms)
//const unsigned long buttonPressWaitTime = 500;

//AsyncWebServer server(80);

void setup() {
  /* Start Serial Monitor for debugging
   * Open Serial Monitor by going to Tools > Serial Monitor > Set baud to 115200
   * Does not work over WiFi
  */
  Serial.begin(115200);
  Serial.println("Sketch starting...");
  
  // Set up LEDs
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  turnOffLamp();

  //  Set up Wifi
  WiFi.mode(WIFI_STA);

  //start connecting to Adafruit IO
  Serial.print("Connecting to Adafruit IO");
  io.connect();
  // our custom AdafruitIO library includes WifiManager, so we don't need to call WiFi connect—it's handled in io.connect()

  // when a message is received, 
  lampo->onMessage(handleMessage);

  // connect to Adafruit IO and turn it red to show it's connecting until and connection is established
   while(io.status() < AIO_CONNECTED) {
     Serial.print(".");
     blinkLEDs(CRGB::Red);
   }

  // //when a connection to Adafruit IO is made write the status in the Serial monitor and flash the Neopixels green
   Serial.println();
   Serial.println(io.statusText());
   flash(CRGB::Green);

  // once it is connected, print SSID, password, MAC Address
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.psk());
  Serial.print("ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Set up Over-The-Air Updates (ArduinoOTA)
  ArduinoOTA.setHostname(LAMP_NAME);
  ArduinoOTA.setPassword("lampo");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // ** Set up buttons **
  pinMode(TOUCH_SW_TOP_PIN,INPUT);
  pinMode(TOUCH_SW_BOTTOM_PIN,INPUT);
  // TODO: may need to set up sensor as an input?
  // what does this do
  //  attachInterrupt(digitalPinToInterrupt(TOUCH_SW_TOP_PIN), touch, CHANGE);
  //  attachInterrupt(digitalPinToInterrupt(TOUCH_SW_BOTTOM_PIN), touch, CHANGE);

}

/************************ MODE SWITCH FUNCTIONS *******************************/

void gotoSelectHueMode() {
  Serial.println("SELECT HUE MODE: Strip turns on and cycles through hues");
  lampMode = 1;
  setColor();
  // wait one second before colors start changing
  // allows for same color to be turned on and off
  delay(2000);
}

void gotoSelectSaturationMode() {
  Serial.println("SELECT SAT MODE: Strip turns on and cycles through saturation");
  lampMode = 3;
  setColor();
  // wait one second before colors start changing
  // allows for same color to be turned on and off
  delay(2000);
}


void gotoOffMode() {
  Serial.println("OFF MODE: Strip is turned off");
  lampMode = 0;
  turnOffLamp();
}

/************************ FASTLED FUNCTIONS *******************************/

// Turns off the lamp by clearing the LED settings and then "FastLED.show"
// "show" is like printing
void turnOffLamp() {
  FastLED.clear(true);
  FastLED.show();
}

void blinkLEDs(CRGB colorToFlash) {
  fill_solid( leds, NUM_LEDS, colorToFlash);
  FastLED.show();
  delay(300);
}

void flash(CRGB colorToFlash) {
  fill_solid( leds, NUM_LEDS, colorToFlash);
  FastLED.show();
  delay(200);
  FastLED.clear();
}

void setColor() {
  fill_solid( leds, NUM_LEDS, colour);
  FastLED.show();
}

/************************ ADAFRUIT IO FUNCTIONS *******************************/

long convertCHSVToLong(CHSV chsvToConvert) {
  // 1,H value,S value,V value
  long colorAsLong = (lampId * 1000000000L) + (chsvToConvert.h * 1000000L) + (chsvToConvert.s * 1000) + chsvToConvert.v;
  Serial.print("colorAsLong: ");
  Serial.println(colorAsLong);
  return colorAsLong;
}

void sendColor() {
  if (lampo->save(convertCHSVToLong(colour))) {
    Serial.println("Color sent to Adafruit");  
    flash(CRGB::Green);
  } else {
    Serial.println("Unable to send to Adafruit");
    flash(CRGB::Red);
  }
  // Adafruit IO is rate limited for publishing, so a delay is required in
  // between feed->save events.
  delay(3000);
}

// this function is called whenever an 'lampo' feed message
// is received from Adafruit IO. it was attached to
// the 'lampo' feed in the setup() function above. 
void handleMessage(AdafruitIO_Data *data) {
  String colorAsString = data->toString();
  Serial.print("received color ");
  Serial.print(colorAsString);
  
  // Substring: (inclusive, exclusive)
  int recLampId = colorAsString.substring(0, 1).toInt();
  Serial.print("id of lamp that sent it was ");
  Serial.print(recLampId);

  // handleMessage will receive data even when that data was sent from the same lamp
  // only need to set the new color if the data was from the other lamp
  if (recLampId != lampId) {
    int newColorHue = colorAsString.substring(1, 4).toInt();
    int newColorSaturation = colorAsString.substring(4, 7).toInt();
    Serial.print("newColorHue");
    Serial.println(newColorHue);
    Serial.print("newColorSaturation");
    Serial.println(newColorSaturation);
  //  int newColorValue = colorAsString.substring(7); // for changing other person's brightness, but we are not using it for now
    
    colour.h = newColorHue;
    colour.s = newColorSaturation;

    setColor();
    lampMode = 4; // set it to the last mode
    FastLED.show();
  }

}

/************************ LOOP *******************************/
  
void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  
  ArduinoOTA.handle();
  
  // clear anything from the previous loop
  // if there is still color, it will be set again
  // this ensures that it turns off when it should
  FastLED.clear();

  /* Handle Button Presses */
  boolean currentStateTop = digitalRead(TOUCH_SW_TOP_PIN);
  boolean currentStateBottom = digitalRead(TOUCH_SW_BOTTOM_PIN);
  // HIGH = touched, LOW = not touched

  // Handle Top Button Touch
  if(lastStateTop == LOW && currentStateTop == HIGH && currentStateBottom == LOW) { // Top Button Touched

//    ignore for now - code this at the end
//    if (currentStateBottom == LOW) { // if bottom button isn't currently being touched
//      currentMillis = millis();
//      if (topButtonTouchedMillis == 0) {
//        topButtonTouchedMillis = currentMillis; 
//      } else if ((topButtonTouchedMillis - currentMillis) > buttonPressWaitTime){
//        
//      }
//      // check if time since touched is greater than time to wait
//      // once action is taken, set it back to 0
//    }
//    
    Serial.println("Top sensor was touched");
    // if lamp was previously off, go to selecting mode
    if (lampMode == 0) {
      gotoSelectHueMode();
    }
    // if the lamp is steady color, turn off lamp
    if (lampMode == 2) {
      gotoSelectSaturationMode();
    }
    // if the lamp is last mode, turn off lamp
    if (lampMode == 4) {
      gotoOffMode();
    }
  } else if(lastStateTop == HIGH && currentStateTop == LOW) { // Button Let Go
    Serial.println("Top sensor was let go");
    // if lamp was in selecting mode
    if (lampMode == 1) {
      lampMode = 2;
      setColor();
    } else if ( lampMode == 3) {
      lampMode = 4;
      setColor();
    }
  }

  // Handle Bottom Button Pressed
  if(lastStateBottom == LOW && currentStateBottom == HIGH && currentStateTop == LOW) {
    Serial.println("Bottom sensor was touched");
    // if in steady mode
    if (lampMode == 2 || lampMode == 4) {
      sendColor(); 
    }
  }

  // Handle Both Buttons Press & Hold
  if (currentStateTop == HIGH && currentStateBottom == HIGH) {
    Serial.println("Both buttons are being touched");

    if (brightnessIncreasing == HIGH) {
      brightness = brightness + BRIGHTNESS_CHANGE_SPEED;
    } else {
      brightness = brightness - BRIGHTNESS_CHANGE_SPEED;
    }
    
    if (brightness >= MAX_BRIGHTNESS) {
      brightnessIncreasing = LOW;
      colour.val = MAX_BRIGHTNESS;
    } else if (brightness =< MIN_BRIGHTNESS) {
      brightnessIncreasing = HIGH;
      colour.val = MIN_BRIGHTNESS;
    } else {
      colour.val = brightness;
    }
    
    Serial.print("Current brightness: ");
    Serial.println(brightness);
  }

  /* Handle Modes */

  if (lampMode==1) {
    // Hue Select
    colour.hue = (colour.hue + HUE_CHANGE_SPEED) % 256;
    Serial.println("Current hue");
    Serial.println(colour.hue);
    setColor(); 
  } else if (lampMode==3) {
    // Saturation select
    if (saturationIncreasing == HIGH) {
      colour.sat = colour.sat + SATURATION_CHANGE_SPEED;
    } else {
      colour.sat = colour.sat - SATURATION_CHANGE_SPEED;
    }
    
    if (colour.sat >= 255) { // 255 is max saturation 
      saturationIncreasing = LOW;
      colour.sat = 255;
    } else if (colour.sat =< 0) { // 0 is min saturation
      saturationIncreasing = HIGH;
      colour.sat = 0;
    }
    Serial.println("Current sat");
    Serial.println(colour.sat);
    setColor(); 
  } else if (lampMode==2 || lampMode==4) {
    // Steady mode
    setColor();
  }

  delay(100); // small delay to account for button bounce. // do we need this?

  // save the last state of buttons to use in next loop
  lastStateTop = currentStateTop;
  lastStateBottom = currentStateBottom;

  FastLED.show();
}

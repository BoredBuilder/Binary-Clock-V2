//Binary Clock V2!!!
//time library just in case
#include "time.h"
//NTPClient for pulling time data
#include <NTPClient.h>
//WifiManager for wifi provisioning
#include <WiFiManager.h>
//Button Library for Long presses and debouncing
#include <Button2.h>
//library for smoothing the photo resistor data
#include <Smoothed.h>
//Save Timezone Data
#include <Preferences.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiManager wm;
WiFiManagerParameter custom_field;  // global param ( for non blocking w params )
Preferences preferences;

bool res;

//LED pins
const byte hoursPin10 = 48;
const byte hoursPin20 = 35;

const byte hoursPin1 = 13;
const byte hoursPin2 = 14;
const byte hoursPin4 = 21;
const byte hoursPin8 = 47;

const byte minutesPin10 = 10;
const byte minutesPin20 = 11;
const byte minutesPin40 = 12;

const byte minutesPin1 = 18;
const byte minutesPin2 = 8;
const byte minutesPin4 = 3;
const byte minutesPin8 = 9;

const byte secondsPin10 = 15;
const byte secondsPin20 = 16;
const byte secondsPin40 = 17;


const byte secondsPin1 = 4;
const byte secondsPin2 = 5;
const byte secondsPin4 = 6;
const byte secondsPin8 = 7;

#define BUTTON_PIN 42
Button2 button;

const byte photoPin = 1;  //photoresistor pin
#define SENSOR_PIN photoPin
Smoothed<float> mySensor;
int brightVal;
int brightVal2;
int pChannelMos = 36;
const int freq = 1000;
const int resolution = 12;


int hourOffset;


void handleTap(Button2& b) {
  // check for really long clicks
  if (b.wasPressedFor() > 2000) {
    wm.resetSettings();
    ESP.restart();
  }
}



void setup() {
  // Start Serial Communication
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  preferences.begin("Timezoneset", false);

  Serial.begin(115200);

  const char* custom_radio_str = "<br/><label for='customfieldid'>Time Zones</label><br><input type='radio' name='customfieldid' value='5' checked> Eastern<br><input type='radio' name='customfieldid' value='6'> Central<br><input type='radio' name='customfieldid' value='7'> Mountain<br><input type='radio' name='customfieldid' value='8'> Pacific";
  new (&custom_field) WiFiManagerParameter(custom_radio_str);  // custom html input

  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);

  //wm.resetSettings(); // wipe settings

  // Placeholder for the timezone offset variable
  // int utcOffsetInSeconds = getParam("customfieldid") * -3600;

  std::vector<const char*> menu = { "wifi" }; //Selection for web
  wm.setMenu(menu); //Menu

  // set dark theme
  wm.setClass("invert"); //dark mode :b

  mySensor.begin(SMOOTHED_AVERAGE, 8); //Setup the smoothing algo

  wm.setConnectTimeout(10); // Time out for the start connection

  res = wm.autoConnect("Binary_Clock");  // anonymous ap/ set the name of the clock
  if (!res) {
    Serial.println("Failed to connect"); //Failed to connect
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)"); // Connected succesfully
  }

  hourOffset = preferences.getInt("timeZone", 0);
  //NPT Sever Setup
  timeClient.begin(); //begin the ntp Client
  //timeClient.setTimeOffset(utcOffsetInSeconds);
  //printLocalTime();
  timeClient.setTimeOffset(hourOffset * -3600);
  Serial.println(hourOffset);
  //Brightness PWM Pin gate control of P-Channel mosfet

  //Button PIN Assignments(s)
  button.begin(BUTTON_PIN);
  button.setTapHandler(handleTap);

  //Pin Modes For Leds
  pinMode(hoursPin10, OUTPUT);
  pinMode(hoursPin20, OUTPUT);
  pinMode(hoursPin1, OUTPUT);
  pinMode(hoursPin2, OUTPUT);
  pinMode(hoursPin4, OUTPUT);
  pinMode(hoursPin8, OUTPUT);
  pinMode(minutesPin10, OUTPUT);
  pinMode(minutesPin20, OUTPUT);
  pinMode(minutesPin40, OUTPUT);
  pinMode(minutesPin1, OUTPUT);
  pinMode(minutesPin2, OUTPUT);
  pinMode(minutesPin4, OUTPUT);
  pinMode(minutesPin8, OUTPUT);
  pinMode(secondsPin10, OUTPUT);
  pinMode(secondsPin20, OUTPUT);
  pinMode(secondsPin40, OUTPUT);
  pinMode(secondsPin1, OUTPUT);
  pinMode(secondsPin2, OUTPUT);
  pinMode(secondsPin4, OUTPUT);
  pinMode(secondsPin8, OUTPUT);
  ledcAttach(pChannelMos, freq, resolution);
  mySensor.clear();
}


String getParam(String name) {
  //read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}


void saveParamCallback() {
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
  hourOffset = getParam("customfieldid").toInt();
  Serial.println(hourOffset);
  preferences.putInt("timeZone", hourOffset);
}


void serialCOM() {

  int Hours = timeClient.getHours();
  Serial.print("Hour:  ");
  Serial.println(Hours);
  int Minutes = timeClient.getMinutes();
  Serial.print("Minute:  ");
  Serial.println(Minutes);
  int Seconds = timeClient.getSeconds();
  Serial.print("Second: ");
  Serial.println(Seconds);
  Serial.println();
}

//------Brightness Control---------

void brightness() {

  float currentSensorValue = analogRead(SENSOR_PIN);
  mySensor.add(currentSensorValue);
  float smoothedSensorValueAvg = mySensor.get();
  brightVal = (int)smoothedSensorValueAvg;
  brightVal2 = map(brightVal, 2000, 4095, 4095, 0);
  Serial.println(brightVal);
  if (brightVal > 2000) {
    ledcWrite(pChannelMos, brightVal2);
  } else {
    ledcWrite(pChannelMos, 4095);
  }
}



/*
//------------- Algorithum for LEDS -----------

void setDigitalPins(int value, int pin40, int pin20, int pin10, int pin8, int pin4, int pin2, int pin1) {
  // Set pins for tens place
  if (value >= 40) {
    digitalWrite(pin40, LOW);
    digitalWrite(pin20, HIGH);
    digitalWrite(pin10, value >= 50 ? LOW : HIGH);
  } else if (value >= 20) {
    digitalWrite(pin40, HIGH);
    digitalWrite(pin20, LOW);
    digitalWrite(pin10, value >= 30 ? LOW : HIGH);
  } else if (value >= 10) {
    digitalWrite(pin40, HIGH);
    digitalWrite(pin20, HIGH);
    digitalWrite(pin10, LOW);
  } else {
    digitalWrite(pin40, HIGH);
    digitalWrite(pin20, HIGH);
    digitalWrite(pin10, HIGH);
  }

  // Set pins for units place
  int unit = value % 10;
  digitalWrite(pin8, unit >= 8 ? LOW : HIGH);
  digitalWrite(pin4, (unit >= 4 && unit < 6) || (unit >= 8) ? LOW : HIGH);
  digitalWrite(pin2, (unit >= 2 && unit < 4) || (unit >= 6 && unit < 8) ? LOW : HIGH);
  digitalWrite(pin1, unit % 2 == 1 ? LOW : HIGH);
}

void updateDisplay(int hours, int minutes, int seconds) {
  // Display hours
  setDigitalPins(hours, hoursPin20, hoursPin10, hoursPin8, hoursPin4, hoursPin2, hoursPin1);

  // Display minutes
  setDigitalPins(minutes, minutesPin40, minutesPin20, minutesPin10, minutesPin8, minutesPin4, minutesPin2, minutesPin1);

  // Display seconds
  setDigitalPins(seconds, secondsPin40, secondsPin20, secondsPin10, secondsPin8, secondsPin4, secondsPin2, secondsPin1);
}
*/


//------------ Display Function --------------

void displayLights() {
  //------- Includes automatice DST ajustment --------
  //---------Update client and get time data----------
  timeClient.update();
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
  //--------Automate the daylight saving time---------

#define LEAP_YEAR(Y) ((Y > 0) && !(Y % 4) && ((Y % 100) || !(Y % 400)))

  //----------------Here is all the stolen code :) thanks arduino form!------------------
  unsigned long secs;
  unsigned long rawTime = (secs ? secs : timeClient.getEpochTime()) / 86400L;  // in days
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  while ((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365);  // now it is days in this year, starting at 0
  days = 0;
  for (month = 0; month < 12; month++) {
    uint8_t monthLength;
    if (month == 1) {  // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  month = ++month;
  int dayOfMonth = ++rawTime;


  //Serial.println(month);
  //Serial.println(year);

  int DST;
  // ********************* Calculate offset for Sunday *********************
  int y = year - 2000;          // Get year from NTPClient and subtract 2000
  int x = (y + y / 4 + 2) % 7;  // remainder will identify which day of month
  // is Sunday by subtracting x from the one
  // or two week window. First two weeks for March
  // and first week for November

  // *********** Test DST: BEGINS on 2nd Sunday of March @ 2:00 AM *********
  if (month == 3 && dayOfMonth == (14 - x) /*&& hour >= 2*/) {
    DST = 1;  // Daylight Savings Time is TRUE (add one hour)
  }
  if (month == 3 && dayOfMonth > (14 - x) || month > 3) {
    DST = 1;
  }
  // ************* Test DST: ENDS on 1st Sunday of Nov @ 2:00 AM ************
  if (month == 11 && dayOfMonth == (7 - x) /*&& hour >= 2*/) {
    DST = 0;  // daylight savings time is FALSE (Standard time)
  }
  if (month == 11 && dayOfMonth > (7 - x) || month > 11 || month < 3) {
    DST = 0;
  }

  if (DST == 1)  // Test DST and add one hour if = 1 (TRUE)
  {
    //Serial.println(DST);
    hours = hours + 1;
    //Serial.println(hours);
  }

  //updateDisplay(hours, minutes, seconds);











  //------DISPLAYS HOURS ON THE FIRST TWO COLUMS------
  if (hours >= 20) {
    digitalWrite(hoursPin20, LOW);
    digitalWrite(hoursPin10, HIGH);
  } else if (hours >= 10) {
    digitalWrite(hoursPin20, HIGH);
    digitalWrite(hoursPin10, LOW);
  } else {
    digitalWrite(hoursPin20, HIGH);
    digitalWrite(hoursPin10, HIGH);
  }

  int hoursUnit = hours % 10;
  if (hoursUnit >= 8) {
    digitalWrite(hoursPin8, LOW);
    digitalWrite(hoursPin4, HIGH);
    digitalWrite(hoursPin2, HIGH);
  } else if (hoursUnit >= 6) {
    digitalWrite(hoursPin8, HIGH);
    digitalWrite(hoursPin4, LOW);
    digitalWrite(hoursPin2, LOW);
  } else if (hoursUnit >= 4) {
    digitalWrite(hoursPin8, HIGH);
    digitalWrite(hoursPin4, LOW);
    digitalWrite(hoursPin2, HIGH);
  } else if (hoursUnit >= 2) {
    digitalWrite(hoursPin8, HIGH);
    digitalWrite(hoursPin4, HIGH);
    digitalWrite(hoursPin2, LOW);
  } else {
    digitalWrite(hoursPin8, HIGH);
    digitalWrite(hoursPin4, HIGH);
    digitalWrite(hoursPin2, HIGH);
  }
  if (hoursUnit % 2 == 1) {
    digitalWrite(hoursPin1, LOW);
  } else {
    digitalWrite(hoursPin1, HIGH);
  }

  //-------- DISPLAYS MINUTES ON THE MIDDLE TWO COLUMS ------------
  if (minutes >= 40) {
    digitalWrite(minutesPin40, LOW);
    digitalWrite(minutesPin20, HIGH);
    if (minutes >= 50) {
      digitalWrite(minutesPin10, LOW);
    } else {
      digitalWrite(minutesPin10, HIGH);
    }
  } else if (minutes >= 20) {
    digitalWrite(minutesPin40, HIGH);
    digitalWrite(minutesPin20, LOW);
    if (minutes >= 30) {
      digitalWrite(minutesPin10, LOW);
    } else {
      digitalWrite(minutesPin10, HIGH);
    }
  } else if (minutes >= 10) {
    digitalWrite(minutesPin40, HIGH);
    digitalWrite(minutesPin20, HIGH);
    digitalWrite(minutesPin10, LOW);
  } else {
    digitalWrite(minutesPin40, HIGH);
    digitalWrite(minutesPin20, HIGH);
    digitalWrite(minutesPin10, HIGH);
  }

  int minutesUnit = minutes % 10;
  if (minutesUnit >= 8) {
    digitalWrite(minutesPin8, LOW);
    digitalWrite(minutesPin4, HIGH);
    digitalWrite(minutesPin2, HIGH);
  } else if (minutesUnit >= 6) {
    digitalWrite(minutesPin8, HIGH);
    digitalWrite(minutesPin4, LOW);
    digitalWrite(minutesPin2, LOW);
  } else if (minutesUnit >= 4) {
    digitalWrite(minutesPin8, HIGH);
    digitalWrite(minutesPin4, LOW);
    digitalWrite(minutesPin2, HIGH);
  } else if (minutesUnit >= 2) {
    digitalWrite(minutesPin8, HIGH);
    digitalWrite(minutesPin4, HIGH);
    digitalWrite(minutesPin2, LOW);
  } else {
    digitalWrite(minutesPin8, HIGH);
    digitalWrite(minutesPin4, HIGH);
    digitalWrite(minutesPin2, HIGH);
  }
  if (minutesUnit % 2 == 1) {
    digitalWrite(minutesPin1, LOW);
  } else {
    digitalWrite(minutesPin1, HIGH);
  }

  //------------ DISPLAYS THE SECONDS ON THE LAST TWO COLUMS ----------
  if (seconds >= 40) {
    digitalWrite(secondsPin40, LOW);
    digitalWrite(secondsPin20, HIGH);
    if (seconds >= 50) {
      digitalWrite(secondsPin10, LOW);
    } else {
      digitalWrite(secondsPin10, HIGH);
    }
  } else if (seconds >= 20) {
    digitalWrite(secondsPin40, HIGH);
    digitalWrite(secondsPin20, LOW);
    if (seconds >= 30) {
      digitalWrite(secondsPin10, LOW);
    } else {
      digitalWrite(secondsPin10, HIGH);
    }
  } else if (seconds >= 10) {
    digitalWrite(secondsPin40, HIGH);
    digitalWrite(secondsPin20, HIGH);
    digitalWrite(secondsPin10, LOW);
  } else {
    digitalWrite(secondsPin40, HIGH);
    digitalWrite(secondsPin20, HIGH);
    digitalWrite(secondsPin10, HIGH);
  }

  int secondsUnit = seconds % 10;
  if (secondsUnit >= 8) {
    digitalWrite(secondsPin8, LOW);
    digitalWrite(secondsPin4, HIGH);
    digitalWrite(secondsPin2, HIGH);
  } else if (secondsUnit >= 6) {
    digitalWrite(secondsPin8, HIGH);
    digitalWrite(secondsPin4, LOW);
    digitalWrite(secondsPin2, LOW);
  } else if (secondsUnit >= 4) {
    digitalWrite(secondsPin8, HIGH);
    digitalWrite(secondsPin4, LOW);
    digitalWrite(secondsPin2, HIGH);
  } else if (secondsUnit >= 2) {
    digitalWrite(secondsPin8, HIGH);
    digitalWrite(secondsPin4, HIGH);
    digitalWrite(secondsPin2, LOW);
  } else {
    digitalWrite(secondsPin8, HIGH);
    digitalWrite(secondsPin4, HIGH);
    digitalWrite(secondsPin2, HIGH);
  }
  if (secondsUnit % 2 == 1) {
    digitalWrite(secondsPin1, LOW);
  } else {
    digitalWrite(secondsPin1, HIGH);
  }
}


void loop() {
  //---------Determine if the reset wifi button is pressed------
  button.loop();
  //--Serial communication for debugging--
  serialCOM(); //Serial Data
  //----Set the brightness of the leds----
  brightness();
  //---Physically toggle the leds
  displayLights();
  //---Delay for stability/consistancy----
  delay(10);  //cannot delay due to it being a blocker
}

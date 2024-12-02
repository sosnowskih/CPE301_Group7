//Clauson, John; Sosnowski, Henryk; 
// CPE301 Final Project

#include <dht.h>
#include <Stepper.h>
#include <RTClib.h>
#include <Wire.h>
#include <LiquidCrystal.h>

dht DHT; //Humidity/Temp Sensor
#define DHT11_PIN 8

RTC_DS3231 rtc; //Real Time Clock Module

volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* pin_a = (unsigned char*) 0x20;
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d = (unsigned char*) 0x2A;
volatile unsigned char* pin_d = (unsigned char*) 0x29;
volatile unsigned char* port_e = (unsigned char*) 0x2E; 
volatile unsigned char* ddr_e = (unsigned char*) 0x2D;
volatile unsigned char* pin_e = (unsigned char*) 0x2C;
volatile unsigned char* port_h = (unsigned char*) 0x102; 
volatile unsigned char* ddr_h = (unsigned char*) 0x101;

//Operation variables
unsigned long count;
int state = 2;
//UI controls
const int Start = 19;
//Temperature Sensor
int tempThresh = 27;
//Stepper motor driver
int pin1 = 14; //CHANGE
int pin2 = 15; //CHANGE
int pin3 = 16; //CHANGE
int pin4 = 17; //CHANGE
const int stepsPerRevolution = 2048;
int tracker = 0;
int begin = 0;
//RTC
char t[32];
//LCD Screen
const int RS = 11, EN = 10, D4 = 7, D5 = 6, D6 = 5, D7 = 4;

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Stepper myStepper = Stepper(stepsPerRevolution, pin1, pin3, pin2, pin4);

void setup() {
  Serial.begin(9600); //Serial Monitor Setup
  Serial.println("Serial Monitor Ready");

  attachInterrupt(digitalPinToInterrupt(Start), pin_ISR, RISING); //Setting inturrupt for Start button 

  //Pinmodes for all components
  *ddr_a &= 0xFE; //Sets 22 PA0 to input for Stop Button
  *port_a |= 0x01;
  *ddr_a &= 0xFD; //Sets 23 PA1 to input for Reset Button
  *port_a |= 0x2;
  *ddr_e |= 0x10;  //Sets 2 PE4 to output for Red LED
  *ddr_h |= 0x40; //Sets 9 PH6 to output for Yellow LED
  *ddr_a |= 0x04;  //Sets 24 PA2 to output for Green LED
  *ddr_a |= 0x08;  //Sets 25 PA3 to output for Blue LED
  *ddr_e &= 0xDF; //Sets 3 PE5 to input for water sensor
  *port_e |= 0x20;
  *ddr_a |= 0x80;  //Sets 29 PA7 to output for DC motor control
  *ddr_b |= 0x40;  //Sets 12 PB6 to output for DC motor input 1
  *ddr_b |= 0x80;  //Sets 13 PB7 to output for DC motor input 2

  count = millis(); //inital count value

  Wire.begin(); 
  rtc.begin();
  
  DateTime now = rtc.now(); //Initial Clock Output mm/dd/yyyy "day of the week" hh:mm:ss
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);

  lcd.begin(16,2); //LCD columns and rows
}

void loop() {
  if(((millis() - count) > 1000) && (state != 0)) { //output clock, temperature, and humidity updates every second
    int chk = DHT.read11(DHT11_PIN);
    DateTime now = rtc.now();
    sprintf(t, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.setCursor(0,0);
    lcd.write(t); //prints time to top left corner of lcd screen

    int temperature = DHT.temperature;
    int humidity = DHT.humidity;

    lcd.setCursor(0,1);
    lcd.print(temperature); //prints temp and humidity to lcd screen
    lcd.setCursor(2,1);
    lcd.print("C");
    lcd.setCursor(13,1);
    lcd.print(humidity);
    lcd.setCursor(15,1);
    lcd.print("%");

    count = millis(); //updates "count" value for next iteration
  } 

  if((state != 0) && (digitalRead(ventButton) == 1)) { //Vent control loop //CHANGE
    myStepper.setSpeed(10); //speed set to 10 rpm
    myStepper.step(stepsPerRevolution/10); //rotate a tenth of a rotation every time
    lcd.setCursor(5,1);
    lcd.print("VENT");
    if(tracker == 0) {
      int chk = DHT.read11(DHT11_PIN);
      DateTime now = rtc.now();
      sprintf(t, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
      Serial.print(F("Date/Time: ")); //prints "Date/Time: " to serial monitor 
      Serial.println(t);
      Serial.println("VENT CHANGE");
      tracker++;
    }
  } else {
    lcd.setCursor(5,1);
    lcd.print("    ");
    tracker = 0;
  }

   if(begin == 1) {
    lcd.setCursor(4,0); //reset lcd
    lcd.print("        ");
    state_trans();
    begin = 0;
  }
  
  if(state == 1) { //IDLE STATE
    digitalWrite(GreenLED, HIGH); //CHANGE
    lcd.setCursor(12,0);
    lcd.print("IDLE"); //prints "IDLE" to top right corner of lcd screen
    if((DHT.temperature > tempThresh)) { //if the temp is too high, switch to running
      state = 3;
      digitalWrite(GreenLED, LOW); //CHANGE
      digitalWrite(BlueLED, HIGH); //adjust to running lights //CHANGE
      lcd.clear();
      lcd.setCursor(9,0);
      lcd.print("RUNNING"); //print state to lcd screen
      state_trans(); //run state transition code
    } else if(water_Level() < 100) { //if the water is too low, throw an error
      state = 2;
      digitalWrite(GreenLED, LOW); //CHANGE
      digitalWrite(RedLED, HIGH); //adjust to error lights //CHANGE
      lcd.clear();
      lcd.setCursor(11,0);
      lcd.print("ERROR"); //print state to lcd screen
      state_trans(); //run state transition code
    } else if(digitalRead(Stop) == 1) { //if "stop" is pressed, disable system //CHANGE
      state = 0;
      digitalWrite(GreenLED, LOW); //CHANGE
      digitalWrite(YellowLED, HIGH); //adjust to "stop" lights //CHANGE
      lcd.clear();
      state_trans(); //run state transition code
    }

  } else if(state == 2) { //ERROR STATE
    if(digitalRead(Reset) == 1) { //switch to idle if "reset" is pressed //CHANGE
      state = 1;
      digitalWrite(RedLED, LOW); //CHANGE
      digitalWrite(GreenLED, HIGH); //change lights to idle state //CHANGE
      lcd.clear();
      lcd.setCursor(12,0);
      lcd.print("IDLE"); //print "IDLE" to top right corner of lcd
      state_trans(); //run state transition code
    } else { //otherwise, run error as normal
      digitalWrite(RedLED, HIGH); //CHANGE
      lcd.setCursor(11,0);
      lcd.print("ERROR"); //print "ERROR" to top right corner of lcd
    }

  } else if(state == 3) { //RUNNING STATE
    if(digitalRead(Stop) == 1) { //disable if "stop" is pressed //CHANGE
      state = 0;
      digitalWrite(YellowLED, HIGH); //switch to stop lights //CHANGE
      digitalWrite(BlueLED, LOW); //CHANGE
      digitalWrite(DCmotor, LOW); //turn off fan motor //CHANGE
      lcd.clear();
      state_trans(); //run state transition code
    } else if(DHT.temperature <= tempThresh) { //if temp drops to normal, switch to idle
      state = 1;
      digitalWrite(GreenLED, HIGH); //switch to "IDLE" lights //CHANGE
      digitalWrite(BlueLED, LOW); //CHANGE
      digitalWrite(DCmotor, LOW); //turn off fan motor //CHANGE
      lcd.clear();
      lcd.setCursor(12,0);
      lcd.print("IDLE"); //print idle to top right corner of lcd
      state_trans(); //run state transition code
    } else if(water_Level() < 100) { //if water level drops too low, switch to error
      state = 2;
      digitalWrite(BlueLED, LOW); //CHANGE
      digitalWrite(RedLED, HIGH); //switch to error lights //CHANGE
      digitalWrite(DCmotor, LOW); //turn off fan motor //CHANGE
      lcd.clear();
      lcd.setCursor(11,0);
      lcd.print("ERROR"); //print "ERROR" to top right corner of lcd
      state_trans(); //run state transition code
    } else { //"RUNNING" state
      digitalWrite(BlueLED, HIGH); //CHANGE
      digitalWrite(DCmotor, HIGH); //CHANGE
      lcd.setCursor(9,0);
      lcd.print("RUNNING"); //print "RUNNING" to top right corner of lcd
    }

  } else if(state == 0) { //DISABLED STARE
    digitalWrite(YellowLED, HIGH); //yellow light is on for diabled //CHANGE
    lcd.setCursor(4,0);
    lcd.print("DISABLED"); // only print "DISABLED" on center of lcd
  }
}

void pin_ISR() { //INTERRUPT for "START" button
  if(state == 0) {
    state = 1;
    digitalWrite(YellowLED, LOW); //turn off diabled light //CHANGE
    digitalWrite(GreenLED, HIGH); //turn on IDLE light
    begin = 1;
  }
}

int water_Level() { //function to read water level sensor. Increases simplicity for coding
  return analogRead(water); //CHANGE
}

int state_trans() {
  int chk = DHT.read11(DHT11_PIN);
  DateTime now = rtc.now();
  sprintf(t, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  Serial.print(F("Date/Time: ")); //prints "Date/Time: " to serial monitor
  Serial.print(t);
  Serial.print(" ");
  if(state == 0) { //Prints correct state to serial monitor
    Serial.println("STATE: DISABLED");
    Serial.println("");
  } else if(state == 1) {
    Serial.println("STATE: IDLE");
    Serial.println("");
  } else if(state == 2) {
    Serial.println("STATE: ERROR");
    Serial.println("");
  } else if(state == 3) {
    Serial.println("STATE: RUNNING");
    Serial.println("");
      }
}

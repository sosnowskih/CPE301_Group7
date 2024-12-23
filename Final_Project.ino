//Clauson, John; Sosnowski, Henryk; Carlon, Jordin
// CPE301 Final Project

#include <dht.h>
#include <Stepper.h>
#include <RTClib.h>
#include <Wire.h>
#include <LiquidCrystal.h>

dht DHT; //Humidity/Temp Sensor
#define DHT11_PIN 8

RTC_DS3231 rtc; //Real Time Clock Module

//define RDA and TBE to check for received data and if buffer is empty
#define RDA 0x80
#define TBE 0x20

//Serialprint
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//pin modes
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

//unsigned variables for ADC
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//Operation variables
int water = 3;
unsigned long count;
int state = 2;
int printed = 0; //For serial printing
//UI controls
const int Start = 19;
//Temperature Sensor
int tempThresh = 27;
//Stepper motor driver
int pin1 = 14; 
int pin2 = 15; 
int pin3 = 16; 
int pin4 = 17; 
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
  U0init(9600);
  Serialstring("Serial Monitor Ready");
  U0putchar('\n');

  attachInterrupt(digitalPinToInterrupt(Start), pin_ISR, RISING); //Setting inturrupt for Start button 

  //Pin modes for all components
  *ddr_a &= 0xFE; //Sets 22 PA0 to input for Stop Button
  *port_a |= 0x01;
  *ddr_a &= 0xFD; //Sets 23 PA1 to input for Reset Button
  *port_a |= 0x02;
  *ddr_d &= 0xF7;//Sets 18 PD3 to input for Vent Button
  *port_d |= 0x08;
  *ddr_e |= 0x10;  //Sets 2 PE4 to output for Red LED
  *ddr_h |= 0x40; //Sets 9 PH6 to output for Yellow LED
  *ddr_a |= 0x04;  //Sets 24 PA2 to output for Green LED
  *ddr_a |= 0x08;  //Sets 25 PA3 to output for Blue LED
  *port_e |= 0x20;
  *ddr_a |= 0x80;  //Sets 29 PA7 to output for DC motor enable
  *ddr_b |= 0x40;  //Sets 12 PB6 to output for DC motor input 1
  *ddr_b |= 0x80;  //Sets 13 PB7 to output for DC motor input 2

  count = millis(); //inital count value

  Wire.begin(); 
  rtc.begin();
  
  DateTime now = rtc.now(); //Initial Clock Output mm/dd/yyyy "day of the week" hh:mm:ss
  
  Serialint(now.month());
  Serialstring("/");
  Serialint(now.day());
  Serialstring("/");
  Serialint(now.year());
  Serialstring(" ");
  Serialint(now.hour());
  Serialstring(":");
  Serialint(now.minute());
  Serialstring(":");
  Serialint(now.second());
  Serialstring("\n");

  lcd.begin(16,2); //LCD columns and rows
  adc_init();  //Start the ADC
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

  if((state != 0) && (*pin_d & 0x08)) { //Vent control loop
    myStepper.setSpeed(10); //speed set to 10 rpm
    myStepper.step(stepsPerRevolution/10); //rotate a tenth of a rotation every time
    lcd.setCursor(5,1);
    lcd.print("VENT");
    if(tracker == 0) {
      int chk = DHT.read11(DHT11_PIN);
      DateTime now = rtc.now();
      Serialstring("Date/Time: "); //prints "Date/Time: " to serial monitor 
      Serialint(now.month());
      Serialstring("/");
      Serialint(now.day());
      Serialstring("/");
      Serialint(now.year());
      Serialstring(" ");
      Serialint(now.hour());
      Serialstring(":");
      Serialint(now.minute());
      Serialstring(":");
      Serialint(now.second());
      Serialstring(" ");
      Serialstring("VENT CHANGE\n");
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
    begin = 0;
  }
  
  if(state == 1) { //IDLE STATE
    if(printed == 0){
      state_trans(); //run state transition code
      printed = 1;
    }
    *port_a |= 0x04; //green LED high
    lcd.setCursor(12,0);
    lcd.print("IDLE"); //prints "IDLE" to top right corner of lcd screen
    if((DHT.temperature > tempThresh)) { //if the temp is too high, switch to running
      state = 3;
      printed = 0;
      *port_a &= 0xFB; //green LED low
      *port_a |= 0x08; //blue LED high //adjust to running lights
      lcd.clear();
      lcd.setCursor(9,0);
      lcd.print("RUNNING"); //print state to lcd screen
    } else if(adc_read(0) < 100) { //if the water is too low, throw an error
      state = 2;
      printed = 0;
      *port_a &= 0xFB; //green LED low
      *port_e |= 0x10; // red LED high //adjust to error lights
      lcd.clear();
      lcd.setCursor(11,0);
      lcd.print("ERROR"); //print state to lcd screen
    } else if(*pin_a & 0x01) { //if "stop" is pressed, disable system
      state = 0;
      printed = 0;
      *port_a &= 0xFB; //green LED low
      *port_h |= 0x40; //yellow LED high //adjust to "stop" lights
      lcd.clear();
    }

  } else if(state == 2) { //ERROR STATE
    if(printed == 0){
      state_trans(); //run state transition code
      printed = 1;
    }
    if(*pin_a & 0x02) { //switch to idle if "reset" is pressed
      state = 1;
      printed = 0;
      *port_e &= 0xEF; // red LED low
      *port_a |= 0x04; //green LED high //change lights to idle state 
      lcd.clear();
      lcd.setCursor(12,0);
      lcd.print("IDLE"); //print "IDLE" to top right corner of lcd
    } else { //otherwise, run error as normal
      *port_e |= 0x10; // red LED high
      lcd.setCursor(11,0);
      lcd.print("ERROR"); //print "ERROR" to top right corner of lcd
    }

  } else if(state == 3) { //RUNNING STATE
    if(printed == 0){
      state_trans(); //run state transition code
      printed = 1;
    }
    *port_b |= 0x40; //set DC motor direction
    *port_b &= 0x7F; //set DC motor direction
    if(*pin_a & 0x01) { //disable if "stop" is pressed
      state = 0;
      printed = 0;
      *port_h |= 0x40; //yellow LED high //switch to stop lights
      *port_a &= 0xF7; //blue LED low
      *port_a &= 0x7F; //DC motor disable //turn off fan motor
      lcd.clear();
    } else if(DHT.temperature <= tempThresh) { //if temp drops to normal, switch to idle
      state = 1;
      printed = 0;
      *port_a |= 0x04; //green LED high //switch to "IDLE" lights
      *port_a &= 0xF7; //blue LED low
      *port_a &= 0x7F; //DC motor disable //turn off fan motor
      lcd.clear();
      lcd.setCursor(12,0);
      lcd.print("IDLE"); //print idle to top right corner of lcd
    } else if(adc_read(0) < 100) { //if water level drops too low, switch to error
      state = 2;
      printed = 0;
      *port_a &= 0xF7; //blue LED low
      *port_e |= 0x10; // red LED high //switch to error lights
      *port_a &= 0x7F; //DC motor disable //turn off fan motor
      lcd.clear();
      lcd.setCursor(11,0);
      lcd.print("ERROR"); //print "ERROR" to top right corner of lcd
    } else { //"RUNNING" state
      *port_a |= 0x08; //blue LED high
      *port_a |= 0x80; //DC motor enable
      lcd.setCursor(9,0);
      lcd.print("RUNNING"); //print "RUNNING" to top right corner of lcd
    }

  } else if(state == 0) { //DISABLED STATE
    if(printed == 0){
      state_trans(); //run state transition code
      printed = 1;
    }
    *port_h |= 0x40; //yellow LED high //yellow light is on for diabled
    lcd.setCursor(4,0);
    lcd.print("DISABLED"); // only print "DISABLED" on center of lcd
  }
}

void pin_ISR() { //INTERRUPT for "START" button
  if(state == 0) {
    state = 1;
    *port_h &= 0xBF; //yellow LEF low //turn off diabled light
    *port_a |= 0x04; //green LED high //turn on IDLE light
    begin = 1;
    printed = 0;
  }
}

int state_trans() {
  int chk = DHT.read11(DHT11_PIN);
  DateTime now = rtc.now();
  while(now.day()<1 || now.day()>31){
    now = rtc.now();
  }
  Serialstring("Date/Time: "); //prints "Date/Time: " to serial monitor 
  Serialint(now.month());
  Serialstring("/");
  Serialint(now.day());
  Serialstring("/");
  Serialint(now.year());
  Serialstring(" ");
  Serialint(now.hour());
  Serialstring(":");
  Serialint(now.minute());
  Serialstring(":");
  Serialint(now.second());
  Serialstring(" ");
  if(state == 0) { //Prints correct state to serial monitor
    Serialstring("STATE: DISABLED\n");
  } else if(state == 1) {
    Serialstring("STATE: IDLE\n");
  } else if(state == 2) {
    Serialstring("STATE: ERROR\n");
  } else if(state == 3) {
    Serialstring("STATE: RUNNING\n");
    }
}

//initializes the serial monitor
void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

//prints an int between 1 to 4 digits long to the serial monitor 
void Serialint(int input){
  int length = 1;
  int inputl = input;
  for (int i = 0; inputl/10 !=0; i++){
    length += 1;
    inputl = inputl/10;
  }
  int print[length] = {0};
  for (int n=1; n <= length; n++){
    print[length-n] = input%10;
    input = input/10;
  }
  for (int n=0; n < length; n++){
    while((*myUCSR0A & TBE)==0);
    *myUDR0 = print[n] + '0';
  }
}

//prints a char array to the serial monitor
void Serialstring(unsigned char input[]) {
  for (int i=0; input[i]; i++){
    while((*myUCSR0A & TBE)==0);
    *myUDR0 = input[i];
  }
}

//prints a char to the serial monitor
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE) == 0);
    *myUDR0 = U0pdata;
}

//These functions setup ADC and get data from UART
//setup ADC
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit 7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
//read ADC channel
unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

//check for data
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
//get data from the UART
unsigned char U0getchar()
{
  return *myUDR0;
}

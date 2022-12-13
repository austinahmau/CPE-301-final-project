#include <LiquidCrystal.h>
#include "DHT.h"
#include "RTClib.h"
#include <Wire.h>
#include <Stepper.h>

#define RDA 0x80
#define TBE 0x20

//Temperature and Humidity Initialization
#define DHTPIN 6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//LCD Initialization
LiquidCrystal lcd(7, 8, 9, 10 ,11 , 12); //rs = pin 7, e = pin 8, d4 = pin 9, d5 = pin 10, d6 = pin 11, d7 = pin 12 

//Port A registers for LEDs
volatile unsigned char* port_a =(unsigned char*) 0x22;
volatile unsigned char* ddr_a =(unsigned char*) 0x21;
volatile unsigned char* pin_a =(unsigned char*) 0x20;

//Port C registers for buttons
volatile unsigned char* port_c =(unsigned char*) 0x28;
volatile unsigned char* ddr_c =(unsigned char*) 0x27;
volatile unsigned char* pin_c =(unsigned char*) 0x26;

//Port L registers for motor/fan
volatile unsigned char* port_l =(unsigned char*) 0x10B;
volatile unsigned char* ddr_l =(unsigned char*) 0x10A;
volatile unsigned char* pin_l =(unsigned char*) 0x109;

//ADC registers
volatile unsigned char* my_ADMUX = (unsigned char *) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char *) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char *) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int *) 0x78;

//UART registers
volatile unsigned char *myUCSR0A = (unsigned char *) 0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *) 0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *) 0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *) 0x00C6;

//clock instantiation
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//stepper motor instantiation
const int stepsPerRevolution = 32;
Stepper myStepper = Stepper(stepsPerRevolution, 2, 4, 3, 5);

//Threshold variables
unsigned int water_threshold = 300;
unsigned int humidity_threshold = 0;
unsigned int temp_threshold = 30;

int water_level;
float humidity;
float temp;

unsigned int mode = 0;

void setup() {
  U0init(9600);
  adc_init();

  //set up LCD and DHT
  lcd.begin(16,2);
  dht.begin();
  
  //set port C bits 7-4 to inputs = pins 30, 31, 32, 33 (use for buttons)
  *ddr_c &= 0b00001111;
  //enable pullup resistors for port E bits 7-5
  *port_c |= 0b11110000;

  //set port A bits 0-3 to outputs = pins 22, 23, 24, 25 (use for LEDs)
  *ddr_a |= 0b00001111;

  //set Port L bits 0 and 2 to outputs = pins 49 and 47 (use for motor/fan)
  *ddr_l |= 0b00000101;

  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}

void loop(){
  timeStamp();
  switch(mode){
    case 1:
      Serial.print(" entered idle mode ");
      Serial.println();
      idleMode();
      break;
    case 2:
      Serial.print(" entered running mode ");
      Serial.println();
      runningMode();
      break;
    case 3:
      Serial.print(" entered error mode ");
      Serial.println();
      errorMode();
      break;
    default:
      Serial.print(" entered disabled mode ");
      Serial.println();
      disabledMode();
      break;
  }
}

void LCD_display(){  
  int read_values = dht.read(DHTPIN);
  
  humidity = dht.readHumidity();
  temp = dht.readTemperature(true);

  if(isnan(humidity) || isnan(temp)){
    lcd.setCursor(0,1);
    lcd.print("ERROR           ");
    return;
  }
  lcd.print("Temp    Humidity");
  lcd.setCursor(0,1);
  lcd.print(temp);
  lcd.print((char)223);
  lcd.print("F");
  
  lcd.setCursor(8,1);
  lcd.print(humidity);
  lcd.print("%");

  delay(200);
}

void timeStamp(){
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
}

void ventDirection(){
  bool clockwise = *pin_c & 0b10000000; //left button
  bool anticlockwise = *pin_c & 0b01000000; //right button

  if(clockwise){
    myStepper.setSpeed(200);
    myStepper.step(-50);
    Serial.print("vent up");
  }
  if(anticlockwise){
    myStepper.setSpeed(200);
    myStepper.step(50);
    Serial.print("vent down");
  }
}

int checkStartButton(){
  if(*pin_c & 0b00100000){
    while(*pin_c & 0b00100000){}
    return 0;
  }
  return -1;
}

void disabledMode(){
  *pin_a |= 0b00000001; //yellow LED on
  
  while(mode == 0){    
    if(checkStartButton() == 0){
      mode = (mode == 0 ? 1 : 0);
    }
  }
  *port_a &= 0b11111110;  
}

void idleMode(){
  *pin_a |= 0b00000010; //green LED on
  LCD_display();
  
  while(mode == 1){
    water_level = adc_read(0); 
    temp = dht.readTemperature(true);
    
    if(checkStartButton() == 0){
      mode = (mode == 0 ? 1 : 0);
    }
    if(water_level <= water_threshold){
      mode = 3;
    }
    if(temp > temp_threshold){
      mode = 2;
    }
  }
  *port_a &= 0b11111101; 
}

void runningMode(){
  *pin_a |= 0b00000100; //blue LED on
  *port_l |= 0b00000001;
  *port_l |= 0b00000100;

  while(mode == 2){
    water_level = adc_read(0); 
    temp = dht.readTemperature(true);
    if(water_level <= water_threshold){
      mode = 3;
    }
    if(temp < temp_threshold){
      mode = 1;
    }
    if(checkStartButton() == 0){
      mode = 0;
    }
  }
  
  lcd.clear();  
  *port_l &= 0b11111011;
  *port_a &= 0b11111011;
}

void errorMode(){
  *pin_a |= 0b00001000; //red LED on
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WATER LEVEL LOW");
  
  while(mode == 3){
    water_level = adc_read(0);

    if(water_level > water_threshold){
      if(checkStartButton() == 0){
        mode = 0;
      }
    }
  }
  lcd.clear();
  *port_a &= 0b11110111;
}

void adc_init(){
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 5 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11011111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11011111; // clear bit 5 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11011111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00100000;
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

void print_int(unsigned int out_num){
  // clear a flag (for printing 0's in the middle of numbers)
  unsigned char print_flag = 0;
  // if its greater than 1000
  if(out_num >= 1000)
  {
    // get the 1000's digit, add to '0' 
    U0putchar(out_num / 1000 + '0');
    // set the print flag
    print_flag = 1;
    // mod the out num by 1000
    out_num = out_num % 1000;
  }
  // if its greater than 100 or we've already printed the 1000's
  if(out_num >= 100 || print_flag)
  {
    // get the 100's digit, add to '0'
    U0putchar(out_num / 100 + '0');
    // set the print flag
    print_flag = 1;
    // mod the output num by 100
    out_num = out_num % 100;
  } 
  // if its greater than 10, or we've already printed the 10's
  if(out_num >= 10 || print_flag)
  {
    U0putchar(out_num / 10 + '0');
    print_flag = 1;
    out_num = out_num % 10;
  } 
  // always print the last digit (in case it's 0)
  U0putchar(out_num + '0');
  // print a newline
  U0putchar('\n');
}

void U0init(int U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char U0kbhit(){
  return *myUCSR0A & RDA;
}

unsigned char U0getchar(){
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata){
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

#include <LiquidCrystal.h>
#include "DHT.h"
#include "RTClib.h"
#include <Wire.h>
#include <Stepper.h>


#define DHTPIN 6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//rs = pin 7, e = pin 8, d4 = pin 9, d5 = pin 10, d6 = pin 11, d7 = pin 12 
LiquidCrystal lcd(7, 8, 9, 10 ,11 , 12);

volatile unsigned char* port_a =(unsigned char*) 0x22;
volatile unsigned char* ddr_a =(unsigned char*) 0x21;
volatile unsigned char* pin_a =(unsigned char*) 0x20;

volatile unsigned char* port_c =(unsigned char*) 0x28;
volatile unsigned char* ddr_c =(unsigned char*) 0x27;
volatile unsigned char* pin_c =(unsigned char*) 0x26;

volatile unsigned char* my_ADMUX = (unsigned char *) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char *) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char *) 0x7A;
volatile unsigned char* my_ADC_DATA = (unsigned char *) 0x78;

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const int stepsPerRevolution = 32;
Stepper myStepper = Stepper(stepsPerRevolution, 2, 4, 3, 5);

void setup() {
  Serial.begin(9600);
  adc_init();

  //set up LCD and DHT
  lcd.begin(16,2);
  dht.begin();
  lcd.print("Temp    Humidity");
  
  //set port C bits 7-5 to inputs = pins 30, 31, 32 (use for buttons)
  *ddr_c &= 0b00011111;
  //enable pullup resistors for port E bits 7-5
  *port_c |= 0b11100000;

  //set port A bits 0-3 to outputs = pins 22, 23, 24, 25 (use for LEDs)
  *ddr_a |= 0b00001111;

  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}

void loop() {
    bool clockwise = *pin_c & 0b01000000; //left button
    bool anticlockwise = *pin_c & 0b10000000; //right button

    if(clockwise){
      myStepper.setSpeed(200);
      myStepper.step(50);
      Serial.print("Vent up at: ");
      timeStamp();
    }
    else if(anticlockwise){
      myStepper.setSpeed(200);
      myStepper.step(-50);
      Serial.print("Vent down at: ");
      timeStamp();
    }
}

void adc_init(){
  *my_ADCSRA |= 0b10000000; //set bit 7 to 1 to enable ADC
  *my_ADCSRA &= 0b11011111; //clear bit 5 to 0 to disable ADC trigger mode and interrupt, and set prescalar selection to slow reading

  *my_ADCSRB &= 0b11110000; //clear bit 3 to 0 to reset the channel and gain bits and clear bits 2-0 to set to free running mode

  *my_ADMUX &= 0b01111111; //clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX |= 0b01000000; //set bit 6 to 1 for AVCC analog reference
  *my_ADMUX &= 0b11011111; //clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11100000; //clear bits 4-0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX &= 0b11100000; //clear the channel selection bits
  *my_ADCSRB &= 0b11011111; //clear the channel selection bits

  //set channel selection bits, but remove the MSB (bit 3)
  if(adc_channel_num > 7){
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00100000;
  }

  *my_ADMUX += adc_channel_num; //set channel selection bits
  *my_ADCSRA |= 0b01000000; //set bit 6 of ADCSRA to 1 to start conversion

  //wait for conversion to complete
  while((*my_ADCSRA & 0b01000000) != 0){
    return *my_ADC_DATA; //return result to ADC data register
  }
  
}

void LCD_display(){
  delay(2000);
  
  int read_values = dht.read(DHTPIN);
  
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature(true);

  if(isnan(humidity) || isnan(temp)){
    lcd.setCursor(0,1);
    lcd.print("ERROR           ");
    return;
  }

  lcd.setCursor(0,1);
  lcd.print(temp);
  lcd.print((char)223);
  lcd.print("F");
  
  lcd.setCursor(8,1);
  lcd.print(humidity);
  lcd.print("%");
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
  Serial.println();
}

void disabledMode(){
  *pin_a |= 0b00000001; //yellow LED pin 22
  
}

void idleMode(){
  *pin_a |= 0b00000010; //green LED pin 23
}

void runningMode(){
  *pin_a |= 0b00000100; //blue LED pin 24
}

void errorMode(){
  *pin_a |= 0b00001000; //red LED pin 25
}

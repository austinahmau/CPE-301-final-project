#include <LiquidCrystal.h>
#include "DHT.h"

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

void setup() {
  Serial.begin(9600);

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
}

void loop() {
  
}

void adc_init(){
  
}

unsigned int adc_read(unsigned char adc_channel_num){
  
}

void display_values(){
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

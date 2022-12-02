#include <LiquidCrystal.h> //library for display
#include "DHT.h" //library for Humiditity & Temp monitor

#define DHTPIN 6
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

//rs = pin 7, e = pin 8, d4 = pin 9, d5 = pin 10, d6 = pin 11, d7 = pin 12 
LiquidCrystal lcd(7, 8, 9, 10 ,11 , 12);

void setup() {
  Serial.begin(9600);
  lcd.begin(16,2);
  dht.begin();

  lcd.print("Temp:");
  lcd.setCursor(0,1);
  lcd.print("Humidity:");
}

void loop() {
  delay(2000);
  
  int read_values = dht.read(DHTPIN);
  
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature(true);

  if(isnan(humidity) || isnan(temp)){
    lcd.print("ERROR");
    return;
  }

  lcd.setCursor(6,0);
  lcd.print(temp);
  lcd.print((char)223);
  lcd.print("F");
  
  lcd.setCursor(10,1);
  lcd.print(humidity);
  lcd.print("%");
}

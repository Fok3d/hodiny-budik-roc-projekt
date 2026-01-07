#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h> 


LiquidCrystal_I2C lcd(0x27, 16, 2);


RTC_DS3231 rtc; 

unsigned long previousConsoleMillis = 0;
const long consoleInterval = 1000;

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  lcd.print("Dobry den :)");
  lcd.setCursor(0, 1);
  lcd.print("Startuji...");
  delay(2000);
  lcd.clear();

  if (!rtc.begin()) {
    Serial.println("RTC nenalezeno! Zkontrolujte zapojeni.");
    lcd.print("Chyba RTC!");
    while (1);
  }


  if (rtc.lostPower()) {
    Serial.println("RTC ztratilo napajeni, nastavuji cas!");
    
    
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();


  lcd.setCursor(0, 0);
  if (now.day() < 10) lcd.print('0');
  lcd.print(now.day());
  lcd.print('.');
  if (now.month() < 10) lcd.print('0');
  lcd.print(now.month());
  lcd.print('.');
  lcd.print(now.year());

  lcd.setCursor(0, 1);
  lcd.print("Cas: ");
  
  if (now.hour() < 10) lcd.print('0');
  lcd.print(now.hour());
  lcd.print(':');
  
  if (now.minute() < 10) lcd.print('0');
  lcd.print(now.minute());
  lcd.print(':');
  
  if (now.second() < 10) lcd.print('0');
  lcd.print(now.second());


  unsigned long currentMillis = millis();
  if (currentMillis - previousConsoleMillis >= consoleInterval) {
    previousConsoleMillis = currentMillis;

    Serial.print("Aktualni cas: ");
    if (now.day() < 10) Serial.print('0'); Serial.print(now.day()); Serial.print('.');
    if (now.month() < 10) Serial.print('0'); Serial.print(now.month()); Serial.print('.');
    Serial.print(now.year());
    Serial.print(" ");
    
    if (now.hour() < 10) Serial.print('0'); Serial.print(now.hour()); Serial.print(':');
    if (now.minute() < 10) Serial.print('0'); Serial.print(now.minute()); Serial.print(':');
    if (now.second() < 10) Serial.print('0'); Serial.println(now.second());
  }

  delay(50); 
}
#include <LiquidCrystal_I2C.h> 
#include <Wire.h> 
#include <DS3231.h> 

LiquidCrystal_I2C lcd(0x27, 16, 2); 


DS3231 rtc; 
bool century = false; 
bool h12; 
bool PM;  

//priprava
const int buzzerPin = 8;
const int modeButtonPin = 9;
const int setButtonPin = 10;
const int alarmButtonPin = 7;

unsigned long previousConsoleMillis = 0;
const long consoleInterval = 1000; 

void setup() {
  
  Serial.begin(9600); 
  Serial.println("Arduino Hodiny s RTC - Start");
  
  Wire.begin();
  
  
  rtc.setHour(14); 
  rtc.setMinute(55); 
  rtc.setSecond(0);  
  rtc.setDate(7); 
  rtc.setMonth(12); 
  rtc.setYear(25); 
  

  lcd.init(); 
  lcd.backlight(); 
  
  lcd.print("Hodiny DS3231");
  lcd.setCursor(0, 1);
  lcd.print("Startuji...");
  delay(2000);
  lcd.clear(); 
}

void loop() {
  
  
  int hours = rtc.getHour(h12, PM);
  int minutes = rtc.getMinute(); 
  int seconds = rtc.getSecond(); 
  
  int day = rtc.getDate();
  int month = rtc.getMonth(century);
  int year = rtc.getYear();
  
  // DD.MM:RR
  lcd.setCursor(0, 0);
  if (day < 10) lcd.print('0');
  lcd.print(day);
  lcd.print('.');
  if (month < 10) lcd.print('0');
  lcd.print(month);
  lcd.print('.');
  if (year < 10) lcd.print('0');
  lcd.print(year); 
  lcd.print("    "); 
  

  lcd.setCursor(0, 1);
  lcd.print("Cas: ");
  
  lcd.setCursor(5, 1); 
  if (hours < 10) lcd.print('0');
  lcd.print(hours);
  lcd.print(':');
  
  if (minutes < 10) lcd.print('0');
  lcd.print(minutes);
  lcd.print(':');
  
  if (seconds < 10) lcd.print('0');
  lcd.print(seconds);
  lcd.print("   "); 

  
  unsigned long currentMillis = millis();
  if (currentMillis - previousConsoleMillis >= consoleInterval) {
    previousConsoleMillis = currentMillis;

    Serial.print("Aktualny cas: ");
    
    if (day < 10) Serial.print('0'); Serial.print(day); Serial.print('.');
    if (month < 10) Serial.print('0'); Serial.print(month); Serial.print('.');
    Serial.print(year + 2000);
    Serial.print(" ");
    
    if (hours < 10) Serial.print('0'); Serial.print(hours); Serial.print(':');
    if (minutes < 10) Serial.print('0'); Serial.print(minutes); Serial.print(':');
    if (seconds < 10) Serial.print('0'); Serial.println(seconds);
  }

  delay(50); 
}
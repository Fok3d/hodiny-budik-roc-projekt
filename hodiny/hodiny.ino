#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>


const int PIN_BTN_OK   = 6;  
const int PIN_BTN_BACK = 7;  
const int PIN_BUZZER   = 8;  
const int PIN_BTN_UP   = 9; 
const int PIN_BTN_DOWN = 10;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;


int alarmHour = 6;      
int alarmMinute = 30;   
bool alarmDays[7] = {true, true, true, true, true, false, false}; 


bool alarmActive = false;      
unsigned long lastBuzzerTime = 0;


enum State {
  SHOW_TIME,      
  SET_DAYS,       
  SET_HOUR,       
  SET_MINUTE,     
  ALARM_RINGING    
};

State currentState = SHOW_TIME;
int cursorPosition = 0; 


const char daysChar[] = {'P', 'U', 'S', 'C', 'P', 'S', 'N'};

void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_BTN_BACK, INPUT_PULLUP);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();
  
  if (!rtc.begin()) {
    lcd.print("Chyba RTC!");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
 
  bool btnOk = digitalRead(PIN_BTN_OK) == LOW;
  bool btnBack = digitalRead(PIN_BTN_BACK) == LOW;
  bool btnUp = digitalRead(PIN_BTN_UP) == LOW;
  bool btnDown = digitalRead(PIN_BTN_DOWN) == LOW;

  if (btnOk || btnBack || btnUp || btnDown) {
    delay(150); 
  }

 
  switch (currentState) {
    
    
    case SHOW_TIME:
      displayTime();
      checkAlarm(); 

      if (btnOk) {
        currentState = SET_DAYS;
        cursorPosition = 0;     
        lcd.clear();
      }
      if (btnBack) {
         
      }
      break;

   
    case SET_DAYS:
      displayDaysMenu();
      
      if (btnUp || btnDown) {
        
        alarmDays[cursorPosition] = !alarmDays[cursorPosition];
      }
      
      if (btnOk) {
        cursorPosition++; 
        if (cursorPosition > 6) {
         
          currentState = SET_HOUR;
          lcd.clear();
        }
      }
      
      if (btnBack) {
        currentState = SHOW_TIME; 
        lcd.clear();
      }
      break;

    
    case SET_HOUR:
      displayTimeSetMenu();
      
      if (btnUp) { alarmHour++; if (alarmHour > 23) alarmHour = 0; }
      if (btnDown) { alarmHour--; if (alarmHour < 0) alarmHour = 23; }
      
      if (btnOk) {
        currentState = SET_MINUTE; 
      }
      if (btnBack) {
        currentState = SHOW_TIME;
        lcd.clear();
      }
      break;

    
    case SET_MINUTE:
      displayTimeSetMenu(); 
      
      if (btnUp) { alarmMinute++; if (alarmMinute > 59) alarmMinute = 0; }
      if (btnDown) { alarmMinute--; if (alarmMinute < 0) alarmMinute = 59; }
      
      if (btnOk) {
        
        currentState = SHOW_TIME;
        lcd.clear();
        lcd.print("Budik ulozen!");
        delay(1000);
        lcd.clear();
      }
      if (btnBack) {
        currentState = SET_HOUR; 
      }
      break;

    
    case ALARM_RINGING:
      lcd.setCursor(0, 0);
      lcd.print("!!! VSTAVAT !!!");
      lcd.setCursor(0, 1);
      lcd.print("zmackni cokoliv");
      
      
      if (millis() - lastBuzzerTime > 200) {
        tone(PIN_BUZZER, 1000, 100); 
        lastBuzzerTime = millis();
      }

     
      if (btnOk || btnBack || btnUp || btnDown) {
        alarmActive = false;
        currentState = SHOW_TIME;
        lcd.clear();
        delay(500); 
      }
      break;
  }
}



void displayTime() {
  DateTime now = rtc.now();
  
  lcd.setCursor(0, 0);
  lcd.print("Cas: ");
  if (now.hour() < 10) lcd.print('0'); lcd.print(now.hour()); lcd.print(':');
  if (now.minute() < 10) lcd.print('0'); lcd.print(now.minute()); lcd.print(':');
  if (now.second() < 10) lcd.print('0'); lcd.print(now.second());


  int todayIndex = (now.dayOfTheWeek() == 0) ? 6 : now.dayOfTheWeek() - 1;
  
  lcd.setCursor(0, 1);
  lcd.print("Budik: ");
 
  if (alarmDays[todayIndex]) {
    if (alarmHour < 10) lcd.print('0'); lcd.print(alarmHour);
    lcd.print(':');
    if (alarmMinute < 10) lcd.print('0'); lcd.print(alarmMinute);
  } else {
    lcd.print("--:--");
  }
}

void displayDaysMenu() {
  lcd.setCursor(0, 0);
  lcd.print("Vyber dny:");
  
  lcd.setCursor(0, 1);
  
  for (int i = 0; i < 7; i++) {
    if (alarmDays[i]) {
      lcd.print(daysChar[i]); 
    } else {
      lcd.print('_'); 
    }
    lcd.print(' '); 
  }

  
  lcd.setCursor(cursorPosition * 2, 1);
  lcd.blink(); 
}

void displayTimeSetMenu() {
  lcd.noBlink();
  lcd.setCursor(0, 0);
  lcd.print("Nastav cas budiku");
  
  lcd.setCursor(4, 1);
  if (alarmHour < 10) lcd.print('0'); lcd.print(alarmHour);
  lcd.print(':');
  if (alarmMinute < 10) lcd.print('0'); lcd.print(alarmMinute);

  
  if (currentState == SET_HOUR) {
    lcd.setCursor(4, 1); 
    lcd.blink(); 
  } else if (currentState == SET_MINUTE) {
    lcd.setCursor(7, 1);
    lcd.blink(); 
  }
}

void checkAlarm() {
  DateTime now = rtc.now();
  

  if (now.second() == 0) {
    
    if (now.hour() == alarmHour && now.minute() == alarmMinute) {
      
      int todayIndex = (now.dayOfTheWeek() == 0) ? 6 : now.dayOfTheWeek() - 1;
      
      if (alarmDays[todayIndex] == true) {
        currentState = ALARM_RINGING; 
      }
    }
  }
}
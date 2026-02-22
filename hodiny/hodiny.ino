#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <DCF77.h>
#include <TimeLib.h>//tato knihovna je externi

const int PIN_BTN_OK   = 6;  
const int PIN_BTN_BACK = 7; 
const int PIN_BUZZER   = 8; 
const int PIN_BTN_UP   = 9;  
const int PIN_BTN_DOWN = 10; 

const int PIN_DCF = 2; 
const int DCF_INTERRUPT = 0; 
DCF77 dcf = DCF77(PIN_DCF, DCF_INTERRUPT, false);

LiquidCrystal_I2C lcd(0x27, 16, 2); 
RTC_DS3231 rtc;

int alarmHour = 6;      
int alarmMinute = 30;   
bool alarmDays[7] = {true, true, true, true, true, false, false}; 
bool alarmActive = false;       

unsigned long lastBuzzerTime = 0;
unsigned long lastDisplayUpdate = 0; 
unsigned long lastBtnPress = 0;
unsigned long syncStartTime = 0;
unsigned long alarmRingingStartTime = 0;

enum State {
  SYNC_DCF,        
  SHOW_TIME,       
  SET_ALARM_DAYS,        
  SET_ALARM_HOUR,        
  SET_ALARM_MINUTE,
  SET_YEAR,
  SET_MONTH,
  SET_DAY,
  SET_TIME_HOUR,
  SET_TIME_MINUTE,
  ALARM_RINGING    
};

State currentState = SHOW_TIME;
int cursorPosition = 0; 
const char daysChar[] = {'P', 'U', 'S', 'C', 'P', 'S', 'N'};

int tempYear, tempMonth, tempDay, tempHour, tempMinute;

void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_BTN_BACK, INPUT_PULLUP);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Dobry den :)");
  lcd.setCursor(0, 1);
  lcd.print("Startuji...");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Nacitam z DCF.");
  lcd.setCursor(0, 1);
  lcd.print("Prosim cekejte..");
  delay(3000);
  lcd.clear();
  
  if (!rtc.begin()) {
    lcd.print("Chyba RTC!");
    while (1);
  }
  dcf.Start();

  currentState = SYNC_DCF;
  syncStartTime = millis();
}

void loop() {
  bool btnOk = digitalRead(PIN_BTN_OK) == LOW;
  bool btnBack = digitalRead(PIN_BTN_BACK) == LOW;
  bool btnUp = digitalRead(PIN_BTN_UP) == LOW;
  bool btnDown = digitalRead(PIN_BTN_DOWN) == LOW;

  if (btnOk || btnBack || btnUp || btnDown) {
    if (millis() - lastBtnPress < 200) return; 
    lastBtnPress = millis();
  }

  if (btnOk && btnBack && btnUp && btnDown && currentState == SHOW_TIME) {
    lcd.clear(); lcd.print("MASTER RESET"); delay(1000); lcd.clear();
    DateTime now = rtc.now();
    tempYear = now.year(); tempMonth = now.month(); tempDay = now.day();
    tempHour = now.hour(); tempMinute = now.minute();
    currentState = SET_YEAR;
    return; 
  }

  switch (currentState) {
    case SYNC_DCF:
      {
        lcd.setCursor(0, 0);
        lcd.print("Hledam DCF77...");
    
        unsigned long elapsed = (millis() - syncStartTime) / 1000;
        lcd.setCursor(0, 1);
        lcd.print("Cas: "); lcd.print(elapsed); lcd.print(" s  ");

        // Kontrola dcf
        time_t DCFtime = dcf.getTime(); 
        if (DCFtime != 0) {
          rtc.adjust(DateTime(year(DCFtime), month(DCFtime), day(DCFtime), hour(DCFtime), minute(DCFtime), second(DCFtime)));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("DCF Nacteno!");
          delay(2000);
          lcd.clear();
          currentState = SHOW_TIME;
        }

        //pokud dcf do 3 minu(could be less)nebo zmacknejs back nastavujes manual
        if (elapsed > 180 || btnBack) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("DCF Nenalezeno!");
          delay(2000);
          lcd.clear();
          
          DateTime now = rtc.now();
          tempYear = now.year(); 
          tempMonth = now.month(); 
          tempDay = now.day();
          tempHour = now.hour(); 
          tempMinute = now.minute();
          
          currentState = SET_YEAR; 
        }
      }
      break;

    //ukazovani timu
    case SHOW_TIME:
      if (millis() - lastDisplayUpdate > 500) { 
        displayMainScreen();
        lastDisplayUpdate = millis();
      }
      checkAlarm(); 

      if (btnOk && !btnBack) { 
        currentState = SET_ALARM_DAYS; 
        cursorPosition = 0;      
        lcd.clear();
      }
      break;

    
    case SET_ALARM_DAYS:
      displayDaysMenu();
      if (btnUp || btnDown) alarmDays[cursorPosition] = !alarmDays[cursorPosition];
      if (btnOk) {
        cursorPosition++; 
        if (cursorPosition > 6) { currentState = SET_ALARM_HOUR; lcd.clear(); }
      }
      if (btnBack) { currentState = SHOW_TIME; lcd.clear(); }
      break;

    case SET_ALARM_HOUR:
      displaySetAlarmTime("Budik Hodina:");
      if (btnUp) { alarmHour++; if (alarmHour > 23) alarmHour = 0; }
      if (btnDown) { alarmHour--; if (alarmHour < 0) alarmHour = 23; }
      if (btnOk) { currentState = SET_ALARM_MINUTE; }
      if (btnBack) { currentState = SHOW_TIME; lcd.clear(); }
      break;

    case SET_ALARM_MINUTE:
      displaySetAlarmTime("Budik Minuta:");
      if (btnUp) { alarmMinute++; if (alarmMinute > 59) alarmMinute = 0; }
      if (btnDown) { alarmMinute--; if (alarmMinute < 0) alarmMinute = 59; }
      if (btnOk) {
        currentState = SHOW_TIME; lcd.clear(); lcd.print("Budik ulozen!"); delay(1000); lcd.clear();
      }
      if (btnBack) { currentState = SET_ALARM_HOUR; }
      break;

    // kdyz dcf nenajde nebo zmackens reset
    case SET_YEAR:
      displayConfigVal("Nastav Rok:", tempYear);
      if (btnUp) tempYear++; if (btnDown) tempYear--;
      if (btnOk) { currentState = SET_MONTH; lcd.clear(); }
      if (btnBack) { currentState = SHOW_TIME; lcd.clear(); }
      break;

    case SET_MONTH:
      displayConfigVal("Nastav Mesic:", tempMonth);
      if (btnUp) { tempMonth++; if(tempMonth > 12) tempMonth = 1; }
      if (btnDown) { tempMonth--; if(tempMonth < 1) tempMonth = 12; }
      if (btnOk) { currentState = SET_DAY; lcd.clear(); }
      if (btnBack) currentState = SET_YEAR;
      break;

    case SET_DAY:
      displayConfigVal("Nastav Den:", tempDay);
      if (btnUp) { tempDay++; if(tempDay > 31) tempDay = 1; } 
      if (btnDown) { tempDay--; if(tempDay < 1) tempDay = 31; }
      if (btnOk) { currentState = SET_TIME_HOUR; lcd.clear(); }
      if (btnBack) currentState = SET_MONTH;
      break;

    case SET_TIME_HOUR:
      displayConfigVal("Nastav Hodinu:", tempHour);
      if (btnUp) { tempHour++; if(tempHour > 23) tempHour = 0; }
      if (btnDown) { tempHour--; if(tempHour < 0) tempHour = 23; }
      if (btnOk) { currentState = SET_TIME_MINUTE; lcd.clear(); }
      if (btnBack) currentState = SET_DAY;
      break;

    case SET_TIME_MINUTE:
      displayConfigVal("Nastav Minutu:", tempMinute);
      if (btnUp) { tempMinute++; if(tempMinute > 59) tempMinute = 0; }
      if (btnDown) { tempMinute--; if(tempMinute < 0) tempMinute = 59; }
      if (btnOk) {
        rtc.adjust(DateTime(tempYear, tempMonth, tempDay, tempHour, tempMinute, 0));
        lcd.clear(); lcd.print("Cas nastaven!"); delay(1000); lcd.clear();
        currentState = SHOW_TIME;
      }
      if (btnBack) currentState = SET_TIME_HOUR;
      break;

    //zvonim
    case ALARM_RINGING:
      lcd.setCursor(0, 0);
      lcd.print("!!! VSTAVAT !!!");
      
      if (millis() - lastBuzzerTime > 500) {
        tone(PIN_BUZZER, 2000, 200); 
        lastBuzzerTime = millis();
      }

      
      if (millis() - alarmRingingStartTime >= 600000UL) {
        alarmActive = false;
        currentState = SHOW_TIME;
        lcd.clear();
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

//funkce skibidi

void displayMainScreen() {
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);
  if (now.day() < 10) lcd.print('0'); lcd.print(now.day()); lcd.print('.');
  if (now.month() < 10) lcd.print('0'); lcd.print(now.month()); lcd.print(' ');
  
  if (now.hour() < 10) lcd.print('0'); lcd.print(now.hour()); lcd.print(':');
  if (now.minute() < 10) lcd.print('0'); lcd.print(now.minute()); lcd.print(':');
  if (now.second() < 10) lcd.print('0'); lcd.print(now.second());

  int todayIndex = (now.dayOfTheWeek() == 0) ? 6 : now.dayOfTheWeek() - 1;
  lcd.setCursor(0, 1);
  lcd.print("Budik: ");
  if (alarmDays[todayIndex]) {
    if (alarmHour < 10) lcd.print('0'); lcd.print(alarmHour); lcd.print(':');
    if (alarmMinute < 10) lcd.print('0'); lcd.print(alarmMinute);
  } else {
    lcd.print("--:--");
  }
}

void displayDaysMenu() {
  lcd.setCursor(0, 0); lcd.print("Dny buzeni:"); lcd.setCursor(0, 1);
  for (int i = 0; i < 7; i++) {
    if (alarmDays[i]) lcd.print(daysChar[i]); else lcd.print('_'); 
    lcd.print(' '); 
  }
  lcd.setCursor(cursorPosition * 2, 1); lcd.blink(); 
}

void displaySetAlarmTime(const char* text) {
  lcd.noBlink(); lcd.setCursor(0, 0); lcd.print(text); lcd.setCursor(6, 1);
  if (alarmHour < 10) lcd.print('0'); lcd.print(alarmHour); lcd.print(':');
  if (alarmMinute < 10) lcd.print('0'); lcd.print(alarmMinute);
  if (currentState == SET_ALARM_HOUR) lcd.setCursor(6, 1); else lcd.setCursor(9, 1);
  lcd.blink(); 
}

void displayConfigVal(const char* label, int val) {
  lcd.noBlink(); lcd.setCursor(0, 0); lcd.print(label); lcd.setCursor(0, 1);
  lcd.print(val); lcd.print("    "); 
}

void checkAlarm() {
  DateTime now = rtc.now();
  if (now.second() == 0) { 
    if (now.hour() == alarmHour && now.minute() == alarmMinute) {
      int todayIndex = (now.dayOfTheWeek() == 0) ? 6 : now.dayOfTheWeek() - 1; 
      if (alarmDays[todayIndex] == true) {
        currentState = ALARM_RINGING; 
        alarmRingingStartTime = millis(); 
      }
    }
  }
}
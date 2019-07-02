#include <TimerOne.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Button.h>        
#include <OneWire.h>                
#include <DallasTemperature.h>

//-----------------------------creacion de un objeto (myBtn) de la clase(Button) ---------------------------------------------------------------

#define PULLUP true        
#define INVERT true                                                              
#define DEBOUNCE_MS 20     
#define ACTIVATE_ALARM_BUTTON_PIN 2 // amarillo       
#define SETTING_ALARM_BUTTON_PIN 3 // verde
#define HOUR_BUTTON_PIN 4 // cafe 
#define MINUTES_BUTTON_PIN 5 // azul 
#define LCD_LED_PIN 7 // naranja 

Button activateAlarmBtn(ACTIVATE_ALARM_BUTTON_PIN, PULLUP, INVERT, DEBOUNCE_MS);    
Button setHourBtn(HOUR_BUTTON_PIN, PULLUP, INVERT, DEBOUNCE_MS);    
Button setMinutesBtn(MINUTES_BUTTON_PIN, PULLUP, INVERT, DEBOUNCE_MS);    
Button setAlarmBtn(SETTING_ALARM_BUTTON_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button LCDledBtn(LCD_LED_PIN, PULLUP, INVERT, DEBOUNCE_MS);

LiquidCrystal_I2C lcd(0x27,16,2);

// Sensor variables
#define TEMPERATURE_PIN 6
OneWire temperatureConnection(TEMPERATURE_PIN);
DallasTemperature temperatureSensor(&temperatureConnection);


// clock states 
enum clockStatus {displayTime, alarmInFlow, alarmListening, swapToSetting, settingAlarmStatus, configTime};
clockStatus currentStatus = displayTime;

// LCD led 
const int resetAlarmBtn = 4;
const int activateAlarm = 3;

bool LCDLedOn = false;

//variables para tiempo (hora) actual
bool Update=false;
unsigned char halfsecond = 0;
unsigned char second = 50;
unsigned char minute = 00;
unsigned char hour = 12;

//Strings para variables tiempo actual
String strHour;
String strMinute;
String strSecond;
String strDispTime;

//Variables para Alarma
unsigned char minuteAlarm = 1;
unsigned char hourAlarm = 12;
bool chagingAlarm = false;
bool alarmActivated = false;
bool alarmRining = false;
bool alarmTurnedOff = false;
bool settingAlarm = false;

//Strings para variables alarma
String strMinuteAlarm;
String strHourAlarm;
String strDispAlarm;
String wakeUpAlarmMessage = "Despierta! :D";

bool settingHours = false;
bool settingMinutes = false;

//Variables para Temperatura
unsigned char temp = 17;

//Strings para variables temperatura
String strDispTemp;

//variables para multitasking
unsigned long previousMillis_TaskClock=0;
unsigned long currentMillis;

//multitasking 
unsigned long previousMillis_CheckAlarm=0;
unsigned long previousMillis_BtnActiveAlarm=0;
unsigned long previousMillisCheckTemp=0;
unsigned long previousMillisCheckLCDled=0;

//leds 
const int alarmActivatedLED = 8;
const int alarmInFlowLED = 9;
const int buzzerPin = 10;

void setup() {
  lcd.init();
  lcd.backlight();
  LCDLedOn = true;
  //Leds
  pinMode(alarmActivatedLED, OUTPUT);
  pinMode(alarmInFlowLED, OUTPUT);

  // Change time 
  Timer1.initialize(500000);
  Timer1.attachInterrupt(TimingISR);

  //Setting button's interrupts
  attachInterrupt(digitalPinToInterrupt(ACTIVATE_ALARM_BUTTON_PIN), ChangeAlarmStatusBtn, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SETTING_ALARM_BUTTON_PIN), SettingAlarmBtn, CHANGE);

  // Setting temperature sensor 
  temperatureSensor.begin();
  Serial.begin(9600);
}

void loop() {
  currentMillis=millis();
  TaskClock();
  currentMillis=millis();
  ActivatorAlarmListener();
  SettingHourBtn();
  SettingMinutesBtn();
  currentMillis=millis();
  CheckTemperature();
  currentMillis=millis();
  LCDLedListener();
}


// Functions 
void TaskClock(){
  switch(currentStatus){
    case displayTime: 
      DisplayMainTime();
      break;  
    case alarmInFlow:
      WakeUpAlarm();
      break;  
    case alarmListening:
      TurnOffAlarmListener();
      break;
    case swapToSetting:
      WakeUpSettingAlarm();
      break;
    case settingAlarmStatus:
      AlarmDisplay();
      break;
  }
}

void DisplayMainTime() {
  TimeDisplay();        
  AlarmDisplay();  
  TemperatureDisplay();
  if(alarmActivated) CheckAlarmTiming();
}

void WakeUpSettingAlarm() {
  lcd.clear();
  lcd.print("Setting alarm");
  currentStatus = settingAlarmStatus;
}


void TimeDisplay(void)
{
    if (second<10) strSecond="0" + (String) second;
    else strSecond=(String) second;

    if (minute<10) strMinute="0" + (String) minute;
    else strMinute=(String) minute;

    if (hour<10) strHour=" " + (String) hour;
    else strHour=(String) hour;

    strDispTime="hora " + strHour + ":" + strMinute + ":" + strSecond;

    lcd.setCursor(0,0);
    lcd.print(strDispTime);
    
    Update = false;
}


void AlarmDisplay(void)
{
  if (minuteAlarm<10) strMinuteAlarm="0" + (String) minuteAlarm;
  else strMinuteAlarm=(String) minuteAlarm;

  if (hourAlarm<10) strHourAlarm=" " + (String) hourAlarm;
  else strHourAlarm=(String) hourAlarm;  
  
  strDispAlarm="A " + strHourAlarm + ":" + strMinuteAlarm;

  lcd.setCursor(0,1);
  lcd.print(strDispAlarm);
  
}


void TemperatureDisplay(void)
{
  strDispTemp=(String) temp;
  lcd.setCursor(11,1);
  lcd.print("T");
  lcd.setCursor(13,1);
  lcd.print(strDispTemp);
  lcd.setCursor(15,1);
  lcd.print("c");
  
  
}

void TimingISR()
{
  halfsecond ++;
  Update = true;
  if(halfsecond == 2){
    second ++;
    if(second == 60)
    {
      minute ++;
      if(minute == 60)
      {
        hour ++;
        if(hour == 24)hour = 0;
        minute = 0;
      }
      second = 0;
    }
    halfsecond = 0;
  }
  // if alarm was turned off, activate it in the next minute 
  if (minute >= minuteAlarm + 1 && hour == hourAlarm) alarmTurnedOff = false;
  Serial.println(alarmTurnedOff);
}

void CheckAlarmTiming() {
  if((minute == minuteAlarm && hour == hourAlarm) && !alarmRining && !alarmTurnedOff) {
      currentStatus = alarmInFlow;
  }
}


void TurnOffAlarmListener() {
  if (!alarmRining) {
    digitalWrite(alarmInFlowLED, alarmRining);
    lcd.clear();
    currentStatus = displayTime;
  }
}

void WakeUpAlarm() {  
  lcd.clear();
  lcd.print(wakeUpAlarmMessage);
  alarmRining = true;
  digitalWrite(alarmInFlowLED, alarmRining);
  currentStatus = alarmListening;
  
}
void SettingHourBtn() {
  setHourBtn.read();
  if(setHourBtn.wasPressed()) {
    if(currentStatus == settingAlarmStatus) {
      hourAlarm++;
      alarmTurnedOff = false;
      if (hourAlarm == 24) hourAlarm = 0;
    } else {
      hour++;
      if (hour == 24) hour = 0;
    }
  }
}

void SettingMinutesBtn() {
  setMinutesBtn.read();
  if(setMinutesBtn.wasPressed()) {
    if(currentStatus == settingAlarmStatus) {
      minuteAlarm++;
      alarmTurnedOff = false;
      if (minuteAlarm == 60) minuteAlarm = 0;
    } else {
      minute++;
      if (minute == 60) minute = 0;
    }
    Serial.println(minute);
  }
}

void SettingAlarmBtn() {
  setAlarmBtn.read();
  bool accessToSetAlarm = (currentStatus != alarmListening && currentStatus != alarmInFlow);
  if(setAlarmBtn.wasPressed() && accessToSetAlarm) {
    settingAlarm = !settingAlarm;
    if(settingAlarm) {
      currentStatus = swapToSetting;
    } else {
      currentStatus = displayTime;
    }
  }
};

void ChangeAlarmStatusBtn() {
  activateAlarmBtn.read();
  if (activateAlarmBtn.wasPressed()) {
    if (currentStatus == alarmListening) {
      alarmRining = false;
      alarmTurnedOff = true;
    } else {
      alarmActivated = !alarmActivated;
    }
  }
}

void ActivatorAlarmListener() {
  digitalWrite(alarmActivatedLED, alarmActivated);
}

void CheckTemperature() {
  if (currentMillis - previousMillisCheckTemp >= 100) {
    previousMillisCheckTemp = currentMillis;
    temperatureSensor.requestTemperatures();
    temp = temperatureSensor.getTempCByIndex(0);
  }
}

void LCDLedListener() {
  if (currentMillis - previousMillisCheckLCDled >= 100) {
    previousMillisCheckLCDled = currentMillis;
    LCDledBtn.read();
    
    if(LCDledBtn.wasPressed()) {
      LCDLedOn = !LCDLedOn;  
      if (LCDLedOn) {
        lcd.backlight();
      } else {
        lcd.noBacklight();
      }
    
    }
    
  }
}

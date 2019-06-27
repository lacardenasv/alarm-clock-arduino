#include <TimerOne.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Button.h>        

//-----------------------------creacion de un objeto (myBtn) de la clase(Button) ---------------------------------------------------------------
#define ACTIVATE_ALARM_BUTTON_PIN 2       //Connect a tactile button switch (or something similar)
#define PULLUP true        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
                           //switch is closed, this is negative logic, i.e. a high state
                           //means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.

Button activateAlarmBtn(ACTIVATE_ALARM_BUTTON_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the object myBtn usando el constructor: Button(pin, puEnable, invert, dbTime);
Button resetAlarm(ACTIVATE_ALARM_BUTTON_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the object myBtn usando el constructor: Button(pin, puEnable, invert, dbTime);
//------------------------------------------------------------------------------------------------------------------------------------------------
  

//Crear el objeto lcd  dirección  0x27 y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27,16,2);  //


// clock states 
enum clockStatus {displayTime, alarmInFlow, alarmListening, configAlarm, configTime};
clockStatus currentStatus = displayTime;

// control buttons 
const int resetAlarmPin = 4;
const int activateAlarm = 3;

//variables para tiempo (hora) actual
bool Update=false;
unsigned char halfsecond = 0;
unsigned char second = 55;
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


//Strings para variables alarma
String strMinuteAlarm;
String strHourAlarm;
String strDispAlarm;
String wakeUpAlarmMessage = "Los super amigos a despertar!";

//Variables para Temperatura
unsigned char temp = 17;

//Strings para variables temperatura
String strDispTemp;

//variables para multitasking
unsigned long previousMillis_TaskClock=0;
unsigned long currentMillis;

//alarm multitasking 
unsigned long previousMillis_CheckAlarm=0;
unsigned long previousMillis_BtnActiveAlarm=0;

//leds 
const int alarmActivatedLED = 8;
const int alarmInFlowLED = 9;

void setup()
{

  // Inicializar el LCD
  lcd.init();
  
  //Encender la luz de fondo.
  lcd.backlight();

  //Buttons
  pinMode(resetAlarmPin, INPUT_PULLUP);
  pinMode(activateAlarm, INPUT);

  //Leds
  pinMode(alarmActivatedLED, OUTPUT);
  
  //Inicializar y activar interrupcion por timer uno
  Timer1.initialize(500000);//timing for 500ms
  Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR
  
  Serial.begin(9600);
}
void loop()
{
  currentMillis=millis();
  TaskClock();
  currentMillis=millis();
  ActivatorAlarmListener();
  //ConfigAlarmListener();
  //ConfigTimeListener();
}

void ActivatorAlarmListener() {
  if (!alarmRining) activateAlarmBtn.read();
  if (activateAlarmBtn.wasPressed() && !alarmRining) {
    previousMillis_BtnActiveAlarm = currentMillis;
    alarmActivated = !alarmActivated;
    digitalWrite(alarmActivatedLED, alarmActivated);
  }
}

void DisplayMainTime() {
  TimeDisplay();        
  AlarmDisplay();  
  TemperatureDisplay();

}

void TaskClock(){
  switch(currentStatus){
    case displayTime: 
      DisplayMainTime();
      if (alarmActivated) {
        CheckAlarmTiming();
      }
      break;  
    case alarmInFlow: 
      WakeUpAlarm();
      break;  
    case alarmListening: 
      Serial.println("turnodd listener");
      TurnOffAlarmListener();
      break;  
  }
  
 /* 
  if(currentMillis-previousMillis_TaskClock>=100){
      previousMillis_TaskClock=previousMillis_CheckAlarm;
      if(Update == true && !alarmInFlow){
        DisplayMainTime();
      }
  }
  if(currentMillis-previousMillis_CheckAlarm>=100 && alarmActivated){
    previousMillis_CheckAlarm = currentMillis;
    CheckAlarmTiming();
  }
 */
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

  String alarmStatus = "off";
  if (alarmActivated) alarmStatus = "on";
  
  strDispAlarm="A " + strHourAlarm + ":" + strMinuteAlarm + alarmStatus;

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
}

void CheckAlarmTiming() {
  if((minute == minuteAlarm && hour == hourAlarm) && !alarmRining) {
      currentStatus = alarmInFlow;
  }
}

void TurnOffAlarmListener() {
  
  if(alarmRining) activateAlarmBtn.read();
  

  if (activateAlarmBtn.wasPressed() && alarmRining) {
    Serial.println("entro!!!");
    alarmRining = !alarmRining;
    digitalWrite(alarmInFlowLED, alarmRining);
    lcd.clear();
    currentStatus = displayTime;
  }
  delay(1000);
  //alarmInFlow = false;
  //lcd.clear();
  //DisplayMainTime();    
}

void WakeUpAlarm() {  
  lcd.clear();
  lcd.print(wakeUpAlarmMessage);
  alarmRining = true;
  digitalWrite(alarmInFlowLED, alarmRining);
  currentStatus = alarmListening;
  
}

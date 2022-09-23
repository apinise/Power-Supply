//Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_ADS1015.h>

//i2c Addresses
#define DISPLAYADDRESS 0x27;
Adafruit_ADS1115 ads(0x48);
LiquidCrystal_I2C lcd(0x27, 16, 2); //Address, columns, rows

//16 bit adc to voltage
const float multiplier = 0.0001875;

//PWM Fan Control (Trigger Temps)
#define TWENTYPERCENT 51 //20C
#define FOURTYPERCENT 102 //30C
#define SIXTYPERCENT 153 //45C
#define EIGHTYPERCENT 204 //55C
#define HUNDREDPERCENT 255 //65C

//Temp Calcs
#define SERIESRESISTOR 10000 //Resistor
#define BETA 3425 //Thermistor Beta
#define AMBIENT 25 //Temperature Reference Point
#define AMBIENTRESISTANCE 10000 //Thermistor Resistance
float resistance;
float temp;
float measured_temp;

//PINS
#define FANPIN 3 // PWM Data pin 3
#define THERMALPIN 4 //THERMAL Protection
#define THERMALLED 5 //THERMAL Led
#define VOLTAGEMODE 6 //CV Led
#define CURRENTMODE 7 //CC Led
#define SECONDARYTAP 8 //Transformer Taps

//Analog Inputs
#define THERMISTORPIN A0 //Thermistor Voltage From Heatsink
#define POWERSWITCH A1 //Load output status

void setup() {
  //Set pin types as outputs
  pinMode(FANPIN, OUTPUT); //Pin 3
  pinMode(THERMALPIN, OUTPUT); //Pin 4
  pinMode(THERMALLED, OUTPUT); //Pin 5
  pinMode(VOLTAGEMODE, OUTPUT); //Pin 6
  pinMode(CURRENTMODE, OUTPUT); //Pin 7
  pinMode(SECONDARYTAP, OUTPUT); //Pin 8
  
  Serial.begin(9600);
  setPwmFrequency(3, 1); //Increase Timer2 PWM Frequency

  //LCD initiation
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);

  //Set default digital pin status
  digitalWrite(THERMALPIN, HIGH);
  digitalWrite(THERMALLED, LOW);
  //digitalWrite(SECONDARYTAP, LOW);

  //Start ADC conversion
  ads.begin();
  delay(10);
}
void loop() {
  tempControl();
  adcDisplay();
  delay(100);
}

void adcDisplay() {

  int outputStatus = 0;
  int outputADC = 0;
  float voltage_set, current_set, voltage_act, current_act;
  voltage_set = ads.readADC_SingleEnded(0); //Read voltages for current and voltage
  voltage_act = ads.readADC_SingleEnded(1);
  current_set = ads.readADC_SingleEnded(2);
  current_act = ads.readADC_SingleEnded(3);

  voltage_set = ((voltage_set * multiplier) * 4); //PSU has feedback gain of 4
  voltage_act = ((voltage_act * multiplier) * 4);
  current_set = (current_set * multiplier); //MAX4376 has gain of 20 with 0.1R
  current_act = (current_act * multiplier);

  if(voltage_set < 0){ 
    voltage_set = 0;
  }
  if(voltage_act < 0){ 
    voltage_act = 0;   
  }
  if(current_set < 0){ 
    current_set = 0;   
  }
  if(current_act < 0){
    current_act = 0;
  }
  
  outputADC = analogRead(POWERSWITCH);
  if (outputADC >= 512){
    outputStatus = 1;
  }
  else {
    outputStatus = 0;
  }

  lcd.setCursor(0,0);
  lcd.print("Voltage: ");  
  lcd.print(voltage_act, 2);
  lcd.print(" V"); 
  lcd.setCursor(0,1);

  if (outputStatus == 1){
    lcd.print("Current: "); 
    lcd.print(current_act, 2);
    lcd.print(" A");
  }
  else {
    lcd.print("Current: "); 
    lcd.print(current_set, 2);
    lcd.print(" A");     
  }

  if (voltage_act <= (voltage_set - (voltage_set * 0.05))){
    digitalWrite(VOLTAGEMODE, LOW);
    digitalWrite(CURRENTMODE, HIGH);
  }
  else {
    digitalWrite(VOLTAGEMODE, HIGH);
    digitalWrite(CURRENTMODE, LOW);
  }
  /*
  if (voltage_set <= (5/2)){
    digitalWrite(SECONDARYTAP, HIGH);
  }
  else {
    digitalWrite(SECONDARYTAP, LOW);
  }
  */
}

void tempControl(){
  float thresh = 72.5;
  float margin = 5.0;
  measured_temp = thermistorRead();
  Serial.println(measured_temp);
  //fanSpeed(measured_temp);
  if (measured_temp >= (thresh + margin)){
    digitalWrite(THERMALPIN, LOW);
    digitalWrite(THERMALLED, HIGH);
  }
  if (measured_temp <= (thresh - margin)){
    digitalWrite(THERMALPIN, HIGH);
    digitalWrite(THERMALLED, LOW);
  }
}

float thermistorRead(){
  temp = 0;
  
  //Thermistor resistance calculation
  resistance = analogRead(THERMISTORPIN);
  resistance = 1023 / resistance - 1;
  resistance = SERIESRESISTOR / resistance;
  
  //Temperature approximation calculation
  temp = resistance / AMBIENTRESISTANCE; // (R/Ro)
  temp = log(temp); // ln(R/Ro)
  temp /= BETA; // 1/B * ln(R/Ro)
  temp += 1.0 / (AMBIENT + 273.15); // + (1/To)
  temp = 1.0 / temp; // Invert
  temp -= 273.15; // Convert K -> C
  
  return temp;
}

void fanSpeed(float temperature){
  if (temperature >= 30 && temperature < 45){
    analogWrite(FANPIN, FOURTYPERCENT);
  }
  else if (temperature >= 45 && temperature < 55){
    analogWrite(FANPIN, SIXTYPERCENT);
  }
  else if (temperature >= 55 && temperature < 65){
    analogWrite(FANPIN, EIGHTYPERCENT);
  }
  else if (temperature >= 65){
    analogWrite(FANPIN, HUNDREDPERCENT);
  }
  else {
    analogWrite(FANPIN, TWENTYPERCENT);
  }
}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

/*
ISR(TIMER1_COMPA_vect){
  float thresh = 38.0;
  float margin = 2.0;
  measured_temp = thermistorRead();
  Serial.println(measured_temp);
  fanSpeed(measured_temp);
  if (measured_temp >= (thresh + margin)){
    digitalWrite(THERMALPIN, LOW);
    digitalWrite(THERMALLED, HIGH);
  }
  if (measured_temp <= (thresh - margin)){
    digitalWrite(THERMALPIN, HIGH);
    digitalWrite(THERMALLED, LOW);
  }
}

void Timer1_Setup(void){
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 15624; //15624
  TCCR1B |= (1<<WGM12);
  TCCR1B |= (1<<CS12) | (1<<CS10); //Prescalar = 1024
  TIMSK1 |= (1<<OCIE1A);
}
*/

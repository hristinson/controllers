#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <math.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int analogPin = A1;
const int controlPin = 2; // цифровий пін для керування
const float R_BALANCE = 10000.0;  // опір резистора балансу (10 кОм)
const float B_COEFFICIENT = 3950; // коефіцієнт B термістора (зазвичай 3950)
const float T0 = 298.15;          // температура 25°C у Кельвінах
const float R0 = 10000.0;         // опір термістора при 25°C
const char VERSION[8] = "2.0.1"; 

byte checkmark[8] = {
  B00000,
  B00001,
  B00010,
  B10100,
  B01000,
  B00000,
  B00000,
  B00000
};

unsigned long previousMillis = 0;
const long interval = 1000;
float settedTemp;
int eeAddress = 0;
int status = 0;
float tempC = 0;
float sum = 0;
float globalsum = 0;

const int maxSamples = 100;
float tempArray[maxSamples];
int tempIndex = 0;

const int maxGlobalSamples = 250;
float tempGlobalArray[maxGlobalSamples];
int tempGlobalIndex = 0;

void readButton() {
  int adcKey = analogRead(A0);

  if (adcKey < 60)   return "RIGHT";
  if (adcKey < 200)  {
   settedTemp = settedTemp + 0.1;
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("select Temp (C):");
    lcd.setCursor(0, 1); 
    lcd.print(settedTemp, 1);
    delay(1000);  
    return;
    }
  
  
  if (adcKey < 400)  { 
    settedTemp = settedTemp - 0.1;
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("select Temp (C):");
    lcd.setCursor(0, 1); 
     lcd.print(settedTemp, 1);
     delay(1000); 
     return;
      }

    if (adcKey < 800)  {
    lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("saved Temp (C):");
    lcd.setCursor(0, 1); 
    lcd.print(settedTemp, 1);
    EEPROM.put(eeAddress, settedTemp);
    delay(5000);  
    return;
    }
}

void recordTemperature() {
   if (tempIndex < maxSamples) {
    tempArray[tempIndex] = getTemp();
    tempIndex++;
  } else {
    for (int i = 0; i < maxSamples; i++) {
       sum += tempArray[i];
    }
    float average = sum / maxSamples;
    tempIndex = 0;
    sum = 0;
    tempC = average;
  }
}

float getTemp(){
   int adcValue = analogRead(analogPin);
   // Обчислюємо опір термістора (R_thermistor)
    float voltage = adcValue * (5.0 / 1023.0);
    float R_thermistor = R_BALANCE * ((5.0 / voltage) - 1.0);
    // Обчислюємо температуру у Кельвінах за формулою Стейнхарта-Харта (спрощена)
    float tempK = 1.0 / ( (1.0 / T0) + (1.0 / B_COEFFICIENT) * log(R_thermistor / R0) );
    tempC = tempK - 273.15;
    float tempC_rounded = (int)(tempC * 10.0 + 0.5) / 10.0;

    return tempC_rounded;
}

void setStatus () {
    if(tempGlobalIndex < maxGlobalSamples) 
    {
    tempGlobalArray[tempGlobalIndex] = tempC;
    tempGlobalIndex++;
  }
    else
    {
      for (int i = 0; i < maxGlobalSamples; i++) {
       globalsum += (int)(tempGlobalArray[i] * 10.0 + 0.5) / 10.0;
    }

    float globalAverageTemp = globalsum / maxGlobalSamples;
    float globalAverage = (int)(globalAverageTemp * 10.0 + 0.5) / 10.0;
    if (status == 0 && globalAverage < settedTemp - 0.5) {
      status = 1;  // активовано
    } else if (status == 1 && globalAverage > settedTemp + 0.5) {
      status = 0;  // деактивовано
    }

    Serial.println("---------------------------");
    Serial.print("globalAverage temp - ");
    Serial.println(globalAverage);
    Serial.println("---------------------------");

    tempGlobalIndex = 0;
    globalsum = 0;
    }
}

void setup() {
  lcd.begin(16, 2);   
  EEPROM.get(eeAddress, settedTemp);
  Serial.begin(19200);
  lcd.setCursor(0, 0); 
  lcd.print("SmartHouse termo");  
  lcd.setCursor(0, 1); 
  lcd.print("version: ");
  lcd.print(VERSION);
  delay(3000);
  lcd.createChar(0, checkmark); 
  pinMode(controlPin, OUTPUT);  // налаштовуємо пін 3 як вихід
  digitalWrite(controlPin, LOW); // спочатку вимикаємо
}

void loop() {

  if(getTemp() < -128) {
    lcd.clear();
    lcd.print("fault sensor!");
    digitalWrite(controlPin, LOW);
    status = 0;
    delay(5000);
  } else {

  unsigned long currentMillis = millis();
  recordTemperature();
  readButton();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    setStatus();

    if (status == 1) {
      digitalWrite(controlPin, HIGH);
    } else {
      digitalWrite(controlPin, LOW);
    }

    lcd.setCursor(0, 0); 
    lcd.print("Temp (set) ");
    lcd.print(settedTemp, 1); 
    lcd.print("    "); 
    lcd.setCursor(0, 1); 
    lcd.print("Actual ");
    lcd.print(tempC, 1);
    lcd.print((char)223);
    lcd.print("C ");
    if(status == 1) {lcd.write(byte(0));} else {lcd.setCursor(14, 1); lcd.print(" "); };

   } }
}
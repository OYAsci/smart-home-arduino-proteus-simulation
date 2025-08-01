#include <Keypad.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <DHT.h>

// Pin Definitions
#define DHTPIN A1
#define DHTTYPE DHT11
#define PIR_PIN A0
#define GAS_PIN A2
#define BUZZER A3
#define LED_PIN 13

// Hardware Instances
LiquidCrystal lcd(10, 6, 5, 4, 3, 2);
DHT dht(DHTPIN, DHTTYPE);
Servo servoDoor, servoAlarm;

// Keypad Setup
const byte ROWS = 1, COLS = 2;
char keys[ROWS][COLS] = {{'1', '3'}};
byte rowPins[ROWS] = {9}, colPins[COLS] = {8, 7};
Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// System Variables
const String unlockCode = "13";
String inputCode = "";
float temperature, humidity;
int gasLevel;
bool motionDetected = false, buzzerOn = false, doorOpen = false;
unsigned long systemStart, lastBuzz, lastLCD, motionTime, doorOpenTime;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT); pinMode(BUZZER, OUTPUT);
  pinMode(PIR_PIN, INPUT); pinMode(GAS_PIN, INPUT);
  servoDoor.attach(11); servoAlarm.attach(12);
  servoDoor.write(180); servoAlarm.write(0);
  dht.begin(); lcd.begin(16, 2);

  lcd.print("System Starting"); delay(3000);
  lcd.clear(); lcd.print("System Ready"); delay(1000); lcd.clear();
  systemStart = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastLCD >= 1000) {
    readSensors(); showStatus(); lastLCD = now;
  }
  readKeypad(); checkUnlock(now); detectMotion(now); handleAlarm(now);
}

void readSensors() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;
  gasLevel = analogRead(GAS_PIN);
}

void showStatus() {
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temperature, 1); lcd.print((char)223); lcd.print("C ");
  lcd.print("H:"); lcd.print(humidity, 0); lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("G:"); lcd.print(gasLevel); lcd.print(" C:"); lcd.print(inputCode); lcd.print("   ");
}

void readKeypad() {
  char key = keypad.getKey();
  if (key) {
    inputCode += key;
    if (inputCode.length() > unlockCode.length()) inputCode.remove(0, 1);
  }
}

void checkUnlock(unsigned long now) {
  if (inputCode == unlockCode) {
    servoDoor.write(130); doorOpenTime = now;
    doorOpen = true; inputCode = "";
  } else if (inputCode.length() == unlockCode.length()) inputCode = "";

  if (doorOpen && now - doorOpenTime >= 6000) {
    servoDoor.write(180); doorOpen = false;
  }
}

void detectMotion(unsigned long now) {
  if (digitalRead(PIR_PIN) && !motionDetected) {
    motionDetected = true; motionTime = now;
    digitalWrite(LED_PIN, HIGH);
  } else if (motionDetected && now - motionTime >= 30000) {
    motionDetected = false; digitalWrite(LED_PIN, LOW);
  }
}

void handleAlarm(unsigned long now) {
  if (now - systemStart < 10000) {
    digitalWrite(BUZZER, LOW); servoAlarm.write(0); return;
  }

  bool danger = (temperature > 50 || gasLevel > 200);

  if (danger) {
    if (now - lastBuzz >= 300) {
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER, buzzerOn);
      lastBuzz = now;
    }
    servoAlarm.write(50); servoDoor.write(130);
  } else {
    digitalWrite(BUZZER, LOW); servoAlarm.write(0);
    if (!doorOpen) servoDoor.write(180);
  }
}

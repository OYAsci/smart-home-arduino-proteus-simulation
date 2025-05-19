#include <Keypad.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <DHT.h>

// Pin setup
#define DHTPIN A1
#define DHTTYPE DHT11
#define PIR_PIN A0
#define GAS_PIN A2
#define BUZZER A3
#define LED_PIN 13

LiquidCrystal lcd(10, 6, 5, 4, 3, 2);
DHT dht(DHTPIN, DHTTYPE);
Servo servo2, servo1;

// Keypad setup
const byte ROWS = 1, COLS = 2;
char keys[ROWS][COLS] = {{'1', '3'}};
byte rowPins[ROWS] = {9};
byte colPins[COLS] = {8, 7};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// State variables
String code = "13", input = "";
float temp = 0.0, hum = 0.0;
int gas = 0;
bool motion = false, buzzOn = false;
unsigned long tStart, lastBuzz = 0, lastLCD = 0, motionStart = 0;
unsigned long alarmTriggeredTime = 0;
bool alarmActive = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);

  servo2.attach(11); servo2.write(180);
  servo1.attach(12); servo1.write(0);

  dht.begin(); lcd.begin(16, 2);
  lcd.print("System Starting"); delay(3000);
  lcd.clear(); lcd.print("System Ready"); delay(1000); lcd.clear();

  tStart = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastLCD >= 1000) { updateSensors(); lastLCD = now; }

  handleKeypad();
  handleSecurity();
  handleMotion(now);
  handleAlarm(now);
}

void updateSensors() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t)) temp = t;
  if (!isnan(h)) hum = h;
  gas = analogRead(GAS_PIN);

  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temp, 1); lcd.print((char)223); lcd.print("C ");
  lcd.print("H:"); lcd.print(hum, 0); lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("G:"); lcd.print(gas);
  lcd.print(" C:"); lcd.print(input); lcd.print("   ");
}

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    input += key;
    if (input.length() > code.length()) input = input.substring(1);
  }
}

void handleSecurity() {
  if (input == code) {
    servo2.write(120); delay(6000); servo2.write(180);
    input = "";
  } else if (input.length() == code.length()) input = "";
}

void handleMotion(unsigned long now) {
  if (digitalRead(PIR_PIN) == HIGH && !motion) {
    motion = true; motionStart = now;
    digitalWrite(LED_PIN, HIGH);
  }
  if (motion && now - motionStart >= 30000) {
    motion = false; digitalWrite(LED_PIN, LOW);
  }
}

void handleAlarm(unsigned long now) {
  if (now - tStart < 10000) {
    digitalWrite(BUZZER, LOW);
    servo1.write(0);
    return;
  }

  // Trigger condition
  if (temp > 50 || gas > 200) {
    alarmTriggeredTime = now;
    alarmActive = true;
  }

  // Stay active for 5 seconds after trigger
  if (alarmActive && now - alarmTriggeredTime <= 5000) {
    if (now - lastBuzz >= 300) {
      buzzOn = !buzzOn;
      digitalWrite(BUZZER, buzzOn);
      servo1.write(60);
      servo2.write(120);
      lastBuzz = now;
    }

    // Show alert message
    lcd.setCursor(0, 0);
    lcd.print("!! ALERT ACTIVE !!");

  } else {
    alarmActive = false;
    digitalWrite(BUZZER, LOW);
    servo1.write(0);
    servo2.write(180);
  }
}

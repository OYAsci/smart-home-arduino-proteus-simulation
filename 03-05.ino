#include <Keypad.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <DHT.h>

// LCD setup
LiquidCrystal lcd(10, 6, 5, 4, 3, 2);

// DHT setup
#define DHTTYPE DHT11
DHT dht(A1, DHTTYPE);

// Servo setup
Servo myServo;
Servo othmane;

// Keypad setup
const byte ROWS = 1;
const byte COLS = 2;
char keys[ROWS][COLS] = {
  {'1', '3'}
};
byte rowPins[ROWS] = {9};
byte colPins[COLS] = {8, 7};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables
String correctCode = "13";
String enteredCode = "";

float temp = 0.0;
float hum = 0.0;
int gas = 0;

unsigned long startTime;
unsigned long lastBuzzTime = 0;
bool buzzState = false;

unsigned long lastLCDUpdate = 0;
const unsigned long lcdUpdateInterval = 1000;

// PIR timing
bool motionActive = false;
unsigned long motionStart = 0;
const unsigned long motionDuration = 30000;

void setup() {
  Serial.begin(9600);

  myServo.attach(12);
  myServo.write(0);
  othmane.attach(11);
  othmane.write(0);
  dht.begin();

  pinMode(13, OUTPUT); // Motion LED
  pinMode(A3, OUTPUT); // Buzzer
  pinMode(A0, INPUT);  // PIR Sensor
  pinMode(A2, INPUT);  // Gas Sensor

  lcd.begin(16, 2);
  lcd.print("System Starting");
  delay(3000);

  lcd.clear();
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();

  startTime = millis();
}

void loop() {
  unsigned long now = millis();

  if (now - lastLCDUpdate >= lcdUpdateInterval) {
    readSensors();
    lastLCDUpdate = now;
  }

  handleKeypad();
  handleSecurity();
  handlePIR(now);
  handleBuzzer(now);
}

void readSensors() {
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();

  if (!isnan(newTemp)) temp = newTemp;
  if (!isnan(newHum)) hum = newHum;

  gas = analogRead(A2);

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print((char)223);
  lcd.print("C ");

  lcd.print("H:");
  lcd.print(hum, 0);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("G:");
  lcd.print(gas);
  lcd.print(" C:");
  lcd.print(enteredCode);
  lcd.print("   ");
}

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    enteredCode += key;
    if (enteredCode.length() > correctCode.length()) {
      enteredCode = enteredCode.substring(1);
    }
  }
}

void handleSecurity() {
  if (enteredCode == correctCode) {
    myServo.write(90);
    delay(6000);
    myServo.write(0);
    enteredCode = "";
  } else if (enteredCode.length() == correctCode.length() && enteredCode != correctCode) {
    enteredCode = "";
  }
}

void handlePIR(unsigned long now) {
  int pirState = digitalRead(A0);

  if (pirState == HIGH && !motionActive) {
    motionActive = true;
    motionStart = now;
    digitalWrite(13, HIGH);
  }

  if (motionActive && now - motionStart >= motionDuration) {
    motionActive = false;
    digitalWrite(13, LOW);
  }
}

void handleBuzzer(unsigned long now) {
  if (now - startTime < 10000) {
    digitalWrite(A3, LOW);
    buzzState = false;
    othmane.write(0);
    return;
  }

  if (temp > 50 || gas > 200) {
    if (now - lastBuzzTime >= 300) {
      buzzState = !buzzState;
      digitalWrite(A3, buzzState ? HIGH : LOW);
      othmane.write(90);
      lastBuzzTime = now;
    }
  } else {
    digitalWrite(A3, LOW);
    buzzState = false;
    delay(5000);othmane.write(0);
  }
}

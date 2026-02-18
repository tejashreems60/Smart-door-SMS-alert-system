#include <SoftwareSerial.h>

// SIM900A Serial Pins
SoftwareSerial sim(10, 11);  // 10 = RX, 11 = TX

// Phone Number
const char OWNER_NUMBER[] = "+919380758395";

// Pin Definitions
const int PIN_REED     = 2;
const int PIN_MQ2      = A0;
const int PIN_RELAY_NC = 4;
const int PIN_BUZZER   = 5;
const int PIN_LED      = 6;

// Threshold
int MQ2_THRESHOLD = 400;
int MQ2_EMERGENCY = 70;

// Timers
unsigned long lastDoorSMS  = 0;
unsigned long lastSmokeSMS = 0;
unsigned long lastPowerSMS = 0;
unsigned long SMS_GAP      = 60000;

bool gsmConnected = false;

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  sim.begin(9600);

  pinMode(PIN_REED, INPUT_PULLUP);
  pinMode(PIN_RELAY_NC, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  Serial.println("Booting System...");
  delay(1000);

  gsmConnected = initializeSIM();

  if (!gsmConnected)
    Serial.println("GSM NOT REACHABLE — Normal mode");
  else
    Serial.println("GSM CONNECTED — SMS enabled");

  Serial.println("System Ready!");
}

// ---------------------------------------------------------------------
void loop() {

  unsigned long now = millis();
  int mq = analogRead(PIN_MQ2);
  // ----------- GAS SENSOR BUZZER ALERT (mq > 70) --------------
if (mq > 70) {
  digitalWrite(PIN_BUZZER, HIGH);   // Buzzer ON
} else {
  digitalWrite(PIN_BUZZER, LOW);    // Buzzer OFF
}

  bool doorOpen = digitalRead(PIN_REED);
  bool powerPresent = (digitalRead(PIN_RELAY_NC) == LOW);

  Serial.print("Gas: ");
  Serial.print(mq);
  Serial.print(" | Door: ");
  Serial.print(doorOpen ? "OPEN" : "CLOSED");
  Serial.print(" | Power: ");
  Serial.println(powerPresent ? "ON" : "OFF");

  // Smoke
  // ---------------- GAS / SMOKE ALERTS ----------------
if (mq >= MQ2_THRESHOLD && mq < MQ2_EMERGENCY) {
    // Normal gas detection
    Serial.println("⚠ GAS DETECTED!");

    if (now - lastSmokeSMS > SMS_GAP) {
      lastSmokeSMS = now;
      if (gsmConnected) sendSMS("Warning: Gas detected in the house!");
    }
}

// EMERGENCY LEVEL
if (mq >= MQ2_EMERGENCY) {
    Serial.println("🚨 EMERGENCY GAS LEVEL!");

    // Fast buzzer warning
    for (int i = 0; i < 10; i++) {
        digitalWrite(PIN_BUZZER, HIGH);
        delay(100);
        digitalWrite(PIN_BUZZER, LOW);
        delay(100);
    }

    if (now - lastSmokeSMS > SMS_GAP) {
      lastSmokeSMS = now;
      if (gsmConnected) 
         sendSMS("🔥 EMERGENCY! Gas level above 70! Immediate attention needed!");
    }
}

  // Door
  static bool lastDoorState = false;
  if (doorOpen != lastDoorState) {
    lastDoorState = doorOpen;

    if (doorOpen == HIGH && now - lastDoorSMS > SMS_GAP) {
      lastDoorSMS = now;
      triggerAlarm(4);

      if (gsmConnected)
        sendSMS("Door opened!");
    }
  }

  // Power
// ---------------- POWER ALERT + LED INDICATION ----------------
static bool lastPowerState = true;

// LED always follows power state
if (powerPresent) {
  digitalWrite(PIN_LED, HIGH);   // power ON → LED ON
} else {
  digitalWrite(PIN_LED, LOW);    // power OFF → LED OFF
}

if (powerPresent != lastPowerState) {
  lastPowerState = powerPresent;

  if (!powerPresent) {  // power OFF
    Serial.println("⚠ POWER FAILURE!");
    if (now - lastPowerSMS > SMS_GAP) {
      lastPowerSMS = now;
      triggerAlarm(4);
      if (gsmConnected) sendSMS("POWER FAILURE: Running on battery.");
    }
  } 
  else {  // power ON
    Serial.println("⚡ POWER RESTORED");
    if (now - lastPowerSMS > SMS_GAP) {
      lastPowerSMS = now;
      if (gsmConnected) sendSMS("Power restored at home.");
    }
  }
}


  delay(250);
}

// ---------------------------------------------------------------------
bool initializeSIM() {
  unsigned long start = millis();
  Serial.println("Connecting to GSM...");

  while (millis() - start < 15000) {
    sim.println("AT");
    delay(500);

    if (sim.find("OK")) {
      sim.println("AT+CMGF=1");
      delay(500);
      return true;
    }
  }

  return false;
}

// ---------------------------------------------------------------------
void sendSMS(String msg) {
  sim.println("AT+CMGF=1");
  delay(300);
  sim.print("AT+CMGS=\"");
  sim.print(OWNER_NUMBER);
  sim.println("\"");
  delay(300);
  sim.println(msg);
  delay(300);
  sim.write(26);
  delay(2000);
}

// ---------------------------------------------------------------------
void triggerAlarm(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED, LOW);
    delay(150);
  }
}

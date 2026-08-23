#include <LiquidCrystal.h>

#define BUTTON 2
#define LED 3
#define BUZZER 4

#define LCD_RS 7
#define LCD_E 8
#define LCD_D4 5
#define LCD_D5 6
#define LCD_D6 11
#define LCD_D7 12

#define TRIG 9
#define ECHO 10

#define LDR A0

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

enum State {
  OPEN_SEA,
  ANCHOR_DROPPED,
  STORM,
  CHARYBDIS,
  WRECKED
};

State state = OPEN_SEA;

bool lastButton = HIGH;
bool ledState = LOW;

unsigned long dangerStart = 0;
unsigned long lastBlink = 0;

void displayState(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);

  if (duration == 0)
    return 999;

  return duration * 0.034 / 2;
}

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  lcd.begin(16, 2);

  displayState("ODYSSEUS", "OPEN SEA");

  digitalWrite(LED, LOW);
  digitalWrite(BUZZER, LOW);
}

void loop() {

  bool button = digitalRead(BUTTON);

  if (lastButton == HIGH && button == LOW) {

    // Anchor can be dropped from OPEN_SEA, STORM, or CHARYBDIS,
    // so the player can always escape a wreck before the 5s timer expires.
    if (state == OPEN_SEA || state == STORM || state == CHARYBDIS) {
      state = ANCHOR_DROPPED;

      dangerStart = 0;

      digitalWrite(LED, LOW);
      digitalWrite(BUZZER, LOW);

      displayState("ANCHOR", "DROPPED");
    }

    else if (state == ANCHOR_DROPPED) {
      state = OPEN_SEA;

      dangerStart = 0;

      digitalWrite(LED, LOW);
      digitalWrite(BUZZER, LOW);

      displayState("ODYSSEUS", "OPEN SEA");
    }

    delay(200);
  }

  lastButton = button;

  if (state == WRECKED) {
    digitalWrite(LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    return;
  }

  if (state == ANCHOR_DROPPED) {
    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);
    return;
  }

  int lightValue = analogRead(LDR);
  long distance = getDistance();

  bool stormDetected = lightValue < 512;
  bool charybdisDetected = distance < 100;

  if (state == OPEN_SEA) {

    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);

    if (stormDetected) {

      state = STORM;
      dangerStart = millis();

      displayState("STORM", "DANGER!");
    }

    else if (charybdisDetected) {

      state = CHARYBDIS;
      dangerStart = millis();

      displayState("CHARYBDIS", "DANGER!");
    }

    else {
      displayState("ODYSSEUS", "OPEN SEA");
    }
  }

  else if (state == STORM) {

    digitalWrite(BUZZER, LOW);

    if (!stormDetected) {

      state = OPEN_SEA;
      dangerStart = 0;

      digitalWrite(LED, LOW);

      displayState("ODYSSEUS", "OPEN SEA");
    }

    else {

      if (millis() - lastBlink >= 300) {
        lastBlink = millis();

        ledState = !ledState;
        digitalWrite(LED, ledState);
      }

      if (millis() - dangerStart >= 5000) {

        state = WRECKED;

        digitalWrite(LED, HIGH);
        digitalWrite(BUZZER, HIGH);

        displayState("WRECKED", "STORM");
      }
    }
  }

  else if (state == CHARYBDIS) {

    digitalWrite(LED, LOW);

    if (!charybdisDetected) {

      state = OPEN_SEA;
      dangerStart = 0;

      digitalWrite(BUZZER, LOW);

      displayState("ODYSSEUS", "OPEN SEA");
    }

    else {

      digitalWrite(BUZZER, HIGH);

      if (millis() - dangerStart >= 5000) {

        state = WRECKED;

        digitalWrite(LED, HIGH);
        digitalWrite(BUZZER, HIGH);

        displayState("WRECKED", "CHARYBDIS");
      }
    }
  }

  delay(50);
}

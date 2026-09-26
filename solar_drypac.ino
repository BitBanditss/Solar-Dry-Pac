#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// ── PIN DEFINITIONS ──────────────────────
#define DHT_PIN      2    // DHT22 data pin
#define DHT_TYPE     DHT22
#define RELAY_PIN    7    // Relay controls heating coil
#define GREEN_LED    8    // Drying complete
#define RED_LED      9    // Drying in progress
#define BUZZER_PIN   10   // Alert sound

// ── TEMPERATURE SETTINGS ─────────────────
#define TEMP_LOW     38.0  // Turn coil ON below this
#define TEMP_HIGH    44.0  // Turn coil OFF above this

// ── HUMIDITY THRESHOLD ───────────────────
// Drying is complete when humidity drops below this
#define HUMIDITY_DONE  35.0

// ── MINIMUM DRYING TIME ──────────────────
// System waits at least this long before checking humidity
// 4 hours = 240 minutes = 14400000 milliseconds
#define MIN_DRYING_MINUTES  240

// ── OBJECTS ──────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Note: If LCD does not work try address 0x3F

// ── VARIABLES ────────────────────────────
unsigned long startTime    = 0;
bool timerStarted          = false;
bool dryingComplete        = false;

// ─────────────────────────────────────────
void setup() {
  dht.begin();

  lcd.begin();
  lcd.backlight();

  pinMode(RELAY_PIN,  OUTPUT);
  pinMode(GREEN_LED,  OUTPUT);
  pinMode(RED_LED,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Start with coil OFF
  digitalWrite(RELAY_PIN,  LOW);
  digitalWrite(RED_LED,    LOW);
  digitalWrite(GREEN_LED,  LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Welcome message
  lcd.setCursor(0, 0);
  lcd.print(" SOLAR DRY-PAC ");
  lcd.setCursor(0, 1);
  lcd.print("  Smart Dryer  ");
  delay(3000);
  lcd.clear();

  // Ready message
  lcd.setCursor(0, 0);
  lcd.print("  Loading...   ");
  lcd.setCursor(0, 1);
  lcd.print(" Please Wait   ");
  delay(2000);
  lcd.clear();
}

// ─────────────────────────────────────────
void loop() {

  // ── READ SENSOR ────────────────────────
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();

  // Check if sensor is working
  if (isnan(temp) || isnan(humi)) {
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!   ");
    lcd.setCursor(0, 1);
    lcd.print("Check DHT22     ");
    delay(2000);
    return;
  }

  // ── TEMPERATURE CONTROL ────────────────
  if (!dryingComplete) {
    if (temp < TEMP_LOW) {
      digitalWrite(RELAY_PIN, HIGH); // Turn coil ON
      digitalWrite(RED_LED,   HIGH); // Red LED ON
    }
    if (temp > TEMP_HIGH) {
      digitalWrite(RELAY_PIN, LOW);  // Turn coil OFF
    }
  }

  // ── START TIMER when temp is in range ──
  if (temp >= TEMP_LOW && temp <= TEMP_HIGH
      && !timerStarted && !dryingComplete) {
    startTime    = millis();
    timerStarted = true;
  }

  // Reset timer if temp drops too low
  if (temp < TEMP_LOW - 2 && timerStarted
      && !dryingComplete) {
    timerStarted = false;
  }

  // ── CALCULATE TIME ELAPSED ─────────────
  int minutesElapsed = 0;
  int minutesLeft    = MIN_DRYING_MINUTES;

  if (timerStarted && !dryingComplete) {
    minutesElapsed = (millis() - startTime) / 60000UL;
    minutesLeft    = MIN_DRYING_MINUTES - minutesElapsed;
    if (minutesLeft < 0) minutesLeft = 0;
  }

  // ── CHECK IF DRYING IS DONE ────────────
  // Two conditions must both be true:
  // 1. Minimum drying time has passed
  // 2. Humidity inside chamber is below 35%
  if (timerStarted && !dryingComplete) {
    unsigned long elapsed = millis() - startTime;
    bool timeReached = elapsed >=
        (unsigned long)MIN_DRYING_MINUTES * 60000UL;
    bool humidityLow = humi <= HUMIDITY_DONE;

    if (timeReached && humidityLow) {
      dryingComplete = true;
    }
  }

  // ── UPDATE LCD DISPLAY ─────────────────
  if (!dryingComplete) {

    // Line 1: Temperature
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temp, 1); // 1 decimal place
    lcd.print("C  ");

    // Line 2: Humidity or countdown
    if (!timerStarted) {
      // Heating up — show humidity
      lcd.setCursor(0, 1);
      lcd.print("Humi:");
      lcd.print(humi, 0);
      lcd.print("%  Heating");
    } else if (minutesLeft > 0) {
      // Timer running — show time left
      lcd.setCursor(0, 1);
      lcd.print("Left:");
      lcd.print(minutesLeft);
      lcd.print("min H:");
      lcd.print(humi, 0);
      lcd.print("%  ");
    } else {
      // Time done — waiting for humidity to drop
      lcd.setCursor(0, 1);
      lcd.print("Humi:");
      lcd.print(humi, 0);
      lcd.print("% Wait...");
    }

  } else {

    // ── DRYING COMPLETE ──────────────────
    digitalWrite(RELAY_PIN, LOW);   // Coil OFF
    digitalWrite(RED_LED,   LOW);   // Red LED OFF
    digitalWrite(GREEN_LED, HIGH);  // Green LED ON

    // Buzzer beeps 3 times
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);
      delay(500);
    }

    // Show done message on LCD
    lcd.setCursor(0, 0);
    lcd.print(" DRYING DONE!  ");
    lcd.setCursor(0, 1);
    lcd.print(" Open Box Now  ");

    // Stay on done screen
    while (true) {
      // Green LED stays ON
      // Artisan switches OFF manually
      delay(1000);
    }
  }

  delay(2000); // Read sensor every 2 seconds
}

#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Pin Configurations
#define pHSensorPin A0
#define TDSSensorPin A1
#define DHTPin 2
#define DHTType DHT11
#define buzzerPin 3

// LCD Config
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Sensor Objects
DHT dht(DHTPin, DHTType);

// Constants
#define NUM_SAMPLES 20
#define SAFE_PH_MIN 5
#define SAFE_PH_MAX 7.9
#define SAFE_TDS_MAX 100

// Offset
float pHOffset = 1.5;

// Timing
unsigned long previousPH_TDS_Time = 0;
const unsigned long updateInterval = 120000; // 2 minutes in milliseconds

// Global variables to hold last values
float lastPH = 0;
float lastTDS = 0;

float getStablePH() {
  float values[NUM_SAMPLES];
  float sum = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    int raw = analogRead(pHSensorPin);
    float voltage = raw * (5.0 / 1023.0);
    float pHValue = 7 + ((2.5 - voltage) * 3.5) + pHOffset;
    values[i] = pHValue;
    sum += pHValue;
    delay(10);
  }

  float avg = sum / NUM_SAMPLES;

  float cleanSum = 0;
  int cleanCount = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    if (abs(values[i] - avg) <= 0.3) {
      cleanSum += values[i];
      cleanCount++;
    }
  }

  return cleanCount > 0 ? (cleanSum / cleanCount) : avg;
}

float getStableTDS() {
  float sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int raw = analogRead(TDSSensorPin);
    float voltage = raw * (5.0 / 1023.0);
    float tds = (133.42 * pow(voltage, 3)) - (255.86 * pow(voltage, 2)) + (857.39 * voltage);
    sum += tds;
    delay(100);
  }
  return sum / NUM_SAMPLES;
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  dht.begin();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Quality");
  delay(2000);

  // Initial reading
  lastPH = getStablePH();
  lastTDS = getStableTDS();
  previousPH_TDS_Time = millis();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  unsigned long currentMillis = millis();

  // Update pH and TDS every 2 minutes
  if (currentMillis - previousPH_TDS_Time >= updateInterval) {
    lastPH = getStablePH();
    lastTDS = getStableTDS();
    previousPH_TDS_Time = currentMillis;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("pH:");
  lcd.print(lastPH, 1);
  lcd.print(" TDS:");
  lcd.print(lastTDS, 0);

  lcd.setCursor(0, 1);
  if (isnan(temp) || isnan(hum)) {
    lcd.print("Sensor Error");
  } else {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C ");
    lcd.print("H:");
    lcd.print(hum, 0);
    lcd.print("%");
  }

  Serial.print("pH: "); Serial.println(lastPH);
  Serial.print("TDS: "); Serial.println(lastTDS);
  Serial.print("Temp: "); Serial.println(temp);
  Serial.print("Humidity: "); Serial.println(hum);

  // Buzzer alert
  // Inside loop()
bool isPHUnsafe = (lastPH < SAFE_PH_MIN || lastPH > SAFE_PH_MAX);
bool isTDSUnsafe = (lastTDS > SAFE_TDS_MAX);

if (isPHUnsafe || isTDSUnsafe) {
  digitalWrite(buzzerPin, HIGH);
} else {
  digitalWrite(buzzerPin, LOW);
}


  delay(2500);  // Display update delay
} 
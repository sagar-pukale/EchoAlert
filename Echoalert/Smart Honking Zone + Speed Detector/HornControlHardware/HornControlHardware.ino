#include <SoftwareSerial.h>

// Pin configuration
#define IR_PIN        2
#define MQ_PIN        A0
#define VIB_PIN       3
#define BUTTON_PIN    11
#define RELAY_PIN     9
#define BUZZER_PIN    10

// GPS
#define GPS_RX 5
#define GPS_TX 4
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);

// GSM
#define GSM_RX 8
#define GSM_TX 7
SoftwareSerial gsmSerial(GSM_RX, GSM_TX);

// Variables
int gasValue = 0;
bool seatbeltOn = false;
bool alcoholSafe = false;
bool inNoHornZone = true;  // Default to No-Honking Zone
String latitude = "0.0";
String longitude = "0.0";
bool smsSent = false;

// Define your no-honking zone coordinates (example)
float noHornLat = 19.9975;
float noHornLon = 73.7898;
float zoneRadius = 0.0015;   // ~100m radius

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);

  pinMode(IR_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH); // LOW-level trigger buzzer (OFF initially)

  Serial.println("=== EchoAlert System ===");
  delay(2000);

  gsmSerial.println("AT");
  delay(1000);
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
}

void loop() {
  int irValue = digitalRead(IR_PIN);
  gasValue = analogRead(MQ_PIN);
  int vibValue = digitalRead(VIB_PIN);
  int buttonState = digitalRead(BUTTON_PIN);

  seatbeltOn = (irValue == LOW);
  alcoholSafe = (gasValue < 800);

  getGPSData();
  //inNoHornZone =
bool  isInNoHornZone = true;

  // Car ignition condition
  if (seatbeltOn && alcoholSafe) {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("✅ Car Started (Relay ON)");
  } else {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("❌ Car Locked! Seatbelt or Alcohol check failed");
  }

  // Horn Logic
  if (buttonState == LOW) {
    if (inNoHornZone) {
      tone(BUZZER_PIN, 100,1000); // very low frequency for soft sound
      Serial.println("🔇 Horn Pressed - Low Volume (No Honking Zone)");
    } else {
      tone(BUZZER_PIN, 1000); // normal loud tone
      Serial.println("🔊 Horn Pressed - Normal Volume");
    }
  } else {
    noTone(BUZZER_PIN); // stop sound when button not pressed
  }

  // Accident Detection
  if (vibValue == LOW) {
    Serial.println("💥 Accident Detected!");
    if (!smsSent) {
      sendSMSAlert();
      smsSent = true;
    }
  } else {
    smsSent = false;
  }

  // Buzzer OFF if all safe conditions met
  if (seatbeltOn && alcoholSafe) {
    digitalWrite(BUZZER_PIN, HIGH); // OFF for LOW-level buzzer
    Serial.println("🔕 Buzzer OFF - Safe Conditions Met");
  }

  // Display status
  Serial.print("Seatbelt: ");
  Serial.print(seatbeltOn);
  Serial.print(" | Alcohol: ");
  Serial.print(gasValue);
  Serial.print(" | GPS: ");
  Serial.print(latitude);
  Serial.print(",");
  Serial.print(longitude);
  Serial.print(" | Vib: ");
  Serial.print(vibValue);
  Serial.print(" | In NoHornZone: ");
  Serial.println(inNoHornZone);

  delay(500);
}

// --- Check if inside No-Honking Zone ---
bool isInNoHornZone() {
  float lat = latitude.toFloat();
  float lon = longitude.toFloat();
  if (abs(lat - noHornLat) < zoneRadius && abs(lon - noHornLon) < zoneRadius) {
    return true;
  }
  return false;
}

// --- Read GPS data ---
void getGPSData() {
  while (gpsSerial.available()) {
    String data = gpsSerial.readStringUntil('\n');
    if (data.startsWith("$GPGGA")) {
      int idx1 = data.indexOf(',');
      int idx2 = data.indexOf(',', idx1 + 1);
      int idx3 = data.indexOf(',', idx2 + 1);
      int idx4 = data.indexOf(',', idx3 + 1);
      int idx5 = data.indexOf(',', idx4 + 1);
      latitude = data.substring(idx3 + 1, idx4);
      longitude = data.substring(idx5 + 1, idx5 + 10);
    }
  }
}

// --- Send SMS alert on accident ---
void sendSMSAlert() {
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.println("AT+CMGS=\"+917015169102\"");  // Replace with your number
  delay(1000);
  gsmSerial.print("⚠ Accident Detected!\nLocation: ");
  gsmSerial.print("https://maps.google.com/?q=");
  gsmSerial.print(latitude);
  gsmSerial.print(",");
  gsmSerial.print(longitude);
  gsmSerial.write(26);
  delay(3000);
  Serial.println("📩 Accident Alert SMS Sent!");
}
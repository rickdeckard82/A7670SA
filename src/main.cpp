// main.cpp - Versão atualizada com correções
#define TINY_GSM_MODEM_SIM7600

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <HardwareSerial.h>

#define MODEM_RX 16
#define MODEM_TX 17
#define MODEM_BAUD 115200
#define GPS_UPDATE_INTERVAL 10000
#define NET_TEST_INTERVAL 60000

HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);

bool enableGPS() {
  modem.sendAT("+CGPS=1,1");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("GPS initialization failed");
    return false;
  }
  return true;
}

void testInternet() {
  TinyGsmClient client(modem);
  if (!client.connect("example.com", 80)) {
    Serial.println("❌ Internet test failed");
    return;
  }
  client.stop();
  Serial.println("✅ Internet working");
}

void setup() {
  Serial.begin(115200);
  delay(3000); // Wait for serial monitor
  
  SerialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000); // Critical modem boot delay

  Serial.println("Initializing modem...");
  if (!modem.restart()) { // More reliable than init()
    Serial.println("❌ Modem failed");
    ESP.restart();
  }

  // Safe string handling
  String imei = modem.getIMEI();
  Serial.println("IMEI: " + (imei ? imei : "Unknown"));
  
  // Network connection with timeout
  Serial.println("Connecting to network...");
  if (!modem.waitForNetwork(180000)) { // 3-minute timeout
    Serial.println("❌ Network failed");
    ESP.restart();
  }
  Serial.println("✅ Network connected");

  // APN with fallback
  const char* apn = "smart.m2m.vivo.com.br";
  if (!modem.gprsConnect(apn, "vivo", "vivo")) {
    Serial.println("⚠️ APN failed - trying alternative");
    apn = "zap.vivo.com.br"; // Common fallback
    if (!modem.gprsConnect(apn, "vivo", "vivo")) {
      Serial.println("❌ Couldn't connect to any APN");
    }
  }
  Serial.println("✅ GPRS connected. IP: " + modem.localIP().toString());

  // GPS with verification
  if (enableGPS()) {
    Serial.println("✅ GPS enabled");
  } else {
    Serial.println("⚠️ GPS disabled");
  }
}

void loop() {
  static uint32_t lastGPSTime = 0;
  static uint32_t lastNetTime = 0;
  const uint32_t now = millis();

  // GPS Reading with pointer validation
  if (now - lastGPSTime > GPS_UPDATE_INTERVAL) {
    float lat = 0, lon = 0;
    if (modem.getGPS(&lat, &lon)) { // Library should handle NULL pointers
      Serial.printf("📍 Position: %.6f, %.6f\n", lat, lon);
    } else {
      Serial.println("🔍 Searching for GPS satellites...");
    }
    lastGPSTime = now;
  }

  // Network test
  if (now - lastNetTime > NET_TEST_INTERVAL) {
    testInternet();
    lastNetTime = now;
  }

  delay(1000);
}
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
  Serial.println("\n[GPS] Starting initialization...");
  
  // 1. Check if GPS is already enabled
  modem.sendAT("+CGPS?");
  if (modem.waitResponse(1000L, "+CGPS:") > 0) {
    String response = modem.stream.readStringUntil('\n');
    if (response.indexOf("1,1") >= 0) {
      Serial.println("[GPS] Already enabled");
      return true;
    }
  }

  // 2. Try different enable commands for various SIM7600 variants
  const char* gpsCommands[] = {
    "+CGPS=1,1",  // Standard command
    "+CGPS=1",    // Alternate command
    "+CGPS=1,1,\"127.0.0.1\",\"8001\"",  // For TCP mode
    "+CGPS=1,1,\"127.0.0.1\",\"8001\",1" // With NMEA output
  };

  for (int i = 0; i < sizeof(gpsCommands)/sizeof(gpsCommands[0]); i++) {
    Serial.printf("[GPS] Trying command: %s\n", gpsCommands[i]);
    modem.sendAT(gpsCommands[i]);
    if (modem.waitResponse(10000L) == 1) {
      Serial.println("[GPS] Enable command accepted");
      
      // 3. Verify GPS state
      delay(2000);
      modem.sendAT("+CGPS?");
      if (modem.waitResponse(1000L, "+CGPS:") > 0) {
        String status = modem.stream.readStringUntil('\n');
        Serial.println("[GPS] Current status: " + status);
        if (status.indexOf("1,1") >= 0) {
          Serial.println("[GPS] Successfully enabled");
          return true;
        }
      }
    }
    delay(1000);
  }

  Serial.println("[GPS] All enable attempts failed");
  return false;
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
  if (!modem.waitForNetwork(1800)) { // 3-minute timeout
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

void checkGPS() {
  // 1. Get basic position
  float lat, lon;
  if (modem.getGPS(&lat, &lon)) {
    Serial.printf("[GPS] Position: %.6f,%.6f\n", lat, lon);
    return;
  }

  // 2. If no fix, get detailed status
  modem.sendAT("+CGPSINFO");
  if (modem.waitResponse(1000L, "+CGPSINFO:") > 0) {
    String info = modem.stream.readStringUntil('\n');
    Serial.println("[GPS] Detailed status: " + info);
    
    if (info.indexOf(",,,,,,,") >= 0) {
      Serial.println("[GPS] No fix acquired");
    } else {
      Serial.println("[GPS] Partial data received");
    }
  }

  // 3. Get satellite information
  modem.sendAT("+CGNSSINFO");
  if (modem.waitResponse(1000L, "+CGNSSINFO:") > 0) {
    String satInfo = modem.stream.readStringUntil('\n');
    Serial.println("[GPS] Satellite info: " + satInfo);
  }
}

void loop() {
  static uint32_t lastGPSTime = 0;
  static uint32_t lastNetTime = 0;
  static bool gpsEnabled = false;
  const uint32_t now = millis();

  // GPS Handling
  if (now - lastGPSTime > GPS_UPDATE_INTERVAL) {
    if (!gpsEnabled) {
      gpsEnabled = enableGPS(); // Try to re-enable if previously failed
    }
    
    if (gpsEnabled) {
      float lat = 0, lon = 0, speed = 0, alt = 0;
      int vsat = 0, usat = 0;
      float accuracy = 0;
      
      // Enhanced GPS status check
      if (modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat, &accuracy)) {
        Serial.printf("📍 Position: Lat:%.6f Lon:%.6f\n", lat, lon);
        Serial.printf("   Speed: %.1fkm/h Alt: %.1fm Sats: %d/%d Acc: %.1fm\n",
                     speed, alt, usat, vsat, accuracy);
      } else {
        Serial.println("🔍 Acquiring satellites...");
      }
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
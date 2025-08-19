// main.cpp - Versão adaptada para GPS com comandos AT diretos
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
  // Sequência de inicialização robusta para GPS
  modem.sendAT("+CGNSSPWR=1");  // Liga o GNSS
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("Failed to power GNSS");
    return false;
  }
  
  modem.sendAT("+CGPS=1");  // Modo standalone
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("Failed to set GPS mode");
    return false;
  }

  modem.sendAT("+CGNSSCOLD");  // Cold start
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("Failed cold start");
  }

  Serial.println("GPS initialization commands sent");
  return true;
}

String getGPSInfo() {
  modem.sendAT("+CGPSINFO");
  if (modem.waitResponse(10000L, "+CGPSINFO: ") == 1) {
    String res = modem.stream.readStringUntil('\n');
    res.trim();
    return res;
  }
  return "";
}

void parseGPSData(const String& data, float& lat, float& lon) {
  if (data.length() < 20 || data.startsWith(",,")) {  // Dados inválidos
    lat = 0;
    lon = 0;
    return;
  }

  // Exemplo: "2220.68102,S,04710.74483,W,..."
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);
  int thirdComma = data.indexOf(',', secondComma + 1);
  
  String latStr = data.substring(0, firstComma);
  String latDir = data.substring(firstComma + 1, secondComma);
  String lonStr = data.substring(secondComma + 1, thirdComma);
  String lonDir = data.substring(thirdComma + 1, thirdComma + 2);

  // Converter de DMM.MMMMM para graus decimais
  lat = latStr.substring(0, 2).toFloat() + (latStr.substring(2).toFloat() / 60.0);
  if (latDir == "S") lat *= -1;
  
  lon = lonStr.substring(0, 3).toFloat() + (lonStr.substring(3).toFloat() / 60.0);
  if (lonDir == "W") lon *= -1;
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
  if (!modem.restart()) {
    Serial.println("❌ Modem failed");
    ESP.restart();
  }

  String imei = modem.getIMEI();
  Serial.println("IMEI: " + (imei ? imei : "Unknown"));
  
  Serial.println("Connecting to network...");
  if (!modem.waitForNetwork(180000)) {
    Serial.println("❌ Network failed");
    ESP.restart();
  }
  Serial.println("✅ Network connected");

  const char* apn = "smart.m2m.vivo.com.br";
  if (!modem.gprsConnect(apn, "vivo", "vivo")) {
    Serial.println("⚠️ APN failed - trying alternative");
    apn = "zap.vivo.com.br";
    if (!modem.gprsConnect(apn, "vivo", "vivo")) {
      Serial.println("❌ Couldn't connect to any APN");
    }
  }
  Serial.println("✅ GPRS connected. IP: " + modem.localIP().toString());

  if (enableGPS()) {
    Serial.println("✅ GPS enabled");
  } else {
    Serial.println("⚠️ GPS disabled - using fallback");
  }
}

void loop() {
  static uint32_t lastGPSTime = 0;
  static uint32_t lastNetTime = 0;
  const uint32_t now = millis();

  // GPS Reading
  if (now - lastGPSTime > GPS_UPDATE_INTERVAL) {
    String gpsData = getGPSInfo();
    if (gpsData.length() > 10 && !gpsData.startsWith(",,")) {
      float lat, lon;
      parseGPSData(gpsData, lat, lon);
      Serial.printf("📍 Position: %.6f, %.6f\n", lat, lon);
      
      // Debug raw data
      Serial.println("Raw GPS: " + gpsData);
    } else {
      Serial.println("🔍 Searching for satellites...");
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
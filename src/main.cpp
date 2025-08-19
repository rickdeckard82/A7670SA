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
  // Implementação melhorada mostrada acima
  // ...
}

void testInternet() {
  // Implementação melhorada mostrada acima
  // ...
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  SerialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println("Iniciando modem...");
  if (!modem.init()) {
    Serial.println("❌ Falha na inicialização");
    return;
  }

  Serial.println("✅ Modem OK");
  Serial.println("IMEI: " + String(modem.getIMEI()));
  Serial.println("Operadora: " + String(modem.getOperator()));

  Serial.println("Conectando à rede...");
  if (!modem.waitForNetwork()) {
    Serial.println("❌ Falha na rede");
    return;
  }
  Serial.println("✅ Rede conectada");

  Serial.println("Conectando APN...");
  if (!modem.gprsConnect("smart.m2m.vivo.com.br", "vivo", "vivo")) {
    Serial.println("⚠️ Falha na APN");
  } else {
    Serial.println("✅ APN conectada. IP: " + modem.localIP().toString());
  }

  if (!enableGPS()) {
    Serial.println("⚠️ Continuando sem GPS");
  }
}

void loop() {
  static unsigned long lastGPSTime = 0;
  static unsigned long lastNetTime = 0;

  // Teste periódico do GPS
  if (millis() - lastGPSTime > GPS_UPDATE_INTERVAL) {
    float lat, lon;
    if (modem.getGPS(&lat, &lon)) {
      Serial.print("📍 Posição: ");
      Serial.print(lat, 6);
      Serial.print(", ");
      Serial.println(lon, 6);
    } else {
      Serial.println("🔍 Buscando satélites GPS...");
    }
    lastGPSTime = millis();
  }

  // Teste periódico de internet
  if (millis() - lastNetTime > NET_TEST_INTERVAL) {
    testInternet();
    lastNetTime = millis();
  }

  delay(1000);
}
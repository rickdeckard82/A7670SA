// main.cpp — Teste de conexão com APN M2M Vivo

// 1. DEFINA O MODELO DO MODEM ANTES DE INCLUIR A BIBLIOTECA
#define TINY_GSM_MODEM_SIM7600  // Para o A7670 use SIM7600 (é o mais compatível)
// OU experimente também:
// #define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_DEBUG Serial

// 2. INCLUA AS BIBLIOTECAS DEPOIS DA DEFINIÇÃO
#include <Arduino.h>
#include <TinyGsmClient.h>
#include <HardwareSerial.h>

// Configurações de hardware
#define MODEM_RX 16
#define MODEM_TX 17
#define MODEM_BAUD 115200  // Baud rate padrão do A7670

HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);  // Agora deve reconhecer o tipo
// Adicione esta variável global no início do seu código
unsigned long lastTestTime = 0;
const unsigned long testInterval = 30000; // 30 segundos entre testes


void setup() {
  Serial.begin(115200);
  delay(3000);

  SerialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println("🔌 Inicializando modem A7670...");

  // Inicialização do modem
  if (!modem.init()) {
    Serial.println("❌ Falha ao inicializar modem");
    return;
  }
  
  Serial.println("✅ Modem inicializado");

  // Verificar rede
  Serial.print("Aguardando rede...");
  if (!modem.waitForNetwork()) {
    Serial.println("❌ Falha ao conectar na rede");
    return;
  }
  Serial.println("✅ Conectado na rede");

  // Conectar APN
  Serial.print("Conectando na APN...");
  if (!modem.gprsConnect("smart.m2m.vivo.com.br", "vivo", "vivo")) {
    Serial.println("❌ Falha na conexão GPRS");
    return;
  }
  Serial.println("✅ Conectado na APN");

  // Mostrar IP
  Serial.print("🌐 IP Local: ");
  Serial.println(modem.localIP());
}

void testInternet() {
  TinyGsmClient client(modem);
  
  if (client.connect("example.com", 80)) {
    Serial.println("Conectado ao servidor");
    client.print("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    delay(100);
    
    while (client.available()) {
      Serial.write(client.read());
    }
    client.stop();
  } else {
    Serial.println("Falha na conexão com a internet");
  }
}

void loop() {
 // Manter a conexão ativa (seu código existente)
  if (!modem.isGprsConnected()) {
    Serial.println("❌ Conexão GPRS perdida");
    if (!modem.gprsConnect("smart.m2m.vivo.com.br", "vivo", "vivo")) {
      Serial.println("❌ Falha ao reconectar");
      delay(1000);
      return;
    }
  }

  // Chamar testInternet() periodicamente
  if (millis() - lastTestTime >= testInterval) {
    testInternet();
    lastTestTime = millis(); // Atualiza o tempo do último teste
  }

  delay(1000); // Pequeno delay para evitar sobrecarga
}


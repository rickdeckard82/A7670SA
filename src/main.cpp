#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <HardwareSerial.h>

#define MODEM_RX 16
#define MODEM_TX 17

HardwareSerial SerialAT(2);

unsigned long ultimaConsulta = 0;
const unsigned long intervaloConsulta = 10000;  // 10 segundos

bool gpsAtivo = false;

void enviarComandoAT(const char *comando, unsigned long espera = 800) {
  Serial.print("📤 Enviando: ");
  Serial.println(comando);
  SerialAT.println(comando);
  delay(espera);

  Serial.print("📥 Resposta: ");
  while (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
  Serial.println("\n----------------------");
}

bool registradoNaRede() {
  SerialAT.println("AT+CREG?");
  delay(500);
  String resposta = "";
  while (SerialAT.available()) {
    char c = SerialAT.read();
    resposta += c;
  }
  Serial.print("📶 Estado de registro: ");
  Serial.println(resposta);
  return resposta.indexOf("+CREG: 0,1") != -1 || resposta.indexOf("+CREG: 0,5") != -1;
}

void ativarGPS() {
  Serial.println("📡 Ativando GNSS com AT+CGNSSPWR=1");
  enviarComandoAT("AT+CGNSSPWR=1");
  delay(1000);

  Serial.println("📍 Verificando status do GNSS...");
  enviarComandoAT("AT+CGNSSPWR?");
  gpsAtivo = true;
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("🔌 Iniciando ESP32 + A7670SA");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  enviarComandoAT("AT");
  enviarComandoAT("ATE0");
  enviarComandoAT("AT+CPIN?");
  enviarComandoAT("AT+CSQ");
  enviarComandoAT("AT+COPS?");
  enviarComandoAT("AT+CREG?");

  Serial.println("⏳ Esperando registro na rede celular...");
  int tentativas = 0;
  while (!registradoNaRede() && tentativas < 30) {
    tentativas++;
    Serial.print("⏱️ Tentativa ");
    Serial.println(tentativas);
    delay(3000);
  }

  if (tentativas >= 30) {
    Serial.println("❌ Não conseguiu registrar na rede após várias tentativas.");
  } else {
    Serial.println("✅ Registrado com sucesso na rede celular!");
  }

  ativarGPS();
}

void loop() {
  unsigned long agora = millis();
  if (agora - ultimaConsulta >= intervaloConsulta) {
    ultimaConsulta = agora;

    Serial.println("📍 Consultando localização via AT+CGNSSINFO...");
    SerialAT.println("AT+CGNSSINFO");
    delay(1000);

    String resposta = "";
    while (SerialAT.available()) {
      resposta += (char)SerialAT.read();
    }

    Serial.print("📥 Resposta: ");
    Serial.println(resposta);
    Serial.println("----------------------");

    // Se falhar ou sem fix, tenta reativar
    if (resposta.indexOf("ERROR") != -1 || resposta.indexOf(",,,,,,,") != -1) {
      Serial.println("⚠️ Sem fix ou falha. Reiniciando GNSS...");
      gpsAtivo = false;
      enviarComandoAT("AT+CGNSSPWR=0");
      delay(1000);
      ativarGPS();
    }
  }
}

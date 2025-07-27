#define TINY_GSM_MODEM_SIM7600   // Compatível com A7670SA
#define TINY_GSM_RX_BUFFER 1024

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <HardwareSerial.h>

#define MODEM_RX     16
#define MODEM_TX     17
#define MODEM_PWRKEY 4

HardwareSerial SerialAT(2);

void ligarModem() {
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(3000);
}

void enviarComandoAT(const char *comando, unsigned long espera = 800) {
  Serial.print("📤 Enviando: ");
  Serial.println(comando);
  SerialAT.println(comando);
  delay(espera);

  Serial.print("📥 Resposta: ");
  while (SerialAT.available()) {
    char c = SerialAT.read();
    Serial.write(c);
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

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("🔌 Iniciando ESP32 + A7670SA");

  ligarModem();
  Serial.println("✅ Modem acionado");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  enviarComandoAT("AT");            // Teste comunicação
  enviarComandoAT("ATE0");          // Desativa echo
  enviarComandoAT("AT+CPIN?");      // Verifica SIM
  enviarComandoAT("AT+CSQ");        // Sinal
  enviarComandoAT("AT+CBANDCFG?");  // Bandas ativas
  enviarComandoAT("AT+COPS?");      // Operadora atual

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
}

void loop() {
  // nada a fazer
}

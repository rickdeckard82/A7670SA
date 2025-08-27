// serial_bridge.ino — Ponte serial ESP32 <-> Modem A7670
// Use isto para testar a UART do ESP32 exatamente como você faz no terminal do PC.
// Tudo que você digitar no Serial Monitor (USB) será enviado ao modem, e vice-versa.
//
// Comandos especiais (digite e pressione Enter):
//   ~baud 115200        -> troca a UART do modem para 115200 bps (reinicia UART2)
//   ~pins 16 17         -> troca os pinos RX/TX da UART2 (ex.: RX=16, TX=17)
//   ~help               -> mostra este help
//
// Observação: NÃO envia nada automaticamente. Você controla 100% os AT pela USB.

#include <Arduino.h>

// ---------- ajuste de pinos/baud padrão ----------
static int MODEM_RX = 16;
static int MODEM_TX = 17;
static unsigned long MODEM_BAUD = 115200;

HardwareSerial SerialAT(2);

void printHelp() {
  Serial.println();
  Serial.println(F("Comandos especiais:"));
  Serial.println(F("  ~baud <valor>     -> ex.: ~baud 115200"));
  Serial.println(F("  ~pins <rx> <tx>   -> ex.: ~pins 16 17"));
  Serial.println(F("  ~help             -> este help"));
  Serial.println();
}

void beginAT() {
  SerialAT.end();
  delay(50);
  SerialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(100);
  Serial.print(F("[UART2] RX=")); Serial.print(MODEM_RX);
  Serial.print(F(" TX=")); Serial.print(MODEM_TX);
  Serial.print(F(" BAUD=")); Serial.println(MODEM_BAUD);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println(F("\n=== ESP32 Serial Bridge -> A7670 ==="));
  Serial.println(F("Digite AT+... e pressione Enter."));
  printHelp();
  beginAT();
}

String cmdBuf;

void handleMetaCommand(const String& line) {
  if (line.startsWith("~baud ")) {
    unsigned long b = line.substring(6).toInt();
    if (b >= 1200 && b <= 2000000UL) {
      MODEM_BAUD = b;
      beginAT();
    } else {
      Serial.println(F("[ERRO] Baud inválido."));
    }
  } else if (line.startsWith("~pins ")) {
    int sp1 = line.indexOf(' ', 6);
    if (sp1 > 0) {
      int rx = line.substring(6, sp1).toInt();
      int tx = line.substring(sp1 + 1).toInt();
      if (rx >= 0 && tx >= 0) {
        MODEM_RX = rx; MODEM_TX = tx;
        beginAT();
      } else {
        Serial.println(F("[ERRO] Pinos inválidos."));
      }
    } else {
      Serial.println(F("[ERRO] Use: ~pins <rx> <tx>"));
    }
  } else if (line.startsWith("~help")) {
    printHelp();
  } else {
    Serial.println(F("[INFO] Comando especial desconhecido. Use ~help."));
  }
}

void loop() {
  // USB -> Modem
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;  // ignora CR
    if (c == '\n') {
      // linha completa
      String line = cmdBuf;
      cmdBuf = "";
      if (line.startsWith("~")) {
        handleMetaCommand(line);
      } else {
        SerialAT.println(line);   // envia ao modem com LF
      }
    } else {
      cmdBuf += c;
    }
  }

  // Modem -> USB
  while (SerialAT.available()) {
    char c = (char)SerialAT.read();
    Serial.write(c);
  }
}

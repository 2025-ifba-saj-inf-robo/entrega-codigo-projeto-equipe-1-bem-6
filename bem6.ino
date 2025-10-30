#include "arduino_secrets.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "BluetoothSerial.h"

// --- CONFIGURAÃÃO DO BOTÃO ---
// Defina o pino GPIO onde o botÃ£o estÃ¡ conectado
// IMPORTANTE: O botÃ£o deve ser conectado entre este pino e o GND
#define PINO_BOTAO 4 
int estadoAnteriorBotao = HIGH; // VariÃ¡vel para detectar o clique (debounce)

// === CONFIGURAÃÃO DO LCD ===
LiquidCrystal_I2C lcd(0x27, 16, 2);  // EndereÃ§o do seu display

// === CONFIGURAÃÃO DO BLUETOOTH ===
BluetoothSerial SerialBT;
String device_name = "ESP32-LCD";
String incomingMessage = "";

void setup() {
  // Serial para debug e envio
  Serial.begin(115200);

  // Inicializa Bluetooth
  SerialBT.begin(device_name);
  Serial.println("Bluetooth iniciado! Conecte-se e envie mensagens.");
  Serial.println("Digite no Monitor Serial para enviar dados ao celular.");

  // Inicializa LCD
  lcd.init();      
  lcd.backlight(); 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aguardando...");

  // Configura o pino do botÃ£o como entrada com resistor pull-up interno
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  Serial.println("Pressione o botao no pino " + String(PINO_BOTAO) + " para enviar localizacao.");
}

void loop() {
  // --- PARTE 1: Verifica se o botÃ£o foi pressionado ---
  // (Esta Ã© a parte nova)
  verificarBotao();

  // --- PARTE 2: Recebe do Celular (Bluetooth) e mostra no LCD ---
  if (SerialBT.available()) {
    char c = SerialBT.read();

    if (c == '\n' || c == '\r') {
      if (incomingMessage.length() > 0) {
        mostrarNoLCD(incomingMessage);
        Serial.print("Recebido do Celular: ");
        Serial.println(incomingMessage);
        incomingMessage = "";
      }
    } else {
      incomingMessage += c;
    }
  }

  // --- PARTE 3: Recebe do Monitor Serial e envia para o Celular (Bluetooth) ---
  if (Serial.available()) {
    while (Serial.available()) {
      char c = Serial.read();
      SerialBT.write(c);
    }
  }
}

/**
 * @brief Verifica se o botÃ£o foi pressionado e envia a mensagem.
 * Utiliza detecÃ§Ã£o de borda de descida (HIGH para LOW) para enviar sÃ³ uma vez.
 */
void verificarBotao() {
  // LÃª o estado atual do botÃ£o
  int estadoAtualBotao = digitalRead(PINO_BOTAO);

  // Compara com o estado anterior
  // Se antes estava ALTO (HIGH) e agora estÃ¡ BAIXO (LOW), significa que o botÃ£o
  // acabou de ser pressionado.
  if (estadoAnteriorBotao == HIGH && estadoAtualBotao == LOW) {
    Serial.println("Botao pressionado!");
    
    String mensagem = "enviando localizaÃ§Ã£o atual";
    mostrarNoLCD(mensagem);

    // Envia a mensagem para o Monitor Serial
    Serial.println(mensagem);
    
    // Envia a mensagem para o Bluetooth Serial do celular
    // Usamos println() para que o app no celular receba um "Enter" no final
    SerialBT.println(mensagem);

    // Pequeno atraso (debounce) para evitar leituras mÃºltiplas
    delay(50); 
  }

  // Atualiza o estado anterior do botÃ£o para a prÃ³xima verificaÃ§Ã£o no loop
  estadoAnteriorBotao = estadoAtualBotao;
}


// FunÃ§Ã£o para mostrar a mensagem no LCD
void mostrarNoLCD(String msg) {
  lcd.clear();

  // Cabe em uma linha
  if (msg.length() <= 16) {
    lcd.setCursor(0, 0);
    lcd.print(msg);
  } 
  // Divide em duas linhas (mÃ¡x 32 caracteres)
  else {
    lcd.setCursor(0, 0);
    lcd.print(msg.substring(0, 16));
    lcd.setCursor(0, 1);
    int fim = min((int)msg.length(), 32);
    lcd.print(msg.substring(16, fim));
  }
}
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "MAX30100_PulseOximeter.h"

// ----- LCD -----
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ----- MAX30100 -----
PulseOximeter pox;
uint32_t lastRead = 0;
#define REPORT_PERIOD 1000

// ----- Botão -----
const int BTN = 15;
unsigned long lastPress = 0;
int clickCount = 0;
bool measuring = false;
bool alertMode = false;

// ----- WiFi -----
String ssid = "REDE";
String pass = "SENHA";

// ----- Servidor -----
String emergencyContact = "+5599XXXXXXXX";
String serverURL = "http://SEUSITE.com/api/emergencia";

// =========================
// Funções Auxiliares
// =========================

void sendMessage(String msg) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"contato\": \"" + emergencyContact + "\", \"mensagem\": \"" + msg + "\"}";
    http.POST(payload);
    http.end();
}

void showClock() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Hora Brasilia:");
    lcd.setCursor(0,1);
    lcd.print(buffer);
}

// callback do sensor
void onBeatDetected() {
    Serial.println("Pulso detectado!");
}

void toggleMeasurement() {
    measuring = !measuring;
    lcd.clear();
    if (measuring) {
        lcd.print("Medindo...");
    } else {
        lcd.print("Medicao OFF");
    }
}

// =========================
// setup()
// =========================
void setup() {
    Serial.begin(115200);

    Wire.begin();
    lcd.init();
    lcd.backlight();

    pinMode(BTN, INPUT_PULLUP);

    // --- Conexão WiFi ---
    lcd.print("Conectando...");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) delay(200);
    lcd.clear();

    // Relógio NTP
    configTime(-3 * 3600, 0, "pool.ntp.org");

    // --- Inicializar MAX30100 ---
    lcd.print("Sensor MAX30100...");
    if (!pox.begin()) {
        lcd.clear();
        lcd.print("ERRO SENSOR!");
        while(1);
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);

    lcd.clear();
}

// =========================
// loop()
// =========================
void loop() {

    // ===================================
    // TRATAMENTO DO BOTÃO
    // ===================================
    if (!digitalRead(BTN)) {
        unsigned long now = millis();

        if (now - lastPress > 40) { // Debounce
            clickCount++;
            lastPress = now;
        }

        // Pressão longa → liga/desliga medição
        if (now - lastPress > 800 && !measuring) {
            toggleMeasurement();
            while (!digitalRead(BTN)); // aguarda soltar
            return;
        }
    }

    // Clique simples / duplo
    if (clickCount > 0 && millis() - lastPress > 300) {

        // Clique simples → enviar alerta
        if (clickCount == 1) {
            alertMode = true;
            lcd.clear();
            lcd.print("ALERTA enviado");
            sendMessage("ALERTA! Preciso de ajuda. Minha localizacao: ...");
        }

        // Duplo clique → cancelar alerta
        if (clickCount == 2) {
            alertMode = false;
            lcd.clear();
            lcd.print("Alerta cancelado");
            sendMessage("Desconsidere. Acionamento enganoso.");
        }

        clickCount = 0;
    }

    // ===================================
    // MEDIÇÃO MAX30100
    // ===================================
    pox.update(); // precisa rodar sempre

    if (measuring) {
        if (millis() - lastRead > REPORT_PERIOD) {

            float bpm = pox.getHeartRate();
            float spo2 = pox.getSpO2();

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("BPM: ");
            lcd.print(bpm);

            lcd.setCursor(0,1);
            lcd.print("SpO2: ");
            lcd.print(spo2);
            lcd.print("%");

            lastRead = millis();
        }
    } 
    else {
        showClock();
        delay(500);
    }
}

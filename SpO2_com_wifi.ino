#include <Wire.h>
#include "MAX30105.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ----------------- REDE WiFi -----------------
const char* ssid = "NF_Casa_TpLink";
const char* password = "br20001965";
const char* serverName = "http://192.168.0.107:3000/endpoint"; // Altere conforme o IP/porta do seu backend

// ----------------- SENSOR MAX30102 -----------------
MAX30105 particleSensor;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

long irMin = 0x7FFFFFFF, irMax = 0;
long redMin = 0x7FFFFFFF, redMax = 0;

// ----------------- SENSOR DS18B20 -----------------
const int oneWireBus = 4; // Pino conectado ao sensor de temperatura
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
float temperaturaC = 0.0;

bool detectBeat(long irValue) {
  static long lastIR = 0;
  static bool up = false;
  bool beatDetected = false;

  long delta = irValue - lastIR;
  if (delta > 1000) up = true;

  if (up && delta < -1000 && irValue > 50000) {
    beatDetected = true;
    up = false;
  }

  lastIR = irValue;
  return beatDetected;
}

// ----------------- SETUP -----------------
void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Iniciando sensores...");

  // Conexão WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  // Inicia MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 não encontrado.");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x32);
  particleSensor.setPulseAmplitudeGreen(0);

  // Inicia DS18B20
  sensors.begin();
  Serial.println("DS18B20 iniciado.");
}

// ----------------- LOOP PRINCIPAL -----------------
void loop() {
  static unsigned long lastReport = 0;
  const unsigned long reportInterval = 1000;

  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  bool dedoDetectado = irValue > 10000;

  if (dedoDetectado) {
    if (detectBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute < 180 && beatsPerMinute > 40) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

    if (irValue < irMin) irMin = irValue;
    if (irValue > irMax) irMax = irValue;
    if (redValue < redMin) redMin = redValue;
    if (redValue > redMax) redMax = redValue;
  } else {
    irMin = 0x7FFFFFFF; irMax = 0;
    redMin = 0x7FFFFFFF; redMax = 0;
  }

  if (millis() - lastReport >= reportInterval) {
    sensors.requestTemperatures();
    temperaturaC = sensors.getTempCByIndex(0);

    Serial.print("IR: ");
    Serial.print(irValue);
    Serial.print(" | RED: ");
    Serial.print(redValue);

    if (dedoDetectado) {
      float acIR = irMax - irMin;
      float dcIR = irValue;
      float acRED = redMax - redMin;
      float dcRED = redValue;
      float R = (acRED / dcRED) / (acIR / dcIR + 0.0001);
      float SpO2 = 110.0 - 25.0 * R + 10.0;

      Serial.print(" | BPM: ");
      Serial.print(beatAvg);
      Serial.print(" | SpO2: ");
      Serial.print(SpO2, 1);
      Serial.print(" %");

      // ---------- Enviar para o banco ----------
      if (WiFi.status() == WL_CONNECTED && beatAvg > 0 && temperaturaC != -127.0) {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");

        String json = "{";
        json += "\"temperatura\":" + String(temperaturaC, 1) + ",";
        json += "\"batimentos\":" + String(beatAvg) + ",";
        json += "\"spo2\":" + String(SpO2, 1);
        json += "}";


        int httpResponseCode = http.POST(json);
        Serial.print(" | HTTP: ");
        Serial.println(httpResponseCode);
        http.end();
      }

    } else {
      Serial.print(" -> Sem dedo detectado");
    }

    if (temperaturaC == -127.0) {
      Serial.print(" | Temp: Erro!");
    } else {
      Serial.print(" | Temp: ");
      Serial.print(temperaturaC);
      Serial.print(" °C");
    }

    Serial.println();
    lastReport = millis();
  }

  delay(20); // Delay para estabilização
}

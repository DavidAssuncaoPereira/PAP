#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>

// Configuração do Wi-Fi (Modo Access Point)
const char* ssid = "ESP32-PLANTAS";
const char* password = "12345678";

WebServer server(80);

// Configuração do Sensor de Temperatura Dallas (Pino GPIO 4)
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Configuração do Sensor de Luz BH1750
BH1750 lightMeter;

// Configuração do LCD I2C (Endereço comum: 0x27 ou 0x3F)
LiquidCrystal_PCF8574 lcd(0x27);

// Variáveis para guardar as leituras
float temperatura = 0.0;
float luz = 0.0;

// Função que cria e envia a página Web
void enviarPaginaWeb() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Monitor de Plantas</title>";
  html += "<style>body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f8ff; padding: 20px; }";
  html += "h1 { color: #2e8b57; } .dado { font-size: 24px; margin: 10px; padding: 10px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }</style>";
  html += "<meta http-equiv='refresh' content='5'>"; // Atualiza a página a cada 5 segundos
  html += "</head><body>";
  html += "<h1>Estação de Monitorização</h1>";
  html += "<div class='dado'>️ Temperatura: <strong>" + String(temperatura, 1) + " °C</strong></div>";
  html += "<div class='dado'>☀️ Luminosidade: <strong>" + String(luz, 1) + " lx</strong></div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Inicia o barramento I2C (Pinos padrão: 21 SDA e 22 SCL)

  // Iniciar LCD
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("A iniciar...");

  // Iniciar Sensores
  sensors.begin();
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 iniciado com sucesso");
  } else {
    Serial.println("Erro ao iniciar BH1750");
  }

  // Configurar ESP32 como Ponto de Acesso
  Serial.print("A criar rede Wi-Fi... ");
  WiFi.softAP(ssid, password);
  Serial.println(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("Endereço IP do Servidor: ");
  Serial.println(IP);

  // Mostrar IP no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: ESP-PLANTAS");
  lcd.setCursor(0, 1);
  lcd.print(IP.toString());

  // Configurar as rotas do Servidor Web
  server.on("/", enviarPaginaWeb);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient(); // Processa os pedidos do navegador web

  // Ler os sensores
  sensors.requestTemperatures();
  temperatura = sensors.getTempCByIndex(0);
  luz = lightMeter.readLightLevel();

  // Evita mostrar valores absurdos se o sensor de temperatura não estiver ligado
  if (temperatura == DEVICE_DISCONNECTED_C) {
    temperatura = 0.0;
  }

  // Atualizar dados no LCD
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(temperatura, 1) + "C     ");
  lcd.setCursor(0, 1);
  lcd.print("Luz: " + String(luz, 0) + " lx     ");

  delay(500); // Pequena pausa para estabilidade do sistema
}
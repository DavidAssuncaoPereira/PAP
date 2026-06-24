#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>

// =====================================================
// REDE WI-FI & SERVIDOR WEB
// =====================================================
const char* ssid = "ESP32-PLANTAS";
const char* password = "12345678";
WebServer server(80);

// =====================================================
// PINOS
// =====================================================
#define PINO_TEMPERATURA 4
#define PINO_HUMIDADE    34

#define SDA_I2C 21
#define SCL_I2C 22

// =====================================================
// LCD
// =====================================================
LiquidCrystal_PCF8574 lcd(0x27);

// =====================================================
// DS18B20
// =====================================================
OneWire oneWire(PINO_TEMPERATURA);
DallasTemperature sensorTemp(&oneWire);

// =====================================================
// BH1750
// =====================================================
BH1750 sensorLuz;

// =====================================================
// VARIÁVEIS
// =====================================================
float temperaturaC = 0.0;
float luminosidadeLux = 0.0;

int humidadeRaw = 0;
int humidadePercent = 0;

// =====================================================
// HISTÓRICO
// =====================================================
struct HistoryRecord {
  float temp;
  float lux;
  int hum;
};

const int MAX_HISTORY = 24;
HistoryRecord historico[MAX_HISTORY];
int historicoCount = 0;
int historicoIndex = 0;

// =====================================================
// PAGINAÇÃO E TEMPOS
// =====================================================
int paginaAtual = 0;

unsigned long ultimoTempoLeitura = 0;
unsigned long ultimoTempoPagina = 0;
unsigned long ultimoTempoHistorico = 0;

const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_PAGINA = 5000;
const unsigned long INTERVALO_HISTORICO = 3600000; // 60 minutos

// =====================================================
// GRAU
// =====================================================
byte simboloGrau[8] = {
  B01100,
  B10010,
  B10010,
  B01100,
  B00000,
  B00000,
  B00000,
  B00000
};

// =====================================================
// HISTÓRICO LÓGICA
// =====================================================
void gravarHistorico() {
  historico[historicoIndex].temp = temperaturaC;
  historico[historicoIndex].lux = luminosidadeLux;
  historico[historicoIndex].hum = humidadePercent;

  historicoIndex = (historicoIndex + 1) % MAX_HISTORY;
  if (historicoCount < MAX_HISTORY) {
    historicoCount++;
  }
}

void enviarHistoricoJSON() {
  String json = "[";
  int startIdx = 0;
  if (historicoCount == MAX_HISTORY) {
    startIdx = historicoIndex;
  }

  for (int i = 0; i < historicoCount; i++) {
    int idx = (startIdx + i) % MAX_HISTORY;
    json += "{";
    json += "\"temp\":" + String(historico[idx].temp, 1) + ",";
    json += "\"lux\":" + String(historico[idx].lux, 1) + ",";
    json += "\"hum\":" + String(historico[idx].hum);
    json += "}";
    if (i < historicoCount - 1) {
      json += ",";
    }
  }
  json += "]";
  server.send(200, "application/json", json);
}

// =====================================================
// PÁGINA WEB
// =====================================================
void enviarPaginaWeb() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Monitor de Plantas</title>";
  html += "<style>body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f8ff; padding: 20px; }";
  html += "h1 { color: #2e8b57; } .dado { font-size: 24px; margin: 10px; padding: 10px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "canvas { background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-top: 20px; max-width: 100%; }</style>";
  html += "<meta http-equiv='refresh' content='10'>"; // Atualiza a página a cada 10 segundos
  html += "</head><body>";
  html += "<h1>Estação de Monitorização</h1>";
  html += "<div class='dado'>️ Temperatura: <strong>" + String(temperaturaC, 1) + " °C</strong></div>";
  html += "<div class='dado'>☀️ Luminosidade: <strong>" + String(luminosidadeLux, 1) + " lx</strong></div>";
  html += "<div class='dado'>💧 Humidade: <strong>" + String(humidadePercent) + " %</strong></div>";

  html += "<canvas id='chart' width='400' height='200'></canvas>";
  html += "<script>";
  html += "fetch('/history').then(r=>r.json()).then(data=>{";
  html += "if(data.length===0) return;";
  html += "const ctx = document.getElementById('chart').getContext('2d');";
  html += "const w = 400; const h = 200;";
  html += "const maxTemp = Math.max(...data.map(d=>d.temp), 40);";
  html += "const maxHum = 100;";
  html += "ctx.clearRect(0,0,w,h);";
  html += "ctx.lineWidth=2;";
  html += "const stepX = w / Math.max(data.length - 1, 1);";

  html += "ctx.beginPath(); ctx.strokeStyle='red';";
  html += "data.forEach((d,i)=>{";
  html += "const x = i * stepX;";
  html += "const y = h - (d.temp / maxTemp * h);";
  html += "if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);";
  html += "}); ctx.stroke();";

  html += "ctx.beginPath(); ctx.strokeStyle='blue';";
  html += "data.forEach((d,i)=>{";
  html += "const x = i * stepX;";
  html += "const y = h - (d.hum / maxHum * h);";
  html += "if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);";
  html += "}); ctx.stroke();";

  html += "});";
  html += "</script>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  Wire.begin(SDA_I2C, SCL_I2C);

  // LCD
  lcd.begin(20, 4);
  lcd.setBacklight(255);
  lcd.createChar(0, simboloGrau);
  lcd.clear();

  // Temperatura
  sensorTemp.begin();

  // BH1750
  bool luzOK =
      sensorLuz.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);

  lcd.setCursor(0, 0);
  lcd.print("A iniciar sistema");

  lcd.setCursor(0, 1);

  if (luzOK) {
    lcd.print("BH1750: OK");
    Serial.println("BH1750 iniciado.");
  } else {
    lcd.print("BH1750: ERRO");
    Serial.println("Erro BH1750.");
  }

  lcd.setCursor(0, 2);
  lcd.print("DS18B20: OK");

  lcd.setCursor(0, 3);
  lcd.print("Humidade: OK");

  // Configurar ESP32 como Ponto de Acesso
  Serial.print("A criar rede Wi-Fi... ");
  WiFi.softAP(ssid, password);
  Serial.println(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("Endereço IP do Servidor: ");
  Serial.println(IP);

  // Configurar rotas do Servidor Web
  server.on("/", enviarPaginaWeb);
  server.on("/history", enviarHistoricoJSON);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");

  delay(2500);

  lcd.clear();

  lerSensores();
  gravarHistorico(); // Grava primeiro registo
  ultimoTempoHistorico = millis();

  mostrarPagina();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  server.handleClient(); // Processa os pedidos do navegador web

  unsigned long agora = millis();

  // Atualizar sensores
  if (agora - ultimoTempoLeitura >= INTERVALO_LEITURA) {

    ultimoTempoLeitura = agora;

    lerSensores();

    mostrarPagina();

    mostrarSerial();
  }

  // Atualizar histórico
  if (agora - ultimoTempoHistorico >= INTERVALO_HISTORICO) {
    ultimoTempoHistorico = agora;
    gravarHistorico();
  }

  // Trocar página
  if (agora - ultimoTempoPagina >= INTERVALO_PAGINA) {

    ultimoTempoPagina = agora;

    paginaAtual++;

    // Adiciona uma página extra para mostrar o IP e estado do Wi-Fi
    if (paginaAtual > 3)
      paginaAtual = 0;

    mostrarPagina();
  }

  delay(50);
}

// =====================================================
// LEITURA SENSORES
// =====================================================
void lerSensores() {

  lerTemperatura();
  lerLuminosidade();
  lerHumidade();
}

// -----------------------------------------------------
void lerTemperatura() {

  sensorTemp.requestTemperatures();

  float temp = sensorTemp.getTempCByIndex(0);

  if (temp != DEVICE_DISCONNECTED_C &&
      temp > -55 &&
      temp < 125) {

    temperaturaC = temp;
  }
}

// -----------------------------------------------------
void lerLuminosidade() {

  float lux = sensorLuz.readLightLevel();

  if (lux >= 0) {
    luminosidadeLux = lux;
  }
}

// -----------------------------------------------------
void lerHumidade() {

  humidadeRaw = analogRead(PINO_HUMIDADE);

  // Ajusta conforme o sensor
  humidadePercent = map(humidadeRaw, 4095, 1200, 0, 100);

  if (humidadePercent < 0)
    humidadePercent = 0;

  if (humidadePercent > 100)
    humidadePercent = 100;
}

// =====================================================
// LCD
// =====================================================
void mostrarPagina() {

  lcd.clear();

  switch (paginaAtual) {

    case 0:
      mostrarTemperaturaLCD();
      break;

    case 1:
      mostrarLuzLCD();
      break;

    case 2:
      mostrarHumidadeLCD();
      break;

    case 3:
      mostrarRedeLCD();
      break;
  }
}

// -----------------------------------------------------
void mostrarTemperaturaLCD() {

  lcd.setCursor(0, 0);
  lcd.print("=== TEMPERATURA ===");

  lcd.setCursor(0, 1);
  lcd.print("Valor: ");
  lcd.print(temperaturaC, 1);
  lcd.write((uint8_t)0);
  lcd.print("C");

  lcd.setCursor(0, 2);
  lcd.print("Estado:");
  lcd.setCursor(0, 3);
  lcd.print(classificarTemperatura());
}

// -----------------------------------------------------
void mostrarLuzLCD() {

  lcd.setCursor(0, 0);
  lcd.print("=== LUMINOSIDADE ==");

  lcd.setCursor(0, 1);
  lcd.print("Valor:");

  if (luminosidadeLux < 1000) {
    lcd.print(luminosidadeLux, 0);
    lcd.print(" lux");
  } else {
    lcd.print(luminosidadeLux / 1000.0, 1);
    lcd.print("k lux");
  }

  lcd.setCursor(0, 2);
  lcd.print("Nivel:");

  lcd.setCursor(0, 3);
  lcd.print(classificarLuz());
}

// -----------------------------------------------------
void mostrarHumidadeLCD() {

  lcd.setCursor(0, 0);
  lcd.print("==== HUMIDADE ====");

  lcd.setCursor(0, 1);
  lcd.print("Raw: ");
  lcd.print(humidadeRaw);

  lcd.setCursor(0, 2);
  lcd.print("Humidade: ");
  lcd.print(humidadePercent);
  lcd.print("%");

  lcd.setCursor(0, 3);
  lcd.print(classificarHumidade());
}

// -----------------------------------------------------
void mostrarRedeLCD() {
  lcd.setCursor(0, 0);
  lcd.print("=== REDE WI-FI ===");

  lcd.setCursor(0, 1);
  lcd.print("SSID: ");
  lcd.print(ssid);

  lcd.setCursor(0, 2);
  lcd.print("IP: ");
  lcd.print(WiFi.softAPIP().toString());

  lcd.setCursor(0, 3);
  lcd.print("Ligacoes: ");
  lcd.print(WiFi.softAPgetStationNum());
}

// =====================================================
// CLASSIFICAÇÕES
// =====================================================
const char* classificarTemperatura() {

  if (temperaturaC < 0)
    return "GELO";

  if (temperaturaC < 10)
    return "FRIO";

  if (temperaturaC < 18)
    return "AMENO";

  if (temperaturaC < 25)
    return "CONFORTAVEL";

  if (temperaturaC < 32)
    return "QUENTE";

  if (temperaturaC < 40)
    return "MTO QUENTE";

  return "PERIGO";
}

// -----------------------------------------------------
const char* classificarLuz() {

  if (luminosidadeLux < 1)
    return "ESCURO";

  if (luminosidadeLux < 10)
    return "POUCA LUZ";

  if (luminosidadeLux < 50)
    return "SUAVE";

  if (luminosidadeLux < 200)
    return "NORMAL";

  if (luminosidadeLux < 1000)
    return "BRILHANTE";

  return "SOL DIRETO";
}

// -----------------------------------------------------
const char* classificarHumidade() {

  if (humidadePercent < 20)
    return "Solo muito seco";

  if (humidadePercent < 40)
    return "Solo seco";

  if (humidadePercent < 70)
    return "Solo ideal";

  if (humidadePercent < 90)
    return "Solo humido";

  return "Muito molhado";
}

// =====================================================
// SERIAL
// =====================================================
void mostrarSerial() {

  Serial.println();
  Serial.println("========== LEITURAS ==========");

  Serial.print("Temperatura: ");
  Serial.print(temperaturaC, 1);
  Serial.println(" C");

  Serial.print("Luminosidade: ");
  Serial.print(luminosidadeLux, 1);
  Serial.println(" lux");

  Serial.print("Humidade RAW: ");
  Serial.println(humidadeRaw);

  Serial.print("Humidade: ");
  Serial.print(humidadePercent);
  Serial.println("%");

  Serial.println("==============================");
}
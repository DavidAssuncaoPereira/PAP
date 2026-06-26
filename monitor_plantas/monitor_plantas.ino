#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <EEPROM.h>


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
// EEPROM
// =====================================================
#define EEPROM_SIZE 512
#define EEPROM_MAGIC_NUMBER 0x1A2B3C4D

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
void lerHistoricoEEPROM() {
  uint32_t magic;
  EEPROM.get(0, magic);

  if (magic == EEPROM_MAGIC_NUMBER) {
    int addr = sizeof(magic);
    EEPROM.get(addr, historicoCount);
    addr += sizeof(historicoCount);
    EEPROM.get(addr, historicoIndex);
    addr += sizeof(historicoIndex);
    EEPROM.get(addr, historico);
    Serial.println("Historico carregado da EEPROM.");
  } else {
    Serial.println("EEPROM vazia ou invalida. Inicializando...");
  }
}

void gravarHistoricoEEPROM() {
  uint32_t magic = EEPROM_MAGIC_NUMBER;
  int addr = 0;
  EEPROM.put(addr, magic);
  addr += sizeof(magic);
  EEPROM.put(addr, historicoCount);
  addr += sizeof(historicoCount);
  EEPROM.put(addr, historicoIndex);
  addr += sizeof(historicoIndex);
  EEPROM.put(addr, historico);
  EEPROM.commit();
}

void gravarHistorico() {
  historico[historicoIndex].temp = temperaturaC;
  historico[historicoIndex].lux = luminosidadeLux;
  historico[historicoIndex].hum = humidadePercent;

  historicoIndex = (historicoIndex + 1) % MAX_HISTORY;
  if (historicoCount < MAX_HISTORY) {
    historicoCount++;
  }

  gravarHistoricoEEPROM();
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
  html += "<head><link rel='icon' type='image/png' href='data:image/png;base64,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'><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Monitor de Plantas</title>";
  html += "<style>body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; text-align: center; background-color: #2b2b2b; padding: 20px; color: #f0f0f0; }";
  html += "h1 { color: #4CAF50; font-weight: 600; margin-bottom: 30px; }";
  html += ".container { display: flex; flex-direction: column; align-items: center; gap: 15px; max-width: 600px; margin: 0 auto; }";
  html += ".dado { font-size: 20px; width: 100%; box-sizing: border-box; padding: 20px; background: #3c3f41; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.2); transition: transform 0.2s ease, box-shadow 0.2s ease; border-left: 5px solid #4CAF50; }";
  html += ".dado:hover { transform: translateY(-3px); box-shadow: 0 8px 15px rgba(0,0,0,0.3); }";
  html += ".dado strong { color: #ffffff; font-size: 26px; display: block; margin-top: 5px; }";
  html += "canvas { background: #3c3f41; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.2); margin-top: 30px; max-width: 100%; }</style>";
  html += "<meta http-equiv='refresh' content='10'>"; // Atualiza a página a cada 10 segundos
  html += "</head><body>";
  html += "<h1>Estação de Monitorização</h1>";
  html += "<div class='container'>";
  html += "<div class='dado' style='border-left-color: #ff5555;'>Temperatura <strong>" + String(temperaturaC, 1) + " °C</strong></div>";
  html += "<div class='dado' style='border-left-color: #f39c12;'>Luminosidade <strong>" + String(luminosidadeLux, 1) + " lx</strong></div>";
  html += "<div class='dado' style='border-left-color: #55aaff;'>Humidade do Solo <strong>" + String(humidadePercent) + " %</strong></div>";

  html += "<canvas id='chart' width='400' height='200'></canvas>";
  html += "</div>";
  html += "<script>";
  html += "fetch('/history').then(r=>r.json()).then(data=>{";
  html += "if(data.length===0) return;";
  html += "const ctx = document.getElementById('chart').getContext('2d');";
  html += "const w = 400; const h = 200;";
  html += "const maxTemp = Math.max(...data.map(d=>d.temp), 40);";
  html += "const maxHum = 100;";
  html += "ctx.clearRect(0,0,w,h);";
  html += "ctx.lineWidth=3;";
  html += "const stepX = w / Math.max(data.length - 1, 1);";

  html += "ctx.strokeStyle='rgba(255, 255, 255, 0.1)'; ctx.beginPath();";
  html += "for(let i=0; i<=h; i+=50) { ctx.moveTo(0, i); ctx.lineTo(w, i); }";
  html += "ctx.stroke();";

  html += "ctx.beginPath(); ctx.strokeStyle='#ff5555';";
  html += "data.forEach((d,i)=>{";
  html += "const x = i * stepX;";
  html += "const y = h - (d.temp / maxTemp * h);";
  html += "if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);";
  html += "}); ctx.stroke();";

  html += "ctx.beginPath(); ctx.strokeStyle='#55aaff';";
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


  // EEPROM
  EEPROM.begin(EEPROM_SIZE);
  lerHistoricoEEPROM();

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
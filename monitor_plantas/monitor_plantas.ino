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
#define PINO_BOMBA       16

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
#define EEPROM_SIZE 2048
#define EEPROM_MAGIC_NUMBER 0x1A2B3C4E

// =====================================================
// VARIÁVEIS
// =====================================================
float temperaturaC = 0.0;
float luminosidadeLux = 0.0;

int humidadeRaw = 0;
int humidadePercent = 0;

int limiteBombaHumidade = 30;
int limiteBombaTemperatura = 30;
bool estadoBomba = false;

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
    addr += sizeof(historico);
    EEPROM.get(addr, limiteBombaHumidade);
    addr += sizeof(limiteBombaHumidade);
    EEPROM.get(addr, limiteBombaTemperatura);

    // Validar caso o valor na EEPROM seja inválido (ex: lixo após update firmware)
    if (limiteBombaHumidade < 0 || limiteBombaHumidade > 100) {
      limiteBombaHumidade = 30;
    }
    if (limiteBombaTemperatura < 0 || limiteBombaTemperatura > 100) {
      limiteBombaTemperatura = 30;
    }

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
  addr += sizeof(historico);
  EEPROM.put(addr, limiteBombaHumidade);
  addr += sizeof(limiteBombaHumidade);
  EEPROM.put(addr, limiteBombaTemperatura);
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
  html += "<head><meta charset='UTF-8'><link rel='icon' type='image/png' href='data:image/png;base64,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'>";
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
  html += "<div class='dado' style='border-left-color: #ff5555;'>Temperatura: <strong>" + String(temperaturaC, 1) + " °C</strong></div>";
  html += "<div class='dado' style='border-left-color: #f39c12;'>Luminosidade: <strong>" + String(luminosidadeLux, 1) + " lx</strong></div>";
  html += "<div class='dado' style='border-left-color: #55aaff;'>Humidade: <strong>" + String(humidadePercent) + " %</strong></div>";

  String estadoBombaStr = estadoBomba ? "<span style='color:#4CAF50;'>LIGADA <span style='display:inline-block;width:12px;height:12px;border-radius:50%;background-color:#4CAF50;box-shadow:0 0 8px #4CAF50;'></span></span>" : "<span style='color:#ff5555;'>DESLIGADA <span style='display:inline-block;width:12px;height:12px;border-radius:50%;background-color:#555;'></span></span>";
  html += "<div class='dado' style='border-left-color: #9b59b6;'>Bomba de Água <strong>" + estadoBombaStr + "</strong><br>";
  html += "<form action='/config' method='GET' style='margin-top: 10px; font-size: 16px; display: flex; flex-direction: column; align-items: center; gap: 10px;'>";
  html += "<div><label for='limite'>Ligar quando humidade &lt; </label>";
  html += "<input type='number' id='limite' name='limite' min='0' max='100' value='" + String(limiteBombaHumidade) + "' style='width: 60px; padding: 5px; border-radius: 5px; border: 1px solid #ccc; background-color: #2b2b2b; color: white;'> % </div>";
  html += "<div><label for='limite_temp'>Ou ligar quando temp &gt; </label>";
  html += "<input type='number' id='limite_temp' name='limite_temp' min='0' max='100' value='" + String(limiteBombaTemperatura) + "' style='width: 60px; padding: 5px; border-radius: 5px; border: 1px solid #ccc; background-color: #2b2b2b; color: white;'> °C </div>";
  html += "<input type='submit' value='Guardar' style='padding: 5px 10px; border-radius: 5px; border: none; background-color: #4CAF50; color: white; cursor: pointer;'>";
  html += "</form></div>";

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

// -----------------------------------------------------
void processarConfigBomba() {
  bool updated = false;
  if (server.hasArg("limite")) {
    int novoLimite = server.arg("limite").toInt();
    if (novoLimite >= 0 && novoLimite <= 100) {
      limiteBombaHumidade = novoLimite;
      updated = true;
    }
  }
  if (server.hasArg("limite_temp")) {
    int novoLimiteTemp = server.arg("limite_temp").toInt();
    if (novoLimiteTemp >= 0 && novoLimiteTemp <= 100) {
      limiteBombaTemperatura = novoLimiteTemp;
      updated = true;
    }
  }
  if (updated) {
    gravarHistoricoEEPROM(); // Salvar os novos limites
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Redirecionando...");
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

  // Bomba
  pinMode(PINO_BOMBA, OUTPUT);
  digitalWrite(PINO_BOMBA, LOW);

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
  server.on("/config", processarConfigBomba);
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
  atualizarBomba();
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

// -----------------------------------------------------
void atualizarBomba() {
  if (humidadePercent < limiteBombaHumidade || temperaturaC > limiteBombaTemperatura) {
    digitalWrite(PINO_BOMBA, HIGH);
    estadoBomba = true;
  } else {
    digitalWrite(PINO_BOMBA, LOW);
    estadoBomba = false;
  }
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
  lcd.print("Bmba:");
  lcd.print(estadoBomba ? "ON " : "OFF");
  lcd.print(" LH:");
  lcd.print(limiteBombaHumidade);
  lcd.print(" LT:");
  lcd.print(limiteBombaTemperatura);
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
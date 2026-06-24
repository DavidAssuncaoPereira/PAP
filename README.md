# PAP - Estação de Monitorização de Plantas

Projeto de Aptidão Profissional focado na monitorização das condições de plantas utilizando um microcontrolador ESP32 e múltiplas interfaces de utilizador (Desktop e Mobile). O sistema regista temperatura, luminosidade e a humidade do solo, apresentando estes dados tanto num ecrã LCD local, como via interface Web em tempo real.

---

## Como Tudo Funciona

O projeto é constituído por três componentes principais que trabalham em conjunto:

1. **ESP32 (Backend / Hardware):** Lê dados dos sensores, apresenta num ecrã LCD e atua como Ponto de Acesso (Access Point) Wi-Fi (SSID: `ESP32-PLANTAS`, Senha: `12345678`). Ele aloja um servidor web simples com uma página HTML com as informações e um endpoint JSON para histórico.
2. **Desktop App:** Uma aplicação em Python (usando CustomTkinter, Matplotlib e Plyer) que se liga à rede do ESP32, lê a página HTML gerada ou dados JSON para exibir de forma intuitiva. Contém um ecrã de login de arranque.
3. **Mobile App:** Aplicação nativa para Android escrita em Kotlin (usando Jetpack Compose). Desempenha um papel semelhante à aplicação desktop para leitura dos dados, e também conta com sistema de login na entrada e integração para notificações locais (como alertas de planta desidratada).

*Nota: Em ambas as aplicações (Desktop e Mobile) existe um ecrã de login que utiliza por predefinição a palavra-passe **`admin`**.*

---

## Estrutura do Projeto

- `monitor_plantas/` - Código em C++ para o microcontrolador ESP32.
- `desktop_app/` - Aplicação Desktop em Python.
- `mobile_app/` - Aplicação Mobile (Android) nativa desenvolvida em Kotlin / Jetpack Compose.

---

## Guia Passo a Passo

### 1. Configurar o ESP32 (`monitor_plantas/`)

**Dependências necessárias (Arduino IDE ou arduino-cli):**
`LiquidCrystal_PCF8574`, `OneWire`, `DallasTemperature`, `BH1750`

#### Para Windows
1. Instalar o Arduino IDE.
2. Nas preferências, adicionar o gestor de placas para ESP32 e instalar o core correspondente.
3. Instalar as bibliotecas referidas acima via Gestor de Bibliotecas.
4. Ligar o ESP32 ao computador por USB, e abrir o ficheiro `monitor_plantas/monitor_plantas.ino`.
5. Escolher a porta COM correta e o modelo (ex: "DOIT ESP32 DEVKIT V1") e clicar em "Upload".

#### Para Linux
Utilizando o `arduino-cli`:
1. Instalar `arduino-cli` (por exemplo: `curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh`) e adicionar ao `PATH`.
2. Configurar o core ESP32:
   ```bash
   arduino-cli config init
   arduino-cli core update-index --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
   arduino-cli core install esp32:esp32 --additional-urls https://dl.espressif.com/dl/package_esp32_index.json
   ```
3. Instalar bibliotecas:
   ```bash
   arduino-cli lib install "LiquidCrystal_PCF8574" "OneWire" "DallasTemperature" "BH1750"
   ```
4. Compilar e dar upload (substituir `<PORTA>` por ex. `/dev/ttyUSB0`):
   ```bash
   cd monitor_plantas
   arduino-cli compile --fqbn esp32:esp32:esp32 monitor_plantas.ino
   arduino-cli upload -p <PORTA> --fqbn esp32:esp32:esp32 monitor_plantas.ino
   ```

**Aviso:** Após o upload, o ESP32 iniciará a rede Wi-Fi `ESP32-PLANTAS` (Senha: `12345678`).

---

### 2. Aplicação Desktop (`desktop_app/`)

**Pré-requisitos:** Python 3 instalado no computador e ligação à rede Wi-Fi `ESP32-PLANTAS`.
**Senha de Login:** `admin`

#### Para Windows
1. Abrir a Linha de Comandos (CMD ou PowerShell).
2. Entrar na pasta do projeto:
   ```cmd
   cd desktop_app
   ```
3. Instalar as dependências:
   ```cmd
   pip install -r requirements.txt
   ```
4. Executar a aplicação:
   ```cmd
   python main.py
   ```

#### Para Linux
1. Abrir o terminal.
2. Entrar na pasta do projeto:
   ```bash
   cd desktop_app
   ```
3. Instalar as dependências:
   ```bash
   pip3 install -r requirements.txt
   ```
4. Executar a aplicação:
   ```bash
   python3 main.py
   ```

---

### 3. Aplicação Mobile (`mobile_app/`)

Aplicação nativa Android em Kotlin / Jetpack Compose. (Nota: esta aplicação não é em Flutter).

**Pré-requisitos:**
Ter o Java JDK e Android SDK instalados. O telemóvel (ou emulador) onde testar precisa estar ligado à rede Wi-Fi `ESP32-PLANTAS`.
**Senha de Login:** `admin`

#### Para Windows
1. Abrir a Linha de Comandos na pasta da app:
   ```cmd
   cd mobile_app
   ```
2. Compilar e instalar num telemóvel Android conectado por USB (com Depuração USB ativada):
   ```cmd
   gradlew.bat installDebug
   ```
   *Em alternativa, para apenas gerar o APK sem instalar:*
   ```cmd
   gradlew.bat assembleDebug
   ```
   O APK ficará guardado em `app\build\outputs\apk\debug\app-debug.apk`.

#### Para Linux
1. Abrir o terminal na pasta da app:
   ```bash
   cd mobile_app
   ```
2. Dar permissão de execução ao wrapper (se necessário):
   ```bash
   chmod +x gradlew
   ```
3. Compilar e instalar num telemóvel conectado por USB:
   ```bash
   ./gradlew installDebug
   ```
   *Em alternativa, para apenas gerar o APK:*
   ```bash
   ./gradlew assembleDebug
   ```
   O APK gerado ficará guardado em `app/build/outputs/apk/debug/app-debug.apk`.

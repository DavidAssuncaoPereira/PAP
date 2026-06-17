# PAP - Estação de Monitorização de Plantas

Projeto de Aptidão Profissional focado na monitorização das condições de plantas utilizando um microcontrolador ESP32 e múltiplas interfaces de utilizador (Desktop e Mobile). O sistema regista temperatura, luminosidade e a humidade do solo, apresentando estes dados tanto num ecrã LCD local, como via interface Web em tempo real.

---

## Estrutura do Projeto

O projeto está dividido em três componentes principais:

- `monitor_plantas/` - Código em C++ para o microcontrolador ESP32 (leitura dos sensores, atualização do LCD e servidor web).
- `desktop_app/` - Aplicação Desktop (Windows/Linux) desenvolvida em Python.
- `mobile_app/` - Aplicação Mobile (Android) desenvolvida em Flutter.

---

## Guia Passo a Passo

### 1. Configurar o ESP32 (`monitor_plantas/`)

O código para o ESP32 cria um ponto de acesso Wi-Fi próprio e inicia um servidor Web em `192.168.4.1`, além de atualizar fisicamente o ecrã LCD com dados sobre temperatura, luz e humidade.

**Como compilar e instalar no ESP32:**
1. Instala o [Arduino IDE](https://www.arduino.cc/en/software) ou usa o `arduino-cli`.
2. Adiciona o suporte para as placas ESP32 nas Preferências do Arduino IDE (usando o link do Boards Manager da Espressif).
3. Instala as seguintes bibliotecas através do Gestor de Bibliotecas:
   - `LiquidCrystal_PCF8574`
   - `OneWire`
   - `DallasTemperature`
   - `BH1750`
4. Liga o teu ESP32 ao computador.
5. Abre o ficheiro `monitor_plantas.ino` no Arduino IDE.
6. Seleciona a porta COM e o modelo da tua placa ESP32 (ex: "DOIT ESP32 DEVKIT V1").
7. Clica em **Carregar (Upload)**.
8. Uma vez carregado, o ESP32 vai criar a rede Wi-Fi `ESP32-PLANTAS` com a senha `12345678`.

---

### 2. Aplicação Desktop (`desktop_app/`)

Aplicação gráfica em Python (utilizando a biblioteca `customtkinter`) que comunica com o ESP32 e apresenta os dados de forma moderna.

**Como instalar e executar no Windows/Linux:**
1. Certifica-te de que tens o **Python 3** instalado no teu computador.
2. Abre o terminal (ou a Linha de Comandos) e navega até à pasta `desktop_app`:
   ```bash
   cd desktop_app
   ```
3. Instala as dependências necessárias:
   ```bash
   pip install -r requirements.txt
   ```
4. **Passo crítico:** Conecta o teu computador à rede Wi-Fi que o ESP32 acabou de criar (Nome: `ESP32-PLANTAS`, Senha: `12345678`).
5. Executa a aplicação:
   ```bash
   python main.py
   ```
6. A janela irá abrir e, a cada 5 segundos, atualizará a Temperatura, Luminosidade e Humidade do Solo lidas pelo ESP32.

---

### 3. Aplicação Mobile (`mobile_app/`)

Aplicação desenhada em Flutter que exporta um APK para Android, fornecendo um formato "Dashboard" rápido para ver o estado da tua planta.

**Como compilar e executar no Android:**
1. Instala o [Flutter SDK](https://docs.flutter.dev/get-started/install) e o Android Studio.
2. Abre um terminal e navega para a pasta `mobile_app`:
   ```bash
   cd mobile_app
   ```
3. Instala as dependências do Dart:
   ```bash
   flutter pub get
   ```
4. Conecta um dispositivo Android ao teu PC via USB (com o modo Depuração USB ativado).
5. **Passo crítico:** Liga o telemóvel à rede Wi-Fi `ESP32-PLANTAS` (Senha: `12345678`).
6. Para testar e correr a aplicação no telemóvel:
   ```bash
   flutter run
   ```
7. Para criares o ficheiro `.apk` instalável para partilhar com outras pessoas:
   ```bash
   flutter build apk
   ```
8. O ficheiro APK final estará localizado em: `build/app/outputs/flutter-apk/app-release.apk`.

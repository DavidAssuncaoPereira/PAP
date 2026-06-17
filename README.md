# PAP - Estação de Monitorização de Plantas

Projeto de Aptidão Profissional focado na monitorização das condições de plantas utilizando um microcontrolador ESP32 e múltiplas interfaces de utilizador (Desktop e Mobile).

## Estrutura do Projeto

O projeto está dividido em três componentes principais:

- `monitor_plantas/` - Código C++ para o microcontrolador ESP32 (servidor web e leitura de sensores).
- `desktop_app/` - Aplicação Desktop (Windows/Linux) desenvolvida em Python.
- `mobile_app/` - Aplicação Mobile (Android) desenvolvida em Flutter.

## 1. Código do ESP32 (`monitor_plantas/`)

O código para o ESP32 lê um sensor de temperatura (DS18B20) e um sensor de luminosidade (BH1750), apresentando os valores num LCD I2C e num servidor Web (na rede `ESP32-PLANTAS`, IP `192.168.4.1`).

### Como compilar:
Podes usar o Arduino IDE ou o `arduino-cli`:
1. Instala o suporte para ESP32 nas tuas placas.
2. Instala as bibliotecas: `LiquidCrystal_PCF8574`, `OneWire`, `DallasTemperature`, e `BH1750`.
3. Compila e envia o código (`monitor_plantas.ino`) para a tua placa ESP32.

---

## 2. Aplicação Desktop (`desktop_app/`)

Aplicação em Python (com `customtkinter`) que lê e apresenta os dados do ESP32.

### Como executar no Linux/Windows:

1. Instala o Python 3 (se não estiver instalado).
2. Abre o terminal/linha de comandos e navega até à pasta `desktop_app`.
3. Instala as dependências:
   ```bash
   pip install -r requirements.txt
   ```
4. Conecta o teu computador à rede Wi-Fi do ESP32 (`ESP32-PLANTAS`, senha: `12345678`).
5. Executa a aplicação:
   ```bash
   python main.py
   ```

---

## 3. Aplicação Mobile (`mobile_app/`)

Aplicação em Flutter desenhada para Android, mostrando uma "Dashboard" com a temperatura e luminosidade.

### Como executar/compilar (Android):

1. Instala o [Flutter SDK](https://docs.flutter.dev/get-started/install/linux) e o Android Studio (com o Android SDK configurado).
2. Abre um terminal e navega para a pasta `mobile_app`.
3. Instala as dependências do Dart:
   ```bash
   flutter pub get
   ```
4. Para correr a app num dispositivo Android ligado por USB ou num emulador:
   ```bash
   flutter run
   ```
5. Para criar um ficheiro APK (para instalar noutros dispositivos Android):
   ```bash
   flutter build apk
   ```
   O APK final ficará localizado em `build/app/outputs/flutter-apk/app-release.apk`.
6. Lembra-te de conectar o dispositivo móvel à rede Wi-Fi `ESP32-PLANTAS` para que a aplicação consiga obter os dados do ESP32.

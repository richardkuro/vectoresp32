# vectoresp32
An ESP32-based interactive desktop companion featuring physics-simulated OLED eyes, procedural audio, motion sensing, and a built-in web interface.
# ESP32 Desktop Companion Robot

An interactive, ESP32-based desktop companion robot that features physics-simulated OLED eyes, procedural sound generation, environmental awareness, and a built-in web control interface.

## 🌟 Features

* [cite_start]**Physics-Based UI**: Dynamic eye animations using a custom spring-damper physics engine for natural, fluid movements[cite: 5, 6, 62, 63, 64].
* [cite_start]**Procedural Audio Engine**: Generates unique chip-tune-style sounds without needing external audio files, including "whistle", "purr", "scream", "dizzy", and "snore" profiles[cite: 20, 21, 22, 24, 25, 27].
* [cite_start]**Web Control Panel**: A responsive web interface hosted directly on the ESP32 to control system volume, set alarms, trigger test animations, and switch modes[cite: 33, 34, 35, 36, 42, 47, 48].
* **Interactive Modes**:
  * [cite_start]**Idle/Companion**: Reacts to physical taps, tilts, and shakes using an MPU6050 accelerometer and gyroscope[cite: 89, 111, 113]. [cite_start]Gets "bored" over time, whistles randomly, and eventually displays sleeping "Zzz" animations[cite: 80, 81, 107, 119, 120].
  * [cite_start]**Pomodoro Timer**: A 25-minute focus mode where the robot dons a "Hachimaki" (Japanese headband) and displays a working hourglass[cite: 41, 71, 72, 74].
  * [cite_start]**Guard Mode**: Secures your space by sensing high G-force movement (> 12.0) to trigger a screaming alarm[cite: 10, 51, 103, 104].
* [cite_start]**Environmental Awareness**: Syncs with `pool.ntp.org` for accurate timekeeping and fetches local weather from the Open-Meteo API to react to temperature (e.g., shivering in the cold, sweating in the heat)[cite: 31, 32, 87].

## 🛠️ Hardware Requirements



* [cite_start]**Microcontroller**: ESP32[cite: 1].
* [cite_start]**Display**: 128x64 OLED Display (SSD1306)[cite: 1, 3].
* [cite_start]**Sensor**: MPU6050 (Accelerometer & Gyroscope)[cite: 1, 4].
* [cite_start]**Audio**: Passive Buzzer[cite: 3, 13].
* [cite_start]**Indicators**: Red LED, Blue LED[cite: 3, 86].
* [cite_start]**Input**: Push Button (for Snooze/Interaction)[cite: 3, 86].
* [cite_start]**Power**: Battery setup with a voltage divider (22k + 22k) for monitoring[cite: 3].

## 📌 Pin Configuration

| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **I2C SDA** | `21` | [cite_start]Connects to OLED and MPU6050[cite: 87]. |
| **I2C SCL** | `22` | [cite_start]Connects to OLED and MPU6050[cite: 87]. |
| **Buzzer** | `25` | [cite_start]Uses DAC/ledc PWM[cite: 3, 13, 87]. |
| **Red LED** | `4` | [cite_start]General Output[cite: 3, 86]. |
| **Blue LED** | `12` | [cite_start]General Output[cite: 3, 86]. |
| **Snooze Button** | `15` | [cite_start]Pulled to Ground (`INPUT_PULLUP`)[cite: 3, 86]. |
| **Battery Monitor** | `35` | [cite_start]ADC Pin for voltage divider[cite: 3, 59, 86]. |
| **Bluetooth Power** | `27` | [cite_start]General Output control[cite: 3, 86]. |

## 📦 Software Dependencies

[cite_start]Ensure the following libraries are installed in your Arduino IDE[cite: 1]:
* `WiFi.h`, `WebServer.h`, `HTTPClient.h`, `Wire.h` (Built-in for ESP32)
* `ArduinoJson` (For parsing Open-Meteo API data)
* `Adafruit_GFX` & `Adafruit_SSD1306` (For OLED rendering)
* `Adafruit_MPU6050` & `Adafruit_Sensor` (For motion processing)

## 🚀 Setup & Installation

1. [cite_start]**Network Configuration**: Open the `.ino` sketch and update the user configuration section with your WiFi credentials[cite: 1, 2]:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";

Location Configuration: Update the lat and lon variables with your local coordinates to ensure accurate weather reactions:
+1

C++

String lat = "27.1329";
String lon = "93.7465";

Compile and Upload: Select your ESP32 board in the Arduino IDE and upload the code.


Access the Web UI: Open the Serial Monitor at 115200 baud to verify the connection. Find the ESP32's IP address on your network and enter it into a web browser to access the control panel.

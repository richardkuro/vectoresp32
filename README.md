# vectoresp32
An ESP32-based interactive desktop companion featuring physics-simulated OLED eyes, procedural audio, motion sensing, and a built-in web interface.
# ESP32 Desktop Companion Robot

An interactive, ESP32-based desktop companion robot that features physics-simulated OLED eyes, procedural sound generation, environmental awareness, and a built-in web control interface.

## 🌟 Features

* **Physics-Based UI**: Dynamic eye animations using a custom spring-damper physics engine for natural, fluid movements.
* **Procedural Audio Engine**: Generates unique chip-tune-style sounds without needing external audio files, including "whistle", "purr", "scream", "dizzy", and "snore" profiles.
* **Web Control Panel**: A responsive web interface hosted directly on the ESP32 to control system volume, set alarms, trigger test animations, and switch modes.
* **Interactive Modes**:
  * **Idle/Companion**: Reacts to physical taps, tilts, and shakes using an MPU6050 accelerometer and gyroscope. Gets "bored" over time, whistles randomly, and eventually displays sleeping "Zzz" animations.
  * **Pomodoro Timer**: A 25-minute focus mode where the robot dons a "Hachimaki" (Japanese headband) and displays a working hourglass.
  * **Guard Mode**: Secures your space by sensing high G-force movement (> 12.0) to trigger a screaming alarm.
* **Environmental Awareness**: Syncs with `pool.ntp.org` for accurate timekeeping and fetches local weather from the Open-Meteo API to react to temperature (e.g., shivering in the cold, sweating in the heat).

## 🛠️ Hardware Requirements



* **Microcontroller**: ESP32.
* **Display**: 128x64 OLED Display (SSD1306).
* **Sensor**: MPU6050 (Accelerometer & Gyroscope).
* **Audio**: Passive Buzzer.
* **Indicators**: Red LED, Blue LED.
* **Input**: Push Button (for Snooze/Interaction).
* **Power**: Battery setup with a voltage divider (22k + 22k) for monitoring.

## 📌 Pin Configuration

| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **I2C SDA** | `21` | Connects to OLED and MPU6050. |
| **I2C SCL** | `22` | Connects to OLED and MPU6050. |
| **Buzzer** | `25` | Uses DAC/ledc PWM. |
| **Red LED** | `4` | General Output. |
| **Blue LED** | `12` | General Output. |
| **Snooze Button** | `15` | Pulled to Ground (`INPUT_PULLUP`). |
| **Battery Monitor** | `35` | ADC Pin for voltage divider. |
| **Bluetooth Power** | `27` | General Output control. |

## 📦 Software Dependencies

Ensure the following libraries are installed in your Arduino IDE:
* `WiFi.h`, `WebServer.h`, `HTTPClient.h`, `Wire.h` (Built-in for ESP32)
* `ArduinoJson` (For parsing Open-Meteo API data)
* `Adafruit_GFX` & `Adafruit_SSD1306` (For OLED rendering)
* `Adafruit_MPU6050` & `Adafruit_Sensor` (For motion processing)

## 🚀 Setup & Installation

1. **Network Configuration**: Open the `.ino` sketch and update the user configuration section with your WiFi credentials:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";

Location Configuration: Update the lat and lon variables with your local coordinates to ensure accurate weather reactions:
```cpp
String lat = "27.1329";
String lon = "93.7465";
```
Compile and Upload: Select your ESP32 board in the Arduino IDE and upload the code.


Access the Web UI: Open the Serial Monitor at 115200 baud to verify the connection. Find the ESP32's IP address on your network and enter it into a web browser to access the control panel.

## 🎬 Preview
![ezgif-450e643db8f25474](https://github.com/user-attachments/assets/e6ea5899-ee20-4cc3-93df-94f5637ff266)


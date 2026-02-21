#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <time.h>
#include <vector>

// ================= USER CONFIGURATION =================
const char* ssid     = "testwifi";
const char* password = "12345678";
String lat = "27.1329";
String lon = "93.7465";

// --- Pins ---
#define PIN_BT_POWER   27
#define PIN_RED_LED    4    
#define PIN_BLUE_LED   12
#define PIN_SNOOZE     15  // Button to Ground
#define PIN_BUZZER     25  // DAC Pin
#define PIN_BATTERY    35  // 22k + 22k Divider

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ================= GLOBALS =================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;
WebServer server(80);

// --- State Timers ---
unsigned long reactionTimer = 0;
unsigned long uiTimer = 0;
unsigned long lastInteract = 0;

// --- Physics Engine ---
struct Eye { float pos; float vel; float target; float k; float d; };
Eye eyeX = {0, 0, 0, 0.35, 0.5}; 
Eye eyeY = {0, 0, 0, 0.35, 0.5};
Eye eyeH = {30, 0, 30, 0.40, 0.4}; 

// Modifiers
float angryAngle = 0; 
float pupilOffsetL = 0, pupilOffsetR = 0; 
String eyeOverlay = ""; 
bool isShivering = false;

// --- Logic Data ---
// Alarm Global Variables
int alarmH = -1;
int alarmM = -1;
bool alarmActive = false;

enum Mode { IDLE, POMODORO, GUARD, ALARM };
volatile Mode currentMode = IDLE;

volatile int batteryPercent = 0;
volatile bool btPowerState = false;
volatile bool is12h = true;
volatile int systemVolume = 60; // 0 to 100
String weatherStatus = "cozy"; 

unsigned long pomodoroStartTime = 0;
unsigned long pomodoroDuration = 25 * 60 * 1000;
unsigned long nextBlink = 3000;

const long gmtOffset_sec = 19800; 
const int daylightOffset_sec = 0;

// ================= AUDIO ENGINE =================

// Added volumeScalar to allow specific sounds (like snore) to be quieter
void playTone(int freq, int duration, float volScalar = 1.0) {
  if (systemVolume == 0) {
    ledcWriteTone(PIN_BUZZER, 0);
    return;
  }

  if (freq == 0) {
    ledcWriteTone(PIN_BUZZER, 0);
  } else {
    ledcWriteTone(PIN_BUZZER, freq);
    // Strict volume mapping
    int baseDuty = map(systemVolume, 0, 100, 0, 255);
    // Apply scalar (e.g., 0.3 for snores)
    int finalDuty = (int)(baseDuty * volScalar);
    if(finalDuty < 1 && systemVolume > 0) finalDuty = 1; // Ensure at least some sound if vol > 0
    
    ledcWrite(PIN_BUZZER, finalDuty); 
  }

  if (duration > 0) {
    delay(duration);
    ledcWriteTone(PIN_BUZZER, 0);
  }
}

void chirp(String type) {
  if (systemVolume == 0) return;

  if (type == "wow") { 
    for(int f=800; f<1600; f+=100) { playTone(f, 5); } 
    playTone(2000, 60); 
  } 
  else if (type == "whistle") { 
    // R2-D2 Style Randomizer
    int len = random(4, 8);
    for(int i=0; i<len; i++) {
        int f = random(1000, 3000);
        int d = random(50, 150);
        playTone(f, d);
        delay(random(10, 50));
    }
  }
  else if (type == "scream") { 
    for(int k=0; k<2; k++) {
      for(int f=2000; f>1000; f-=300) { playTone(f, 5); } 
    }
  } 
  else if (type == "purr") {
    for(int i=0; i<6; i++) { playTone(60, 40); delay(30); }
  } 
  else if (type == "huh") { 
      playTone(600, 80); delay(50); playTone(900, 120);
  }
  else if (type == "snore") {
    // QUIETER SNORE: scaler 0.2
    for(int f=100; f>50; f-=5) { playTone(f, 15, 0.2); }
  }
  else if (type == "dizzy") {
    for(int i=0; i<3; i++) { playTone(1200, 30); playTone(800, 30); } 
  } 
  else if (type == "click") {
    playTone(2500, 8);
  }
}

// ================= CORE 0: WEB & NETWORK =================
void fetchWeather() {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://api.open-meteo.com/v1/forecast?latitude="+lat+"&longitude="+lon+"&current_weather=true");
    if(http.GET() > 0) {
      DynamicJsonDocument doc(1024); deserializeJson(doc, http.getString());
      float t = doc["current_weather"]["temperature"];
      int c = doc["current_weather"]["weathercode"];
      if(t > 28) weatherStatus="sweat";
      else if(t < 15) weatherStatus="freeze";
      else if(c>=51) weatherStatus="wet";
      else weatherStatus="cozy";
    }
    http.end();
  }
}

String getHTML() {
  String p = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
  p += "body{background:#111;color:#0f0;font-family:monospace;text-align:center}.btn{padding:10px;margin:5px;border:none;border-radius:5px;width:100px;font-weight:bold;cursor:pointer}";
  p += ".red{background:#f00;color:#fff}.grn{background:#0f0;color:#000}.blu{background:#00f;color:#fff}.org{background:#FFA500;color:#000} ul{list-style:none;padding:0} li{padding:5px;background:#222;margin:2px}";
  p += "input[type=range] {width: 80%;} input[type=number]{width:50px;padding:5px;}"; 
  p += "</style></head><body><h1>ROBOT CONTROL</h1>";
  
  p += "<h3>VOLUME: " + String(systemVolume) + "%</h3>";
  p += "<form action='/vol' method='get' oninput='this.submit()'><input type='range' name='v' min='0' max='100' value='" + String(systemVolume) + "' onchange='this.form.submit()'></form>";

  // --- ALARM SECTION ADDED ---
  p += "<h3>ALARM SYSTEM</h3>";
  if(alarmActive) {
      p += "<p style='color:yellow'>ALARM SET FOR " + String(alarmH) + ":" + (alarmM < 10 ? "0" : "") + String(alarmM) + "</p>";
      p += "<a href='/alarm_off'><button class='btn red'>DISABLE ALARM</button></a>";
  } else {
      p += "<p>ALARM OFF</p>";
  }
  p += "<form action='/alarm_set' method='get'>";
  p += "HH: <input type='number' name='h' min='0' max='23'> MM: <input type='number' name='m' min='0' max='59'>";
  p += "<br><br><input type='submit' value='SET ALARM' class='btn org'></form>";
  // ---------------------------

  p += "<h3>MODES</h3>";
  p += "<form action='/pomo'><input type='number' name='t' value='25'> min <input type='submit' value='FOCUS' class='btn blu'></form>";
  p += "<a href='/stop_pomo'><button class='btn red'>STOP FOCUS</button></a>";
  p += "<br><a href='/guard'><button class='btn red'>GUARD</button></a>";
  p += "<a href='/safe'><button class='btn grn'>SAFE MODE</button></a>";

  p += "<h3>AUDIO TEST</h3>";
  p += "<a href='/test/scream'><button class='btn red'>SCREAM</button></a>";
  p += "<a href='/test/whistle'><button class='btn blu'>WHISTLE</button></a>";
  p += "<a href='/test/purr'><button class='btn grn'>PURR</button></a>";
  p += "<a href='/test/wow'><button class='btn org'>WOW</button></a>";
  p += "<a href='/test/huh'><button class='btn blu'>HUH?</button></a>";
  p += "<a href='/test/snore'><button class='btn grn'>SNORE</button></a>";
  
  p += "<h3>SETTINGS</h3><a href='/mode'><button class='btn blu'>" + String(is12h ? "MODE: 12H" : "MODE: 24H") + "</button></a>";
  p += "<a href='/bt?s=" + String(!btPowerState) + "'><button class='btn grn'>BT: " + String(btPowerState?"ON":"OFF") + "</button></a>";
  
  return p + "</body></html>";
}

void taskLogic(void * p) {
  server.on("/", [](){ server.send(200, "text/html", getHTML()); });
  server.on("/mode", [](){ is12h=!is12h; server.sendHeader("Location","/"); server.send(303); });
  server.on("/bt", [](){ btPowerState=!btPowerState; digitalWrite(PIN_BT_POWER, btPowerState); server.sendHeader("Location","/"); server.send(303); });
  
  server.on("/vol", [](){ if(server.hasArg("v")) { systemVolume = server.arg("v").toInt(); chirp("click"); } server.sendHeader("Location","/"); server.send(303); });

  // --- ALARM ROUTES ---
  server.on("/alarm_set", [](){ 
      if(server.hasArg("h") && server.hasArg("m")) {
          alarmH = server.arg("h").toInt();
          alarmM = server.arg("m").toInt();
          alarmActive = true;
          chirp("click");
      }
      server.sendHeader("Location","/"); server.send(303); 
  });
  server.on("/alarm_off", [](){ alarmActive = false; chirp("click"); server.sendHeader("Location","/"); server.send(303); });
  // --------------------

  server.on("/pomo", [](){ pomodoroDuration=server.arg("t").toInt()*60000; pomodoroStartTime=millis(); currentMode=POMODORO; chirp("click"); server.sendHeader("Location","/"); server.send(303); });
  server.on("/stop_pomo", [](){ currentMode=IDLE; chirp("click"); server.sendHeader("Location","/"); server.send(303); });
  
  server.on("/guard", [](){ currentMode=GUARD; delay(2000); server.sendHeader("Location","/"); server.send(303); });
  server.on("/safe", [](){ currentMode=IDLE; lastInteract=millis(); chirp("click"); server.sendHeader("Location","/"); server.send(303); });
  
  // Reaction Routes
  server.on("/test/angry", [](){ angryAngle=20; eyeH.target=5; chirp("scream"); reactionTimer=millis()+3000; server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/dizzy", [](){ pupilOffsetL=-8; pupilOffsetR=8; eyeH.target=35; chirp("dizzy"); reactionTimer=millis()+3000; server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/sus", [](){ eyeH.target=5; eyeX.target=20; chirp("huh"); reactionTimer=millis()+2000; server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/happy", [](){ eyeH.target=15; eyeOverlay="sweat"; chirp("purr"); reactionTimer=millis()+3000; server.sendHeader("Location","/"); server.send(303); });
  
  // Audio Routes
  server.on("/test/scream", [](){ chirp("scream"); server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/whistle", [](){ chirp("whistle"); server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/purr", [](){ chirp("purr"); server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/wow", [](){ chirp("wow"); server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/huh", [](){ chirp("huh"); server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/snore", [](){ chirp("snore"); server.sendHeader("Location","/"); server.send(303); });
  
  server.on("/test/led/red", [](){ digitalWrite(PIN_RED_LED, HIGH); delay(500); digitalWrite(PIN_RED_LED, LOW); server.sendHeader("Location","/"); server.send(303); });
  server.on("/test/led/blue", [](){ digitalWrite(PIN_BLUE_LED, HIGH); delay(500); digitalWrite(PIN_BLUE_LED, LOW); server.sendHeader("Location","/"); server.send(303); });

  server.begin();
  unsigned long lastBat = 0, lastWeather = 0;

  for(;;) {
    server.handleClient();
    if(millis()-lastBat > 1000) {
      int adc = analogRead(PIN_BATTERY);
      batteryPercent = constrain(map(adc, 1850, 2350, 0, 100), 0, 100);
      lastBat = millis();
    }
    if(millis()-lastWeather > 900000) { fetchWeather(); lastWeather = millis(); }
    vTaskDelay(20); 
  }
}

// ================= CORE 1: PHYSICS & RENDERING =================

void updatePhysics() {
  eyeX.vel += (-eyeX.k * (eyeX.pos - eyeX.target) - eyeX.d * eyeX.vel); eyeX.pos += eyeX.vel;
  eyeY.vel += (-eyeY.k * (eyeY.pos - eyeY.target) - eyeY.d * eyeY.vel); eyeY.pos += eyeY.vel;
  eyeH.vel += (-eyeH.k * (eyeH.pos - eyeH.target) - eyeH.d * eyeH.vel); eyeH.pos += eyeH.vel;
}

void render() {
  display.clearDisplay();
  
  int cX = 64, cY = 32, gap = 26;
  if (isShivering) { cX += random(-1, 2); cY += random(-1, 2); }

  int lx = cX - gap + (int)eyeX.pos;
  int rx = cX + gap + (int)eyeX.pos;
  int y = cY + (int)eyeY.pos;
  int h = (int)eyeH.pos; if(h<2) h=2;
  int w = 28;

  // Draw Eyes
  display.fillRoundRect(lx-w/2, y-h/2+pupilOffsetL, w, h, 8, SSD1306_WHITE);
  display.fillRoundRect(rx-w/2, y-h/2+pupilOffsetR, w, h, 8, SSD1306_WHITE);

  if(angryAngle > 0) { 
      display.fillTriangle(lx-18, y-25, lx+18, y-25+angryAngle, lx-18, y-25+angryAngle, SSD1306_BLACK);
      display.fillTriangle(rx+18, y-25, rx-18, y-25+angryAngle, rx+18, y-25+angryAngle, SSD1306_BLACK);
  }
  
  if(currentMode == POMODORO) { 
    unsigned long elapsed = millis() - pomodoroStartTime;
    if(elapsed < pomodoroDuration) {
       angryAngle = 15; eyeH.target = 10; 
       
       // --- HACHIMAKI (HEADBAND) ---
       // White strip on forehead
       display.fillRect(0, 5, 128, 6, SSD1306_WHITE);
       // Circle in middle (Simulating Japanese flag Rising Sun) - Black on White
       display.fillCircle(64, 8, 4, SSD1306_BLACK);
       
       // Hourglass (Moved down slightly to clear headband)
       int hx = 64; int hy = 55;
       display.drawLine(hx-6, hy-6, hx+6, hy-6, SSD1306_WHITE); 
       display.drawLine(hx-6, hy-6, hx, hy, SSD1306_WHITE);      
       display.drawLine(hx+6, hy-6, hx, hy, SSD1306_WHITE);      
       display.drawLine(hx-6, hy+6, hx+6, hy+6, SSD1306_WHITE); 
       display.drawLine(hx-6, hy+6, hx, hy, SSD1306_WHITE);      
       display.drawLine(hx+6, hy+6, hx, hy, SSD1306_WHITE);      

       float progress = (float)elapsed / (float)pomodoroDuration;
       int sandTop = 6 * (1.0 - progress);
       int sandBot = 6 * progress;        
       for(int i=0; i<sandTop; i++) display.drawLine(hx-i, hy-1-i, hx+i, hy-1-i, SSD1306_WHITE);
       for(int i=0; i<sandBot; i++) display.drawLine(hx-i, hy+6-i, hx+i, hy+6-i, SSD1306_WHITE);

    } else { currentMode = IDLE; chirp("wow"); lastInteract = millis(); }
  }

  // ZZZ Logic: ONLY when actually sleeping (>90s)
  if (currentMode == IDLE && millis() - lastInteract > 90000) {
      if(millis()%2000 < 1000) { display.setCursor(110, 10); display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.print("z"); }
      if(millis()%2000 < 500)  { display.setCursor(120, 5);  display.print("Z"); }
  }

  if(eyeOverlay == "wet") {
    display.drawLine(lx, y+15, lx, y+25, SSD1306_WHITE); display.drawLine(rx, y+15, rx, y+25, SSD1306_WHITE);
  } else if(eyeOverlay == "sweat") {
    display.fillCircle(10, 20, 2, SSD1306_WHITE); display.fillCircle(15, 30, 3, SSD1306_WHITE);
  }

  display.display();
}

void showStats() {
  display.clearDisplay();
  struct tm t; getLocalTime(&t);
  display.setTextSize(3); display.setTextColor(SSD1306_WHITE); display.setCursor(20, 15);
  int h = t.tm_hour; if(is12h && h>12) h-=12; if(h==0) h=12;
  display.printf("%02d:%02d", h, t.tm_min);
  display.setTextSize(1);
  display.setCursor(20, 45); display.print("BAT: "); display.print(batteryPercent); display.print("%");
  display.setCursor(80, 45); display.print(weatherStatus);
  display.display();
}

void setup() {
  pinMode(PIN_BT_POWER, OUTPUT); digitalWrite(PIN_BT_POWER, LOW);
  Serial.begin(115200);
  pinMode(PIN_RED_LED, OUTPUT); pinMode(PIN_BLUE_LED, OUTPUT);
  pinMode(PIN_SNOOZE, INPUT_PULLUP); pinMode(PIN_BATTERY, INPUT);
  ledcAttach(PIN_BUZZER, 2000, 8); ledcWrite(PIN_BUZZER, 0);

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  mpu.begin(); mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  WiFi.begin(ssid, password);
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
  
  xTaskCreatePinnedToCore(taskLogic, "Logic", 10000, NULL, 0, NULL, 0);
  lastInteract = millis();
}

void loop() {
  sensors_event_t a, g, temp; mpu.getEvent(&a, &g, &temp);
  float totalG = sqrt(sq(a.acceleration.x)+sq(a.acceleration.y)+sq(a.acceleration.z));
  static float lastZ = 0;

  struct tm t; getLocalTime(&t);

  // 1. ALARM CHECK (Global Trigger)
  if (alarmActive && currentMode != ALARM && t.tm_sec == 0) {
      if(t.tm_hour == alarmH && t.tm_min == alarmM) {
          currentMode = ALARM;
          chirp("scream");
      }
  }

  // 2. BUTTON CHECK
  if (digitalRead(PIN_SNOOZE) == LOW) {
    lastInteract = millis(); 
    chirp("click");
    if (currentMode == ALARM || currentMode == GUARD) {
      currentMode = IDLE; digitalWrite(PIN_RED_LED, LOW); digitalWrite(PIN_BLUE_LED, LOW);
      delay(500); return;
    }
    unsigned long pressTime = millis();
    while(digitalRead(PIN_SNOOZE) == LOW) {
      if(millis()-pressTime < 300) { showStats(); } 
      else {
         int s = (millis()-pressTime-300)/2; if(s>25) s=25;
         display.clearDisplay();
         display.fillRoundRect(30, 32+s-10, 28, 10, 4, SSD1306_WHITE); 
         display.fillRoundRect(70, 32+s-10, 28, 10, 4, SSD1306_WHITE); 
         display.setTextSize(3); display.setCursor(20, s-25); 
         int h = t.tm_hour; if(is12h && h > 12) h -= 12; if(is12h && h == 0) h = 12;
         display.printf("%02d:%02d", h, t.tm_min);
         display.display();
      }
    }
    if (millis() - pressTime < 1000) { uiTimer = millis() + 4000; }
    return;
  }
  
  if (millis() < uiTimer) { showStats(); return; }

  // 3. ALARM MODE
  if (currentMode == ALARM) {
      eyeH.target = 40; angryAngle = 0; 
      unsigned long t = millis() % 300; 
      bool rState = (t < 150);
      digitalWrite(PIN_RED_LED, rState); digitalWrite(PIN_BLUE_LED, !rState);
      playTone(rState ? 1500 : 800, 0); 
      render(); return; 
  } else {
      if(digitalRead(PIN_RED_LED)) { digitalWrite(PIN_RED_LED, LOW); digitalWrite(PIN_BLUE_LED, LOW); ledcWriteTone(PIN_BUZZER, 0); }
  }

  // 4. GUARD
  if (currentMode == GUARD) {
      display.clearDisplay(); display.display();
      if(totalG > 12.0) currentMode = ALARM;
      return;
  }

  // 5. POMODORO
  if (currentMode == POMODORO) {
      updatePhysics(); render();
      return;
  }

  // 6. IDLE & BOREDOM SYSTEM
  if (currentMode == IDLE) {
      
      if (millis() < reactionTimer) {
         updatePhysics(); render(); return;
      } else {
         angryAngle = 0; pupilOffsetL = 0; pupilOffsetR = 0; eyeOverlay = ""; isShivering = false;
      }

      unsigned long boredTime = millis() - lastInteract;

      // --- RANDOM WHISTLES ANYWHERE ---
      // Low chance to whistle even if not "bored"
      if(random(0, 8000) < 5) {
          chirp("whistle");
      }

      // --- SENSOR TRIGGER TUNING ---
      
      // A. ANGRY 
      if (totalG < 6.0 || totalG > 13.0) { 
         angryAngle = 20; eyeH.target = 5; chirp("scream");
         reactionTimer = millis() + 2000; lastInteract = millis();
      } 
      
      // B. DIZZY
      else if (abs(g.gyro.x) > 4.0 || abs(g.gyro.y) > 4.0) { 
         pupilOffsetL = -8; pupilOffsetR = 8; eyeH.target = 35; chirp("dizzy");
         reactionTimer = millis() + 3000; lastInteract = millis();
      }
      
      // C. HAPPY
      else if (abs(a.acceleration.z - 9.8) > 2.5) {
         static int tapCount = 0; tapCount++;
         if(tapCount > 4) { 
            eyeH.target=15; eyeOverlay="sweat"; chirp("purr"); 
            tapCount=0; reactionTimer = millis() + 4000; lastInteract = millis();
         }
      } 

      // D. SUS (HUH?) - TUNED DOWN
      // Increased threshold to 8.5 to make it happen less
      else if (abs(a.acceleration.x) > 8.5) {
         eyeH.target = 5; eyeX.target = 20; chirp("huh");
         reactionTimer = millis() + 2000; lastInteract = millis();
      }
      
      else {
         eyeOverlay=(weatherStatus=="wet"?"wet":(weatherStatus=="sweat"?"sweat":""));
         if(weatherStatus=="freeze") isShivering = true;
      }

      // --- BOREDOM STAGES ---
      if (boredTime > 30000 && boredTime < 60000) {
          // Less frequent WOW (reduced probability)
          if (random(0, 200) < 2) { eyeH.target = 40; eyeX.target=random(-10,10); chirp("wow"); } 
      }
      else if (boredTime > 60000 && boredTime < 90000) {
          // Whistling logic merged into global or specific here
          if (random(0, 100) < 2) { eyeY.target = -10; chirp("whistle"); } 
      }
      else if (boredTime > 90000) {
          eyeH.target = 1; 
          // Less Frequent Snore (5000ms) and Quieter (Handled in chirp)
          if (millis() % 5000 < 50) chirp("snore"); 
      }

      float tiltX = a.acceleration.y; float tiltY = a.acceleration.x; 
      if (boredTime < 90000) { 
         eyeX.target = -tiltX * 2.5; 
         eyeY.target = tiltY * 2.0;
      }

      if(boredTime < 90000 && millis()>nextBlink) { 
         eyeH.target=2; 
         if(millis()>nextBlink+150){ eyeH.target=30; nextBlink=millis()+random(2000,6000); }
      }
      
      updatePhysics();
      render();
  }
}

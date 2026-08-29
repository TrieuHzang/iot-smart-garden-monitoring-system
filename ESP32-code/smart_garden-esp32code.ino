
#define BLYNK_TEMPLATE_ID   ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN    ""

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "";
char pass[] = "";

// UART2 cho ESP32
#define RXD2 16
#define TXD2 17

// Datastream values
int soil = 0;
int temp = 0;
int hum = 0;
int light = 0;

int timeToWater = 0;
String currentPlant = "Dua leo";
String pumpStatus = "OFF";
String lightStatus = "OFF";
String systemStatus = "Normal";

bool pumpState = false;
bool lightState = false;

// timer gửi định kỳ lên Blynk
BlynkTimer timer;

// Chống spam event
unsigned long lastHighTempEvent = 0;
unsigned long lastLowSoilEvent  = 0;
unsigned long lastLowLightEvent = 0;
const unsigned long EVENT_COOLDOWN_MS = 60000; // 60 giây

String uartBuffer = "";

// Parse chuỗi từ STM32:
// SOIL:50,TEMP:30,HUM:60,LIGHT:70
bool parseSTM32Data(const String& data) {
  int s, t, h, l;

  if (sscanf(data.c_str(), "SOIL:%d,TEMP:%d,HUM:%d,LIGHT:%d", &s, &t, &h, &l) == 4) {
    soil = constrain(s, 0, 100);
    temp = constrain(t, -20, 80);
    hum = constrain(h, 0, 100);
    light = constrain(l, 0, 100);
    return true;
  }

  return false;
}

// Tính trạng thái hệ thống
void updateDerivedStates() {

  pumpState = (soil < 35);
  lightState = (light < 40);

  pumpStatus = pumpState ? "ON" : "OFF";
  lightStatus = lightState ? "ON" : "OFF";

  if (soil >= 80)       timeToWater = 120;
  else if (soil >= 60)  timeToWater = 90;
  else if (soil >= 40)  timeToWater = 60;
  else if (soil >= 20)  timeToWater = 30;
  else                  timeToWater = 10;

  if (temp > 35) {
    systemStatus = "High Temp";
  } else if (soil < 30) {
    systemStatus = "Low Soil";
  } else if (light < 30) {
    systemStatus = "Low Light";
  } else {
    systemStatus = "Normal";
  }
}

// Gửi toàn bộ datastream lên Blynk
void sendDataToBlynk() {
  Blynk.virtualWrite(V0, soil);
  Blynk.virtualWrite(V1, temp);
  Blynk.virtualWrite(V2, hum);
  Blynk.virtualWrite(V3, light);

  Blynk.virtualWrite(V4, pumpStatus);
  Blynk.virtualWrite(V5, lightStatus);
  Blynk.virtualWrite(V8, systemStatus);
}

// Gửi event cảnh báo lên Blynk
void checkAndSendAlerts() {
  unsigned long now = millis();

  if (temp > 35 && (now - lastHighTempEvent > EVENT_COOLDOWN_MS)) {
    Blynk.logEvent("high_temp", String("Nhiet do cao: ") + temp + "C");
    lastHighTempEvent = now;
  }

  if (soil < 30 && (now - lastLowSoilEvent > EVENT_COOLDOWN_MS)) {
    Blynk.logEvent("low_soil", String("Do am dat thap: ") + soil + "%");
    lastLowSoilEvent = now;
  }

  if (light < 30 && (now - lastLowLightEvent > EVENT_COOLDOWN_MS)) {
    Blynk.logEvent("low_light", String("Anh sang thap: ") + light + "%");
    lastLowLightEvent = now;
  }
}

// Đọc UART không block
void readUARTTask() {
  while (Serial2.available()) {
    char c = (char)Serial2.read();

    if (c == '\n') {
      uartBuffer.trim();

      if (uartBuffer.length() > 0) {
        Serial.print("UART RX: ");
        Serial.println(uartBuffer);

        if (parseSTM32Data(uartBuffer)) {
          updateDerivedStates();
          sendDataToBlynk();
          checkAndSendAlerts();
        } else {
          Serial.println("Parse failed");
        }
      }

      uartBuffer = "";
    } else if (c != '\r') {
      uartBuffer += c;
    }
  }
}

// Đồng bộ định kỳ nếu cần
void periodicSync() {
  updateDerivedStates();
  sendDataToBlynk();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // gửi định kỳ mỗi 5 giây
  timer.setInterval(5000L, periodicSync);

  Serial.println("ESP32 started");
}

void loop() {
  Blynk.run();
  timer.run();
  readUARTTask();
}
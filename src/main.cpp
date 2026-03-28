#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GyverOLED.h>
#include <time.h>
#include <vector>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <uButton.h>
#include "driver/ledc.h"
#include "Heater.h"
#include "TGClient.h"
#include "MenuBase.h"
#include "ConfigStruct.h"
#include "AppMenus.h"
#include "GlobalVars.h"
#include "TelegramManager.h"
#include "LocalUI.h"

#define SENSOR_PIN 10
#define SCL_PIN 9
#define SDA_PIN 8
#define RELAY_PIN 2
#define BUZZER_PIN 1
#define BUZZER_CHANNEL 0
#define BUZZER_RES LEDC_TIMER_13_BIT

uButton btnUp(6);
uButton btnDown(5);
uButton btnOk(4);
uButton btnBack(3);

uint32_t sensorTimer = 0;

OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

const char *ntpServer = "pool.ntp.org";

Config config; 
State state;

namespace Buzzer
{
  void play(int freq, int durationMs)
  {
    if (freq > 0)
      ledcWriteTone(BUZZER_CHANNEL, freq);
    delay(durationMs);
    ledcWriteTone(BUZZER_CHANNEL, 0);
  }

  void init()
  {
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RES);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    play(1500, 100); // beepStart
  }

  void signal(int repeat = 2, int count = 3)
  {
    for (int i = 0; i < repeat; i++)
    {
      for (int j = 0; j < count; j++)
      {
        play(2000, 100);
        if (j < count - 1)
          delay(100);
      }
      if (i < repeat - 1)
        delay(400);
    }
  }
}

void loadSettings()
{
  if (!LittleFS.exists("/settings.json"))
    return;
  File file = LittleFS.open("/settings.json", "r");
  if (!file)
    return;

  JsonDocument doc;
  if (!deserializeJson(doc, file))
  {
    config.hysteresis = doc["hyst"] | 0.2;
    config.targetCubeTemp = doc["targetT"] | 0.0;
    config.isSelectionActive = doc["isSel"] | false;
    config.baseSelectionTemp = doc["baseT"] | 0.0;
    config.tzOffset = doc["tz"] | 5;
    config.botToken = doc["botToken"] | "";
    config.chat_id = doc["chat_id"] | "";
    config.ssid = doc["ssid"] | "";
    config.password = doc["password"] | "";
    config.urlWorker = doc["urlWorker"] | "your-worker.workers.dev"; // заглушка
    config.targetColTemp = doc["colT"] | 78.0;
    config.isAutoMode = doc["autoM"] | true;
    config.oledBrightness = doc["oledB"] | 255.0;
  }
  file.close();
}

void saveSettings()
{
  JsonDocument doc;
  doc["hyst"] = config.hysteresis;
  doc["targetT"] = config.targetCubeTemp;
  doc["isSel"] = config.isSelectionActive;
  doc["baseT"] = config.baseSelectionTemp;
  doc["tz"] = config.tzOffset;
  doc["botToken"] = config.botToken;
  doc["chat_id"] = config.chat_id;
  doc["ssid"] = config.ssid;
  doc["password"] = config.password;
  doc["urlWorker"] = config.urlWorker;
  doc["colT"] = config.targetColTemp;
  doc["autoM"] = config.isAutoMode;
  doc["oledB"] = config.oledBrightness;

  File file = LittleFS.open("/settings.json", "w");
  if (file)
  {
    serializeJson(doc, file);
    file.close();
  }
}

void requestTgUpdate(bool force = false);

TelegramManager bot;
Heater tankHeater(RELAY_PIN); 

void requestTgUpdate(bool force)
{
  bot.updateTelegram(force);
}

LocalUI localMenu;

void handleMashLogic()
{
  if (!state.mashActive || state.currentPauseIdx < 0 || state.currentPauseIdx >= (int)state.pauses.size())
    return;

  MashPause &current = state.pauses[state.currentPauseIdx];

  if (state.pauseTimer == 0)
  {
    if (state.currentT1 >= current.temp)
    {
      state.pauseTimer = millis();
      TGClient::sendAlert("⏱ Пауза №" + String(state.currentPauseIdx + 1) + " (" + String(current.temp, 1) + "°C): Отсчет пошел!");
      Buzzer::signal(1, 2);
      bot.updateTelegram(true);
    }
  }
  else
  {
    unsigned long elapsed = (millis() - state.pauseTimer) / 60000;
    int remaining = current.duration - (int)elapsed;

    if (remaining == 1 && !state.warningSent)
    {
      Buzzer::signal(1);
      TGClient::sendAlert("🔔 *ВНИМАНИЕ!*\nОсталась 1 минута до конца текущей паузы!");
      state.warningSent = true;
    }

    if (elapsed >= (unsigned long)current.duration)
    {
      state.currentPauseIdx++;
      state.pauseTimer = 0;
      state.warningSent = false;

      if (state.currentPauseIdx >= (int)state.pauses.size())
      {
        state.mashActive = false;
        state.currentPauseIdx = -1;
        TGClient::sendAlert("🏁 *ЗАТИРАНИЕ ЗАВЕРШЕНО!*");
        Buzzer::signal();
      }
      else
      {
        TGClient::sendAlert("⏭ Следующая пауза: " + String(state.pauses[state.currentPauseIdx].temp, 1) + "°C");
        Buzzer::signal(1);
      }
      bot.updateTelegram(true);
    }
  }
}

void handleCubeTempLogic()
{
  if (!state.isWaitForHeat || config.targetCubeTemp <= 0)
    return;

  bool reached = state.isHeating ? (state.currentT1 >= config.targetCubeTemp) : (state.currentT1 <= config.targetCubeTemp);

  if (reached)
  {
    Buzzer::signal();
    state.isWaitForHeat = false;
    TGClient::sendAlert((state.isHeating ? "✅ Нагрев завершен!" : "✅ Охлаждение завершено!") + String("\nТемпература: ") + String(state.currentT1, 1) + "°C");
    saveSettings();
    requestTgUpdate(true);
  }
}

void handleSelectionLogic()
{
  if (!config.isSelectionActive)
    return;

  if (state.currentT2 >= (config.baseSelectionTemp + config.hysteresis))
  {
    if (!state.hysteresisReached)
    {
      state.hysteresisReached = true;
      Buzzer::signal(1, 3);
      TGClient::sendAlert("⚠️ ПРЕВЫШЕНИЕ Т2! Отбор приостановлен.");
      requestTgUpdate(true);
    }
  }
  else if (state.currentT2 <= config.baseSelectionTemp)
  {
    if (state.hysteresisReached)
    {
      state.hysteresisReached = false;
      Buzzer::signal(1, 2);
      TGClient::sendAlert("✅ Т2 пришла в норму.");
      requestTgUpdate(true);
    }
  }
}

void processBrewLogic()
{
  handleMashLogic();
  handleCubeTempLogic();
  handleSelectionLogic();
}

void networkTask(void *pvParameters)
{
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      processBrewLogic();
      bot.handleUpdates();
      bot.updateTelegram();
    }
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  oled.init();
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  pinMode(RELAY_PIN, OUTPUT);
  state.currentT1 = sensors.getTempCByIndex(0);
  state.currentT2 = sensors.getTempCByIndex(1);
  LittleFS.begin(true);
  loadSettings();

  oled.clear();
  oled.print("AP WiFi: BREW_CTRL");


  WiFiManager wm;
  char tzStr[4];
  itoa(config.tzOffset, tzStr, 10);
  WiFiManagerParameter custom_worker_url("worker_url", "Worker URL", config.urlWorker.c_str(), 64);
  WiFiManagerParameter custom_tg_token("botToken", "Telegram Token", config.botToken.c_str(), 64);
  WiFiManagerParameter custom_tg_chat_id("chat_id", "Chat ID", config.chat_id.c_str(), 20);
  WiFiManagerParameter custom_tz("tz", "Смещение часового пояса", tzStr, 4, "type=\"number\" min=\"-12\" max=\"14\"");

  wm.setConnectTimeout(10);
  wm.setConfigPortalTimeout(300);
  wm.addParameter(&custom_worker_url);
  wm.addParameter(&custom_tg_token);
  wm.addParameter(&custom_tg_chat_id);
  wm.addParameter(&custom_tz);

  if (!wm.autoConnect("BREW_CTRL"))
  {
    delay(5000);
    ESP.restart();
  }

  config.botToken = custom_tg_token.getValue();
  config.urlWorker = custom_worker_url.getValue();
  config.chat_id = custom_tg_chat_id.getValue();
  config.tzOffset = atoi(custom_tz.getValue());
  config.ssid = wm.getWiFiSSID();
  config.password = wm.getWiFiPass();
  saveSettings();

  oled.clear();
  oled.setCursor(0, 0);
  oled.setScale(2);
  oled.print("Syncing");

  configTime(config.tzOffset * 3600, 0, ntpServer);

  TGClient::sendMessage("✅ BrewDist Monitor Online");
  requestTgUpdate(true);

  xTaskCreatePinnedToCore(networkTask, "Net", 10000, NULL, 1, NULL, 0);
  localMenu.init();
  Buzzer::init();
}

void loop()
{
  btnUp.tick();
  btnDown.tick();
  btnOk.tick();
  btnBack.tick();
  if (millis() - sensorTimer >= 1000)
  {
    sensorTimer = millis();
    sensors.requestTemperatures();
    state.currentT1 = sensors.getTempCByIndex(0);
    state.currentT2 = sensors.getTempCByIndex(1);
  }
  localMenu.tick();
}
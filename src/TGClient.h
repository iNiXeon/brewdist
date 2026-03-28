#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class TGClient {
public:
    static String sendRequest(const String &method, const String &jsonBody, int timeoutS = 10);
    static String sendMessage(const String &text);
    static void sendAlert(const String &text);
};
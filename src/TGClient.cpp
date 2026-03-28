#include "TGClient.h"
#include "GlobalVars.h" 

extern Config config;
extern State state;

String TGClient::sendRequest(const String &method, const String &jsonBody, int timeoutS) {
    if (WiFi.status() != WL_CONNECTED || config.botToken.isEmpty())
        return "{\"ok\":false}";
    if (time(nullptr) < 100000)
        return "{\"ok\":false}";

    WiFiClientSecure client;
    client.setInsecure();
    client.setHandshakeTimeout(10000);

    HTTPClient http;
    http.begin(client, "https://" + config.urlWorker + "/bot" + config.botToken + "/" + method);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "close");
    http.setTimeout(timeoutS * 1000);

    int code = http.POST(jsonBody);
    String resp = "{\"ok\":false}";

    if (code > 0) {
        resp = (method == "editMessageText") ? "{\"ok\":true}" : http.getString();
    } else {
        Serial.printf("HTTP Error: %s\n", http.errorToString(code).c_str());
    }

    http.end();
    client.stop();
    return resp;
}

String TGClient::sendMessage(const String &text) {
    JsonDocument doc;
    doc["chat_id"] = config.chat_id;
    doc["text"] = text;
    doc["parse_mode"] = "Markdown";
    String body;
    serializeJson(doc, body);
    return sendRequest("sendMessage", body);
}

void TGClient::sendAlert(const String &text) {
    JsonDocument doc;
    doc["chat_id"] = config.chat_id;
    doc["text"] = text;
    doc["parse_mode"] = "Markdown";
    doc["reply_markup"] = serialized("{\"inline_keyboard\":[[{\"text\":\"✅ ОК\",\"callback_data\":\"/delete_msg\"}]]}");
    String body;
    serializeJson(doc, body);
    sendRequest("sendMessage", body);
}
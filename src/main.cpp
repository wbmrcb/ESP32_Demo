#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#define SSID "ESP32-Demo"
// #define PASSWD "12345678"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len) {
      // the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(),
                    client->id(), (info->opcode == WS_TEXT) ? "text" : "binary",
                    info->len);
      if (info->opcode == WS_TEXT) {
        data[len] = 0;
        Serial.printf("%s\r\n", (char *)data);
        ws.textAll(data, len);
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (!SPIFFS.begin(true)) {
    Serial.println("Formatting SPIFFS...");
  }

  ws.onEvent(onEvent);

  server.serveStatic("/", SPIFFS, "/")
      .setDefaultFile("index.html")
      .setCacheControl("max-age=600");

  server.addHandler(&ws);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}

uint64_t startTime = 0;
uint16_t period = 5000;

void loop() {
  ws.cleanupClients();

  if (millis() - startTime > period) {
    startTime = millis();
    float temp = (temperatureRead() - 32.0) / 1.8;
    String t = String("T:") + String(temp);
    ws.textAll(t);
  }
}
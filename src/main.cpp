#include <Arduino.h>
#include <NTPClient.h>
#include <RestClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <FirebaseESP32.h>
#include <WiFiUdp.h>
#include <config.h>

FirebaseData firebaseData;
FirebaseJson fJson;

#define DHTPIN 14 //here we use pin IO14 of ESP32 to read data
#define DHTTYPE DHT11 //our sensor is DHT22 type

const int LEDPIN =  23;

extern char* FIREBASE_HOST;
extern char* FIREBASE_AUTH;
extern char* ssid;
extern char* password;

int counter = 0;
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String formattedDate;
String dayStamp;
String timeStamp;

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "application/json", "{\"message\":\"Not found\"}");
}
void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 sensor!");
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  //call begin to start sensor
  dht.begin();
  WiFi.begin(ssid, password);
  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
   Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.print("Received request from client with IP: ");
    Serial.println(request->client()->remoteIP());
      StaticJsonDocument<100> data;
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (isnan(h) || isnan(t)) {
          data["temperature"] = "error";
          data["humidity"] = "error";
        }
        else {
          data["temperature"] = dht.readTemperature();
          data["humidity"] = dht.readHumidity();
        }
        
      String response;
      serializeJson(data, response);
      request->send(200, "application/json", response);
      Serial.println(response);
      
  });
  server.onNotFound(notFound);
  server.begin();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
   Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  timeClient.begin();
}
void loop() { 
  while(!timeClient.update()) {
  timeClient.forceUpdate();
}
  formattedDate = timeClient.getFormattedTime();
  Serial.println(formattedDate);
  counter++;
  fJson.set("/temperature", dht.readTemperature());
  fJson.set("/humidity", dht.readHumidity());
  fJson.set("/time", formattedDate);
  if (Firebase.updateNode(firebaseData, "/Sensor", fJson)) {
    Serial.println(firebaseData.dataPath());
    Serial.println(firebaseData.dataType());
    Serial.println(firebaseData.jsonString()); 
  } else {
    Serial.println(firebaseData.errorReason());
  }
  if(counter == 5){
    digitalWrite(LEDPIN, HIGH);
    delay(200);
    digitalWrite(LEDPIN, LOW);
    Serial.println(counter);
    counter = 0;
  }
  delay(25000); 
}



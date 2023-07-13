#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
#include <ArduinoJson.h>

#define WIFI_SSID "realme 5i"
#define WIFI_PASSWORD "qwerty123"

#define SERVER_NAME "http://192.168.43.94:8000/store-data"

#define DHTPIN 33
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN 34
#define RELAY_FAN1_PIN 17
#define RELAY_FAN2_PIN 5
#define RELAY_PUMP1_PIN 19
#define RELAY_PUMP2_PIN 18
#define LED_FAN1_PIN 16
#define LED_FAN2_PIN 4
#define LED_PUMP1_PIN 13
#define LED_PUMP2_PIN 12

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

// threshold untuk controlling
#define Temp 24
#define Hum_Air 70
#define Soil 48

unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

bool fan1_status;
bool fan2_status;
bool pump1_status;
bool pump2_status;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Wire.begin();
  dht.begin();
  lightMeter.begin();
  pinMode(DHTPIN, INPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(RELAY_FAN1_PIN, OUTPUT);
  pinMode(RELAY_FAN2_PIN, OUTPUT);
  pinMode(RELAY_PUMP1_PIN, OUTPUT);
  pinMode(RELAY_PUMP2_PIN, OUTPUT);
  pinMode(LED_FAN1_PIN, OUTPUT);
  pinMode(LED_FAN2_PIN, OUTPUT);
  pinMode(LED_PUMP1_PIN, OUTPUT);
  pinMode(LED_PUMP2_PIN, OUTPUT);
}

void loop() {
  double air_humidity = dht.readHumidity();
  double temperature = dht.readTemperature();
  double light_intensity = lightMeter.readLightLevel();
  double read_soil_moist = analogRead(SOIL_MOISTURE_PIN);
  double soil_moisture = (100-(read_soil_moist/4095.00)*100);

  if (isnan(temperature)|| isnan(air_humidity)){
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (temperature > Temp){
    digitalWrite(RELAY_FAN1_PIN, HIGH); fan1_status = true;
    digitalWrite(RELAY_FAN2_PIN, HIGH); fan2_status = true;
    digitalWrite(LED_FAN1_PIN, HIGH);
    digitalWrite(LED_FAN2_PIN, HIGH);
  } else {
    digitalWrite(RELAY_FAN1_PIN, LOW); fan1_status = false;
    digitalWrite(RELAY_FAN2_PIN, LOW); fan2_status = false;
    digitalWrite(LED_FAN1_PIN, LOW);
    digitalWrite(LED_FAN2_PIN, LOW);
  }

  if (air_humidity < Hum_Air){
    digitalWrite(RELAY_PUMP1_PIN, HIGH); pump1_status = true;
    digitalWrite(LED_PUMP1_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PUMP1_PIN, LOW); pump1_status = false;
    digitalWrite(LED_PUMP1_PIN, LOW);
  }

  if (soil_moisture < Soil){
    digitalWrite(RELAY_PUMP2_PIN, HIGH); pump2_status = true;
    digitalWrite(LED_PUMP2_PIN, HIGH);
  }  else {
    digitalWrite(RELAY_PUMP2_PIN, LOW); pump2_status = false;
    digitalWrite(LED_PUMP2_PIN, LOW);
  }

  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      http.begin(SERVER_NAME);
      http.addHeader("Content-Type", "application/json");

      StaticJsonDocument<200> doc;

      doc["temperature"] = round2(temperature);
      doc["air_humidity"] = round2(air_humidity);
      doc["soil_moisture"] = round2(soil_moisture);
      doc["light_intensity"] = round2(light_intensity);
      doc["fan1_status"] = fan1_status;
      doc["fan2_status"] = fan2_status;
      doc["pump1_status"] = pump1_status;
      doc["pump2_status"] = pump2_status;

      String requestBody;
      serializeJson(doc, requestBody);
  
      int httpResponseCode = http.POST(requestBody);
      Serial.println("\n" + requestBody + "\n");

      if(httpResponseCode > 0) {
        String response = http.getString();

        Serial.print("code: ");
        Serial.println(httpResponseCode);
        Serial.println(response + "\n");
      } else {
        Serial.printf("Error occured while sending HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str()); 
      }
    }
  }

  // print
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(air_humidity);
  Serial.println("%");

  Serial.print("Soil Moisture: ");
  Serial.print(soil_moisture);
  Serial.println("%");

  Serial.print("Light Intensity: ");
  Serial.print(light_intensity);
  Serial.println(" lux");
}

double round2(double value) {
  return (int)(value * 100 + 0.5)/100.00;
}
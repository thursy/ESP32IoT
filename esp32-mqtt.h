/******************************************************************************
   Copyright 2018 Google
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *****************************************************************************/
// This file contains static methods for API requests using Wifi / MQTT
#ifndef __ESP32_MQTT_H__
#define __ESP32_MQTT_H__

#include "time.h"
#include <Client.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <MQTT.h>

#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>
#include "ciotc_config.h" // Update this file with your configuration

#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
const float hourOffset = 7;

// !!REPLACEME!!
// The MQTT callback function for commands and configuration updates
// Place your message handler code here.
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}
///////////////////////////////

// Initialize WiFi and MQTT for this board
Client *netClient;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
unsigned long iss = 0;
String jwt;


//////////////////////////////
// DHT11 Read Temp and create json
/////////////////////////////
float getTemperature() {
  float temp = dht.readTemperature();
  Serial.print("Temperature:");
  Serial.println(temp);
  return temp;
}

float getHumidity() {
  float humd =  dht.readHumidity();
  Serial.print("Humidity:");
  Serial.println(humd);
  return humd;
}

String getTime() {
  delay(1000);
  time_t rawtime;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
  }

  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  //print like "const char*"
  Serial.println(timeStringBuff);

  //Optional: Construct String object
  return String(timeStringBuff);

}

String getTemperatureJSON() {
  return String("{\"timestamp\":\"") + String(getTime()) + ("\", \"temperature\":\"") + String(getTemperature()) + String("\", \"humidity\":\"") + String(getHumidity()) + String("\", \"device\":\"device-1") + String("\"}");
  //return String("{\"temperature\":\"")+String(getTemperature())+String("\", \"humidity\":\"")+String(getHumidity())+String("\"}");
  //return String("{\"temperature\":\"")+String(getTemperature())+",\"humidity\":\""+String(getHumidity())+String("\"}");
  //return String("{\"device\":\"1")+String(",\"timestamp\":\"GMT+8")+ String(",\"temperature\":\"")+String(getTemperature())+String(",\"humidity\":\"")+String(getHumidity())+String("\"}");
}



///////////////////////////////
// Helpers specific to this board
///////////////////////////////
String getDefaultSensor() {
  return  "Wifi: " + String(WiFi.RSSI()) + "db";
}

String getJwt() {
  iss = time(nullptr);
  Serial.println("Refreshing JWT");
  jwt = device->createJWT(iss, jwt_exp_secs);
  return jwt;
}

void setupWifi() {
  Serial.println("Starting wifi");

  WiFi.mode(WIFI_STA);
  // WiFi.setSleep(false); // May help with disconnect? Seems to have been removed from WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  configTime(0, 0, ntp_primary, ntp_secondary);
  Serial.println("Waiting on time sync...");
  while (time(nullptr) < 1510644967) {
    delay(10);
  }
}

void connectWifi() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
}

///////////////////////////////
// Orchestrates various methods from preceeding code.
///////////////////////////////
bool publishTelemetry(String data) {
  return mqtt->publishTelemetry(data);
}

bool publishTelemetry(const char* data, int length) {
  return mqtt->publishTelemetry(data, length);
}

bool publishTelemetry(String subfolder, String data) {
  return mqtt->publishTelemetry(subfolder, data);
}

bool publishTelemetry(String subfolder, const char* data, int length) {
  return mqtt->publishTelemetry(subfolder, data, length);
}

void connect() {
  connectWifi();
  mqtt->mqttConnect();
}

void setupCloudIoT() {
  device = new CloudIoTCoreDevice(
    project_id, location, registry_id, device_id,
    private_key_str);

  setupWifi();
  netClient = new WiFiClientSecure();
  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(500, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);
  mqtt->setUseLts(true);
  mqtt->startMQTT();
}
#endif //__ESP32_MQTT_H__

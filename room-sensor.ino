#include <Arduino.h>
#ifndef ARDUINO
#define ARDUINO 150
#endif

#include <ESP8266WiFi.h>
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// MQTT client
#include "MqttClient.h"
#include "secrets.h"

MqttClient mqttClient(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

// OLED display
#define UPDATE_TIMEOUT 2000
#define DISPLAY_UPDATE_DELAY 10000

#define I2C_SCL D1
#define I2C_SDA D2
#define OLED_RESET LED_BUILTIN

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

// DHT22 sensor
#include "SensorDHT.h"
SensorDHT sensor(DHT_IN); // setup temp sensor

/**
 * Callback for switch relay topic
 */
bool handleToggleSwitchMessage(std::string topicName, std::string message)
{
    Serial.printf("[main] handle switch message for '%s' with content '%s'\n", topicName.c_str(), message.c_str());

    bool enabledState = (std::string(message) == "true");
    // --- relays.togglePort(0, enabledState);
    Serial.printf("[main] switch '%s' set to '%s'\n", topicName.c_str(), enabledState ? "ON" : "OFF");
    return true;
}

/**
 * Initial setup of serial debug console and connections
 */
void setup()
{
    Serial.begin(115200);

    // run tests
    TestTemperatureSensor testSensor;
    testSensor.runTests();

    // MQTT
    // Connect to WiFi access point.
    Serial.println();
    Serial.println();
    Serial.printf("Connecting to WLAN '%s'...", WLAN_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASS);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && ++retries < 3) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi not connected!");
    }

    WiFi.printDiag(Serial);

    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();

    // Clear the buffer.
    display.clearDisplay();

    delay(1000);

    mqttClient.createPublishTopic("temperature", "/arbeitszimmer/temperature", MqttClient::SENSOR);
    mqttClient.createPublishTopic("humidity", "/arbeitszimmer/humidity", MqttClient::SENSOR);
    mqttClient.createPublishTopic("lights", "/arbeitszimmer/lights", MqttClient::SWITCH);
    mqttClient.createSubscribeTopic("lights", "/arbeitszimmer/lights/set", MqttClient::SWITCH);
    mqttClient.createSubscribeTopic("lights_available", "/arbeitszimmer/lights/available", MqttClient::SWITCH);

    mqttClient.addNotifyCallback("lights", &handleToggleSwitchMessage);

    // first read often gets invalid values
    sensor.temperature();
    sensor.humidity();
}

/**
 * Read sensor, output data on OLED and via MQTT
 */
void loop()
{
    float temperature = sensor.temperature();
    float humidity    = sensor.humidity();

    display.clearDisplay();
    display.setCursor(8, 0);
    display.setTextColor(WHITE); // 'inverted' text

    if (!sensor.isTemperatureValid() || !sensor.isHumidityValid()) {
        delay(UPDATE_TIMEOUT);
        return;
    }

    display.setTextSize(4);
    char temperatureStr[5];
    sprintf(temperatureStr, "%02.1f", temperature);
    display.print(temperatureStr);
    display.println((char)247);
    display.setCursor(32, 48);
    display.setTextSize(2);

    char humidityStr[5];
    sprintf(humidityStr, "%02.1f", humidity);
    display.print(humidityStr);
    display.println("%");

    display.display();

    // publish temp+humidity via MQTT
    Serial.print(F("\nSending temp val "));
    Serial.print(temperatureStr);
    Serial.print("...");
    if (!mqttClient.publish("temperature", temperatureStr)) {
        Serial.println(F("Failed"));
    } else {
        Serial.println(F("OK!"));
    }

    Serial.print(F("\nSending humidity val "));
    Serial.print(humidityStr);
    Serial.print("...");
    if (!mqttClient.publish("humidity", humidityStr)) {
        Serial.println(F("Failed"));
    } else {
        Serial.println(F("OK!"));
    }

    if (mqttClient.connected()) {
        if (!mqttClient.waitForMessages(DISPLAY_UPDATE_DELAY)) {
            Serial.println("[mqtt] wait for messages aborted");
        } else {
            Serial.println("[mqtt] wait for messages successful");
        }
    } else {
        delay(DISPLAY_UPDATE_DELAY);
    }
}

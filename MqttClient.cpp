#include "MqttClient.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESP8266WiFi.h>

/**
 * Replace all occurrences of given searchStr with replaceText and return the new string
 */
std::string stringReplaceAll(const std::string& text, const std::string searchStr, const std::string replaceStr)
{
    std::string data = text;
    size_t      pos  = data.find(searchStr);

    while (pos != std::string::npos) {
        data.replace(pos, searchStr.size(), replaceStr);
        pos = data.find(searchStr, pos + replaceStr.size() - 1);
    }
    return data;
}

/**
 * Constructor to prepare connection to MQTT Broker
 */
MqttClient::MqttClient(WiFiClient* client, const char* serverHost, int serverPort, const char* userName, const char* password) :
    mqttClient(new Adafruit_MQTT_Client(client, serverHost, serverPort, userName, password))
{
}

/**
 * Returns true when connected to MQTT Broker
 */
bool MqttClient::connected()
{
    return mqttClient->connected();
}

/**
 * Connect and reconnect as necessary to the MQTT server.
 * Should be called in the loop function and it will take care if connecting.
 */
bool MqttClient::connect(void)
{
    if (mqttClient->connected()) // Stop if already connected.
        return true;

    Serial.print("Connecting to MQTT... ");

    int8_t  ret;
    uint8_t retries = MQTT_RECONNECT_RETRIES;
    while ((ret = mqttClient->connect()) != 0) // connect will return 0 for connected
    {
        Serial.println(mqttClient->connectErrorString(ret));
        Serial.println("Retrying MQTT connection...");
        mqttClient->disconnect();
        if (--retries == 0) {
            break;
        }
        delay(MQTT_TIMEOUT);
    }
    bool online = connected();
    Serial.println(online ? "MQTT connected successfully" : "MQTT connection failed!");
    return online;
}

/**
 * Disconnect from the MQTT server.
 */
bool MqttClient::disconnect(void)
{
    if (!mqttClient->connected()) {
        return true;
    }
    return mqttClient->disconnect();
}

/**
 * Register given MQTT topic (the MQTT publish path name) with given short name
 *
 * @param topicName - string with short name for given MQTT publish topic
 * @param mqttPath  - string with full MQTT publish topic name in format "maintopic/topic/subtopic"
 * 
 * @return false when topic already exists and on any error
 */
bool MqttClient::createPublishTopic(const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType)
{
    if (publishTopics.find(topicName) != publishTopics.end() || topicName.length() < 1) {
        Serial.printf("error: createPublishTopic failed for topic '%s' - given topic name is already registered!\n", topicName.c_str());
        return false;
    }

    std::string prefix;

    switch (topicType) {
    case SENSOR:
        prefix = "/sensor/";
        break;
    case SWITCH:
        prefix = "/switch/";
        break;
    default:
        Serial.printf("error: invalid MqttTopicType given to createPublishTopic for topic '%s'!", topicName.c_str());
        return false;
    }

    MqttTopicData data;
    data.shortName      = topicName;
    data.pathName       = stringReplaceAll(std::string("/" + prefix + "/" + mqttPath), "//", "/");
    data.topicType      = topicType;
    data.publishHandler = std::shared_ptr<Adafruit_MQTT_Publish>(new Adafruit_MQTT_Publish(mqttClient.get(), data.pathName.c_str()));

    publishTopics[topicName] = data;

    Serial.printf("created MQTT publish topic '%s' with MQTT path '%s'\n", data.shortName.c_str(), data.pathName.c_str());
}

/**
 * Removes given publish topic
 */
bool MqttClient::removePublishTopic(const std::string& topicName)
{
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("error: removePublishTopic failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    publishTopics.erase(topicName);
    return true;
}

/**
 * Assign the given callback function to the given topic, so it gets called on each publish() call
 * 
 * There is only one callback per topic possible. Any existing callback will be removed before setting the new one.
 */
bool MqttClient::addNotifyCallback(const std::string& topicName, NotifyCallbackFunction callback)
{
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("error: addNotifyCallback failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    publishTopics.at(topicName).notifyCallback = callback;
    return true;
}

/**
 * Remove the callback function of givem topic
 */
bool MqttClient::removeNotifyCallback(const std::string& topicName)
{
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("error: removeNotifyCallback failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    publishTopics.at(topicName).notifyCallback = nullptr;
    return true;
}

/**
 * Return notify callback function for given topic or nullptr, if missing or on errors
 */
MqttClient::NotifyCallbackFunction MqttClient::notifyCallback(const std::string& topicName)
{
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("error: notifyCallback failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return nullptr;
    }
    if (!publishTopics.at(topicName).notifyCallback) {
        Serial.printf("error: notifyCallback failed for topic '%s' - invalid callback function!\n", topicName.c_str());
        return nullptr;
    }
    return publishTopics.at(topicName).notifyCallback;
}

/**
 * Publish given message using the short name for the topic (not the full MQTT publish topic path name)
 */
bool MqttClient::publish(const std::string& topicName, const std::string& message)
{
    if (!connect()) {
        Serial.printf("error: publish failed for topic '%s' - connection failed!\n", topicName.c_str());
        return false;
    }
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("error: publish failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    if (!publishTopics.at(topicName).publishHandler) {
        Serial.printf("error: publish failed for topic '%s' - invalid publish handler object!\n", topicName.c_str());
        return false;
    }
    if (!publishTopics.at(topicName).publishHandler->publish(message.c_str())) {
        Serial.printf("error: publish failed for topic '%s' - error on sending message '%s'\n", topicName.c_str(), message.c_str());
        return false;
    }
    return true;
}

/**
 * Handle incoming MQTT messages
 */
void MqttClient::incomingMessageCallback(char* topic, byte* payload, unsigned int length)
{
    std::string tmp          = (const char*)payload;
    bool        enabledState = (tmp == "true");
    std::string topicStr     = topic;

    Serial.printf("MQTT Message arrived [%s]\n", tmp.c_str());

    for (auto& pair : publishTopics) {
        if (pair.first != topicStr /* --- handle second topic for [pathName]/available status here --- */) {
            break;
        }

        MqttTopicData& data = pair.second;

        if (data.publishHandler) {
            std::string newMsg = topicStr + "/" + std::string(enabledState ? "true" : "false");
            data.publishHandler->publish(newMsg.c_str()); // send answer
        }

        if (data.notifyCallback) {
            data.notifyCallback(topic, tmp);
        }

        // --- relays.togglePort(0, enabledState);
        Serial.print("switch '");
        Serial.print(topic);
        Serial.print("' set to ");
        Serial.println(enabledState ? "ON" : "OFF");
    }
}
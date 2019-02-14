#include "MqttClient.h"
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
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

    Serial.print("[mqtt] connecting to MQTT server... ");

    int8_t  ret;
    uint8_t retries = MQTT_RECONNECT_RETRIES;
    while ((ret = mqttClient->connect()) != 0) // connect will return 0 for connected
    {
        Serial.println(mqttClient->connectErrorString(ret));
        Serial.println("[mqtt] retrying client connection...");
        mqttClient->disconnect();
        if (--retries == 0) {
            break;
        }
        delay(MQTT_TIMEOUT);
    }
    bool online = connected();
    Serial.println(online ? "[mqtt] client connected successfully" : "[mqtt] client connection failed!");
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
 * Register given MQTT topic (the MQTT publish path name) to publish (send) MQTT messages
 *
 * @param topicName - string with short name for given MQTT publish topic
 * @param mqttPath  - string with full MQTT publish topic name in format "maintopic/topic/subtopic"
 * 
 * @return false when topic already exists and on any error
 */
bool MqttClient::createPublishTopic(const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType)
{
    return createMqttTopic(publishTopics, topicName, mqttPath, topicType, false);
}

/**
 * Removes given publish topic
 */
bool MqttClient::removePublishTopic(const std::string& topicName)
{
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("[mqtt] error: removePublishTopic failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    publishTopics.erase(topicName);
    return true;
}

/**
 * Register given MQTT topic (the MQTT subscribe path name) to listen for incoming MQTT messages
 *
 * @param topicName - string with short name for given MQTT subscribe topic
 * @param mqttPath  - string with full MQTT subscribe topic name in format "maintopic/topic/subtopic"
 * 
 * @return false when topic already exists and on any error
 */
bool MqttClient::createSubscribeTopic(const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType)
{
    return createMqttTopic(subscribeTopics, topicName, mqttPath, topicType, true);
}

/**
 * Removes given subscription topic
 */
bool MqttClient::removeSubscribeTopic(const std::string& topicName)
{
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("[mqtt] error: removePublishTopic failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    if (mqttClient && publishTopics.at(topicName).subscribeHandler) {
        mqttClient->unsubscribe(publishTopics.at(topicName).subscribeHandler.get());
    }
    subscribeTopics.erase(topicName);
    return true;
}

/**
 * Register given MQTT topic (the MQTT publish path name) with given short name
 *
 * @param topicList - target list of publish or subscribe topics to store results
 * @param topicName - string with short name for given MQTT publish topic
 * @param mqttPath  - string with full MQTT publish topic name in format "maintopic/topic/subtopic"
 * @param subscribe - toggles between publish and subscribe topics
 * 
 * @return false when topic already exists and on any error
 */
bool MqttClient::createMqttTopic(std::map<std::string, MqttTopicData>& topicList, const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType, bool subscribe)
{
    std::string mode = subscribe ? "subscribe" : "publish";
    if (topicList.find(topicName) != topicList.end() || topicName.length() < 1) {
        Serial.printf("[mqtt] error: create %s topic failed for topic '%s' - given topic name is already registered!\n", mode.c_str(), topicName.c_str());
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
        Serial.printf("[mqtt] error: invalid MqttTopicType given to create %s topic for topic '%s'!", mode.c_str(), topicName.c_str());
        return false;
    }

    MqttTopicData data;
    data.topicName = topicName;
    data.pathName  = stringReplaceAll(std::string("/" + prefix + "/" + mqttPath), "//", "/");
    data.topicType = topicType;
    if (subscribe) {
        data.subscribeHandler = std::shared_ptr<Adafruit_MQTT_Subscribe>(new Adafruit_MQTT_Subscribe(mqttClient.get(), data.pathName.c_str()));
        mqttClient->subscribe(data.subscribeHandler.get());
    } else {
        data.publishHandler = std::shared_ptr<Adafruit_MQTT_Publish>(new Adafruit_MQTT_Publish(mqttClient.get(), data.pathName.c_str()));
    }

    topicList[topicName] = data;

    Serial.printf("[mqtt] created %s topic '%s' with MQTT path '%s'\n", mode.c_str(), data.topicName.c_str(), data.pathName.c_str());
}

/**
 * Assign the given callback function to the given topic, so it gets called on each publish() call
 * 
 * There is only one callback per topic possible. Any existing callback will be removed before setting the new one.
 */
bool MqttClient::addNotifyCallback(const std::string& topicName, NotifyCallbackFunction callback)
{
    if (subscribeTopics.find(topicName) == subscribeTopics.end()) {
        Serial.printf("[mqtt] error: addNotifyCallback failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    subscribeTopics.at(topicName).notifyCallback = callback;
    return true;
}

/**
 * Remove the callback function of givem topic
 */
bool MqttClient::removeNotifyCallback(const std::string& topicName)
{
    if (subscribeTopics.find(topicName) == subscribeTopics.end()) {
        Serial.printf("[mqtt] error: removeNotifyCallback failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    subscribeTopics.at(topicName).notifyCallback = nullptr;
    return true;
}

/**
 * Return notify callback function for given topic or nullptr, if missing or on errors
 */
MqttClient::NotifyCallbackFunction MqttClient::notifyCallback(const std::string& topicName)
{
    if (subscribeTopics.find(topicName) == subscribeTopics.end()) {
        Serial.printf("[mqtt] error: notifyCallback failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return nullptr;
    }
    if (!subscribeTopics.at(topicName).notifyCallback) {
        Serial.printf("[mqtt] error: notifyCallback failed for topic '%s' - invalid callback function!\n", topicName.c_str());
        return nullptr;
    }
    return subscribeTopics.at(topicName).notifyCallback;
}

/**
 * Publish given message using the short name for the topic (not the full MQTT publish topic path name)
 */
bool MqttClient::publish(const std::string& topicName, const std::string& message)
{
    if (!connect()) {
        Serial.printf("[mqtt] error: publish failed for topic '%s' - connection failed!\n", topicName.c_str());
        return false;
    }
    if (publishTopics.find(topicName) == publishTopics.end()) {
        Serial.printf("[mqtt] error: publish failed for topic '%s' - given topic name is unknown!\n", topicName.c_str());
        return false;
    }
    if (!publishTopics.at(topicName).publishHandler) {
        Serial.printf("[mqtt] error: publish failed for topic '%s' - invalid publish handler object!\n", topicName.c_str());
        return false;
    }
    if (!publishTopics.at(topicName).publishHandler->publish(message.c_str())) {
        Serial.printf("[mqtt] error: publish failed for topic '%s' - error on sending message '%s'\n", topicName.c_str(), message.c_str());
        return false;
    }
    return true;
}

/**
 * Wait for incoming messages and check if they are for subscribed topics 
 * 
 * @param timeout  - polling timeout in milliseconds
 * @return  true for received messages, false for wait without incoming packets
 */
bool MqttClient::waitForMessages(int timeout)
{
    if (!connect()) {
        Serial.printf("[mqtt] error: wait for messages failed - client not connected!\n");
        return false;
    }

    Adafruit_MQTT_Subscribe* subscription;
    while ((subscription = mqttClient->readSubscription(timeout))) {
        Serial.printf("[mqtt] received incoming messages\n");

        for (auto& pair : subscribeTopics) {
            if (pair.second.subscribeHandler == nullptr) {
                break;
            }
            std::shared_ptr<Adafruit_MQTT_Subscribe> handler = pair.second.subscribeHandler;
            if (handler.get() == subscription) {
                Serial.printf("[mqtt] received message for topic %s\n", pair.first.c_str());
                incomingMessageCallback(pair.second, (const char*)handler->lastread);
                return true;
            }
        }
    }
    return false;
}

/**
 * Handle incoming MQTT messages
 */
void MqttClient::incomingMessageCallback(MqttTopicData& data, const char* lastRead)
{
    bool enabledState = (std::string(lastRead) == "true");

    Serial.printf("[mqtt] message for topic '%s' arrived: '%s'\n", data.topicName.c_str(), lastRead);

    if (data.publishHandler) {
        std::string newMsg = std::string(enabledState ? "true" : "false");
        data.publishHandler->publish(newMsg.c_str()); // send answer
    }

    if (data.notifyCallback) {
        Serial.printf("[mqtt] calling notity function for topic '%s'\n", data.topicName.c_str());
        data.notifyCallback(data.topicName, lastRead);
    }
}
#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

class WiFiClient;
class Adafruit_MQTT_Publish;
class Adafruit_MQTT_Subscribe;
class Adafruit_MQTT_Client;

// WLAN + MQTT settings
#include "secrets.h"

#include <Arduino.h>
#include <functional>
#include <map>
#include <memory>
#include <string>

std::string stringReplaceAll(const std::string& str, const std::string searchText, const std::string replaceText);

/**
 * MQTT client class
 * 
 * Send (publish) adhoc messages and receive (via callback handler) subscribed Mqtt Topics.
 * 
 * Depending on the MqttTopicTypes set for createPublishTopic() the topics get predefined prefixes and topic path names:
 * 
 * @li SENSOR: Topic gets prefix "/sensor/" and should be created as followed:  /sensor/[building]/[room]/[sensorname]
 * @li SWITCH: 
 *      Topic gets prefix "/switch/" and should be created as followed:  /switch/[building]/[room]/[switchname]
 *      The topic name holds the status "true" for switch enabled or "false" for disabled.
 *      The availability of the switch is sent to MQTT Broker with topic: /switch/[building]/[room]/[switchname]/available.
 *      The switch can be toggled by receiving a MQTT message of format: /switch/[building]/[room]/[switchname]/set
 */
class MqttClient
{
public:
    MqttClient(WiFiClient* client, const char* serverHost, int serverPort, const char* userName, const char* password);

    bool connected(void);
    bool connect(void);
    bool disconnect(void);

    enum MqttTopicTypes
    {
        UNKNOWN = 0,
        SENSOR  = 10,
        SWITCH  = 20,
        COMMAND = 30,
        STATUS  = 40,
        HEARTBEAT = 50
    };

    typedef std::function<bool(std::string topic, std::string message)> NotifyCallbackFunction;

    bool createPublishTopic(const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType = MqttTopicTypes::SENSOR);
    bool removePublishTopic(const std::string& topicName);

    bool createSubscribeTopic(const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType = MqttTopicTypes::SENSOR);
    bool removeSubscribeTopic(const std::string& topicName);

    bool addNotifyCallback(const std::string& topicName, NotifyCallbackFunction callback);
    bool removeNotifyCallback(const std::string& topicName);

    NotifyCallbackFunction notifyCallback(const std::string& topicName);

    bool publish(const std::string& topicName, const std::string& message);

    bool waitForMessages(int timeout = 200);

protected:
    /**
     * MQTT topic properties
     */
    struct MqttTopicData
    {
        MqttTopicData() {}
        MqttTopicData(const MqttTopicData& ref) :
            topicName(ref.topicName),
            pathName(ref.pathName),
            topicType(ref.topicType),
            publishHandler(ref.publishHandler),
            subscribeHandler(ref.subscribeHandler),
            notifyCallback(ref.notifyCallback)
        {
        }

        std::string                              topicName;
        std::string                              pathName;
        MqttTopicTypes                           topicType;
        std::shared_ptr<Adafruit_MQTT_Publish>   publishHandler;
        std::shared_ptr<Adafruit_MQTT_Subscribe> subscribeHandler;
        NotifyCallbackFunction                   notifyCallback; // used to notify about incoming MQTT messages and/or state changes
    };

    bool createMqttTopic(std::map<std::string, MqttTopicData>& topicList, const std::string& topicName, const std::string& mqttPath, MqttTopicTypes topicType, bool subscribe);

    void incomingMessageCallback(MqttTopicData& data, const char* lastRead);

private:
    std::shared_ptr<Adafruit_MQTT_Client> mqttClient;
    std::map<std::string, MqttTopicData>  publishTopics;
    std::map<std::string, MqttTopicData>  subscribeTopics;
};

#endif // MQTTCLIENT_H
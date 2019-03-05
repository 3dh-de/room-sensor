/**@file 
 * Temperature sensor DS18b20 based on base class TemperatureSensor
 */
#ifndef SENSORDS18B20_H
#define SENSORDS18B20_H

#include "TemperatureSensor.h"

#include <Arduino.h>
#ifndef ARDUINO
#define ARDUINO 150
#endif

#include <DallasTemperature.h>
#include <OneWire.h>
#include <assert.h>
#include <list>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <string>

#define ONEWIRE_IN D4 // what pin the DS18b20 is connected to

/**
 * Read and display temperature via Dallas DS18b20 1wire sensor
 * 
 * Manages all Dallas DS18b20 temperature sensors connected to 1wire bus.
 */
class SensorDS18B20 : public TemperatureSensor
{
public:
    SensorDS18B20(int pin = ONEWIRE_IN, float temperatureOffset = 0.0);

    virtual float temperature(void);
    virtual float temperature(const std::string& name);

    bool        setCurrentSensor(const std::string& name);
    std::string currentSensor(void) const;
    bool        registerSensor(DeviceAddress address, std::string name);
    bool        unregisterSensor(DeviceAddress address, std::string name);

    /**
     * Attributes of a single DS18b20 one-wire sensor device
     * 
     * @c index  - a numerical increment showing the order the sensors are found on the one wire bus and can be arbitrary
     * @c addess - the unique hardware sensor address of a DS18B20 device
     * @c name   - a user defined name to identify the sensor - by default constructed as "sensor[index]", e.g. "sensor0"
     */
    struct SensorData
    {
        SensorData(void) :
            index(-1),
            connected(false),
            lastTemperature(NAN)
        {}

        SensorData(int deviceIndex, DeviceAddress deviceAddress, const std::string deviceName = "") :
            index(deviceIndex),
            name(deviceName),
            connected(true),
            lastTemperature(NAN)
        {
            for (int i = 0; i < 8; ++i) {
                this->address[i] = deviceAddress[i];
            }

            if (deviceName.empty()) {
                /*
                std::ostringstream str;
                str << "sensor" << index;
                name = str.str();
                */
                char tmp[255];
                name = std::string("sensor") + std::string(itoa(index, tmp, 10));
            }
        }

        SensorData(const SensorData& right) :
            index(right.index),
            name(right.name),
            connected(right.connected),
            lastTemperature(right.lastTemperature)
        {
            for (int i = 0; i < 8; ++i) {
                this->address[i] = right.address[i];
            }
        }

        bool isEqual(DeviceAddress deviceAddress)
        {
            for (int i = 0; i < 8; ++i) {
                if (this->address[i] != deviceAddress[i]) {
                    return false;
                }
            }
            return true;
        }

        bool operator<(const SensorData& right)
        {
            return index < right.index;
        }

        bool operator>(const SensorData& right)
        {
            return index > right.index;
        }

        int           index;
        DeviceAddress address;
        std::string   name;
        bool          connected;
        float         lastTemperature;
    };

    std::list<SensorData> sensorsAvailable(void);
    std::list<SensorData> sensorsRegistered(void);

protected:
    bool searchSensors(void);
    bool readSensorTemperature(SensorData& data, float offset = 0.0);

private:
    typedef uint8_t                   DeviceScratchPad[9]; // 9 data bytes of one-wire device
    DallasTemperature                 sensors;             // API for 1wire bus
    bool                              sensorsInitialized = false; // was sensors.begin() called already?
    std::map<std::string, SensorData> registeredSensors;   // list of registered sensors
    std::string                       currentSensorName;   // name of registered default sensor
    float                             temperatureOffset;   // offset to normalize temperature values i.e. in cause of shifted sensor values
};

/**
 * Unit test for SensorDS18B20 class
 */
class TestSensorDS18B20 : public TestTemperatureSensor
{
public:
    virtual bool runTests();

private:
    SensorDS18B20 sensor;
};

#endif // SENSORDS18B20_H
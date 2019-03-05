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
#include <list>
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
    std::string setCurrentSensor(void) const;
    bool        registerSensor(DeviceAddress address, std::string name);
    bool        unregisterSensor(DeviceAddress address, std::string name);

    /**
     * Attributes of a single DS18b20 one-wire sensor device
     */
    struct SensorData
    {
        DeviceAddress address;
        std::string   name;
        bool          initialized;
        float         lastTemperature;
    };

    std::list<SensorData> sensorsAvailable(void);
    std::list<SensorData> sensorsRegistered(void);

protected:
    bool searchSensors(void);
    bool readSensorTemperature(const std::string& name, float offset = 0.0);

private:
    DallasTemperature     sensors;           // API for 1wire bus
    std::list<SensorData> registeredSensors; // list of registered sensors
    float                 temperatureOffset; // offset to normalize temperature values i.e. in cause of shifted sensor values
};

#endif // SENSORDS18B20_H
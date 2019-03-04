#ifndef SENSORDS18B20_H
#define SENSORDS18B20_H

/** 
 * Temperature sensor DS18b20 based on base class TemperatureSensor
 */

#include "TemperatureSensor.h"

#include <Arduino.h>
#ifndef ARDUINO
#define ARDUINO 150
#endif

#include <DallasTemperature.h>
#include <OneWire.h>

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

protected:
    bool readSensorTemperature(float& temperature, float offset = 0.0);

private:
    DallasTemperature sensor;                 // API for 1wire bus
    float             temperatureOffsetValue; // offset to normalize temperature values i.e. in cause of shifted sensor values
};

#endif // SENSORDS18B20_H
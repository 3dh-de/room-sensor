#ifndef SENSORDHT_H
#define SENSORDHT_H

/** 
 * Temperature and humidity sensor DHT11/DHT22/AM2302 based on base class TemperatureSensor
 */

#include "TemperatureSensor.h"
#include <Arduino.h>
#include <DHT.h>

#define DHT_IN D5 // what pin the DHT is connected to
#define DHT_TEMP_OFFSET -2.7

/**
 * Read and display temperature via DHT11 or DHT22/AM2302 sensor
 */
class SensorDHT : public TemperatureSensor
{
public:
    SensorDHT(int pin = DHT_IN, int model = DHT22, float temperatureOffset = 0.0);

    virtual float temperature(void);
    virtual float humidity(void);

protected:
    bool readSensorTemperature(float& temperature, float offset = 0.0);
    bool readSensorHumidity(float& humidity);

private:
    DHT   sensor;                 // API for AM23xx and DHTxx sensors
    float temperatureOffsetValue; // offset to normalize temperature values i.e. in cause of shifted sensor values
};

#endif // SENSORDHT_H
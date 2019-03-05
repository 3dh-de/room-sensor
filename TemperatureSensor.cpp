#include "TemperatureSensor.h"
#include <assert.h>
#include <math.h>

void TemperatureSensor::clearTemperature(void)
{
    temperatureInitialized = false;
    temperatureValue       = NAN;
}

void TemperatureSensor::clearHumidity(void)
{
    humidityInitialized = false;
    humidityValue       = NAN;
}

void TemperatureSensor::setTemperatureUnit(TemperatureUnits unit)
{
    if (unit == temperatureUnitValue) {
        return;
    }
    clearTemperature(); // reset temperature value to prevent bad values on unit changes
}

void TemperatureSensor::setTemperature(float degrees)
{
    temperatureValue       = degrees;
    temperatureInitialized = true;
}

void TemperatureSensor::setHumidity(float percent)
{
    humidityValue       = percent;
    humidityInitialized = true;
}

/**
 * Basic unit tests for interface methods
 */
bool TestTemperatureSensor::runTests()
{
    assert(std::isnan(sensor.temperature()) == true);
    assert(sensor.isTemperatureValid() == false);
    assert(sensor.isHumidityValid() == false);

    sensor.setTemperatureUnit(TemperatureSensor::CELSIUS_DEGREES);
    assert(sensor.temperatureUnit() == TemperatureSensor::CELSIUS_DEGREES);

    sensor.setTemperature(25.0);
    assert(sensor.isTemperatureValid() == true);
    assert(sensor.temperature() == 25.0);

    sensor.setTemperatureUnit(TemperatureSensor::FAHRENHEIT_DEGREES);
    assert(sensor.temperatureUnit() == TemperatureSensor::CELSIUS_DEGREES);
    assert(sensor.isTemperatureValid() == false);
    assert(sensor.temperature() != 25.0);

    sensor.setHumidity(60.0);
    assert(sensor.isHumidityValid() == true);
    assert(sensor.humidity() == 60.0);
}
#ifndef TEMPERATURESENSOR_H
#define TEMPERATURESENSOR_H

#include <cmath>

/**
 * Base class for temperature sensor
 * 
 * Invalid temperature and humidity values are returned as NaN floats (not a number) -
 * which can be checked by isTemperatureValid() and isHumidityValid() or manually by
 * calling std::isnan(temperature).
 */
class TemperatureSensor
{
public:
    enum TemperatureUnits
    {
        CELSIUS_DEGREES    = 1,
        FAHRENHEIT_DEGREES = 2
    };

    virtual bool isTemperatureValid(void) const { return temperatureInitialized && !std::isnan(temperatureValue); }
    virtual bool isHumidityValid(void) const { return humidityInitialized && !std::isnan(humidityValue); }

    virtual void clearTemperature(void);
    virtual void clearHumidity(void);

    TemperatureUnits temperatureUnit(void) { return temperatureUnitValue; }
    virtual float    temperature(void) { return temperatureValue; }
    virtual float    humidity(void) { return humidityValue; }

    virtual void setTemperatureUnit(TemperatureUnits unit);
    virtual void setTemperature(float degrees);
    virtual void setHumidity(float percent);

    virtual float fahrenheitToCelsius(float fahrenheit) const { return (fahrenheit - 32) * 5 / 9; }
    virtual float celsiusToFahrenheit(float celsius) const { return (celsius * 9) / 5 + 32; }

private:
    TemperatureUnits temperatureUnitValue   = CELSIUS_DEGREES; // can be set to CELSIUS_DEGREES or FAHRENHEIT_DEGREES
    float            temperatureValue       = NAN;             // last temperature value in degrees of unit temperatureUnit
    float            humidityValue          = NAN;             // last humidity in percent
    bool             temperatureInitialized = false;           // if FALSE the temperature values are invalid
    bool             humidityInitialized    = false;           // if FALSE the humidity values are invalid
};

/**
 * Simple unit test for base class
 */
class TestTemperatureSensor
{
public:
    virtual bool runTests();

private:
    TemperatureSensor sensor;
};

#endif // TEMPERATURESENSOR_H
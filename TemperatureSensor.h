#ifndef TEMPERATURESENSOR_H
#define TEMPERATURESENSOR_H

/**
 * Base class for temperature sensor
 */
class TemperatureSensor
{
public:
    enum TemperatureUnits {
        CELSIUS_DEGREES    = 1,
        FAHRENHEIT_DEGREES = 2
    };

    virtual bool isTemperatureValid(void) const { return temperatureInitialized; }
    virtual bool isHumidityValid(void) const { return humidityInitialized; }

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
    float            temperatureValue       = 0.0;             // last temperature value in degrees of unit temperatureUnit
    float            humidityValue          = 0.0;             // last humidity in percent
    bool             temperatureInitialized = false;           // if FALSE the temperature values are invalid
    bool             humidityInitialized    = false;           // if FALSE the humidity values are invalid
};

/**
 * Simple unit test for base class
 */
class TestTemperatureSensor
{
public:
    bool runTests();

private:
    TemperatureSensor sensor;
};

#endif // TEMPERATURESENSOR_H
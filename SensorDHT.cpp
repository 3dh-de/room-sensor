/** 
 * Temperature and humidity sensor DHT11/DHT22/AM2302 based on base class TemperatureSensor
 */

#include "SensorDHT.h"

/**
 * Constructor with default pin setting
 */
SensorDHT::SensorDHT(int pin, int model, float temperatureOffset) :
    sensor(pin, model),
    temperatureOffsetValue(temperatureOffset)
{
    setTemperatureUnit(CELSIUS_DEGREES); // sensor data are in celsius degrees by default
}

/**
 * Read current sensor value and return its current value on success, otherwise return last value
 */
float SensorDHT::temperature(void)
{
    float value = 0.0;
    if (readSensorTemperature(value, temperatureOffsetValue) == false) {
        clearTemperature();
    } else {
        if (temperatureUnit() == FAHRENHEIT_DEGREES) {
            setTemperature(celsiusToFahrenheit(value));
        } else {
            setTemperature(value);
        }
    }
    return TemperatureSensor::temperature();
}

/**
     * Read current sensor value and return its current value on success, otherwise return last value
     */
float SensorDHT::humidity(void)
{
    float value = 0.0;
    if (readSensorHumidity(value) == false) {
        clearHumidity();
    } else {
        setHumidity(value);
    }
    return TemperatureSensor::humidity();
}

/**
     * Read sensor value for temperature in celsius degrees and return FALSE on any error
     */
bool SensorDHT::readSensorTemperature(float& temperature, float offset)
{
    float sensorValue = sensor.readTemperature();

    if (isnan(sensorValue)) {
        Serial.println("Failed to read from DHT sensor!");
        return false;
    }
    Serial.print("Temperature: ");
    Serial.print(sensorValue);
    Serial.print(" °C (raw)");

    sensorValue += offset;
    Serial.print(sensorValue);
    Serial.println(" °C (corrected)");

    temperature = sensorValue;
    return true;
}

/**
    * Read sensor value for humidity in percent and return FALSE on any error
    */
bool SensorDHT::readSensorHumidity(float& humidity)
{
    float sensorValue = sensor.readHumidity();

    if (isnan(sensorValue)) {
        Serial.println("Failed to read from DHT sensor!");
        return false;
    }
    Serial.print("Humidity: ");
    Serial.print(sensorValue);
    Serial.println(" %");

    humidity = sensorValue;
    return true;
}

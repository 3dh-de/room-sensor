/**@file 
 * Temperature sensor DS18b20 based on base class TemperatureSensor
 */
#include "SensorDS18B20.h"

#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONEWIRE_IN);

/**
 * Initialize Dallas sensor API and temperature offset in Celsius degrees
 */
SensorDS18B20::SensorDS18B20(int pin, float temperatureOffset) :
    sensors(&oneWire),
    sensorsInitialized(false),
    temperatureOffset(temperatureOffset)
{
}

/**
 * Return current temperature of current sensor or of first one, if no specific sensor was chosen
 * 
 * @return NAN on any error, otherwise last temperature value of current or first sensor.
 */
float SensorDS18B20::temperature(void)
{
    if (registeredSensors.empty()) {
        return NAN;
    }

    // try to find current sensor name otherwise take first registered sensor
    std::map<std::string, SensorData>::iterator it = registeredSensors.find(currentSensorName);
    if (it == registeredSensors.end()) {
        it = registeredSensors.begin();
    }

    return temperature(it->first);
}

/**
 * Read and return current temperature value from given sensor
 * 
 * @return NAN on any error, otherwise last temperature value read form given sensor
 */
float SensorDS18B20::temperature(const std::string& name)
{
    std::map<std::string, SensorData>::iterator it = registeredSensors.find(name);
    if (it == registeredSensors.end()) {
        return NAN;
    }

    SensorData& data = it->second;
    if (!readSensorTemperature(data)) {
        return NAN;
    }

    return data.lastTemperature;
}

/**
 * Set name of current sensor
 * 
 * @return FALSE on any error, e.g. if no sensor of given name was found
 */
bool SensorDS18B20::setCurrentSensor(const std::string& name)
{
    std::map<std::string, SensorData>::iterator it = registeredSensors.find(name);
    if (it == registeredSensors.end()) {
        return false;
    }

    currentSensorName = name;
    return true;
}

/**
 * Return name of current default sensor or empty string, if none was set
 */
std::string SensorDS18B20::currentSensor(void) const
{
    return currentSensorName;
}

/**
 * Assign given name to given sensor hardware address - works for 
 */
bool SensorDS18B20::registerSensor(DeviceAddress address, std::string name)
{
    if (name.empty()) {
        return false;
    }

    std::map<std::string, SensorData>::iterator it = registeredSensors.begin();
    while (it != registeredSensors.end()) {
        if (it->second.isEqual(address)) {
            SensorData data = it->second;
            registeredSensors.erase(it);
            registeredSensors[name] = data;
            return true;
        }
        ++it;
    }

    registeredSensors[name] = SensorData(0, address, name);
    return true;
}

/**
 * Remove given sensor assignment, if the sensor is still online it gets a generic name "sensor[index]"
 */
bool SensorDS18B20::unregisterSensor(DeviceAddress address, std::string name)
{
    std::map<std::string, SensorData>::iterator it = registeredSensors.find(name);
    if (it == registeredSensors.end()) {
        return false;
    }
    registeredSensors.erase(it);
    return true;
}

/**
 * Search and return currently available DS18B20 devices on the one-wire bus
 */
std::list<SensorDS18B20::SensorData> SensorDS18B20::sensorsAvailable(void)
{
    searchSensors();

    std::list<SensorData>           sensorList = sensorsRegistered();
    std::list<SensorData>::iterator it         = sensorList.begin();

    while (it != sensorList.end()) {
        DeviceScratchPad tmp;
        bool             connected = sensors.isConnected(it->address, tmp);

        Serial.printf("[ds18b20] device: %s address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X state: %s\n", it->name.c_str(),
                      it->address[0], it->address[1], it->address[2], it->address[3], it->address[4], it->address[5], it->address[6], it->address[7],
                      connected ? "ONLINE" : "OFFLINE. Skipping this sensor.");

        if (connected) {
            it->connected = connected;
        } else {
            sensorList.erase(it);
        }
        ++it;
    }
    return sensorList;
}

/**
 * Return a list of all registered sensors (which were online before but without checking if they are still connected)
 */
std::list<SensorDS18B20::SensorData> SensorDS18B20::sensorsRegistered(void)
{
    std::list<SensorData>                             sensorList;
    std::map<std::string, SensorData>::const_iterator it = registeredSensors.begin();
    while (it != registeredSensors.end()) {
        sensorList.push_back(it->second);
        ++it;
    }
    return sensorList;
}

/**
 * Scan for devices and update the registered sensors list
 */
bool SensorDS18B20::searchSensors(void)
{
    if (!sensorsInitialized) {
        sensors.begin();
        sensorsInitialized = true;

        Serial.printf("[ds18b20] parasite power is: %s\n", sensors.isParasitePowerMode() ? "ON" : "OFF");
    }

    Serial.printf("[ds18b20] searching available devices (current count: %d)...\n", sensors.getDeviceCount());

    std::map<std::string, SensorData>::iterator it = registeredSensors.begin();
    std::list<SensorData>                       newSensorList;
    SensorData                                  newDevice;
    DeviceAddress                               newAddress;
    int                                         newIndex = -1;

    // first mark all previously registered sensors as offline
    while (it != registeredSensors.end()) {
        it->second.index     = -1;
        it->second.connected = false;
        ++it;
    }

    // search for new devices and check each address against registered sensors
    while (sensors.getAddress(newAddress, ++newIndex)) {
        newDevice = SensorData(newIndex, newAddress);
        newSensorList.push_back(newDevice);

        sensors.setResolution(newAddress, TEMPERATURE_PRECISION);

        it                            = registeredSensors.begin();
        bool addressAlreadyRegistered = false;

        while (it != registeredSensors.end()) {
            // mark each matching registered sensor as "connected"
            if (it->second.isEqual(newAddress)) {
                addressAlreadyRegistered = true;
                it->second.index         = newIndex; // update the index
                it->second.connected     = true;
                if (it->second.name.find("sensor") != std::string::npos) {
                    char tmp[255];
                    it->second.name = std::string("sensor") + std::string(itoa(newIndex, tmp, 10));
                } else {
                    newDevice.name = it->second.name;
                }
                break;
            }
            ++it;
        }
        // add new sensor missing in previously registered sensors list
        if (!addressAlreadyRegistered) {
            registeredSensors[newDevice.name] = newDevice;
        }

        Serial.printf("[ds18b20] device: %s address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X state: %s\n", newDevice.name.c_str(),
                      newAddress[0], newAddress[1], newAddress[2], newAddress[3], newAddress[4], newAddress[5], newAddress[6], newAddress[7],
                      addressAlreadyRegistered ? "already registered" : "new device");
    }

    return true;
}

/**
 * Read current temperature value from given sensor, corrected by given offset
 * 
 * @return FALSE on any error, otherwise TRUE
 */
bool SensorDS18B20::readSensorTemperature(SensorData& data, float offset)
{
    sensors.requestTemperatures();

    float temperature = sensors.getTempC(data.address);
    if (temperature == DEVICE_DISCONNECTED_C) {
        Serial.printf("[ds18b20] failed to read temperature value of device: %s\n", data.name.c_str());
        return false;
    }

    data.lastTemperature = temperature;
    Serial.printf("[ds18b20] device: %s  current temperature: %02.1f °C\n", data.name.c_str(), temperature);

    return true;
}

/**
 * Unit tests for base class and public interfaces of SensorDS18B20
 */
bool TestSensorDS18B20::runTests(void)
{
    assert(sensor.currentSensor().empty());
    assert(std::isnan(sensor.temperature()));
    assert(std::isnan(sensor.humidity()));
    assert(std::isnan(sensor.temperature("sensorXYZ***Just*a*test!")));

    return TestTemperatureSensor::runTests();
}

#include "Relays.h"

RelayPorts::RelayPorts(const std::vector<int> ports) :
    outputPorts(ports),
    currentPortNumber(-1)
{
    for (int i = 0; i < outputPorts.size(); ++i) {
        pinMode(outputPorts[i], OUTPUT);
        digitalWrite(outputPorts[i], HIGH);
    }
}

bool RelayPorts::isValidPort(int port)
{
    if (port < 0 || port >= outputPorts.size()) {
        return false;
    }
    return true;
}

int RelayPorts::setCurrentPort(int port)
{
    if (isValidPort(port)) {
        currentPortNumber = port;
    } else {
        currentPortNumber = 0;
    }
    return currentPortNumber;
}

int RelayPorts::togglePort(bool state)
{
    return togglePort(currentPortNumber, state);
}

int RelayPorts::togglePort(int port, bool state)
{
    if (!isValidPort(port)) {
        return -1;
    }
    digitalWrite(outputPorts[port], state ? LOW : HIGH);
}

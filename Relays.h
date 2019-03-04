#ifndef RELAYS_H
#define RELAYS_H

#include <Arduino.h>
#include <vector>

/**
 * Toggle digital outputs for digital relays
 */
class RelayPorts
{
public:
    RelayPorts(const std::vector<int> outputPorts = std::vector<int>());

    bool isValidPort(int port);
    int  setCurrentPort(int port);
    int  currentPort(void) const { return currentPortNumber; }
    int  availableRelays(void) const { outputPorts.size(); }

    int togglePort(bool state);
    int togglePort(int port, bool state);

private:
    std::vector<int> outputPorts;
    int              currentPortNumber;
};

#endif // RELAYS_H
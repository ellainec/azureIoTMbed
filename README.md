## About
This repo was originally cloned from the example implementation over at[Avnet/azure-iot-mbed-client](https://github.com/Avnet/azure-iot-mbed-client)

The original code had no implementation for Power Saving Mode and the devices only lasted for ~30 hours with a 6700mAh battery. Additions to this code is an attempt to get BG96 PSM working. In addition to building on the BG96 library, the code was reorganized to tear down and create a new connection handler to Microsoft Azure after every message.

## Work In Progress
BG96 goes into PSM successfully, however the program does not continue to send telemetry as expected. Modem does not wake up, and attempts to drive PWR KEY results in a Hard Fault. Further effort is required.

# esparrag32
## A common library for esp32 projects
### introduction

This project intends to create a common library to be the basic layer of esp32 projects.
it is written in modern c++ 17, uses esp-idf framework and compiled and built with platformio.
This library strives to minimum dynamic allocation and virtual inheritence as they are considered harmful for embedded environments.
The only dynamic allocation in the project is the one that is being used in esp-idf framework. The only virtual inheritance is from etlcpp (dependency). 

### core components
1. **Database** - This is deprecated and will be replaced by a settings module
2. **NVS** - Non volatile storage (flash storage).
   * Each instance of this class is a namespace in flash.
   * Supports key value setters/getters.
   * Erase all namespace.
3. **GPIO** - Classes for general digital gpios. Work in progres...
   * Should provide simple and clear way to control gpios
   * Analog and pwm gpios are to be decided later...
4. **Buttons** - Buttons allow to register with callback to several type of events
   * Fast press and fast release
   * Timed presses and releases, more than one (configurable in code)
   * Two button presses and releases that override the one button behaviour
   * Buttons behaviour can change in runtime according to the user's use of the module
5. **I2C** - *TODO...*
   
### networking
1. **Wifi driver** - A wifi class to provide smart wifi provisioning. AP and STA.
2. **Http server** - Http server
   * Allow subscribing to uri's and methods with a callback.
   * can parse json and html(key, value) body.
   * Uses callbacks with Request and Response structs.
3. **MDNS** - *TODO...*
4. **MQTT** - *TODO...*

#### other utilities and future ideas
  * SNTP - Sync time with the internet.
  * Logging to flash/cloud/udp. Different log types...


## Usage:
...


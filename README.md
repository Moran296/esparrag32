# esparrag32
## A common library for esp32 projects
### introduction

This project intends to create a common library to be the basic layer of esp32 projects.
it is written in modern c++ 17, uses esp-idf framework and compiled and built with platformio.
This library strives to minimum dynamic allocation and virtual inheritence as they are considered harmful for embedded environments.
The only dynamic allocation in the project is the one that is being used in esp-idf framework. The only virtual inheritance is from etlcpp (dependency). 

### core components
1. **Database** - This is the heart of the system. A tuple class that holds heterogenous data, mainly system configurations and statuses. It is the main building stone for the library architecture. it allows:
   * *Persistency*: Data can be declared persistent and so it will be saved in flash and load on system start.
   * *PubSub*: Users can subscribe to certain data changes events in the database, and so respond to system changes and events 
   * *Type safety*: The database is programmed with c++17 compile time programming concepts and so the database strives to create compile time errors if it is used wrongfully. This includes type safety in getters and setters.
   * *Input validation*: Data in the database is validated at runtime. User can supply max, min values. 
   * *Additional flexibility* - Currently the database supports POD types, cstrings (currently with default size), and any class with comparison operators and few needed functions. Supports factory reset.
   - The only tradeoff is the size in flash is potentially big due to template metaprogramming creating invisible code..
2. **NVS** - Non volatile storage (flash storage).
   * Each instance of this class is a namespace in flash.
   * Supports key value setters/getters.
   * Erase all namespace.
3. **GPIO** - Classes for general digital gpios. Work in progres...
   * Should provide simple and clear way to control gpios
   * Analog and pwm gpios are to be decided later...
4. **I2C** - *TODO...*
   
### networking
1. **Wifi driver** - A wifi class to provide smart wifi provisioning. AP and STA.
2. **Http server** - Http server
   * Allow subscribing to uri's and methods with a callback.
   * can parse json and html(key, value) body.
   * Uses callbacks with Request and Response structs.
3. **MDNS** - *TODO...*
4. **MQTT** - *TODO...*

#### other utilities and future ideas
  * Buttons - to allow different button behaviours and functionalities..
  * SNTP - Sync time with the internet.
  * Logging to flash/cloud/udp. Different log types...


## Usage:
...


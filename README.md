# esparrag32

This is a library to run on esp32 as the basic layer to take care of data managing, wifi provisioning, http, i2c, gpio, time units etc..

It is written in c++17. Not using dynamic allocation/virtual function (except for the already provided esp-idf sdk and etl that may use some).

The project strives for loosley coupled architechture. Classes should generally not speak to each other. They should try to only update the main database with status and configuration,
and the database will invoke events upon this changes.

This is a work in progress.

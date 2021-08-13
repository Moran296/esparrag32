#ifndef ESPARRAG_DB__
#define ESPARRAG_DB__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <tuple>
#include "etl/bitset.h"    //dirty list
#include "etl/delegate.h"  // subscriber callback
#include "etl/vector.h"    //subscribers list
#include "esparrag_data.h" // data struct
#include "esparrag_nvs.h"  // Save and load from flash. remove and fine solution if generic

#define DB_PARAM_DIRTY_LIST(DATABASE) decltype(DATABASE)::dirty_list_t &&
#define DB_PARAM_CALLBACK(DATABASE) decltype(DATABASE)::db_change_event_cb

template <typename T>
constexpr bool isValidData = false;
// a valid database data must be of type Data (from "esparrag_data.h")
template <size_t ID, typename T>
constexpr bool isValidData<Data<ID, T>> = true;

template <class... DATA_TYPES>
class Database
{
    static_assert((isValidData<DATA_TYPES> && ...),
                  "data types for database must be of class Data");

public:
    static constexpr int DB_MAX_SUBSCRIBERS = 15;                  // max num of subscribers
    static constexpr int MAX_TIME_TO_COMMIT = pdMS_TO_TICKS(5000); // time to commit after change. otherwise assert.

    using dirty_list_t = etl::bitset<sizeof...(DATA_TYPES)>;                                                 //bitset of changed data members
    using db_change_event_cb = etl::delegate<void(dirty_list_t &&)>;                                         //callback to be run on publish
    using subscribers_list_t = etl::vector<etl::pair<dirty_list_t, db_change_event_cb>, DB_MAX_SUBSCRIBERS>; //list of subscribers. subscribed events bitset and callback to run

    /* CTOR -
        example:
        Database db("new_db",             min max default (persistent = true)
            Data<0, int>         (10,  20,   15),
            Data<1, char>        ('a', 'z', 'd', false),  ---> false meaning not persistent
            Data<2, const char *>("hello my friend"));

            Note: Data id's must start from 0, be unique, and climb up consistently.
            Note: default values must be valid (between min and max value)
            Note: name must be unique and shorter than NVS_KEY_NAME_MAX_SIZE - 1 (15 bytes)
    */
    Database(const char *name, DATA_TYPES... args);

    /*Set value to data member.
        return: true if change took place. false if (new value == old value) or new value is invalid
        example: db.Set<Data_id>(10); - will work if the data is any integral number (and value is legal for this data)
        example: db.Set<Data_id>("a new value"); - will work if the data is of string type (and string is shorter than string default len)
    */
    template <size_t ID, class TYPE>
    bool Set(TYPE val);

    /*Set values to data members.
        This function changes values to all data members supplied and then commits changes
        example:
            db.Set<1, 2, 3>(43, 'd', "is for door");
    */
    template <size_t... ID, class... TYPE>
    void Set(TYPE... val);

    /*Get data value
      example:
        int x = 0;
        example: db.Get<Data_id>(x); - will work if the data is an int;
        const char *p = nullptr;
        example: db.Get<Data_id>(p); - will work if the data is a str;
    */
    template <size_t ID, class TYPE>
    void Get(TYPE &val);

    /* Get several data members values togather.
    example:
        int result = 10;
        char charResult = 'a';
        const char *stringResult = nullptr;
        db.Get<0, 1, 2>(result, charRes, p);
    */
    template <size_t... ID, class... TYPE>
    void Get(TYPE &...val);

    /* Commit changes
        This function commits all previous changes made by set function.
        It first writes all changed data to flash,
        than publish the changes to all relevant subscribers using the bitset dirty_list_t

        Note: this function must be called after data changes has been made
    */
    void Commit();

    /* Restores all data members to default values.
       Note: This function erases all flash namespace and calls publish for changed data
    */
    void RestoreDefault();

    /* Subscribe to data changes
    Using this function, caller's callback will be invoked after relevant changes happend.

    example:
    **create a callback function**
    auto lambda = [](auto &&dirtyList)  // auto will be deduced as decltype(Database)::dirty_list_t&&
    {
        for (size_t i = 0; i < data_ID::NUM; i++)
        {
            if (dirtyList[i])
                LOG_INFO("data %d changed", i);
        }
    };

    **Use the subscribe with relevant data and the generic lambda:**
    Database.Subscribe<2, 3>(lambda); */

    template <size_t... DATA_ID>
    void Subscribe(db_change_event_cb callback);

private:
    std::tuple<DATA_TYPES...> m_data;
    NVS m_nvs;
    dirty_list_t m_dirtyList;
    subscribers_list_t m_subscribers;
    xTimerHandle m_commitTimer;
    xSemaphoreHandle m_mutex;

    template <class DATA>
    void writeData(DATA &data);
    void writeToFlash();

    template <class DATA>
    void readData(DATA &data);
    void readFromFlash();

    template <class DATA>
    void restoreData(DATA &data);

    void publish();

    static const char *getKey(int id);
    static void userFailedToCommit(TimerHandle_t timer);

    Database(Database const &) = delete;
    Database &operator=(Database const &) = delete;
};

#include "esparrag_database.hpp"

#endif
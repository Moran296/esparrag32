#include "esparrag_common.h" // ESPARRAG_ASSERT. remove if becomes generic
#include "esparrag_log.h"    // ESPARRAG LOG. remove if changed to generic
#include "lock.h"

template <class... DATA_TYPES>
Database<DATA_TYPES...>::Database(const char *name, DATA_TYPES... args) : m_data(args...), m_nvs(name)
{
    if (strlen(name) > NVS_KEY_NAME_MAX_SIZE - 1)
    {
        ESPARRAG_LOG_ERROR("Database name too long");
        ESPARRAG_ASSERT(false);
    }

    int checker = 0;
    auto lambda = [&checker](auto &data)
    {
        if (checker++ != data.m_id)
        {
            ESPARRAG_LOG_ERROR("Database initialization failed!");
            ESPARRAG_LOG_ERROR("Order of data member must start from 0. and be consistent.");
            ESPARRAG_LOG_ERROR("failed in data id %d", data.m_id);
            ESPARRAG_ASSERT(false);
        }
    };

    (lambda(args), ...);

    m_mutex = xSemaphoreCreateMutex();
    ESPARRAG_ASSERT(m_mutex != nullptr);
    m_commitTimer = xTimerCreate("commit timer", MAX_TIME_TO_COMMIT, pdFALSE, this, userFailedToCommit);
    ESPARRAG_ASSERT(m_commitTimer != nullptr);

    readFromFlash();
}

template <class... DATA_TYPES>
template <size_t ID, class TYPE>
bool Database<DATA_TYPES...>::Set(TYPE val)
{
    Lock lock(m_mutex);
    auto &data = std::get<Data<ID, TYPE>>(m_data);
    if (data == val)
    {
        return false;
    }

    if (!data.isValid(val))
    {
        ESPARRAG_LOG_WARNING("set data %d with invalid value", ID);
        return false;
    }

    data = val;
    m_dirtyList.set(ID);
    xTimerReset(m_commitTimer, portMAX_DELAY);
    return true;
}

template <class... DATA_TYPES>
template <size_t... ID, class... TYPE>
void Database<DATA_TYPES...>::Set(TYPE... val)
{
    (Set<ID>(val), ...);
    Commit();
}

template <class... DATA_TYPES>
template <size_t ID, class TYPE>
void Database<DATA_TYPES...>::Get(TYPE &val)
{
    Lock lock(m_mutex);
    auto &c = std::get<Data<ID, TYPE>>(m_data);
    val = c.m_val;
}

template <class... DATA_TYPES>
template <size_t... ID, class... TYPE>
void Database<DATA_TYPES...>::Get(TYPE &...val)
{
    (Get<ID, TYPE>(val), ...);
}

template <class... DATA_TYPES>
void Database<DATA_TYPES...>::Commit()
{
    xTimerStop(m_commitTimer, portMAX_DELAY);
    writeToFlash(); //write to flash is locked with mutex
    publish();      //publish is not (because a callback invoked from here also sets/gets data he might cause deadlock)
}

template <class... DATA_TYPES>
const char *Database<DATA_TYPES...>::getKey(int id)
{
    constexpr int KEY_SIZE = 6;
    static char key[KEY_SIZE] = {};
    memset(key, 0, KEY_SIZE);
    snprintf(key, KEY_SIZE, "%d", id);
    return key;
}

template <class... DATA_TYPES>
template <class DATA>
void Database<DATA_TYPES...>::writeData(DATA &data)
{
    if (!data.m_isPersistent)
        return;

    if (!m_dirtyList[data.m_id])
        return;

    const char *key = getKey(data.m_id);
    eResult res = m_nvs.SetBlob(key, &data, data.m_size);
    if (res != eResult::SUCCESS)
        ESPARRAG_LOG_ERROR("write flash failed. data %d", data.m_id);
}

template <class... DATA_TYPES>
void Database<DATA_TYPES...>::writeToFlash()
{
    Lock lock(m_mutex);
    std::apply([this](auto &...data)
               { (this->writeData(data), ...); },
               m_data);
}

#include <type_traits>

template <class... DATA_TYPES>
template <class DATA>
void Database<DATA_TYPES...>::readData(DATA &data)
{
    if (!data.m_isPersistent)
        return;

    const char *key = getKey(data.m_id);
    size_t actualLen = 0;
    eResult res = m_nvs.GetBlob(key, &data, data.m_size, actualLen);
    if (res != eResult::SUCCESS && res != eResult::ERROR_FLASH_NOT_FOUND)
        ESPARRAG_LOG_ERROR("problem reading flash");
    else if (actualLen > 0)
        ESPARRAG_LOG_INFO("data %d read from flash - %d bytes", data.m_id, actualLen);

    // protect from size of data change between versions. (This mechanism limits string data to have fixed size in flash)
    if (res == eResult::SUCCESS && actualLen != data.m_size)
    {
        ESPARRAG_LOG_WARNING("detected size conflict in data %d. restoring default", data.m_id);
        data = data.m_default;
    }
}

template <class... DATA_TYPES>
template <class DATA>
void Database<DATA_TYPES...>::restoreData(DATA &data)
{

    if (data == data.m_default)
        return;

    data = data.m_default;
    m_dirtyList.set(data.m_id);
}

template <class... DATA_TYPES>
void Database<DATA_TYPES...>::readFromFlash()
{
    std::apply([this](auto &...data)
               { (this->readData(data), ...); },
               m_data);
}

template <class... DATA_TYPES>
void Database<DATA_TYPES...>::RestoreDefault()
{
    {
        Lock lock(m_mutex);
        std::apply([this](auto &...data)
                   { (this->restoreData(data), ...); },
                   m_data);
    }

    //consider - instead of commit. delete entire flash namespace and then only publis
    // ESPARRAG_ASSERT(eResult::SUCCESS == m_nvs.Erase());
    // publish();
    Commit();
}

template <class... DATA_TYPES>
template <size_t... DATA_ID>
void Database<DATA_TYPES...>::Subscribe(db_change_event_cb callback)
{
    ESPARRAG_ASSERT(m_subscribers.size() != m_subscribers.capacity());
    dirty_list_t bitset;
    (bitset.set(DATA_ID), ...);
    m_subscribers.push_back(std::make_pair(bitset, callback));
}

template <class... DATA_TYPES>
void Database<DATA_TYPES...>::publish()
{
    for (auto &[list, cb] : m_subscribers)
    {
        dirty_list_t intersected_bits = m_dirtyList & list;
        if (intersected_bits.any())
        {
            cb(std::move(intersected_bits));
        }
    }

    m_dirtyList.reset();
}

template <class... DATA_TYPES>
void Database<DATA_TYPES...>::userFailedToCommit(TimerHandle_t timer)
{
    ESPARRAG_LOG_ERROR("User failed to commit after changes");
    ESPARRAG_ASSERT(false);
}
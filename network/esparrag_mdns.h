#ifndef MDNS_H__
#define MDNS_H__

#include "esparrag_common.h"
#include "esparrag_settings.h"
#include "esparrag_database.h"

class Mdns
{
public:
    static bool Init();
    static const char* FindBroker();

private:

};

#endif

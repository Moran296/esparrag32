#ifndef MDNS_H__
#define MDNS_H__

#include "esparrag_common.h"
#include "esparrag_result.h"

class Mdns
{
public:
    static bool Init();
    static EsparragResult<const char*> FindBroker();

};

#endif

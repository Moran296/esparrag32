#ifndef DNS_RESOLVER_H__
#define DNS_RESOLVER_H__

#include "esparrag_common.h"

class DnsResolver
{
public:
    eResult operator()(const char *uri, char *resultIP);
    //eResult operator()(const char *address, in_addr &res);

    //TODO add cache!
};

#endif
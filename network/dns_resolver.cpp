#include "dns_resolver.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "esparrag_log.h"
#include <netdb.h>

eResult DnsResolver::operator()(const char *uri, char *resultIP)
{
    ip_addr_t target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    struct addrinfo *res = NULL;
    int getaddrErr = getaddrinfo(uri, NULL, &hint, &res);
    if (getaddrErr != 0 || res == NULL)
    {
        ESPARRAG_LOG_ERROR("DNS lookup failed err=%d res=%p", getaddrErr, res);
        return eResult::ERROR_NOT_FOUND;
    }
    else
    {
        ESPARRAG_LOG_INFO("DNS lookup success");
    }

    if (res->ai_family != AF_INET)
    {
        ESPARRAG_LOG_ERROR("Dns lookup, address aint ipv4");
        return eResult::ERROR_NOT_FOUND;
    }

    struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    freeaddrinfo(res);

    strcpy(resultIP, inet_ntoa(target_addr.u_addr.ip4));
    return eResult::SUCCESS;
}

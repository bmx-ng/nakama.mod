#ifndef WSLAY_CONFIG_H
#define WSLAY_CONFIG_H

#if defined(_WIN32)
#define HAVE_WINSOCK2_H 1
#else
#define HAVE_ARPA_INET_H 1
#define HAVE_NETINET_IN_H 1
#endif

#endif


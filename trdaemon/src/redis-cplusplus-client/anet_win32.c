/**
 *  Implement the 3 functions used by redis-cpp boost impl.
 *  Use winsock2 as backend.
 *                                   hklo.tw@gmail.com
 */

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif

#include "anet_win32.h"

int anetTcpNoDelay(char *err, int fd)
{
    int yes = 1;
    SOCKET sock = (SOCKET)fd;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&yes, sizeof(yes)) == -1)
    {
        if (err) {
            sprintf_s(err, ANET_ERR_LEN, "%s", "Failed to do setsockopt(TCP_NODELAY).");
        }
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetTcpConnect(char *err, char *addr, int port)
{
    int retry;
    SOCKET sock;
    SOCKADDR_IN sockaddr;
//    struct hostent *ht;

    // Assume client side has already initiated winsock2.
    // Do initialize if this assumption failed.
    retry = 1;
do_retry:

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if ( INVALID_SOCKET == sock && WSANOTINITIALISED == WSAGetLastError()) {
        if (retry) {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(1, 0), &wsaData);
            retry = 0;
            goto do_retry;
        } else {
            if (err) {
                sprintf_s(err, ANET_ERR_LEN, "%s", "Failed to initialize WinSock2.");
            }
            return ANET_ERR;
        }
    }

	struct addrinfo *ga_result = NULL;
	struct addrinfo ga_hints = {0};
	ga_hints.ai_family = AF_INET;
	ga_hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *ptr = NULL;

	sockaddr.sin_family = 0;
	char ga_port[8];
	_itoa_s(port, ga_port, sizeof(ga_port), 10);

	int ga_err = getaddrinfo(addr, ga_port, &ga_hints, &ga_result);

	if (ga_err == 0)
	{
		for (ptr = ga_result; ptr != NULL; ptr = ptr->ai_next)
		{
			if (ptr->ai_family == AF_INET)
			{
				//sockaddr.sin_family = AF_INET;
				memcpy(&sockaddr, ptr->ai_addr, ptr->ai_addrlen);
				sockaddr.sin_port = htons(port);
				break;
			}
		}
	}

	if (!ptr)
	{
		if (err) {
			sprintf_s(err, ANET_ERR_LEN, "%s", "Failed to resolve target host.");
		}
		return ANET_ERR;
	}
    
    if (0 != connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen))
    {
        if (err) {
            sprintf_s(err, ANET_ERR_LEN, "%s", "Failed to connect.");
        }
        return ANET_ERR;
    }

    return (int)sock;
}

int anetWrite(int fd, char *buf, int count)
{
    SOCKET sock = (SOCKET)fd;
    int nwritten, totlen = 0;
    while(totlen != count) {
        nwritten = send(sock, buf, count-totlen, 0);
        if (nwritten == 0) return totlen;
        if (nwritten == SOCKET_ERROR) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

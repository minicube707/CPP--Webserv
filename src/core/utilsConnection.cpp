/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsConnection.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 14:55:37 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/05 19:23:30 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utilsConnection.hpp"
#include "execption.hpp"

#include <cstring>
#include <netdb.h>

void handleSigint(int sig)
{
    (void)sig;
    stop_webserv = 1;
}

void initializeSignal(void)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handleSigint);
}

int setNonblocking(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        throw ExecptionErrorFunction("fcntl");

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        throw ExecptionErrorFunction("fcntl");
    return 0;
}

sockaddr_in createSocketAddress(std::string ip_address, unsigned int port_number)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // If empty read all interfaces

    // build address from the config host
    const char *node = ip_address.empty() ? NULL : ip_address.c_str();
    if (getaddrinfo(node, NULL, &hints, &res) != 0 || res == NULL)
        throw ExecptionErrorFunction("getaddrinfo");

    sockaddr_in serverAddress = *reinterpret_cast<sockaddr_in *>(res->ai_addr);
    freeaddrinfo(res);

    serverAddress.sin_port = htons(port_number);
    return serverAddress;
}

int createServerSocket(std::string ip_address, unsigned int port_number, unsigned int max_client)
{
    int serverSocket;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw ExecptionErrorFunction("socket");

    try
    {
        sockaddr_in serverAddress = createSocketAddress(ip_address, port_number);

        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
            throw ExecptionErrorFunction("setsockopt");

        if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
            throw ExecptionErrorFunction("bind");

        if (listen(serverSocket, max_client) == -1)
            throw ExecptionErrorFunction("listen");
    }
    catch (...)
    {
        close(serverSocket);
        throw;
    }

    return serverSocket;
}

void removeFdFromEvent(EventData *eventData, int epoll_webserv)
{
    epoll_ctl(epoll_webserv, EPOLL_CTL_DEL, eventData->fd, NULL);
    delete eventData;
}

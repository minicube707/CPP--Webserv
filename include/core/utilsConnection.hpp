/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsConnection.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 14:55:35 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:10:24 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Client.hpp"
#include "struct.hpp"

extern volatile sig_atomic_t stop_webserv;

class Client;

void handleSigint(int sig);
void initializeSignal(void);
int setNonblocking(int fd);

sockaddr_in createSocketAddress(std::string ip_address, unsigned int port_number);
int createServerSocket(std::string ip_address, unsigned int port_number, unsigned int max_client);
void removeFdFromEvent(EventData *eventData, int epoll_webserv);

template <typename PTR> EventData *addFdToEvent(int epoll_fd, int socket_fd, uint32_t event, FdType type, PTR *ptr)
{
    setNonblocking(socket_fd);

    struct epoll_event ev;
    ev.events = event;

    EventData *eventData = new EventData;
    eventData->ptr = ptr;
    eventData->fd = socket_fd;
    eventData->type = type;
    eventData->time = getCurrentTime();

    ev.data.ptr = eventData;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
    {
        delete eventData;
        throw ExecptionErrorFunction("epoll_ctl");
    }

    return eventData;
}

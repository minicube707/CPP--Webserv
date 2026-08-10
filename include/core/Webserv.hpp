/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 17:09:20 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:11:33 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"
#include <ctime>
#include <exception>
#include <map>
#include <set>

#define MAX_CLIENT 128
#define MAX_EVENTS 10
#define SIZE_BUFFER 65536
#define SESSION_TTL 3600
#define SESSION_CLEANUP_INTERVAL 5
#define MAX_SESSIONS 10000
#define DELAY 5000
#define MAX_LIVE_CLIENTS 512

#define CLIENT_TIMEOUT 30000
#define TIMEOUT_CHECK_INTERVAL 500

#define MAX_REQUEST_LINE 8192
#define MAX_HEADER_SIZE 16384

class Server;
class Client;
class CGIRequest;

struct SessionInfo
{
    int visits;
    time_t lastSeen;
};

enum RequestState
{
    REQUEST_DISCONNECTED,
    REQUEST_INCOMPLETE,
    REQUEST_COMPLETE,
    REQUEST_URI_TOO_LONG,
    REQUEST_HEADER_TOO_LARGE,
    REQUEST_BODY_TOO_LARGE
};

class Webserv
{
  private:
    std::vector<Server *> _vectorServer;
    std::vector<Client *> _vectorClient;
    std::map<int, std::set<Server *> > _mapFdToServer;
    std::map<std::string, SessionInfo> _sessions;
    std::set<EventData *> _setEventData;

    int _webserEpoll;
    uint64_t _lastTimeoutCheck;
    time_t _lastSessionCleanup;

    void cleanupSessions(void);
    void dropOldestSessions(void);
    void clearServers(void);

    Webserv(const Webserv &other);
    Webserv &operator=(const Webserv &other);

  public:
    // =====================
    // == Canonical Form  ==
    // =====================

    Webserv();
    ~Webserv();

    // =====================
    // == Getter & Setter ==
    // =====================

    // SERVERS
    const std::vector<Server *> &getServers(void) const;
    const std::vector<Client *> &getClients(void) const;
    const std::map<int, std::set<Server *> > &getFdToServersMap(void) const;
    void setEpollFd(const int epoll);
    int getEpollFd(void);
    std::set<EventData *> getSetEventData(void) const;
    void addSetEventData(EventData *eventData);

    // SESSION
    int touchSession(const std::string &sessionId);

    // =====================
    // ==     Method      ==
    // =====================

    bool initializeWebserv(std::vector<std::string> &tokens);
    bool splitIntoServers(std::vector<std::string> &tokens);

    bool initializeConnection();
    void initializeSocket();
    void registerNewSocket(std::map<Listen, int> &map_socket_fd, Listen *listenConfig, Server *server);
    void registerExistingSocket(int serverSocket, Server *server);
    void listenToWebserv();

    void handleConnection(struct epoll_event &events);
    void handleNewClient(int server_fd);

    void processClient(EventData *eventData);
    void executeBufferedRequest(Client *client);
    void processClientResponse(Client *client);
    void applyErrorToResponse(Client *client, const std::exception &e);
    bool sendResponseToClient(Client *client);
    void sendPendingDataToClient(Client *client);
    void updateClientEpollEvents(Client *client, uint32_t epollEvents);

    void writeToChild(EventData *eventData);
    void readToChild(EventData *eventData);

    RequestState readAndCheckRequestCompletion(Client *client);
    RequestState checkRequestLimits(const std::string &request);
    void sendImmediateError(Client *client, int statusCode);
    size_t largestConfiguredMaxBodySize();
    void handleDisconnect(Client *client);
    void deleteClient(Client *client);
    void closeConnection();

    void checkTimeOutIfNeeded();
    void checkTimeOut();
    void timeOutCGI(CGIRequest *cgiRequest);
    void timeOutClient(Client *client);
};

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 17:09:17 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:28:07 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

#include "ARequest.hpp"
#include "CGIRequest.hpp"
#include "Client.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"
#include "Server.hpp"
#include "StaticRequest.hpp"

#include "colors.hpp"
#include "execption.hpp"
#include "utilsConnection.hpp"
#include "utilsRequest.hpp"
#include "utilsResponse.hpp"

#include <cstring>
#include <sstream>
#include <sys/wait.h>

// =====================
// == Canonical Form  ==
// =====================

Webserv::Webserv() : _vectorServer(0), _vectorClient(0), _webserEpoll(-1), _lastTimeoutCheck(0), _lastSessionCleanup(0)
{
}

Webserv::~Webserv()
{
    closeConnection();
}

// =====================
// == Getter & Setter ==
// =====================

// SERVERS
const std::vector<Server *> &Webserv::getServers(void) const
{
    return _vectorServer;
}

const std::vector<Client *> &Webserv::getClients(void) const
{
    return _vectorClient;
}

const std::map<int, std::set<Server *> > &Webserv::getFdToServersMap(void) const
{
    return _mapFdToServer;
}

void Webserv::setEpollFd(const int epoll)
{
    _webserEpoll = epoll;
}

int Webserv::getEpollFd(void)
{
    return _webserEpoll;
}

int Webserv::touchSession(const std::string &sessionId)
{
    cleanupSessions();

    SessionInfo &session = _sessions[sessionId];
    session.lastSeen = time(NULL);
    return ++session.visits;
}


void Webserv::dropOldestSessions(void)
{
    std::vector<time_t> seen;

    seen.reserve(_sessions.size());

    std::map<std::string, SessionInfo>::iterator it = _sessions.begin();
    for (; it != _sessions.end(); ++it)
        seen.push_back(it->second.lastSeen);

    std::sort(seen.begin(), seen.end());
    time_t cutoff = seen[seen.size() / 2];

    it = _sessions.begin();
    while (it != _sessions.end())
    {
        if (it->second.lastSeen <= cutoff)
            _sessions.erase(it++); // post-increment keeps a valid iterator (C++98)
        else
            ++it;
    }
}


void Webserv::cleanupSessions(void)
{
    time_t now = time(NULL);

    if (now - _lastSessionCleanup < SESSION_CLEANUP_INTERVAL && _sessions.size() < MAX_SESSIONS)
        return;

    _lastSessionCleanup = now;

    std::map<std::string, SessionInfo>::iterator it = _sessions.begin();
    while (it != _sessions.end())
    {
        if (now - it->second.lastSeen > SESSION_TTL)
            _sessions.erase(it++); // post-increment keeps a valid iterator (C++98)
        else
            ++it;
    }

    if (_sessions.size() >= MAX_SESSIONS)
        dropOldestSessions();
}

std::set<EventData *> Webserv::getSetEventData(void) const
{
    return _setEventData;
}

void Webserv::addSetEventData(EventData *eventData)
{
    if (eventData == NULL)
        throw ExecptionErrorUninitializedVariable("*eventData", "Webserv");

    _setEventData.insert(eventData);
}

// =====================
// ==     Method      ==
// =====================

bool Webserv::initializeWebserv(std::vector<std::string> &tokens)
{
    return splitIntoServers(tokens);
}

void Webserv::clearServers(void)
{
    for (std::vector<Server *>::iterator it = _vectorServer.begin(); it != _vectorServer.end(); ++it)
        delete *it;
    _vectorServer.clear();
}

bool Webserv::splitIntoServers(std::vector<std::string> &tokens)
{
    clearServers();

    try
    {
        while (!tokens.empty())
        {
            Server *server = new Server(this);
            try
            {
                server->initializeServer(tokens);
                server->initializeCheck();
                _vectorServer.push_back(server);
            }
            catch (...)
            {
                delete server;
                throw;
            }
        }
    }
    catch (const std::exception &e)
    {
        clearServers();
        std::cerr << e.what() << '\n';
        return (true);
    }

    if (_vectorServer.empty())
    {
        std::cerr << "Error: No server defined in the config file\n";
        return (true);
    }
    return (false);
}

void Webserv::registerNewSocket(std::map<Listen, int> &map_socket_fd, Listen *listenConfig, Server *server)
{
    int serverSocket = createServerSocket(listenConfig->ip, listenConfig->port, MAX_CLIENT);
    EventData *eventData = NULL;

    try
    {
        eventData = addFdToEvent(getEpollFd(), serverSocket, EPOLLIN, SERVER, server);
        server->addEventData(eventData);
        server->setWebserv(this);
        addSetEventData(eventData);

        map_socket_fd.insert(std::make_pair(*listenConfig, serverSocket));
        _mapFdToServer.insert(std::make_pair(serverSocket, std::set<Server *>()));
        _mapFdToServer[serverSocket].insert(server);
    }
    catch (...)
    {
        map_socket_fd.erase(*listenConfig);
        _mapFdToServer.erase(serverSocket);
        _setEventData.erase(eventData);
        if (eventData != NULL)
        {
            epoll_ctl(getEpollFd(), EPOLL_CTL_DEL, serverSocket, NULL);
            server->removeEventData(eventData);
            delete eventData;
        }
        close(serverSocket);
        throw;
    }
}

void Webserv::registerExistingSocket(int serverSocket, Server *server)
{
    std::set<Server *> &set_server = _mapFdToServer[serverSocket];

    server->setWebserv(this);

    if (set_server.find(server) == set_server.end())
        set_server.insert(server);
}

void Webserv::initializeSocket()
{
    Listen *listenConfig;
    Server *server;

    std::map<Listen, int> map_socket_fd;
    std::map<Listen, int>::iterator it;

    std::vector<Server *> vector_server = getServers();

    for (size_t i = 0; i < vector_server.size(); ++i)
    {
        server = vector_server[i];

        for (size_t j = 0;; ++j)
        {
            listenConfig = server->getListen(j);

            if (!listenConfig)
                break;

            it = map_socket_fd.find(*listenConfig);

            if (it == map_socket_fd.end())
                registerNewSocket(map_socket_fd, listenConfig, server);

            else
                registerExistingSocket(it->second, server);
        }
    }
}

void Webserv::handleNewClient(int server_fd)
{
    int clientSocket;

    if ((clientSocket = accept(server_fd, NULL, NULL)) == -1)
        throw ExecptionErrorFunction("accept");

    if (_vectorClient.size() >= MAX_LIVE_CLIENTS)
    {
        close(clientSocket);
        std::cerr << "Connection refused: " << MAX_LIVE_CLIENTS << " clients already connected\n";
        return;
    }

    Client *client = NULL;
    try
    {
        client = new Client;
        client->setClientFd(clientSocket);
        client->setServerFd(server_fd);
        client->setWebserv(this);
        _vectorClient.push_back(client);

        EventData *eventData = addFdToEvent(getEpollFd(), clientSocket, EPOLLIN | EPOLLRDHUP, CLIENT, client);
        client->setEventData(eventData);
        addSetEventData(eventData);
    }
    catch (...)
    {
        if (client == NULL)
            close(clientSocket);
        else if (std::find(_vectorClient.begin(), _vectorClient.end(), client) != _vectorClient.end())
            deleteClient(client);
        else
            delete client;
        throw;
    }

    std::cout << "Nouveau client connecté: fd=" << client->getClientFd() << "\n";
}

void Webserv::deleteClient(Client *client)
{
    std::vector<Client *>::iterator it = std::find(_vectorClient.begin(), _vectorClient.end(), client);

    if (it != _vectorClient.end())
        _vectorClient.erase(it);

    _setEventData.erase(client->getEventData());
    delete client;

    std::cout << "Client is disconnected\n";
}

void Webserv::handleDisconnect(Client *client)
{
    if (client->isPendingDelete())
        return;

    epoll_ctl(getEpollFd(), EPOLL_CTL_DEL, client->getClientFd(), NULL);
    _setEventData.erase(client->getEventData());

    if (client->isCGIProcessing())
        client->setPendingDelete(true);
    else
        deleteClient(client);
}

size_t Webserv::largestConfiguredMaxBodySize()
{
    size_t largest = 0;

    for (size_t i = 0; i < _vectorServer.size(); ++i)
    {
        Server *server = _vectorServer[i];
        if (server == NULL)
            continue;

        if (server->getClientMaxBodySize() > largest)
            largest = server->getClientMaxBodySize();

        for (size_t j = 0; server->getLocation(j) != NULL; ++j)
        {
            if (server->getLocation(j)->getClientMaxBodySize() > largest)
                largest = server->getLocation(j)->getClientMaxBodySize();
        }
    }

    if (largest == 0)
        largest = DEFAULT_CLIENT_MAX_BODY_SIZE;

    return largest;
}

RequestState Webserv::checkRequestLimits(const std::string &request)
{
    if (request.find("\r\n\r\n") == std::string::npos)
    {
        if (request.find("\r\n") == std::string::npos && request.size() > MAX_REQUEST_LINE)
            return REQUEST_URI_TOO_LONG;

        if (request.size() > MAX_HEADER_SIZE)
            return REQUEST_HEADER_TOO_LARGE;

        return REQUEST_INCOMPLETE;
    }

    if (request.size() > largestConfiguredMaxBodySize() + MAX_HEADER_SIZE)
        return REQUEST_BODY_TOO_LARGE;

    return REQUEST_INCOMPLETE;
}

RequestState Webserv::readAndCheckRequestCompletion(Client *client)
{
    int bytes;
    char buffer[SIZE_BUFFER];

    if ((bytes = recv(client->getClientFd(), buffer, sizeof(buffer), 0)) < 0)
        return REQUEST_DISCONNECTED;

    if (bytes == 0)
    {
        client->setPeerClosed(true);
        if (isCompleteRequest(client->getContentRequest()))
            return REQUEST_COMPLETE;
        return REQUEST_DISCONNECTED;
    }

    client->getEventData()->time = getCurrentTime();

    std::string s;
    s.assign(buffer, buffer + bytes);
    client->appendContentRequest(s);

    if (isCompleteRequest(client->getContentRequest()))
        return REQUEST_COMPLETE;

    RequestState oversized = checkRequestLimits(client->getContentRequest());
    if (oversized != REQUEST_INCOMPLETE)
        return oversized;

    if (isDeclaredBodySizeExceeding(client->getContentRequest(), largestConfiguredMaxBodySize()))
        return REQUEST_COMPLETE;

    return REQUEST_INCOMPLETE;
}

void Webserv::sendImmediateError(Client *client, int statusCode)
{
    std::string reason = httpStatusToString(statusCode);
    std::string body = "<html><body><h1>" + intToString(statusCode) + " " + reason + "</h1></body></html>";

    std::string response = "HTTP/1.1 " + intToString(statusCode) + " " + reason + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + sizeToString(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;

    std::cerr << "Request rejected with status " << statusCode << " (" << reason << ")\n";

    client->clearContentRequest();
    client->setSendBuffer(response);
    client->setCloseAfterSend(true);
    updateClientEpollEvents(client, EPOLLOUT);
}

static int statusCodeFromException(const std::exception &e)
{
    std::stringstream stream(e.what());
    int statusCode = 0;

    if (!(stream >> statusCode) || !stream.eof() || statusCode < 100 || statusCode > 599)
        return 500;
    return statusCode;
}

void Webserv::applyErrorToResponse(Client *client, const std::exception &e)
{
    int statusCode = statusCodeFromException(e);

    std::cerr << "Request failed with status " << statusCode << " (" << e.what() << ")\n";

    // response sent through the STATIC path, even for a failed CGI
    client->setTypeRequest(STATIC);

    if (client->getARequest() != NULL && client->getARequest()->getResponseContext() != NULL)
        client->getARequest()->getResponseContext()->setStatusCode(statusCode);
}

bool Webserv::sendResponseToClient(Client *client)
{
    if (client->isPendingDelete())
    {
        deleteClient(client);
        return true;
    }

    if (client->getARequest() == NULL || client->getARequest()->getResponseContext() == NULL)
        return false;

    HttpResponse response(client->getARequest());
    response.initialisationHttpResponse();

    client->setSendBuffer(response.getResponseContent());
    client->setCloseAfterSend(response.getShouldCloseConnection());

    size_t requestLength = completeRequestLength(client->getContentRequest());
    if (requestLength == 0)
        client->clearContentRequest();
    else
        client->consumeContentRequest(requestLength);

    updateClientEpollEvents(client, EPOLLOUT);
    return false;
}

// One send per EPOLLOUT event; -1 or 0 -> the client is removed,
// partial send -> keep EPOLLOUT armed and wait for the next event
void Webserv::sendPendingDataToClient(Client *client)
{
    if (!client->hasPendingSend())
    {
        updateClientEpollEvents(client, EPOLLIN);
        return;
    }

    const std::string &buffer = client->getSendBuffer();
    ssize_t bytes = send(client->getClientFd(), buffer.c_str() + client->getSendOffset(),
                         buffer.size() - client->getSendOffset(), 0);

    if (bytes <= 0)
    {
        handleDisconnect(client);
        return;
    }

    client->getEventData()->time = getCurrentTime();

    client->setSendOffset(client->getSendOffset() + bytes);
    if (client->hasPendingSend())
        return;

    client->clearSendState();

    if (client->getCloseAfterSend())
    {
        handleDisconnect(client);
        return;
    }

    if (isCompleteRequest(client->getContentRequest()))
        executeBufferedRequest(client);
    else if (client->isPeerClosed())
        handleDisconnect(client); // FIN received and nothing left to answer
    else
        updateClientEpollEvents(client, EPOLLIN);
}

void Webserv::updateClientEpollEvents(Client *client, uint32_t epollEvents)
{
    struct epoll_event ev;

    ev.events = epollEvents | EPOLLRDHUP;
    ev.data.ptr = client->getEventData();

    if (epoll_ctl(getEpollFd(), EPOLL_CTL_MOD, client->getClientFd(), &ev) == -1)
        handleDisconnect(client);
}

static int earlyErrorStatus(RequestState state)
{
    if (state == REQUEST_URI_TOO_LONG)
        return 414;
    if (state == REQUEST_HEADER_TOO_LARGE)
        return 431;
    return 413;
}

void Webserv::processClient(EventData *eventData)
{
    Client *client = static_cast<Client *>(eventData->ptr);

    RequestState state = readAndCheckRequestCompletion(client);

    if (state == REQUEST_DISCONNECTED)
    {
        handleDisconnect(client);
        return;
    }
    if (state == REQUEST_INCOMPLETE)
        return;
    if (state != REQUEST_COMPLETE)
    {
        sendImmediateError(client, earlyErrorStatus(state));
        return;
    }

    if (client->isCGIProcessing())
    {
        if (client->isPeerClosed())
            updateClientEpollEvents(client, 0);
        return;
    }

    executeBufferedRequest(client);
}

void Webserv::executeBufferedRequest(Client *client)
{
    try
    {
        processClientResponse(client);
    }
    catch (const std::exception &e)
    {
        applyErrorToResponse(client, e);
    }

    if (client->getTypeRequest() == STATIC)
        sendResponseToClient(client);
}

void Webserv::writeToChild(EventData *eventData)
{
    CGIRequest *cgiRequest = static_cast<CGIRequest *>(eventData->ptr);
    
    if (cgiRequest->sendDataToChild())
        _setEventData.erase(cgiRequest->geteventDataWrite());
}

void Webserv::readToChild(EventData *eventData)
{
    CGIRequest *cgiRequest = static_cast<CGIRequest *>(eventData->ptr);
    Client *client = cgiRequest->getRequestContext()->getClient();

    try
    {
        if (!cgiRequest->receivedDataFromChild())
            return;
        cgiRequest->closeStdoutPipe();

        cgiRequest->processDataFromChild();
    }
    catch (const std::exception &e)
    {
        applyErrorToResponse(client, e);
    }

    client->setCGIProcessing(false);

    _setEventData.erase(cgiRequest->geteventDataWrite());
    _setEventData.erase(cgiRequest->geteventDataRead());

    sendResponseToClient(client);
}

static std::string scriptExtension(const std::string &scriptName)
{
    std::string::size_type pos = scriptName.find_last_of('.');

    return (pos != std::string::npos) ? scriptName.substr(pos) : "";
}

void Webserv::processClientResponse(Client *client)
{
    client->initialisationClient();
    client->selectTypeRequest();

    client->getARequest()->validateRequest();

    if (client->getTypeRequest() == STATIC)
    {
        StaticRequest *staticRequest = dynamic_cast<StaticRequest *>(client->getARequest());
        staticRequest->selectMethodHttp();
    }
    else
    {
        CGIRequest *cgiRequest = dynamic_cast<CGIRequest *>(client->getARequest());
        RequestContext *requestContext = cgiRequest->getRequestContext();

        std::string extension = scriptExtension(requestContext->getHttpRequest()->getHeader()->getScriptName());
        std::string interpreter = requestContext->resolveCgiInterpreter(extension);

        if (interpreter.empty())
        {
            std::cerr << "CGI: no interpreter available for '" << extension << "' (see the cgi_pass directive)\n";
            throw std::runtime_error("502");
        }

        cgiRequest->initializationCGIRequest(interpreter);
        client->setCGIProcessing(true);

        addSetEventData(cgiRequest->geteventDataWrite());
        addSetEventData(cgiRequest->geteventDataRead());
    }
}

void Webserv::handleConnection(struct epoll_event &events)
{
    EventData *eventData = static_cast<EventData *>(events.data.ptr);

    if (_setEventData.find(eventData) == _setEventData.end())
        return;

    switch (eventData->type)
    {

    case (SERVER):
        if (events.events & EPOLLIN)
            handleNewClient(eventData->fd);
        break;
    case (CLIENT):
        if (events.events & EPOLLOUT)
            sendPendingDataToClient(static_cast<Client *>(eventData->ptr));
        else if (events.events & EPOLLIN)
            processClient(eventData);
        else if (events.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            handleDisconnect(static_cast<Client *>(eventData->ptr));
        break;
    case (WRITECHILD):
        if (events.events & EPOLLOUT)
            writeToChild(eventData);
        else if (events.events & (EPOLLERR | EPOLLHUP))
        {
            CGIRequest *cgiRequest = static_cast<CGIRequest *>(eventData->ptr);
            cgiRequest->closeStdinPipe();
            _setEventData.erase(eventData);
        }
        break;
    case (READCHILD):
        if (events.events & (EPOLLIN | EPOLLHUP))
            readToChild(eventData);
        break;
    }
}

void Webserv::listenToWebserv()
{
    int nfds;
    struct epoll_event events[MAX_EVENTS];

    std::cout << "Serveur en attente..." << std::endl;

    while (1)
    {
        nfds = epoll_wait(getEpollFd(), events, MAX_EVENTS, 1000);

        if (stop_webserv)
            return;

        // A signal (EINTR) must not kill the server: retry on the next loop turn
        if (nfds == -1)
            continue;

        // Collect every finished CGI child so none is left as a zombie
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;

        checkTimeOutIfNeeded();

        for (int i = 0; i < nfds; ++i)
        {
            try
            {
                handleConnection(events[i]);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Unhandled event error: " << e.what() << "\n";
            }
        }
    }
}

bool Webserv::initializeConnection()
{
    int epoll_fd;
    bool res = false;

    initializeSignal();

    try
    {
        if ((epoll_fd = epoll_create(1)) == -1)
            throw ExecptionErrorFunction("epoll_create");

        setEpollFd(epoll_fd);
        if (fcntl(epoll_fd, F_SETFD, FD_CLOEXEC) == -1)
            throw ExecptionErrorFunction("fcntl");

        initializeSocket();
        listenToWebserv();
    }
    catch (const std::exception &e)
    {
        if (!stop_webserv)
        {
            std::cerr << e.what() << '\n';
            res = true;
        }
    }
    closeConnection();
    return res;
}

void Webserv::checkTimeOutIfNeeded()
{
    uint64_t now = getCurrentTime();

    if (now - _lastTimeoutCheck < TIMEOUT_CHECK_INTERVAL)
        return;

    _lastTimeoutCheck = now;
    checkTimeOut();
}

void Webserv::timeOutCGI(CGIRequest *cgiRequest)
{
    std::cout << "KILL process\n";

    kill(cgiRequest->getPid(), SIGKILL);

    Client *client = cgiRequest->getRequestContext()->getClient();

    applyErrorToResponse(client, std::runtime_error("504"));
    client->setCGIProcessing(false);

    _setEventData.erase(cgiRequest->geteventDataWrite());
    _setEventData.erase(cgiRequest->geteventDataRead());

    // 504 is an error status: the connection closes once it is sent
    sendResponseToClient(client);
}

void Webserv::timeOutClient(Client *client)
{
    if (client->isCGIProcessing())
        return;

    if (!client->getContentRequest().empty() && !client->hasPendingSend())
    {
        client->getEventData()->time = getCurrentTime();
        sendImmediateError(client, 408);
        return;
    }

    std::cout << "Client timed out\n";
    handleDisconnect(client);
}

void Webserv::checkTimeOut()
{
    uint64_t now = getCurrentTime();
    std::vector<EventData *> expired;

    std::set<EventData *>::iterator it = _setEventData.begin();
    for (; it != _setEventData.end(); ++it)
    {
        if ((*it)->type == READCHILD && now > (*it)->time + DELAY)
            expired.push_back(*it);
        else if ((*it)->type == CLIENT && now > (*it)->time + CLIENT_TIMEOUT)
            expired.push_back(*it);
    }

    for (size_t i = 0; i < expired.size(); ++i)
    {
        if (_setEventData.find(expired[i]) == _setEventData.end())
            continue;

        if (expired[i]->type == READCHILD)
            timeOutCGI(static_cast<CGIRequest *>(expired[i]->ptr));
        else
            timeOutClient(static_cast<Client *>(expired[i]->ptr));
    }
}

void Webserv::closeConnection()
{
    std::vector<Client *>::iterator it_client = _vectorClient.begin();
    for (; it_client != _vectorClient.end(); ++it_client)
    {
        if ((*it_client)->isCGIProcessing())
        {
            CGIRequest *cgiRequest = dynamic_cast<CGIRequest *>((*it_client)->getARequest());
            if (cgiRequest != NULL && cgiRequest->getPid() > 0)
            {
                kill(cgiRequest->getPid(), SIGKILL);
                waitpid(cgiRequest->getPid(), NULL, 0);
            }
        }
        delete *it_client;
    }
    _vectorClient.clear();

    std::map<int, std::set<Server *> >::iterator it_fd = _mapFdToServer.begin();
    for (; it_fd != _mapFdToServer.end(); ++it_fd)
        close((*it_fd).first);
    _mapFdToServer.clear();

    clearServers();
    _setEventData.clear();

    if (_webserEpoll >= 0)
    {
        close(_webserEpoll);
        _webserEpoll = -1;
    }
}

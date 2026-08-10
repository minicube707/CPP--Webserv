/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 14:43:09 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:32:24 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include "ARequest.hpp"
#include "CGIRequest.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "RequestContext.hpp"
#include "StaticRequest.hpp"

// =====================
// == Canonical Form  ==
// =====================

Client::Client()
    : _client_fd(-1), _server_fd(-1), _server(0), _webserv(0), _ARequest(NULL), _typeResquest(STATIC),
      _contentRequest(""), _sessionId(""), _sendBuffer(""), _sendOffset(0), _closeAfterSend(false),
      _CGIProcessing(false), _pendingDelete(false), _peerClosed(false), _eventData(NULL)
{
}

Client::~Client()
{
    closeClientFd();
    delete getARequest();
    delete getEventData();
}

const std::string Client::_listCGI[2] = {".py", ".php"};

// =====================
// == Getter & Setter ==
// =====================

// CLIENT-FD
int Client::getClientFd(void)
{
    return _client_fd;
}
void Client::setClientFd(int client_fd)
{
    _client_fd = client_fd;
}
void Client::closeClientFd(void)
{
    if (_client_fd >= 0)
    {
        close(_client_fd);
        _client_fd = -1;
    }
}

// SERVER-FD
int Client::getServerFd(void)
{
    return _server_fd;
}
void Client::setServerFd(int server_fd)
{
    _server_fd = server_fd;
}

// SERVER-PTR
Server *Client::getServerPtr(void)
{
    return _server;
}
void Client::setServerPtr(Server *server)
{
    if (server == NULL)
        throw ExecptionErrorUninitializedVariable("*server", "Client");

    _server = server;
}

// REQUEST
std::string &Client::getContentRequest(void)
{
    return _contentRequest;
}
void Client::clearContentRequest(void)
{
    _contentRequest.clear();
}
void Client::consumeContentRequest(size_t length)
{
    _contentRequest.erase(0, length);
}
void Client::appendContentRequest(std::string &request)
{
    _contentRequest.append(request);
}

// RESPONSE SEND BUFFER
void Client::setSendBuffer(const std::string &sendBuffer)
{
    _sendBuffer = sendBuffer;
    _sendOffset = 0;
}

const std::string &Client::getSendBuffer(void) const
{
    return _sendBuffer;
}

size_t Client::getSendOffset(void) const
{
    return _sendOffset;
}

void Client::setSendOffset(size_t sendOffset)
{
    _sendOffset = sendOffset;
}

bool Client::hasPendingSend(void) const
{
    return _sendOffset < _sendBuffer.size();
}

void Client::clearSendState(void)
{
    _sendBuffer.clear();
    _sendOffset = 0;
}

bool Client::getCloseAfterSend(void) const
{
    return _closeAfterSend;
}

void Client::setCloseAfterSend(bool closeAfterSend)
{
    _closeAfterSend = closeAfterSend;
}

Webserv *Client::getWebserv(void)
{
    return _webserv;
}

void Client::setWebserv(Webserv *webserv)
{
    if (webserv == NULL)
        throw ExecptionErrorUninitializedVariable("*webserv", "Client");

    _webserv = webserv;
}

// SESSION
std::string Client::getSessionId(void)
{
    return _sessionId;
}

void Client::setSessionId(const std::string &sessionId)
{
    _sessionId = sessionId;
}

bool Client::hasSession(void)
{
    return !_sessionId.empty();
}

ARequest *Client::getARequest() const
{
    return _ARequest;
}

void Client::setARequest(ARequest *ARequest)
{
    if (ARequest == NULL)
        throw ExecptionErrorUninitializedVariable("*ARequest", "Client");

    _ARequest = ARequest;
}

TypeRequest Client::getTypeRequest() const
{
    return _typeResquest;
}

void Client::setTypeRequest(TypeRequest typeRequest)
{
    _typeResquest = typeRequest;
}

// CGI / DISCONNECT STATE
bool Client::isCGIProcessing() const
{
    return _CGIProcessing;
}

void Client::setCGIProcessing(bool CGIProcessing)
{
    _CGIProcessing = CGIProcessing;
}

bool Client::isPendingDelete() const
{
    return _pendingDelete;
}

void Client::setPendingDelete(bool pendingDelete)
{
    _pendingDelete = pendingDelete;
}

bool Client::isPeerClosed() const
{
    return _peerClosed;
}

void Client::setPeerClosed(bool peerClosed)
{
    _peerClosed = peerClosed;
}

void Client::setEventData(EventData *eventData)
{
    if (eventData == NULL)
        throw ExecptionErrorUninitializedVariable("*eventData", "Client");

    _eventData = eventData;
}

EventData *Client::getEventData(void) const
{
    return _eventData;
}

// =====================
// ==     Method      ==
// =====================
void Client::initialisationClient()
{
    ARequest *arequest = new ARequest();
    try
    {
        arequest->initialisationARequest();
        arequest->getRequestContext()->setClient(this);
    }
    catch (...)
    {
        delete arequest;
        throw;
    }

    delete getARequest();
    setARequest(arequest);

    getARequest()->getRequestContext()->initialisationRequestContext();
}

void Client::selectTypeRequest()
{
    std::string uri = getARequest()->getRequestContext()->getHttpRequest()->getHeader()->getScriptName();

    size_t pos = uri.find_last_of('.');
    std::string extension;
    if (pos != std::string::npos)
        extension = uri.substr(pos);

    if (!getARequest()->getRequestContext()->isCgiExtension(extension))
    {
        setTypeRequest(STATIC);
        ARequest *arequest = getARequest();
        StaticRequest *staticRequest = new StaticRequest(*arequest); // check if fail
        delete arequest;
        setARequest(staticRequest);
    }
    else
    {
        setTypeRequest(CGI);
        ARequest *arequest = getARequest();
        CGIRequest *cgiRequest = new CGIRequest(*arequest);
        delete arequest;
        setARequest(cgiRequest);
    }
}

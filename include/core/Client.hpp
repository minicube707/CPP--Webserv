/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 14:43:11 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:31:51 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Server.hpp"
#include "struct.hpp"

class Server;
class Webserv;
class ARequest;

class Client
{
  private:
    static const std::string _listCGI[2];

    // split between link and info
    int _client_fd;
    int _server_fd;
    Server *_server;
    Webserv *_webserv;
    ARequest *_ARequest;
    TypeRequest _typeResquest;
    std::string _contentRequest;
    std::string _sessionId;
    std::string _sendBuffer;
    size_t _sendOffset;
    bool _closeAfterSend;
    bool _CGIProcessing;
    bool _pendingDelete;
    bool _peerClosed;
    EventData *_eventData;

    Client(const Client &other);
    Client &operator=(const Client &other);

  public:
    // =====================
    // == Canonical Form  ==
    // =====================
    Client();
    ~Client();

    // =====================
    // == Getter & Setter ==
    // =====================

    // CLIENT-FD
    int getClientFd(void);
    void setClientFd(int client_fd);
    void closeClientFd(void);

    // SERVER-FD
    int getServerFd(void);
    void setServerFd(int server_fd);

    // SERVER-PTR
    Server *getServerPtr(void);
    void setServerPtr(Server *server);

    // REQUEST
    std::string &getContentRequest(void);
    void clearContentRequest(void);
    void consumeContentRequest(size_t length);
    void appendContentRequest(std::string &request);

    // RESPONSE SEND BUFFER
    void setSendBuffer(const std::string &sendBuffer);
    const std::string &getSendBuffer(void) const;
    size_t getSendOffset(void) const;
    void setSendOffset(size_t sendOffset);
    bool hasPendingSend(void) const;
    void clearSendState(void);
    bool getCloseAfterSend(void) const;
    void setCloseAfterSend(bool closeAfterSend);

    // WEBSERV
    Webserv *getWebserv(void);
    void setWebserv(Webserv *webserv);

    // SESSION
    std::string getSessionId(void);
    void setSessionId(const std::string &sessionId);
    bool hasSession(void);

    ARequest *getARequest() const;
    void setARequest(ARequest *ARequest);

    TypeRequest getTypeRequest() const;
    void setTypeRequest(TypeRequest typeRequest);

    // CGI / DISCONNECT STATE
    bool isCGIProcessing() const;
    void setCGIProcessing(bool CGIProcessing);
    bool isPendingDelete() const;
    void setPendingDelete(bool pendingDelete);
    bool isPeerClosed() const;
    void setPeerClosed(bool peerClosed);

    void setEventData(EventData *eventData);
    EventData *getEventData(void) const;

    // =====================
    // ==     Method      ==
    // =====================
    bool isCGIExtension(const std::string &extension) const;
    void initialisationClient();
    void selectTypeRequest();
};

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestContext.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/05 21:33:50 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/22 13:45:41 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>

class Server;
class HttpRequest;
class Client;
class Location;
class ARequest;

class RequestContext
{
  private:
    // =====================
    // ==    Attributs    ==
    // =====================
    Client *_client;
    Server *_server;
    Location *_location;
    ARequest *_ARequest;
    HttpRequest *_httpRequest;

    RequestContext();

  public:
    // =====================
    // ==       OCF       ==
    // =====================
    RequestContext(ARequest *arequest);
    ~RequestContext();
    RequestContext(const RequestContext &other);

    // =====================
    // ==     Getters     ==
    // =====================
    Client *getClient(void) const;
    void setClient(Client *client);
    Server *getServer(void) const;
    void setServer(Server *server);
    HttpRequest *getHttpRequest(void) const;
    void setHttpRequest(HttpRequest *httpRequest);
    Location *getLocation(void) const;
    void setLocation(Location *location);
    ARequest *getARequest(void) const;
    void setARequest(ARequest *arequest);

    // =====================
    // == 	  Member	  ==
    // =====================
    void initialisationRequestContext();

    bool isCgiExtension(const std::string &extension) const;
    std::string resolveCgiInterpreter(const std::string &extension) const;

    void linkToServer(void);
    Location *findLocation(void);
};

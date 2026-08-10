/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestContext.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/05 21:33:57 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:27:40 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestContext.hpp"

#include "ARequest.hpp"
#include "Client.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "ResponseContext.hpp"
#include "Server.hpp"

#include "execption.hpp"
#include "utilsRequest.hpp"

// =====================
// == Canonical Form  ==
// =====================

RequestContext::RequestContext(ARequest *arequest)
    : _client(NULL), _server(NULL), _location(NULL), _ARequest(NULL), _httpRequest(NULL)
{
    setARequest(arequest);
}

RequestContext::~RequestContext()
{
    delete getHttpRequest();
}

RequestContext::RequestContext(const RequestContext &other)
    : _client(other._client), _server(other._server), _location(other._location), _ARequest(other._ARequest),
      _httpRequest(new HttpRequest(*other._httpRequest))
{
    _httpRequest->setRequestContext(this);
}

// =====================
// == Getter & Setter ==
// =====================

Client *RequestContext::getClient(void) const
{
    return _client;
}

void RequestContext::setClient(Client *client)
{
    if (client == NULL)
        throw ExecptionErrorUninitializedVariable("*client", "RequestContext");

    _client = client;
}

Server *RequestContext::getServer(void) const
{
    return _server;
}

void RequestContext::setServer(Server *server)
{
    if (server == NULL)
        throw ExecptionErrorUninitializedVariable("*server", "RequestContext");

    _server = server;
}

Location *RequestContext::getLocation(void) const
{
    return _location;
}

void RequestContext::setLocation(Location *location)
{
    if (location == NULL)
        throw ExecptionErrorUninitializedVariable("*location", "RequestContext");

    _location = location;
}

ARequest *RequestContext::getARequest(void) const
{
    return _ARequest;
}

void RequestContext::setARequest(ARequest *arequest)
{
    if (arequest == NULL)
        throw ExecptionErrorUninitializedVariable("*arequest", "RequestContext");

    _ARequest = arequest;
}

HttpRequest *RequestContext::getHttpRequest(void) const
{
    return _httpRequest;
}

void RequestContext::setHttpRequest(HttpRequest *httpRequest)
{
    if (httpRequest == NULL)
        throw ExecptionErrorUninitializedVariable("*httpRequest", "RequestContext");

    _httpRequest = httpRequest;
}

// =====================
// ==       CGI       ==
// =====================

static const std::string knownCgiExtensions[2] = {".py", ".php"};

static std::string firstExecutable(const char *const *candidates, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (access(candidates[i], X_OK) == 0)
            return candidates[i];

    return "";
}

static std::string defaultInterpreter(const std::string &extension)
{
    if (extension == ".py")
    {
        static const char *const python[] = {"/usr/bin/python3", "/usr/local/bin/python3", "/bin/python3"};
        return firstExecutable(python, sizeof(python) / sizeof(python[0]));
    }

    if (extension == ".php")
    {
        static const char *const php[] = {"/usr/bin/php-cgi", "/usr/local/bin/php-cgi", "/bin/php-cgi",
                                          "/opt/homebrew/bin/php-cgi"};
        return firstExecutable(php, sizeof(php) / sizeof(php[0]));
    }

    return "";
}

bool RequestContext::isCgiExtension(const std::string &extension) const
{
    if (extension.empty())
        return false;

    if (getLocation() != NULL && getLocation()->hasCgiPass(extension))
        return true;

    if (getServer() != NULL && getServer()->hasCgiPass(extension))
        return true;

    return (std::find(knownCgiExtensions, knownCgiExtensions + 2, extension) != knownCgiExtensions + 2);
}

std::string RequestContext::resolveCgiInterpreter(const std::string &extension) const
{
    std::string configured;

    if (getLocation() != NULL)
        configured = getLocation()->getCgiPass(extension);
    if (configured.empty() && getServer() != NULL)
        configured = getServer()->getCgiPass(extension);

    if (!configured.empty())
    {
        if (access(configured.c_str(), X_OK) == 0)
            return configured;

        std::cerr << "CGI: " << configured << " is not executable, falling back to the known paths\n";
    }

    return defaultInterpreter(extension);
}

// =====================
// ==     Method      ==
// =====================
void RequestContext::initialisationRequestContext()
{
    HttpRequest *httpRequest = new HttpRequest(this); // check fail
    setHttpRequest(httpRequest);

    getHttpRequest()->initHeader(getClient()->getContentRequest());
    linkToServer();

    Location *location = findLocation();
    if (location != NULL)
        setLocation(location);

    getHttpRequest()->initBody();
}

void RequestContext::linkToServer(void)
{
    int fd_server = getClient()->getServerFd();
    std::set<Server *>::iterator it;
    std::set<Server *> set_server = getClient()->getWebserv()->getFdToServersMap().find(fd_server)->second;

    for (it = set_server.begin(); it != set_server.end(); ++it)
    {
        for (size_t i = 0;; ++i)
        {
            if ((*it)->getNameServer(i) == "")
                break;

            // std::cout << "\n";
            // std::cout << "Name: " << (*it)->getNameServer(i) << "\n";
            // std::cout << "HOST: " << getHttpRequest()->getHeader()->getHost() << "\n";

            if ((*it)->getNameServer(i) == getHttpRequest()->getHeader()->getHost())
            {
                setServer(*it);
                getClient()->setServerPtr(*it);
                return;
            }
        }
    }
    setServer(*(set_server.begin()));
    getClient()->setServerPtr(*(set_server.begin()));
}

Location *RequestContext::findLocation(void)
{
    Location *location;
    Location *best_location = NULL;
    int best_score = -1;
    int score;

    for (int i = 0; (location = getClient()->getServerPtr()->getLocation(i)) != NULL; ++i)
    {
        score = longestPrefixMatch(getHttpRequest()->getHeader()->getScriptName(), location->getName());

        if (score > best_score)
        {
            best_location = location;
            best_score = score;
        }
    }
    return best_location;
}
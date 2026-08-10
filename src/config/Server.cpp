/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/04 14:50:27 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/23 17:46:26 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

// =====================
// == Canonical Form  ==
// =====================

Server::Server()
    : _listens(0), _name_servers(0), _locations(0), _root(""), _index_files(0), _cgi_pass(), _auto_index(false),
      _error_page(0),
      _client_max_body_size(0), _ret(HttpReturn()), _webserv(NULL)
{
}

Server::Server(Webserv *webserv)
    : _listens(0), _name_servers(0), _locations(0), _root(""), _index_files(0), _cgi_pass(), _auto_index(false),
      _error_page(0),
      _client_max_body_size(0), _ret(HttpReturn()), _webserv(webserv)
{
}

Server::~Server()
{
    EventData *eventData;

    std::set<EventData *>::iterator it;
    for (it = _setEventData.begin(); it != _setEventData.end(); it++)
    {
        eventData = *it;
        if (getWebserv() != NULL && getWebserv()->getEpollFd() >= 0)
            epoll_ctl(getWebserv()->getEpollFd(), EPOLL_CTL_DEL, eventData->fd, NULL);
        delete eventData;
    }
}

// =====================
// == Getter & Setter ==
// =====================

// NAME-SERVERS
void Server::addNameServer(std::string name)
{
    _name_servers.push_back(name);
}

std::string Server::getNameServer(size_t i)
{
    if (i >= _name_servers.size())
        return "";
    else
        return _name_servers[i];
}

// LISTEN
void Server::addListen(Listen listen)
{
    for (size_t i = 0; i < _listens.size(); ++i)
    {
        if (!(_listens[i] < listen) && !(listen < _listens[i]))
            throw ExecptionDuplicateElement("listen " + listen.ip + ":" + intToString(listen.port));
    }
    _listens.push_back(listen);
}

Listen *Server::getListen(size_t i)
{
    if (i >= _listens.size())
        return NULL;
    else
        return &_listens[i];
}

// LOCATION
void Server::addLocation(Location &location)
{
    for (size_t i = 0; i < _locations.size(); ++i)
    {
        if (_locations[i].getName() == location.getName())
            throw ExecptionDuplicateElement("location " + location.getName());
    }
    _locations.push_back(location);
}
Location *Server::getLocation(size_t i)
{
    if (i < _locations.size())
        return &_locations[i];
    else
        return NULL;
}

// ROOT
void Server::setRoot(std::string root)
{
    _root = root;
}
std::string Server::getRoot(void)
{
    return _root;
}

// INDEX
void Server::addIndex(std::string index)
{
    _index_files.push_back(index);
}
std::string Server::getIndex(size_t i)
{
    if (i < _index_files.size())
        return _index_files[i];
    else
        return "";
}

// CGI-PASS
void Server::addCgiPass(const std::string &extension, const std::string &interpreter)
{
    _cgi_pass[extension] = interpreter;
}

std::string Server::getCgiPass(const std::string &extension) const
{
    std::map<std::string, std::string>::const_iterator it = _cgi_pass.find(extension);

    return (it == _cgi_pass.end()) ? "" : it->second;
}

bool Server::hasCgiPass(const std::string &extension) const
{
    return (_cgi_pass.find(extension) != _cgi_pass.end());
}

// AUTO-INDEX
void Server::setAutoIndex(bool auto_index)
{
    _auto_index = auto_index;
}
bool Server::getAutoIndex(void)
{
    return _auto_index;
}

// ERROR-PAGE
void Server::addErrorPage(HttpErrorPage error_page)
{
    _error_page.push_back(error_page);
}

HttpErrorPage *Server::getErrorPage(size_t i)
{
    if (i < _error_page.size())
        return &_error_page[i];
    else
        return NULL;
}

// CLIENT-MAX-BODY-SIZE
void Server::setClientMaxBodySize(unsigned int client_max_body_size)
{
    _client_max_body_size = client_max_body_size;
}
unsigned int Server::getClientMaxBodySize(void)
{
    return _client_max_body_size;
}

// RETURN
void Server::setReturn(HttpReturn ret)
{
    _ret = ret;
}
HttpReturn *Server::getReturn(void)
{
    return &_ret;
}

void Server::addEventData(EventData *eventData)
{
    if (eventData == NULL)
        throw ExecptionErrorUninitializedVariable("*eventData", "Server");

    _setEventData.insert(eventData);
}

void Server::removeEventData(EventData *eventData)
{
    _setEventData.erase(eventData);
}

std::set<EventData *> Server::getEventData(void) const
{
    return _setEventData;
}

void Server::setWebserv(Webserv *webserv)
{
    if (webserv == NULL)
        throw ExecptionErrorUninitializedVariable("*webserv", "Server");

    _webserv = webserv;
}

Webserv *Server::getWebserv(void) const
{
    return _webserv;
}

// =====================
// ==     Method      ==
// =====================

// bool Server::initialisatio

void Server::initializeServer(std::vector<std::string> &tokens)
{
    if (tokens.empty())
        throw ExecptionMissBrace();

    if (popToken(tokens) != "server")
        throw ExecptionWrongArgument("expected server");

    if (tokens.empty())
        throw ExecptionMissBrace();

    if (tokens[0] != "{")
        throw ExecptionMissBrace();

    tokens.erase(tokens.begin());
    size_t tokens_size = tokens.size();
    size_t new_tokens_size;
    while (true)
    {
        if (tokens.empty())
            throw ExecptionMissBrace();
        if (tokens[0] == "}")
            break;

        initializeListens(tokens);
        initializeNameServers(tokens);
        initializeLocation(tokens);
        initializeRoot(tokens);
        initializeIndexFiles(tokens);
        initializeCgiPass(tokens);
        initializeAutoIndex(tokens);
        initializeErrorPage(tokens);
        initializeClientMaxBodySize(tokens);
        initializeReturn(tokens);

        // Security to avoid inifite loop
        new_tokens_size = tokens.size();
        if (tokens_size == new_tokens_size)
            throw ExecptionWrongArgument(tokens[0]);
        tokens_size = new_tokens_size;
    }

    tokens.erase(tokens.begin());
}

void Server::initializeCheck()
{
    if (_listens.size() == 0)
        throw ExecptionMissElement("listen");
}

void Server::initializeNameServers(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "server_name")
        return;

    popToken(tokens);

    if (frontToken(tokens) == ";")
        throw ExecptionWrongArgument("server_name");

    while (frontToken(tokens) != ";")
    {
        std::string name = popToken(tokens);
        if (name == "{" || name == "}")
            throw ExecptionWrongArgument(name);
        addNameServer(toLowerString(name));
    }

    popToken(tokens);
}

void Server::initializeListens(std::vector<std::string> &tokens)
{
    char sep = ':';

    std::string sub_string;
    Listen listenAddr;

    listenAddr.ip = DEFAULT_IP;
    listenAddr.port = DEFAULT_PORT;

    if (frontToken(tokens) != "listen")
        return;

    popToken(tokens);

    if (frontToken(tokens) == ";")
    {
        popToken(tokens);
        addListen(listenAddr);
        return;
    }

    std::string address = popToken(tokens);
    if (countOccurrences(address, sep) > 1)
        throw ExecptionWrongArgument(address);

    std::string::size_type colon = address.find(sep);
    if (colon != std::string::npos)
    {
        listenAddr.ip = address.substr(0, colon);
        sub_string = address.substr(colon + 1);
        if (listenAddr.ip.empty() || sub_string.empty())
            throw ExecptionWrongArgument(address);
    }
    else if (countOccurrences(address, '.') == 3)
        listenAddr.ip = address;
    else
        sub_string = address;

    if (!sub_string.empty())
    {
        std::istringstream convert(sub_string);
        convert >> listenAddr.port;
        if (convert.fail() || !convert.eof() || listenAddr.port == 0 || listenAddr.port > 65535)
            throw ExecptionFailConvertion(sub_string);
    }

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    addListen(listenAddr);
}

void Server::initializeLocation(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "location")
        return;

    popToken(tokens);

    Location location;
    location.initializeLocation(tokens);
    addLocation(location);
}

void Server::initializeIndexFiles(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "index")
        return;

    popToken(tokens);

    if (frontToken(tokens) == ";")
        throw ExecptionWrongArgument("index");

    while (frontToken(tokens) != ";")
    {
        std::string index = popToken(tokens);
        if (index == "{" || index == "}")
            throw ExecptionWrongArgument(index);
        addIndex(index);
    }

    popToken(tokens);
}

void Server::initializeRoot(std::vector<std::string> &tokens)
{
    std::string root = parseRootDirective(tokens);
    if (root != "")
        setRoot(root);
}

void Server::initializeCgiPass(std::vector<std::string> &tokens)
{
    std::string extension;
    std::string interpreter;

    if (!parseCgiPassDirective(tokens, extension, interpreter))
        return;

    addCgiPass(extension, interpreter);
}

void Server::initializeAutoIndex(std::vector<std::string> &tokens)
{
    int auto_index = parseAutoIndexDirective(tokens);

    if (auto_index == 0)
        setAutoIndex(false);
    else if (auto_index == 1)
        setAutoIndex(true);
}

void Server::initializeErrorPage(std::vector<std::string> &tokens)
{
    bool is_init = false;
    HttpErrorPage error_page = parseErrorPageDirective(tokens, is_init);

    if (is_init)
        addErrorPage(error_page);
}

void Server::initializeClientMaxBodySize(std::vector<std::string> &tokens)
{
    unsigned int client_max_body_size = parseClientMaxBodySizeDirective(tokens);

    if (client_max_body_size != 0)
        setClientMaxBodySize(client_max_body_size);
}

void Server::initializeReturn(std::vector<std::string> &tokens)
{
    bool is_init = false;
    HttpReturn ret = parseReturnDirective(tokens, is_init);

    if (is_init)
        setReturn(ret);
}

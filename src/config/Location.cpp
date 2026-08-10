/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/06 15:45:46 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:44:14 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Location.hpp"
#include "Server.hpp"
#include "execption.hpp"
#include "utilsDuplicate.hpp"

// =====================
// == Canonical Form  ==
// =====================

Location::Location()
    : _name(""), _allowed_methods(), _root(""), _index(""), _upload_store(""), _cgi_pass(), _auto_index(false),
      _error_page(HttpErrorPage()), _client_max_body_size(0), _ret(HttpReturn())
{
}

Location::~Location()
{
}

Location::Location(const Location &other)
{
    *this = other;
}

Location &Location::operator=(const Location &other)
{
    _name = other._name;
    _allowed_methods = other._allowed_methods;
    _root = other._root;
    _index = other._index;
    _upload_store = other._upload_store;
    _cgi_pass = other._cgi_pass;
    _auto_index = other._auto_index;
    _error_page = other._error_page;
    _client_max_body_size = other._client_max_body_size;
    _ret = other._ret;

    return (*this);
}

const std::string Location::list_allowed_methods[4] = {"GET", "POST", "DELETE", "HEAD"};

// =====================
// == Getter & Setter ==
// =====================

// NAME
void Location::setName(std::string name)
{
    _name = name;
}
std::string Location::getName(void)
{
    return _name;
}

// METHOD-HTTP
void Location::addAllowedMethod(HttpMethod i)
{
    _allowed_methods.insert(i);
}
std::set<HttpMethod> Location::getAllowedMethods(void)
{
    return _allowed_methods;
}

// INDEX
void Location::setIndex(std::string index)
{
    _index = index;
}
std::string Location::getIndex(void)
{
    return _index;
}

// ROOT
void Location::setRoot(std::string root)
{
    _root = root;
}
std::string Location::getRoot(void)
{
    return _root;
}

// UPLOAD-STORE
void Location::setUploadStore(std::string upload_store)
{
    _upload_store = upload_store;
}
std::string Location::getUploadStore(void)
{
    return _upload_store;
}

// CGI-PASS
void Location::addCgiPass(const std::string &extension, const std::string &interpreter)
{
    _cgi_pass[extension] = interpreter;
}

std::string Location::getCgiPass(const std::string &extension) const
{
    std::map<std::string, std::string>::const_iterator it = _cgi_pass.find(extension);

    return (it == _cgi_pass.end()) ? "" : it->second;
}

bool Location::hasCgiPass(const std::string &extension) const
{
    return (_cgi_pass.find(extension) != _cgi_pass.end());
}

// AUTO-INDEX
void Location::setAutoIndex(bool auto_index)
{
    _auto_index = auto_index;
}
bool Location::getAutoIndex(void)
{
    return _auto_index;
}

// CLIENT-MAX-BODY-SIZE
void Location::setClientMaxBodySize(unsigned int client_max_body_size)
{
    _client_max_body_size = client_max_body_size;
}
unsigned int Location::getClientMaxBodySize(void)
{
    return _client_max_body_size;
}

// ERROR-PAGE
void Location::setErrorPage(HttpErrorPage error_page)
{
    _error_page = error_page;
}
HttpErrorPage *Location::getErrorPage(void)
{
    return &_error_page;
}

// RETURN
void Location::setReturn(HttpReturn ret)
{
    _ret = ret;
}
HttpReturn *Location::getReturn(void)
{
    return &_ret;
}

// =====================
// ==     Method      ==
// =====================

void Location::initializeLocation(std::vector<std::string> &tokens)
{
    if (tokens.empty())
        throw ExecptionMissBrace();

    std::string name = popToken(tokens);
    if (name.empty() || name[0] != '/' || name == "{" || name == "}" || name == ";")
        throw ExecptionWrongArgument(name);
    setName(name);

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

        initializeLocationAllowedMethods(tokens);
        initializeLocationRoot(tokens);
        initializeLocationUploadStore(tokens);
        initializeLocationCgiPass(tokens);
        initializeLocationIndex(tokens);
        initializeLocationAutoIndex(tokens);
        initializeLocationClientMaxBodySize(tokens);
        initializeLocationErrorPage(tokens);
        initializeLocationReturn(tokens);

        // Security to avoid inifite loop
        new_tokens_size = tokens.size();
        if (tokens_size == new_tokens_size)
            throw ExecptionWrongArgument(tokens[0]);
        tokens_size = new_tokens_size;
    }
    tokens.erase(tokens.begin());
}

void Location::initializeLocationAllowedMethods(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "allowed_methods")
        return;

    popToken(tokens);

    if (frontToken(tokens) == ";")
        throw ExecptionWrongArgument("allowed_methods");

    while (frontToken(tokens) != ";")
    {
        std::string method = popToken(tokens);

        if (method == "GET")
            addAllowedMethod(GET);
        else if (method == "POST")
            addAllowedMethod(POST);
        else if (method == "DELETE")
            addAllowedMethod(DELETE);
        else if (method == "HEAD")
            addAllowedMethod(HEAD);
        else
            throw ExecptionIllegalMethod(method);
    }
    popToken(tokens);
}

void Location::initializeLocationRoot(std::vector<std::string> &tokens)
{
    std::string root = parseRootDirective(tokens);
    if (root != "")
        setRoot(root);
}

void Location::initializeLocationIndex(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "index")
        return;

    popToken(tokens);
    std::string index = popToken(tokens);
    if (index == ";" || index == "{" || index == "}")
        throw ExecptionWrongArgument("index");
    setIndex(index);

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();
}

void Location::initializeLocationUploadStore(std::vector<std::string> &tokens)
{
    std::string uploadStore = parseUploadStoreDirective(tokens);

    if (uploadStore != "")
        setUploadStore(uploadStore);
}

void Location::initializeLocationCgiPass(std::vector<std::string> &tokens)
{
    std::string extension;
    std::string interpreter;

    if (!parseCgiPassDirective(tokens, extension, interpreter))
        return;

    addCgiPass(extension, interpreter);
}

void Location::initializeLocationAutoIndex(std::vector<std::string> &tokens)
{
    int auto_index = parseAutoIndexDirective(tokens);

    if (auto_index == 0)
        setAutoIndex(false);
    else if (auto_index == 1)
        setAutoIndex(true);
}

void Location::initializeLocationClientMaxBodySize(std::vector<std::string> &tokens)
{
    unsigned int client_max_body_size = parseClientMaxBodySizeDirective(tokens);

    if (client_max_body_size != 0)
        setClientMaxBodySize(client_max_body_size);
}

void Location::initializeLocationErrorPage(std::vector<std::string> &tokens)
{
    bool is_init = false;
    HttpErrorPage error_page = parseErrorPageDirective(tokens, is_init);

    if (is_init)
        setErrorPage(error_page);
}

void Location::initializeLocationReturn(std::vector<std::string> &tokens)
{
    bool is_init = false;
    HttpReturn ret = parseReturnDirective(tokens, is_init);

    if (is_init)
        setReturn(ret);
}

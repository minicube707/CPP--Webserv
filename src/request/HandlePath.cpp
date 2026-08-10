/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HandlePath.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 05:37:38 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:36:59 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HandlePath.hpp"

#include "Header.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "RequestContext.hpp"
#include "Server.hpp"

#include "execption.hpp"
#include "utilsParsing.hpp"
#include "utilsRequest.hpp"

#include <algorithm>
#include <cctype>
#include <dirent.h>

// =====================
// ==       OCF       ==
// =====================
HandlePath::HandlePath(HttpRequest *httpRequest) : _path(""), _isAutoIndex(false), _httpRequest(NULL)
{
    setHttpRequest(httpRequest);
}

HandlePath::~HandlePath()
{
}

// =====================
// ==     Getters     ==
// =====================
std::string HandlePath::getPath() const
{
    return _path;
}

void HandlePath::setPath(std::string path)
{
    _path = path;
}

bool HandlePath::getIsAutoIndex() const
{
    return _isAutoIndex;
}

void HandlePath::setIsAutoIndex(bool isAutoIndex)
{
    _isAutoIndex = isAutoIndex;
}

HttpRequest *HandlePath::getHttpRequest() const
{
    return _httpRequest;
}

void HandlePath::setHttpRequest(HttpRequest *httpRequest)
{
    if (httpRequest == NULL)
        throw ExecptionErrorUninitializedVariable("*httpRequest", "HandlePath");

    _httpRequest = httpRequest;
}

// =====================
// == 	  Member	  ==
// =====================
std::string HandlePath::createPath(Location *location)
{
    if (location != NULL)
        return createPathWithLocation(location);

    return createPathWithServer();
}

std::string HandlePath::selectRoot(Location *location)
{
    std::string pathRoot;

    if (location != NULL && location->getRoot() != "")
        pathRoot = location->getRoot();
    else
        pathRoot = getHttpRequest()->getRequestContext()->getServer()->getRoot();

    // A server whose content is entirely defined by locations is valid, but an
    // unmatched URI must never be resolved against the process cwd or host /.
    if (pathRoot.empty())
        throw std::runtime_error("404");

    return pathRoot;
}

std::string HandlePath::requestUriPath()
{
    std::string uri = getHttpRequest()->getHeader()->getScriptName();

    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos)
        uri = uri.substr(0, qpos);

    return uri;
}

std::string HandlePath::mapUriToLocation(Location *location)
{
    return joinPath(selectRoot(location), requestUriPath());
}

bool HandlePath::isRequestForLocationRoot(const std::string &locationName)
{
    std::string uri = getHttpRequest()->getHeader()->getScriptName();

    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos)
        uri = uri.substr(0, qpos);

    while (uri.size() > 1 && uri[uri.size() - 1] == '/')
        uri.erase(uri.size() - 1);

    std::string loc = locationName;
    while (loc.size() > 1 && loc[loc.size() - 1] == '/')
        loc.erase(loc.size() - 1);

    return uri == loc;
}

std::string HandlePath::createPathPost(Location *location, const std::string &base)
{
    std::string uploadStore = location->getUploadStore();

    if (isRequestForLocationRoot(location->getName()))
        return (uploadStore != "") ? uploadStore : base;

    if (uploadStore == "")
        throw std::runtime_error("404");

    return uploadStore;
}

std::string HandlePath::createPathWithLocation(Location *location)
{
    std::string base = mapUriToLocation(location);
    HttpMethod method = getHttpRequest()->getHeader()->getMethod();

    if (method == POST)
        return createPathPost(location, base);

    // The URI points straight at an existing file
    if (isFinishByFile(base))
        return base;

    if (method == GET || method == HEAD)
    {
        if (location->getIndex() != "")
        {
            std::string indexPath = joinPath(base, location->getIndex());
            if (isFinishByFile(indexPath))
                return indexPath;
        }

        if (location->getAutoIndex() && isFinishByFolder(base))
        {
            setIsAutoIndex(true);
            return base;
        }
    }

    if (!isRequestForLocationRoot(location->getName()))
        throw std::runtime_error("404");

    return createPathWithServer();
}

// In case of POST on CGI we need to have a ≠ path. We need to set to the CGI itself
std::string HandlePath::createPathCgi(Location *location)
{
    std::string base;

    if (location != NULL)
        base = mapUriToLocation(location);
    else
        base = joinPath(selectRoot(NULL), requestUriPath());

    if (!isFinishByFile(base))
        throw std::runtime_error("404");

    return base;
}

std::string HandlePath::createPathWithServer()
{
    std::string pathFile;
    std::string checkPath;
    std::string pathRoot;
    std::string index;

    pathRoot = selectRoot(NULL);
    HttpMethod method = getHttpRequest()->getHeader()->getMethod();

    bool noLocation = (getHttpRequest()->getRequestContext()->getLocation() == NULL);

    if (method == POST)
    {
        if (noLocation && !isRequestForLocationRoot("/"))
            throw std::runtime_error("404");
        return pathRoot;
    }

    pathFile = joinPath(pathRoot, requestUriPath());
    if (isFinishByFile(pathFile))
        return pathFile;

    // Without location, an URI that matches no file must not fall back on the index
    if (noLocation && !isRequestForLocationRoot("/"))
        throw std::runtime_error("404");

    if (method == GET || method == HEAD)
    {
        for (size_t i = 0; (index = getHttpRequest()->getRequestContext()->getServer()->getIndex(i)) != ""; ++i)
        {
            checkPath = joinPath(pathRoot, index);
            if (access(checkPath.c_str(), F_OK) != -1 && access(checkPath.c_str(), R_OK) != -1)
                return (checkPath);
        }

        if (getHttpRequest()->getRequestContext()->getServer()->getAutoIndex())
        {
            setIsAutoIndex(true);
            return pathRoot;
        }
    }
    throw std::runtime_error("404");
}

void HandlePath::listContentFolder(const std::string &path, std::vector<std::string> &entries)
{
    DIR *dir;
    struct dirent *ent;

    std::cout << "Auto index\n";
    std::cout << "Path: " << path << "\n";

    if ((dir = opendir(path.c_str())) == NULL)
        throw std::runtime_error("500");

    while ((ent = readdir(dir)) != NULL)
    {
        std::string name = ent->d_name;
        if (name == ".")
            continue;

        if (ent->d_type == DT_DIR)
            name += "/";

        entries.push_back(name);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());
}

static std::string escapeHtml(const std::string &text)
{
    std::string escaped;

    for (std::string::size_type i = 0; i < text.size(); ++i)
    {
        if (text[i] == '&')
            escaped += "&amp;";
        else if (text[i] == '<')
            escaped += "&lt;";
        else if (text[i] == '>')
            escaped += "&gt;";
        else if (text[i] == '"')
            escaped += "&quot;";
        else if (text[i] == '\'')
            escaped += "&#39;";
        else
            escaped += text[i];
    }
    return escaped;
}

static std::string encodeUriPath(const std::string &path)
{
    const std::string unreserved = "-_.~/";
    const char *hexDigits = "0123456789ABCDEF";
    std::string encoded;

    for (std::string::size_type i = 0; i < path.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(path[i]);

        if (std::isalnum(c) || unreserved.find(static_cast<char>(c)) != std::string::npos)
            encoded += static_cast<char>(c);
        else
        {
            encoded += '%';
            encoded += hexDigits[(c >> 4) & 0x0F];
            encoded += hexDigits[c & 0x0F];
        }
    }
    return encoded;
}

std::string HandlePath::createContentAutoIndex(const std::string &path)
{
    std::string uri = requestUriPath();
    if (uri.empty() || uri[uri.size() - 1] != '/')
        uri += '/';

    std::vector<std::string> entries;
    listContentFolder(path, entries);

    std::string title = escapeHtml(uri);
    std::string payload = "<!DOCTYPE html>\n<html><head><meta charset=\"UTF-8\"><title>Index of ";
    payload += title;
    payload += "</title></head><body><h1>Index of ";
    payload += title;
    payload += "</h1><ul>";

    for (size_t i = 0; i < entries.size(); ++i)
    {
        payload += "<li><a href=\"";
        payload += escapeHtml(encodeUriPath(uri + entries[i]));
        payload += "\">";
        payload += escapeHtml(entries[i]);
        payload += "</a></li>";
    }

    payload += "</ul></body></html>";

    return payload;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AResponse.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 17:25:00 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:42:07 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "AResponse.hpp"

#include "ARequest.hpp"
#include "Body.hpp"
#include "Client.hpp"
#include "Cookie.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"
#include "Webserv.hpp"

#include "execption.hpp"
#include "utilsDuplicate.hpp"
#include "utilsResponse.hpp"

#include <cctype>

// =====================
// == Canonical Form  ==
// =====================
AResponse::AResponse(HttpResponse *httpResponse, int statusCode)
    : _httpResponse(NULL), _statusCode(-1), _statusMessage("")
{
    setHttpResponse(httpResponse);
    updateCodeError(statusCode);
}

AResponse::~AResponse()
{
}

// =====================
// ==     Getters     ==
// =====================
int AResponse::getStatusCode() const
{
    return _statusCode;
}

void AResponse::setStatusCode(int statusCode)
{
    _statusCode = statusCode;
}

std::string AResponse::getStatusMessage() const
{
    return _statusMessage;
}

void AResponse::setStatusMessage(const std::string &statusMessage)
{
    _statusMessage = statusMessage;
}

HeaderContent AResponse::getHeaderContent(void) const
{
    return _headerContent;
}

// Header names are case insensitive: they are folded to the conventional
// capitalised form so "Content-Type" and "content-type" cannot both be emitted,
// while the response still reads like the one of any HTTP/1.1 server
static std::string canonicalHeaderName(const std::string &key)
{
    std::string canonical = toLowerString(key);
    bool startOfWord = true;

    for (std::string::size_type i = 0; i < canonical.size(); ++i)
    {
        if (startOfWord)
            canonical[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(canonical[i])));

        startOfWord = (canonical[i] == '-');
    }
    return canonical;
}

void AResponse::addHeaderContent(std::string key, std::string value)
{
    _headerContent[canonicalHeaderName(key)] = value;
}

bool AResponse::hasHeader(const std::string &key) const
{
    return (_headerContent.find(canonicalHeaderName(key)) != _headerContent.end());
}

void AResponse::setHeaderContent(HeaderContent headerContent)
{
    _headerContent = headerContent;
}

void AResponse::addSetCookie(const std::string &setCookieValue)
{
    _setCookies.push_back(setCookieValue);
}

HttpResponse *AResponse::getHttpResponse() const
{
    return _httpResponse;
}

void AResponse::setHttpResponse(HttpResponse *httpResponse)
{
    if (httpResponse == NULL)
        throw ExecptionErrorUninitializedVariable("*httpResponse", "AResponse");

    _httpResponse = httpResponse;
}

// =====================
// == 	  Methods	  ==
// =====================

void AResponse::updateCodeError(int statusCode)
{
    setStatusCode(statusCode);
    setStatusMessage(httpStatusToString(statusCode));
}

std::string AResponse::makeStatusLine()
{
    std::string firstLine;
    firstLine += "HTTP/1.1";

    firstLine += " ";
    firstLine += intToString(getStatusCode());

    firstLine += " ";
    firstLine += getStatusMessage();
    firstLine += "\r\n";

    return firstLine;
}

void AResponse::makeHeader()
{
    addHeaderContent("date", makeHttpDate());
    handleSession();
    handleConnection();
}

void AResponse::handleConnection()
{
    bool closeConnection = true;

    if (getStatusCode() < 400)
    {
        HttpRequest *httpRequest = getHttpResponse()->getARequest()->getRequestContext()->getHttpRequest();
        Body *body = (httpRequest != NULL) ? httpRequest->getBody() : NULL;

        closeConnection = (body != NULL) ? !body->getKeepAlive() : false;
    }

    addHeaderContent("Connection", closeConnection ? "close" : "keep-alive");
    getHttpResponse()->setShouldCloseConnection(closeConnection);
}

void AResponse::handleSession()
{
    ARequest *arequest = getHttpResponse()->getARequest();
    Client *client = arequest->getRequestContext()->getClient();
    HttpRequest *httpRequest = arequest->getRequestContext()->getHttpRequest();

    if (client == NULL || httpRequest == NULL)
        return;

    bool isNewSession = false;
    if (!client->hasSession())
    {
        CookieMap cookies = httpRequest->getCookies();
        CookieMap::const_iterator it = cookies.find("session_id");

        if (it != cookies.end() && !it->second.empty())
            client->setSessionId(it->second);
        else
        {
            client->setSessionId(generateSessionId());
            isNewSession = true;
        }
    }

    int visits = client->getWebserv()->touchSession(client->getSessionId());

    if (isNewSession)
    {
        Cookie session("session_id", client->getSessionId());
        session.setPath("/");
        session.setHttpOnly(true);
        session.setMaxAge(3600);
        addSetCookie(session.toSetCookieValue());
    }
    addHeaderContent("X-Visit-Count", intToString(visits));

    const std::vector<std::string> &cgiCookies = arequest->getResponseContext()->getCgiSetCookies();
    for (size_t i = 0; i < cgiCookies.size(); ++i)
        addSetCookie(cgiCookies[i]);
}

void AResponse::applyCgiHeaders()
{
    const HeaderContent &cgiHeaders = getHttpResponse()->getARequest()->getResponseContext()->getCgiHeaders();

    for (HeaderContent::const_iterator it = cgiHeaders.begin(); it != cgiHeaders.end(); ++it)
        addHeaderContent(it->first, it->second);
}

std::string AResponse::makeHttpDate()
{
    time_t now = time(NULL);

    struct tm *gmt = gmtime(&now);
    if (gmt == NULL)
        return "Thu, 01 Jan 1970 00:00:00 GMT";
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);

    return std::string(buffer);
}

std::string AResponse::headerToString()
{
    std::string header;

    HeaderContent headerContent = getHeaderContent();
    for (HeaderContent::iterator it = headerContent.begin(); it != headerContent.end(); ++it)
    {
        header += it->first;
        header += ": ";
        header += it->second;
        header += "\r\n";
    }

    // Cookies <-> CGI
    for (size_t i = 0; i < _setCookies.size(); ++i)
    {
        header += "Set-Cookie: ";
        header += _setCookies[i];
        header += "\r\n";
    }
    return header;
}

bool AResponse::containsHtmlTags(const std::string &body)
{
    return (body.find("<!DOCTYPE html>") != std::string::npos && body.find("</html>") != std::string::npos);
}

void AResponse::applyContentType(const std::string &body)
{
    if (hasHeader("content-type"))
        return;

    std::string contentType = getHttpResponse()->getARequest()->getResponseContext()->getContentType();

    if (contentType.empty() && containsHtmlTags(body))
        contentType = "text/html";
    if (contentType.empty())
        contentType = "text/plain";

    addHeaderContent("content-type", contentType);
}

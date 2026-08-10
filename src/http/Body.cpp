/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Body.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/14 19:53:56 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:07:02 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Body.hpp"

#include "ARequest.hpp"
#include "Client.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "RequestContext.hpp"

#include "colors.hpp"
#include "execption.hpp"
#include "utilsDuplicate.hpp"
#include "utilsRequest.hpp"

// =====================
// ==       OCF       ==
// =====================
Body::Body(const HttpRequest &httpRequest)
    : _bodyContent(0), _keepAlive(true), _contentLength(0), _totalChunked(0), _httpRequest(NULL)
{
    setHttpRequest(const_cast<HttpRequest *>(&httpRequest));
}

Body::~Body()
{
}

Body::Body(const Body &other)
{
    *this = other;
}

Body &Body::operator=(const Body &other)
{
    if (this != &other)
    {
        _bodyContent = other._bodyContent;
        _keepAlive = other._keepAlive;
        _contentLength = other._contentLength;
        _totalChunked = other._totalChunked;
        _httpRequest = other._httpRequest;
    }
    return (*this);
}

// =====================
// ==     Getters     ==
// =====================
const BodyContent &Body::getBodyContent(void) const
{
    return _bodyContent;
}

void Body::setBodyContent(BodyContent body)
{
    _bodyContent = body;
}

void Body::addBodyContent(uint8_t bytes)
{
    _bodyContent.push_back(bytes);
}

bool Body::getKeepAlive(void) const
{
    return _keepAlive;
}

void Body::setKeepAlive(bool keepAlive)
{
    _keepAlive = keepAlive;
}

size_t Body::getContentLenght(void) const
{
    return _contentLength;
}

void Body::setContentLenght(size_t contentLength)
{
    _contentLength = contentLength;
}

size_t Body::getTotalChunked(void) const
{
    return _totalChunked;
}

void Body::setTotalChunked(size_t totalChunked)
{
    _totalChunked = totalChunked;
}
void Body::addTotalChunked(size_t totalChunked)
{
    _totalChunked += totalChunked;
}

HttpRequest *Body::getHttpRequest(void) const
{
    return _httpRequest;
}

void Body::setHttpRequest(HttpRequest *httpRequest)
{
    if (httpRequest == NULL)
        throw ExecptionErrorUninitializedVariable("*httpRequest", "Body");

    _httpRequest = httpRequest;
}

// =====================
// == 	  Member	  ==
// =====================
void Body::initialisationBody()
{
    parseBody(getHttpRequest()->getRequestContext()->getClient()->getContentRequest());
}

void Body::bodyprint(void)
{
    std::cout << YELLOW << "_body content |";
    for (std::vector<uint8_t>::const_iterator it = _bodyContent.begin(); it != _bodyContent.end(); it++)
        std::cout << *it;
    std::cout << "|" << RESET << std::endl;
}

void Body::configureKeepAlive(const HeaderContent &header)
{
    HeaderContent::const_iterator connectionIt = header.find("connection");
    HeaderContent::const_iterator itEnd = header.end();

    if (connectionIt != itEnd && toLowerString(connectionIt->second) == "close")
        setKeepAlive(false);
}

bool Body::handleTransferEncoding(const HeaderContent &header, const std::string &headerContent)
{
    HeaderContent::const_iterator itTransferEncoding = header.find("transfer-encoding");
    HeaderContent::const_iterator itContentLength = header.find("content-length");
    HeaderContent::const_iterator itEnd = header.end();

    if (itTransferEncoding != itEnd && itContentLength != itEnd)
        throw std::runtime_error("400");

    if (itTransferEncoding == itEnd)
        return false;

    if (toLowerString(itTransferEncoding->second) != "chunked")
        throw std::runtime_error("501");

    std::cout << GREEN << "Body treatment method : " << itTransferEncoding->first << " | " << itTransferEncoding->second
              << RESET << std::endl;

    parseChunkedBody(headerContent);
    setContentLenght(getBodyContent().size());
    return true;
}

bool Body::parseContentLengthBody(const HeaderContent &header, const std::string &headerContent)
{
    HeaderContent::const_iterator itContentLength = header.find("content-length");
    HeaderContent::const_iterator itEnd = header.end();

    if (itContentLength != itEnd)
    {
        size_t declaredLength = 0;
        if (!parseDecimalLength(itContentLength->second, declaredLength))
            throw std::runtime_error("400");
        setContentLenght(declaredLength);

        if (getContentLenght() > initMaxBodySize())
            throw std::runtime_error("413");

        std::string::size_type bodyStart = headerContent.find("\r\n\r\n");
        if (bodyStart == std::string::npos)
            throw std::runtime_error("400");

        bodyStart += 4;
        if (bodyStart + _contentLength > headerContent.size())
            throw std::runtime_error("400");

        std::cout << GREEN << "Body treatment method : " << itContentLength->first << " | " << itContentLength->second
                  << RESET << std::endl;
        _bodyContent.reserve(_contentLength);
        _bodyContent.insert(_bodyContent.end(), headerContent.begin() + bodyStart,
                            headerContent.begin() + bodyStart + _contentLength);
        return true;
    }
    return false;
}

void Body::parseBody(const std::string &headerContent)
{
    _bodyContent.clear();

    HeaderContent header = getHttpRequest()->getHeader()->getHeaderContent();

    configureKeepAlive(header);

    if (handleTransferEncoding(header, headerContent))
        return;

    if (parseContentLengthBody(header, headerContent))
        return;

    if (getHttpRequest()->getHeader()->getMethod() == POST)
        throw std::runtime_error("411");
}

void Body::appendBodyBytes(const std::string &data)
{
    _bodyContent.insert(_bodyContent.end(), data.begin(), data.end());
}

size_t Body::initMaxBodySize()
{
    Location *location = getHttpRequest()->getRequestContext()->getLocation();

    if (location != NULL && location->getClientMaxBodySize() != 0)
        return location->getClientMaxBodySize();

    size_t serverMaxBodySize =
        getHttpRequest()->getRequestContext()->getClient()->getServerPtr()->getClientMaxBodySize();
    if (serverMaxBodySize != 0)
        return serverMaxBodySize;

    return DEFAULT_CLIENT_MAX_BODY_SIZE;
}

bool Body::handleLastChunk(const std::string &headerContent, size_t chunkSize, std::string::size_type current)
{
    if (chunkSize == 0)
    {
        if (headerContent.substr(current, 2) == "\r\n")
            return true;

        std::string::size_type trailersEnd = headerContent.find("\r\n\r\n", current);
        if (trailersEnd == std::string::npos)
            throw std::runtime_error("400");
        return true;
    }
    return false;
}

std::string::size_type Body::initCurrent(const std::string &headerContent)
{
    std::string::size_type current = headerContent.find("\r\n\r\n");
    if (current == std::string::npos)
        return 0;
    return current + 4;
}

void Body::updateTotalChunked(size_t chunkSize, size_t maxBodySize)
{
    addTotalChunked(chunkSize);
    std::cout << RED << "Total Chunked :" << getTotalChunked() << GREEN << "Max Body Size: " << maxBodySize << RESET
              << std::endl;

    if (getTotalChunked() > maxBodySize)
        throw std::runtime_error("413");
}

void Body::parseChunkedBody(const std::string &headerContent)
{
    size_t current = initCurrent(headerContent);

    if (current == 0)
        return;

    size_t maxBodySize = initMaxBodySize();

    while (current < headerContent.size())
    {
        std::string::size_type lineEnd = headerContent.find("\r\n", current);
        if (lineEnd == std::string::npos)
            throw std::runtime_error("400");

        size_t chunkSize = parseChunkSize(headerContent.substr(current, lineEnd - current));

        updateTotalChunked(chunkSize, maxBodySize);

        current = lineEnd + 2;
        if (handleLastChunk(headerContent, chunkSize, current))
            return;

        if (chunkSize > headerContent.size() - current)
            throw std::runtime_error("400");

        appendBodyBytes(headerContent.substr(current, chunkSize));
        current += chunkSize;

        if (headerContent.substr(current, 2) != "\r\n")
            throw std::runtime_error("400");

        current += 2;
    }
}

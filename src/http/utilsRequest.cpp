/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsRequest.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/21 13:53:46 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:15:18 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utilsRequest.hpp"
#include "utilsDuplicate.hpp"

#include <limits>

std::string stripTrailingSlashes(const std::string &path)
{
    std::string result = path;

    while (result.size() > 1 && result[result.size() - 1] == '/')
        result.erase(result.size() - 1);

    return result;
}

int longestPrefixMatch(std::string uri, std::string location)
{
    std::string cleanUri = stripTrailingSlashes(uri);
    std::string cleanLocation = stripTrailingSlashes(location);

    if (cleanLocation.empty())
        return (-1);

    if (cleanLocation == "/")
        return (0);

    if (cleanUri.compare(0, cleanLocation.size(), cleanLocation) != 0)
        return (-1);

    if (cleanUri.size() != cleanLocation.size() && cleanUri[cleanLocation.size()] != '/')
        return (-1);

    return (static_cast<int>(cleanLocation.size()));
}

static int hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    if (c >= 'A' && c <= 'F')
        return (c - 'A' + 10);
    return (-1);
}

std::string percentDecode(const std::string &value)
{
    std::string decoded;
    decoded.reserve(value.size());

    for (std::string::size_type i = 0; i < value.size(); ++i)
    {
        if (value[i] != '%')
        {
            decoded += value[i];
            continue;
        }

        if (i + 2 >= value.size())
            throw std::runtime_error("400");

        int high = hexValue(value[i + 1]);
        int low = hexValue(value[i + 2]);
        if (high < 0 || low < 0)
            throw std::runtime_error("400");

        char decodedChar = static_cast<char>((high << 4) | low);
        if (decodedChar == '\0')
            throw std::runtime_error("400");

        decoded += decodedChar;
        i += 2;
    }
    return (decoded);
}

std::string normalizeUriPath(const std::string &path)
{
    if (path.empty() || path[0] != '/')
        throw std::runtime_error("400");

    std::vector<std::string> segments;
    std::string::size_type current = 0;

    while (current < path.size())
    {
        std::string::size_type next = path.find('/', current);
        if (next == std::string::npos)
            next = path.size();

        std::string segment = path.substr(current, next - current);

        if (segment == "..")
        {
            if (segments.empty())
                throw std::runtime_error("403");
            segments.pop_back();
        }
        else if (!segment.empty() && segment != ".")
            segments.push_back(segment);

        current = next + 1;
    }

    std::string normalized = "/";
    for (size_t i = 0; i < segments.size(); ++i)
    {
        if (i != 0)
            normalized += '/';
        normalized += segments[i];
    }

    if (normalized.size() > 1 && path[path.size() - 1] == '/')
        normalized += '/';

    return (normalized);
}

static std::string trimChunkSizeToken(const std::string &token)
{
    std::string::size_type begin = token.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return ("");
    std::string::size_type end = token.find_last_not_of(" \t");
    return (token.substr(begin, end - begin + 1));
}

size_t parseChunkSize(const std::string &line)
{
    std::string sizeToken = line;
    std::string::size_type semicolon = sizeToken.find(';');
    if (semicolon != std::string::npos)
        sizeToken = sizeToken.substr(0, semicolon);
    sizeToken = trimChunkSizeToken(sizeToken);
    if (sizeToken.empty())
        throw std::runtime_error("400");

    std::stringstream ss(sizeToken);
    size_t chunkSize = 0;
    ss >> std::hex >> chunkSize;
    if (ss.fail())
        throw std::runtime_error("400");
    if (!ss.eof())
        throw std::runtime_error("400");
    return (chunkSize);
}

std::string returnLastElementPath(std::string path)
{
    std::string sub_string;
    std::vector<std::string> token_str;

    // Avoid index redirection when Query is present, parse '?'!!
    std::string::size_type qpos = path.find('?');
    if (qpos != std::string::npos)
        path = path.substr(0, qpos);

    std::stringstream iss1(path);
    while (getline(iss1, sub_string, '/'))
        ;
    return sub_string;
}

void checkPermisionReadFile(std::string path)
{
    if (access(path.c_str(), F_OK) == -1)
        throw std::runtime_error("404");

    if (access(path.c_str(), R_OK) == -1)
        throw std::runtime_error("403");
}

bool isFinishByFile(std::string path)
{
    struct stat buff;

    if (access(path.c_str(), F_OK) == -1)
        return false;

    if (stat(path.c_str(), &buff) != 0)
        return false;

    if (S_ISREG(buff.st_mode))
        return true;

    return false;
}

bool isFinishByFolder(std::string path)
{
    struct stat buff;

    if (stat(path.c_str(), &buff) != 0)
        return false;

    return (S_ISDIR(buff.st_mode));
}

// std::hex find a real exadecimal to deal with method chunked that is expecting and send them 
static std::string::size_type finalChunkEnd(const std::string &request, std::string::size_type current)
{
    if (request.size() < current + 2)
        return (std::string::npos);

    if (request.substr(current, 2) == "\r\n")
        return (current + 2);

    std::string::size_type trailersEnd = request.find("\r\n\r\n", current);
    if (trailersEnd == std::string::npos)
        return (std::string::npos);
    return (trailersEnd + 4);
}

static std::string::size_type chunkedRequestEnd(const std::string &request, std::string::size_type bodyStart)
{
    std::string::size_type current = bodyStart;

    while (current < request.size())
    {
        std::string::size_type lineEnd = request.find("\r\n", current);
        if (lineEnd == std::string::npos)
            return (std::string::npos);

        std::string sizeToken = initSizeToken(request, current, lineEnd);

        std::stringstream ss(sizeToken);
        size_t chunkSize = 0;
        ss >> std::hex >> chunkSize;
        if (sizeToken.empty() || ss.fail() || !ss.eof())
            return (request.size());

        current = lineEnd + 2;
        if (chunkSize == 0)
            return (finalChunkEnd(request, current));

        std::string::size_type available = request.size() - current;
        if (chunkSize > available || available - chunkSize < 2)
            return (std::string::npos);

        if (request.substr(current + chunkSize, 2) != "\r\n")
            return (request.size());

        current += chunkSize + 2;
    }
    return (std::string::npos);
}

size_t completeRequestLength(const std::string &request)
{
    std::string::size_type headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return (0);

    std::string::size_type bodyStart = headerEnd + 4;
    std::string transferEncoding = getHeaderValue(request, "transfer-encoding");
    transferEncoding = toLowerString(trimSpaces(transferEncoding));
    if (transferEncoding == "chunked")
    {
        std::string::size_type end = chunkedRequestEnd(request, bodyStart);
        return ((end == std::string::npos) ? 0 : end);
    }

    std::string contentLengthValue = getHeaderValue(request, "content-length");
    if (!contentLengthValue.empty())
    {
        size_t contentLength = 0;
        if (!parseDecimalLength(contentLengthValue, contentLength))
            return (request.size());
        if ((request.size() - bodyStart) < contentLength)
            return (0);
        return (bodyStart + contentLength);
    }
    return (bodyStart);
}

bool isCompleteRequest(const std::string &request)
{
    return (completeRequestLength(request) > 0);
}

bool isDeclaredBodySizeExceeding(const std::string &request, size_t maxBodySize)
{
    if (maxBodySize == 0)
        return (false);

    std::string::size_type headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return (false);

    std::string transferEncoding = toLowerString(trimSpaces(getHeaderValue(request, "transfer-encoding")));
    if (transferEncoding == "chunked")
    {
        std::string::size_type bodyStart = headerEnd + 4;
        std::string::size_type lineEnd = request.find("\r\n", bodyStart);
        if (lineEnd == std::string::npos)
            return (false);

        std::string sizeToken = initSizeToken(request, bodyStart, lineEnd);
        std::stringstream ss(sizeToken);
        size_t chunkSize = 0;
        if (sizeToken.empty() || !(ss >> std::hex >> chunkSize) || !ss.eof())
            return (false);
        return (chunkSize > maxBodySize);
    }

    std::string contentLengthValue = getHeaderValue(request, "content-length");
    if (!contentLengthValue.empty())
    {
        size_t contentLength = 0;
        if (!parseDecimalLength(contentLengthValue, contentLength))
            return (false);
        return (contentLength > maxBodySize);
    }
    return (false);
}

bool isCompleteChunkedBody(const std::string &request, std::string::size_type bodyStart)
{
    return (chunkedRequestEnd(request, bodyStart) != std::string::npos);
}

bool isFinalChunkComplete(const std::string &request, std::string::size_type current)
{
    return (finalChunkEnd(request, current) != std::string::npos);
}

std::string initSizeToken(const std::string &request, const std::string::size_type &current,
                          const std::string::size_type &lineEnd)
{
    std::string sizeToken = request.substr(current, lineEnd - current);
    std::string::size_type semicolon = sizeToken.find(';');

    if (semicolon != std::string::npos)
        sizeToken = sizeToken.substr(0, semicolon);

    sizeToken = trimSpaces(sizeToken);
    return sizeToken;
}

bool parseDecimalLength(const std::string &value, size_t &contentLength)
{
    if (value.empty())
        return (false);

    size_t parsed = 0;
    for (std::string::size_type i = 0; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
            return (false);

        size_t digit = static_cast<size_t>(value[i] - '0');
        if (parsed > (std::numeric_limits<size_t>::max() - digit) / 10)
            return (false);

        parsed = parsed * 10 + digit;
    }

    contentLength = parsed;
    return (true);
}

std::string getHeaderValue(const std::string &request, const std::string &headerName)
{
    std::string::size_type headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return ("");

    std::string loweredHeaderName = toLowerString(headerName);
    std::string::size_type current = request.find("\r\n");
    if (current == std::string::npos)
        return ("");
    current += 2;

    while (current < headerEnd)
    {
        std::string::size_type lineEnd = request.find("\r\n", current);
        if (lineEnd == std::string::npos || lineEnd > headerEnd)
            break;

        std::string line = request.substr(current, lineEnd - current);
        std::string::size_type colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string key = toLowerString(trimSpaces(line.substr(0, colon)));
            if (key == loweredHeaderName)
                return (trimSpaces(line.substr(colon + 1)));
        }
        current = lineEnd + 2;
    }
    return ("");
}
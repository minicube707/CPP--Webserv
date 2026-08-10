/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PostMethod.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:46:04 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:01:09 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "PostMethod.hpp"

#include "ARequest.hpp"
#include "Body.hpp"
#include "HandlePath.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"

#include "utilsDuplicate.hpp"
#include "utilsParsing.hpp"
#include "utilsRequest.hpp"

// =====================
// ==       OCF       ==
// =====================
PostMethod::PostMethod(HttpRequest *httpRequest) : AMethod(httpRequest, POST)
{
}

PostMethod::~PostMethod()
{
}

PostMethod::PostMethod(const PostMethod &other) : AMethod(other)
{
    *this = other;
}

PostMethod &PostMethod::operator=(const PostMethod &other)
{
    AMethod::operator=(other);
    return (*this);
}

// =====================
// ==     Helpers     ==
// =====================

static std::string sanitizeFileName(const std::string &name)
{
    std::string::size_type slash = name.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? name : name.substr(slash + 1);

    if (base == "." || base == "..")
        return "";

    return base;
}

static std::string extractBoundary(const std::string &contentType)
{
    std::string lowered = toLowerString(contentType);

    if (lowered.find("multipart/form-data") == std::string::npos)
        return "";

    std::string::size_type pos = lowered.find("boundary=");
    if (pos == std::string::npos)
        return "";

    std::string boundary = trimSpaces(contentType.substr(pos + 9));

    std::string::size_type end = boundary.find(';');
    if (end != std::string::npos)
        boundary = trimSpaces(boundary.substr(0, end));

    if (boundary.size() >= 2 && boundary[0] == '"' && boundary[boundary.size() - 1] == '"')
        boundary = boundary.substr(1, boundary.size() - 2);

    return boundary;
}

static std::string extractPartFileName(const std::string &partHeaders)
{
    const std::string marker = "filename=\"";

    std::string::size_type pos = toLowerString(partHeaders).find(marker);
    if (pos == std::string::npos)
        return "";

    std::string::size_type start = pos + marker.size();
    std::string::size_type end = partHeaders.find('"', start);
    if (end == std::string::npos)
        return "";

    return sanitizeFileName(partHeaders.substr(start, end - start));
}

static bool parseMultipartBody(const std::string &contentType, std::string &content, std::string &filename)
{
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty())
        return false;

    std::string delimiter = "--" + boundary;
    std::string::size_type current = content.find(delimiter);

    while (current != std::string::npos)
    {
        std::string::size_type partStart = current + delimiter.size();
        if (partStart + 2 > content.size() || content.compare(partStart, 2, "--") == 0)
            break; // closing delimiter, nothing left to read

        std::string::size_type headersEnd = content.find("\r\n\r\n", partStart);
        if (headersEnd == std::string::npos)
            break;

        std::string::size_type bodyStart = headersEnd + 4;
        std::string::size_type bodyEnd = content.find("\r\n" + delimiter, bodyStart);
        if (bodyEnd == std::string::npos)
            break;

        std::string partFileName = extractPartFileName(content.substr(partStart, headersEnd - partStart));
        if (!partFileName.empty())
        {
            filename = partFileName;
            content = content.substr(bodyStart, bodyEnd - bodyStart);
            return true;
        }
        current = content.find(delimiter, bodyEnd);
    }
    return false;
}

// =====================
// == 	  Member	  ==
// =====================


std::string PostMethod::uriFileName(Location *location)
{
    std::string uri = getHttpRequest()->getHeader()->getScriptName();

    if (uri.empty() || uri[uri.size() - 1] == '/')
        return "";

    HandlePath handlePath(getHttpRequest());
    if (location != NULL && handlePath.isRequestForLocationRoot(location->getName()))
        return "";
    if (location == NULL && handlePath.isRequestForLocationRoot("/"))
        return "";

    return sanitizeFileName(uri.substr(uri.find_last_of('/') + 1));
}

std::string PostMethod::requestContentType()
{
    HeaderContent headers = getHttpRequest()->getHeader()->getHeaderContent();
    HeaderContent::const_iterator it = headers.find("content-type");

    return (it == headers.end()) ? "" : it->second;
}

void PostMethod::writeUploadedFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);

    if (!file.is_open())
        throw std::runtime_error("500");

    file.write(content.c_str(), static_cast<std::streamsize>(content.size()));
    if (file.fail())
    {
        file.close();
        throw std::runtime_error("500");
    }
    file.close();
}

std::string PostMethod::applyMethod(Location *location)
{
    HandlePath handlePath(getHttpRequest());
    std::string directory = handlePath.createPath(location);

    const BodyContent &body = getHttpRequest()->getBody()->getBodyContent();
    std::string content;
    content.assign(body.begin(), body.end());

    std::string filename = uriFileName(location);
    std::string multipartName;

    if (parseMultipartBody(requestContentType(), content, multipartName) && filename.empty())
        filename = multipartName;

    if (filename.empty())
        filename = "PostContent";

    std::string path = joinPath(directory, filename);
    std::cout << "Path to write: " << path << "\n";

    writeUploadedFile(path, content);

    if (location != NULL && location->getUploadStore() != "")
    {
        getHttpRequest()->getRequestContext()->getARequest()->getResponseContext()->setStatusCode(201);
        return "Uploaded " + filename + "\n";
    }

    return "";
}

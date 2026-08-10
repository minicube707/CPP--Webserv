/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetMethod.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 14:13:06 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:58:25 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "GetMethod.hpp"

#include "ARequest.hpp"
#include "HandlePath.hpp"
#include "HttpRequest.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"

#include "utilsParsing.hpp"
#include "utilsRequest.hpp"
#include "utilsResponse.hpp"

// =====================
// ==       OCF       ==
// =====================
GetMethod::GetMethod(HttpRequest *httpRequest) : AMethod(httpRequest, GET)
{
}

GetMethod::~GetMethod()
{
}

GetMethod::GetMethod(const GetMethod &other) : AMethod(other)
{
    *this = other;
}

GetMethod &GetMethod::operator=(const GetMethod &other)
{
    AMethod::operator=(other);
    return (*this);
}

// =====================
// == 	  Member	  ==
// =====================
std::string GetMethod::applyMethod(Location *location)
{
    std::string contentFile;

    HandlePath handlePath(getHttpRequest());
    std::string path = handlePath.createPath(location);

    ResponseContext *responseContext = getHttpRequest()->getRequestContext()->getARequest()->getResponseContext();

    if (handlePath.getIsAutoIndex())
    {
        responseContext->setContentType("text/html");
        return handlePath.createContentAutoIndex(path);
    }

    std::string::size_type qpos = path.find('?');
    if (qpos != std::string::npos)
        path = path.substr(0, qpos);

    std::cout << "Path to read: " << path << "\n";

    checkPermisionReadFile(path);

    readRawFile(path.c_str(), contentFile);

    responseContext->setContentType(mimeTypeFromPath(path));

    return contentFile;
}

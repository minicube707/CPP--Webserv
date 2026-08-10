/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ARequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 14:35:51 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/06 00:25:58 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ARequest.hpp"

#include "Client.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"

#include "execption.hpp"

// =====================
// == Canonical Form  ==
// =====================

ARequest::ARequest() : _requestContext(NULL), _responseContext(NULL)
{
}


ARequest::ARequest(const ARequest &other)
    : _requestContext(NULL), _responseContext(NULL)
{
    RequestContext *requestContext = NULL;
    ResponseContext *responseContext = NULL;
    try
    {
        requestContext = new RequestContext(*other._requestContext);
        responseContext = new ResponseContext(*other._responseContext);
    }
    catch (...)
    {
        delete requestContext;
        delete responseContext;
        throw;
    }

    _requestContext = requestContext;
    _responseContext = responseContext;
    _requestContext->setARequest(this);
    _responseContext->setARequest(this);
}

ARequest::~ARequest()
{
    delete _requestContext;
    delete _responseContext;
}

// =====================
// == Getter & Setter ==
// =====================

RequestContext *ARequest::getRequestContext() const
{
    return _requestContext;
}

void ARequest::setRequestContext(RequestContext *requestContext)
{
    if (requestContext == NULL)
        throw ExecptionErrorUninitializedVariable("*requestContext", "ARequest");

    _requestContext = requestContext;
}

ResponseContext *ARequest::getResponseContext() const
{
    return _responseContext;
}

void ARequest::setResponseContext(ResponseContext *responseContext)
{
    if (responseContext == NULL)
        throw ExecptionErrorUninitializedVariable("*responseContext", "ARequest");

    _responseContext = responseContext;
}

// =====================
// ==     Method      ==
// =====================
int ARequest::initialisationARequest()
{
    RequestContext *requestContext = NULL;
    ResponseContext *responseContext = NULL;
    try
    {
        requestContext = new RequestContext(this);
        responseContext = new ResponseContext(this);
    }
    catch (...)
    {
        delete requestContext;
        delete responseContext;
        throw;
    }

    delete _requestContext;
    delete _responseContext;
    setRequestContext(requestContext);
    setResponseContext(responseContext);
    return 0;
}

void ARequest::checkAllowedMethods(Location *location)
{
    if (location == NULL)
        return;

    std::set<HttpMethod> set_allowed_methods = location->getAllowedMethods();

    if (set_allowed_methods.size() == 0)
        return;

    std::set<HttpMethod>::iterator it = set_allowed_methods.begin();
    for (; it != set_allowed_methods.end(); ++it)
    {
        if (*it == getRequestContext()->getHttpRequest()->getHeader()->getMethod())
            return;
    }
    throw std::runtime_error("405");
}

void ARequest::checkServerIsOpen()
{
    int code = getRequestContext()->getClient()->getServerPtr()->getReturn()->code;

    if (code != 0)
    {
        std::string payload = getRequestContext()->getClient()->getServerPtr()->getReturn()->value;
        getResponseContext()->setPayload(payload);
        throw std::runtime_error(intToString(code));
    }
}

void ARequest::checkLocationIsOpen(Location *location)
{
    if (location == NULL)
        return;

    int code = location->getReturn()->code;
    if (code != 0)
    {
        std::string payload = location->getReturn()->value;
        getResponseContext()->setPayload(payload);
        throw std::runtime_error(intToString(code));
    }
}

void ARequest::validateRequest()
{
    Location *location = getRequestContext()->getLocation();

    checkServerIsOpen();
    checkLocationIsOpen(location);
    checkAllowedMethods(location);
}

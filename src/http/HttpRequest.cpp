/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 13:15:18 by erpascua          #+#    #+#             */
/*   Updated: 2026/08/10 04:13:16 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

#include "Body.hpp"
#include "Client.hpp"
#include "Cookie.hpp"
#include "Header.hpp"
#include "RequestContext.hpp"

#include "DeleteMethod.hpp"
#include "GetMethod.hpp"
#include "HeadMethod.hpp"
#include "PostMethod.hpp"

// =====================
// == Canonical Form  ==
// =====================

HttpRequest::HttpRequest(RequestContext *requestContext) : _header(NULL), _body(NULL), _requestContext(requestContext)
{
}

HttpRequest::~HttpRequest()
{
    delete getHeader();
    delete getBody();
}

// Deep copy !!
HttpRequest::HttpRequest(const HttpRequest &other)
    : _header(NULL), _body(NULL), _requestContext(other._requestContext)
{
    Header *header = NULL;
    Body *body = NULL;
    try
    {
        header = new Header(*other._header);
        body = new Body(*other._body);
    }
    catch (...)
    {
        delete header;
        delete body;
        throw;
    }

    _header = header;
    _body = body;
    _body->setHttpRequest(this);
}
// =====================
// == Getter & Setter ==
// =====================
Header *HttpRequest::getHeader() const
{
    return _header;
}

void HttpRequest::setHeader(Header *header)
{
    if (header == NULL)
        throw ExecptionErrorUninitializedVariable("*header", "HttpRequest");

    _header = header;
}

RequestContext *HttpRequest::getRequestContext() const
{
    return _requestContext;
}

void HttpRequest::setRequestContext(RequestContext *requestContext)
{
    if (requestContext == NULL)
        throw ExecptionErrorUninitializedVariable("*requestContext", "HttpRequest");

    _requestContext = requestContext;
}

Body *HttpRequest::getBody() const
{
    return _body;
}

void HttpRequest::setBody(Body *body)
{
    if (body == NULL)
        throw ExecptionErrorUninitializedVariable("*body", "HttpRequest");

    _body = body;
}

// =====================
// ==     Method      ==
// =====================
void HttpRequest::initHeader(const std::string &headerContent)
{
    Header *header = new Header();
    setHeader(header);
    header->initialisationHeader(headerContent);
}

void HttpRequest::initBody()
{
    Body *body = new Body(*this);
    setBody(body);
    body->initialisationBody();
}

CookieMap HttpRequest::getCookies() const
{
    HeaderContent headers = getHeader()->getHeaderContent();
    HeaderContent::const_iterator it = headers.find("cookie");

    if (it == headers.end())
        return CookieMap();

    return parseCookieHeader(it->second);
}

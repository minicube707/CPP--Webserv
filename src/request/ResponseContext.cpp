/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseContext.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/05 21:33:55 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:40:52 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseContext.hpp"

#include "ARequest.hpp"

#include "execption.hpp"

// =====================
// == Canonical Form  ==
// =====================
ResponseContext::ResponseContext(ARequest *arequest)
    : _statusCode(200), _payload(""), _contentType(""), _cgiSetCookies(0), _cgiHeaders(), _ARequest(NULL)
{
    setARequest(arequest);
}

ResponseContext::~ResponseContext()
{
}

ResponseContext::ResponseContext(const ResponseContext &other)
    : _statusCode(other._statusCode), _payload(other._payload), _contentType(other._contentType),
      _cgiSetCookies(other._cgiSetCookies), _cgiHeaders(other._cgiHeaders), _ARequest(other._ARequest)
{
}

// =====================
// == Getter & Setter ==
// =====================

int ResponseContext::getStatusCode() const
{
    return _statusCode;
}

void ResponseContext::setStatusCode(int statusCode)
{
    _statusCode = statusCode;
}

std::string ResponseContext::getPayload() const
{
    return _payload;
}

void ResponseContext::setPayload(std::string payload)
{
    _payload = payload;
}

std::string ResponseContext::getContentType() const
{
    return _contentType;
}

void ResponseContext::setContentType(const std::string &contentType)
{
    _contentType = contentType;
}

const std::vector<std::string> &ResponseContext::getCgiSetCookies() const
{
    return _cgiSetCookies;
}

const HeaderContent &ResponseContext::getCgiHeaders() const
{
    return _cgiHeaders;
}

void ResponseContext::addCgiHeader(const std::string &key, const std::string &value)
{
    _cgiHeaders[key] = value;
}

void ResponseContext::addCgiSetCookie(const std::string &setCookieValue)
{
    _cgiSetCookies.push_back(setCookieValue);
}

ARequest *ResponseContext::getARequest(void) const
{
    return _ARequest;
}

void ResponseContext::setARequest(ARequest *arequest)
{
    if (arequest == NULL)
        throw ExecptionErrorUninitializedVariable("*arequest", "ResponseContext");

    _ARequest = arequest;
}

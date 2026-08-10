/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CorrectResponse.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 15:44:05 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:54:51 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CorrectResponse.hpp"

#include "ARequest.hpp"
#include "HttpResponse.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"
#include "Server.hpp"

// =====================
// == Canonical Form  ==
// =====================
CorrectResponse::CorrectResponse(HttpResponse *httpResponse, int statusCode) : AResponse(httpResponse, statusCode)
{
}

CorrectResponse::~CorrectResponse()
{
}

// =====================
// ==     Member      ==
// =====================
std::string CorrectResponse::getCorrectPage()
{
    for (size_t i = 0;; ++i)
    {
        HttpErrorPage *errorPage = getHttpResponse()->getARequest()->getRequestContext()->getServer()->getErrorPage(i);

        if (errorPage == NULL)
            return "";

        if (errorPage->code == getStatusCode())
        {
            addHeaderContent("content-type", "text/html");
            return errorPage->path_page;
        }
    }
}

void CorrectResponse::applyResponse()
{
    HttpResponse *response = getHttpResponse();

    std::string statusLine = makeStatusLine();
    makeHeader();
    applyCgiHeaders();

    std::string body = response->getARequest()->getResponseContext()->getPayload();

    addHeaderContent("Content-Length", sizeToString(body.size()));
    applyContentType(body);

    response->addResponseContent(statusLine);
    response->addResponseContent(headerToString());
    response->addResponseContent("\r\n");
    response->addResponseContent(body);
}
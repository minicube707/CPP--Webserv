/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RedirResponse.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 14:31:47 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:53:16 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RedirResponse.hpp"

#include "ARequest.hpp"
#include "HttpResponse.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"

RedirResponse::RedirResponse(HttpResponse *httpResponse, int statusCode) : AResponse(httpResponse, statusCode)
{
}

RedirResponse::~RedirResponse()
{
}

// =====================
// ==     Member      ==
// =====================
void RedirResponse::applyResponse()
{
    HttpResponse *response = getHttpResponse();

    std::string statusLine = makeStatusLine();
    makeHeader();
    applyCgiHeaders();

    if (!hasHeader("Location"))
        addHeaderContent("Location", response->getARequest()->getResponseContext()->getPayload());

    addHeaderContent("Content-Length", "0");

    response->addResponseContent(statusLine);
    response->addResponseContent(headerToString());
    response->addResponseContent("\r\n");
}

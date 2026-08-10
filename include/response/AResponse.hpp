/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AResponse.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 17:25:03 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/02 04:42:42 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"

class HttpResponse;

class AResponse
{
  private:
    // =====================
    // ==    Attributs    ==
    // =====================
    HttpResponse *_httpResponse;
    int _statusCode;
    std::string _statusMessage;
    HeaderContent _headerContent;
    std::vector<std::string> _setCookies;

  public:
    // =====================
    // == Canonical Form  ==
    // =====================
    AResponse(HttpResponse *httpResponse, int statusCode);
    virtual ~AResponse();

    // =====================
    // ==     Getters     ==
    // =====================
    HttpResponse *getHttpResponse() const;
    void setHttpResponse(HttpResponse *httpResponse);
    int getStatusCode() const;
    void setStatusCode(int statusCode);
    std::string getStatusMessage() const;
    void setStatusMessage(const std::string &statusMessagex);
    HeaderContent getHeaderContent(void) const;
    void setHeaderContent(HeaderContent headerContent);
    void addHeaderContent(std::string key, std::string value);
    bool hasHeader(const std::string &key) const;
    void addSetCookie(const std::string &setCookieValue);

    // =====================
    // == 	  Methods	  ==
    // =====================
    void updateCodeError(int statusCode);
    std::string makeStatusLine();
    void makeHeader();
    void handleSession();
    void handleConnection();
    void applyCgiHeaders();
    std::string makeHttpDate();
    std::string headerToString();
    bool containsHtmlTags(const std::string &body);
    void applyContentType(const std::string &body);

    virtual void applyResponse() = 0;
};

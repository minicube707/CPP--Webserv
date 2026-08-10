/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseContext.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/05 21:33:52 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/22 13:45:46 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "struct.hpp"

class ARequest;

class ResponseContext
{
  private:
    // =====================
    // ==    Attributs    ==
    // =====================
    int _statusCode;
    std::string _payload;
    std::string _contentType;
    std::vector<std::string> _cgiSetCookies;
    HeaderContent _cgiHeaders;
    ARequest *_ARequest;

    ResponseContext();

  public:
    // =====================
    // ==       OCF       ==
    // =====================
    ResponseContext(ARequest *arequest);
    ~ResponseContext();
    ResponseContext(const ResponseContext &other);

    // =====================
    // ==     Getters     ==
    // =====================
    int getStatusCode() const;
    void setStatusCode(int statusCode);
    std::string getPayload() const;
    void setPayload(std::string payload);
    std::string getContentType() const;
    void setContentType(const std::string &contentType);
    const std::vector<std::string> &getCgiSetCookies() const;
    void addCgiSetCookie(const std::string &setCookieValue);
    const HeaderContent &getCgiHeaders() const;
    void addCgiHeader(const std::string &key, const std::string &value);
    ARequest *getARequest(void) const;
    void setARequest(ARequest *arequest);
};

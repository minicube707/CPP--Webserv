/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Header.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/17 17:37:05 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 04:11:57 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"

#define MAX_URI_LENGTH 8192

class Header
{
  private:
    static const std::string _listMethods[4];

    // =====================
    // ==    Attributs    ==
    // =====================
    HttpMethod _method;
    std::string _query;
    std::string _scriptName;
    std::string _protocol;
    HeaderContent _headerContent;
    std::string _host;

  public:
    // =====================
    // ==       OCF       ==
    // =====================
    Header();
    ~Header();
    Header(const Header &other);
    Header &operator=(const Header &other);

    // =====================
    // ==     Getters     ==
    // =====================
    HttpMethod getMethod(void) const;
    void setMethod(const std::string &method);
    std::string getQuery(void) const;
    void setQuery(const std::string &query);
    std::string getScriptName(void) const;
    void setScriptName(const std::string &scriptName);
    std::string getProtocol(void) const;
    void setProtocol(const std::string &protocol);
    HeaderContent getHeaderContent(void) const;
    void setHeaderContent(HeaderContent headerContent);
    void addHeaderContent(std::string key, std::string value);
    std::string getHost(void) const;
    void setHost(const std::string &host);

    // =====================
    // == 	  Member	  ==
    // =====================
    void initialisationHeader(const std::string &headerContent);
    void parseHeaderInfo(const std::string &headerContent);
    void parseHeaderContent(const std::string &headerContent);
    HttpMethod parseMethodToken(const std::string &method);
    std::string::size_type findEnd(const std::string &headerContent, const std::string &end);
    void sliptUriNQuery(std::string uri);
    static std::string stripAbsoluteForm(const std::string &uri);
    static bool isFramingHeader(const std::string &key);
};

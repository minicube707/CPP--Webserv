/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/06 15:45:26 by fmotte            #+#    #+#             */
/*   Updated: 2026/05/27 13:33:40 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"

class Location
{
  private:
    std::string _name;
    std::set<HttpMethod> _allowed_methods;
    std::string _root;
    std::string _index;
    std::string _upload_store;
    std::map<std::string, std::string> _cgi_pass;
    bool _auto_index;
    HttpErrorPage _error_page;
    unsigned int _client_max_body_size;
    HttpReturn _ret;

  public:
    // =====================
    // == Canonical Form  ==
    // =====================

    Location();
    ~Location();
    Location(const Location &other);
    Location &operator=(const Location &other);

    static const std::string list_allowed_methods[4];

    // =====================
    // == Getter & Setter ==
    // =====================

    // NAME
    void setName(std::string name);
    std::string getName(void);

    // METHOD-HTTP
    void addAllowedMethod(HttpMethod i);
    std::set<HttpMethod> getAllowedMethods(void);

    // INDEX
    void setIndex(std::string index);
    std::string getIndex(void);

    // ROOT
    void setRoot(std::string root);
    std::string getRoot(void);

    // UPLOAD-STORE
    void setUploadStore(std::string upload_store);
    std::string getUploadStore(void);

    // CGI-PASS
    void addCgiPass(const std::string &extension, const std::string &interpreter);
    std::string getCgiPass(const std::string &extension) const;
    bool hasCgiPass(const std::string &extension) const;

    // AUTO-INDEX
    void setAutoIndex(bool auto_index);
    bool getAutoIndex(void);

    // CLIENT-MAX-BODY-SIZE
    void setClientMaxBodySize(unsigned int client_max_body_size);
    unsigned int getClientMaxBodySize(void);

    // ERROR-PAGE
    void setErrorPage(HttpErrorPage error_page);
    HttpErrorPage *getErrorPage(void);

    // RETURN
    void setReturn(HttpReturn ret);
    HttpReturn *getReturn(void);

    // =====================
    // ==     Method      ==
    // =====================

    void initializeLocation(std::vector<std::string> &tokens);

    void initializeLocationAllowedMethods(std::vector<std::string> &tokens);
    void initializeLocationRoot(std::vector<std::string> &tokens);
    void initializeLocationUploadStore(std::vector<std::string> &tokens);
    void initializeLocationCgiPass(std::vector<std::string> &tokens);

    void initializeLocationIndex(std::vector<std::string> &tokens);
    void initializeLocationAutoIndex(std::vector<std::string> &tokens);

    void initializeLocationClientMaxBodySize(std::vector<std::string> &tokens);
    void initializeLocationErrorPage(std::vector<std::string> &tokens);
    void initializeLocationReturn(std::vector<std::string> &tokens);
};

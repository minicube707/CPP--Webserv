/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HandlePath.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 05:37:27 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:42:43 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"

class Location;
class HttpRequest;

class HandlePath
{
  private:
    // =====================
    // ==    Attributs    ==
    // =====================
    std::string _path;
    bool _isAutoIndex;
    HttpRequest *_httpRequest;

    HandlePath();

  public:
    // =====================
    // ==       OCF       ==
    // =====================
    HandlePath(HttpRequest *httpRequest);
    ~HandlePath();

    // =====================
    // ==     Getters     ==
    // =====================
    std::string getPath() const;
    void setPath(std::string path);
    bool getIsAutoIndex() const;
    void setIsAutoIndex(bool isAutoIndex);
    HttpRequest *getHttpRequest() const;
    void setHttpRequest(HttpRequest *httpRequest);

    // =====================
    // == 	  Member	  ==
    // =====================
    std::string selectRoot(Location *location);
    std::string requestUriPath();
    std::string mapUriToLocation(Location *location);
    bool isRequestForLocationRoot(const std::string &locationName);

    std::string createPath(Location *location);
    std::string createPathWithLocation(Location *location);
    std::string createPathPost(Location *location, const std::string &base);
    std::string createPathWithServer();
    std::string createPathCgi(Location *location);

    void listContentFolder(const std::string &path, std::vector<std::string> &entries);
    std::string createContentAutoIndex(const std::string &path);
};

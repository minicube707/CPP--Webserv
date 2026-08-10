/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PostMethod.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:46:40 by fmotte            #+#    #+#             */
/*   Updated: 2026/05/25 11:35:25 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "AMethod.hpp"

class HttpRequest;

class PostMethod : public AMethod
{
  private:
    // =====================
    // ==    Attributs    ==
    // =====================

    PostMethod();

  public:
    // =====================
    // ==       OCF       ==
    // =====================
    PostMethod(HttpRequest *httpRequest);
    ~PostMethod();
    PostMethod(const PostMethod &other);
    PostMethod &operator=(const PostMethod &other);

    // =====================
    // == 	  Member	  ==
    // =====================
    std::string applyMethod(Location *location);

    std::string uriFileName(Location *location);
    std::string requestContentType();
    void writeUploadedFile(const std::string &path, const std::string &content);
};
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponse.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 16:30:50 by fmotte            #+#    #+#             */
/*   Updated: 2026/05/29 15:47:16 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "AResponse.hpp"
#include "struct.hpp"

class ErrorResponse : public AResponse
{
  private:
    // =====================
    // ==    Attributs    ==
    // =====================

    ErrorResponse();

  public:
    // =====================
    // == Canonical Form  ==
    // =====================
    ErrorResponse(HttpResponse *httpResponse, int statusCode);
    ~ErrorResponse();

    // =====================
    // ==     Member      ==
    // =====================
    std::string makeErrorPage();
    std::string builtErrorPage();
    std::string errorPageStyle();
    std::string getRightPageError();

    void applyResponse();
};

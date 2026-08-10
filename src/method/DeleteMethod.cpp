/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DeleteMethod.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 14:13:06 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/06 06:01:27 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DeleteMethod.hpp"

#include "ARequest.hpp"
#include "HandlePath.hpp"
#include "HttpRequest.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"

#include <cstdio>
#include <sys/stat.h>

// =====================
// ==       OCF       ==
// =====================
DeleteMethod::DeleteMethod(HttpRequest *httpRequest) : AMethod(httpRequest, DELETE)
{
}

DeleteMethod::~DeleteMethod()
{
}

DeleteMethod::DeleteMethod(const DeleteMethod &other) : AMethod(other)
{
    *this = other;
}

DeleteMethod &DeleteMethod::operator=(const DeleteMethod &other)
{
    AMethod::operator=(other);
    return (*this);
}

// =====================
// == 	  Member	  ==
// =====================
std::string DeleteMethod::applyMethod(Location *location)
{
    struct stat buff;

    HandlePath handlePath(getHttpRequest());
    std::string path = handlePath.createPath(location);

    std::cout << "Path to file to delete " << path << "\n";

    if (access(path.c_str(), F_OK) == -1)
        throw std::runtime_error("404");

    if (stat(path.c_str(), &buff) != 0)
        throw std::runtime_error("500");

    if (!S_ISREG(buff.st_mode))
        throw std::runtime_error("403");

    if (std::remove(path.c_str()) != 0)
        throw std::runtime_error("500");

    getHttpRequest()->getRequestContext()->getARequest()->getResponseContext()->setStatusCode(204);
    return "";
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponse.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 16:30:52 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:54:24 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ErrorResponse.hpp"

#include "ARequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"
#include "Server.hpp"

// =====================
// == Canonical Form  ==
// =====================
ErrorResponse::ErrorResponse(HttpResponse *httpResponse, int statusCode) : AResponse(httpResponse, statusCode)
{
}

ErrorResponse::~ErrorResponse()
{
}

// =====================
// == 	  Methods	  ==
// =====================

/
std::string ErrorResponse::errorPageStyle()
{
    bool serverError = (getStatusCode() >= 500);
    std::string accent = serverError ? "#b4342a" : "#b8860b";
    std::string accentDark = serverError ? "#f08a80" : "#e0b44c";

    std::string style = "<style>";

    style += ":root{--bg:#f6f1e7;--surface:#fff;--border:#e3d9c6;--text:#2a2520;--muted:#7c6f5d;";
    style += "--accent:" + accent + "}";
    style += "@media(prefers-color-scheme:dark){:root{--bg:#17150f;--surface:#221f17;--border:#3a342a;";
    style += "--text:#f0e9dc;--muted:#a1937c;--accent:" + accentDark + "}}";
    style += "*{box-sizing:border-box}";
    style += "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;";
    style += "padding:1.5rem;background:var(--bg);color:var(--text);line-height:1.6;";
    style += "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif}";
    style += ".card{width:min(480px,100%);text-align:center;padding:2.75rem 2rem 2.25rem;";
    style += "background:var(--surface);border:1px solid var(--border);border-radius:14px;";
    style += "box-shadow:0 8px 24px rgba(0,0,0,.10)}";
    style += ".code{margin:0;font-size:clamp(3.5rem,14vw,5rem);font-weight:800;line-height:1;";
    style += "letter-spacing:-.04em;color:var(--accent)}";
    style += "h1{margin:.6rem 0 0;font-size:1.25rem}";
    style += "p{margin:.7rem 0 0;color:var(--muted);font-size:.93rem}";
    style += "a{display:inline-block;margin-top:1.8rem;padding:.6rem 1.3rem;background:transparent;";
    style += "border:1px solid var(--border);color:var(--text);font-weight:600;font-size:.9rem;";
    style += "text-decoration:none;border-radius:10px}";
    style += "a:hover{border-color:var(--accent)}";
    style += ".served{margin-top:1.7rem;padding-top:1.1rem;border-top:1px solid var(--border);";
    style += "color:var(--muted);font-size:.76rem;";
    style += "font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}";
    style += "</style>";

    return style;
}


std::string ErrorResponse::builtErrorPage()
{
    std::string code = intToString(getStatusCode());
    std::string message = getStatusMessage();

    std::string page = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    page += "<meta charset=\"UTF-8\">\n";
    page += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    page += "<title>" + code + " - " + message + "</title>\n";
    page += errorPageStyle();
    page += "\n</head>\n<body>\n<main class=\"card\">\n";
    page += "<p class=\"code\">" + code + "</p>\n";
    page += "<h1>" + message + "</h1>\n";
    page += "<a href=\"/\">Return home</a>\n";
    page += "<p class=\"served\">served by The Tonton Webserv</p>\n";
    page += "</main>\n</body>\n</html>\n";

    addHeaderContent("content-type", "text/html");
    return page;
}

std::string ErrorResponse::getRightPageError()
{
    Location *location = getHttpResponse()->getARequest()->getRequestContext()->getLocation();

    if (location != NULL && location->getErrorPage()->code == getStatusCode())
    {
        addHeaderContent("content-type", "text/html");
        return location->getErrorPage()->path_page;
    }

    Server *server = getHttpResponse()->getARequest()->getRequestContext()->getServer();

    if (server == NULL)
        return "";

    for (size_t i = 0;; ++i)
    {
        HttpErrorPage *errorPage = server->getErrorPage(i);

        if (errorPage == NULL)
            return "";

        if (errorPage->code == getStatusCode())
        {
            addHeaderContent("content-type", "text/html");
            return errorPage->path_page;
        }
    }
}

std::string ErrorResponse::makeErrorPage()
{
    std::string pathPageError = getRightPageError();
    if (pathPageError != "")
    {
        std::string content_file;
        if (parseConfigFile(pathPageError.c_str(), content_file) == 0)
            return content_file;
        updateCodeError(500);
    }
    return builtErrorPage();
}

void ErrorResponse::applyResponse()
{
    HttpResponse *response = getHttpResponse();

    std::string body = makeErrorPage();
    std::string statusLine = makeStatusLine();
    makeHeader();
    addHeaderContent("Content-Length", sizeToString(body.size()));
    applyContentType(body);

    response->addResponseContent(statusLine);
    response->addResponseContent(headerToString());
    response->addResponseContent("\r\n");
    response->addResponseContent(body);
}

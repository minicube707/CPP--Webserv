/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsDuplicate.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/08 12:58:24 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:46:07 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utilsDuplicate.hpp"
#include "execption.hpp"

#include <climits>
#include <ctime>

const std::string &frontToken(const std::vector<std::string> &tokens)
{
    if (tokens.empty())
        throw ExecptionMissBrace();

    return tokens[0];
}

std::string popToken(std::vector<std::string> &tokens)
{
    std::string token = frontToken(tokens);

    tokens.erase(tokens.begin());
    return token;
}

static std::string unquote(const std::string &value)
{
    if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
        return value.substr(1, value.size() - 2);

    return value;
}

static std::string parsePathDirective(std::vector<std::string> &tokens, const std::string &name)
{
    if (frontToken(tokens) != name)
        return "";

    popToken(tokens);
    std::string path = popToken(tokens);

    if (path == ";" || path == "{" || path == "}")
        throw ExecptionWrongArgument(name);
    path = unquote(path);
    if (path.empty())
        throw ExecptionWrongArgument(name);

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    return path;
}

std::string parseRootDirective(std::vector<std::string> &tokens)
{
    return parsePathDirective(tokens, "root");
}

std::string parseUploadStoreDirective(std::vector<std::string> &tokens)
{
    return parsePathDirective(tokens, "upload_store");
}

bool parseCgiPassDirective(std::vector<std::string> &tokens, std::string &extension, std::string &interpreter)
{
    if (frontToken(tokens) != "cgi_pass")
        return false;

    popToken(tokens);
    extension = popToken(tokens);
    interpreter = popToken(tokens);

    if (extension.empty() || extension[0] != '.' || interpreter == ";" || interpreter == "{" || interpreter == "}")
        throw ExecptionWrongArgument(extension);

    interpreter = unquote(interpreter);
    if (interpreter.empty())
        throw ExecptionWrongArgument("cgi_pass");

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    return true;
}

int parseAutoIndexDirective(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "autoindex")
        return -1;

    popToken(tokens);
    std::string value = popToken(tokens);

    if (value != "on" && value != "true" && value != "off" && value != "false")
        throw ExecptionWrongArgument(value);

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    return ((value == "on" || value == "true") ? 1 : 0);
}

unsigned int parseClientMaxBodySizeDirective(std::vector<std::string> &tokens)
{
    if (frontToken(tokens) != "client_max_body_size")
        return 0;

    popToken(tokens);
    std::string value = popToken(tokens);

    if (value.empty())
        throw ExecptionFailConvertion(value);

    unsigned long client_max_body_size = 0;
    for (std::string::size_type i = 0; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
            throw ExecptionFailConvertion(value);
        unsigned int digit = static_cast<unsigned int>(value[i] - '0');
        if (client_max_body_size > (UINT_MAX - digit) / 10)
            throw ExecptionFailConvertion(value);
        client_max_body_size = client_max_body_size * 10 + digit;
    }
    if (client_max_body_size == 0)
        throw ExecptionWrongArgument(value);

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    return static_cast<unsigned int>(client_max_body_size);
}

static int parseStatusCode(const std::string &value)
{
    if (value.empty())
        throw ExecptionFailConvertion(value);

    int code = 0;
    for (std::string::size_type i = 0; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
            throw ExecptionFailConvertion(value);
        code = code * 10 + (value[i] - '0');
        if (code > 599)
            throw ExecptionWrongArgument(value);
    }
    if (code < 100)
        throw ExecptionWrongArgument(value);
    return code;
}

HttpErrorPage parseErrorPageDirective(std::vector<std::string> &tokens, bool &is_init)
{
    HttpErrorPage error_page = HttpErrorPage();

    if (frontToken(tokens) != "error_page")
        return error_page;

    popToken(tokens);
    std::string code = popToken(tokens);

    error_page.code = parseStatusCode(code);
    if (error_page.code < 300)
        throw ExecptionWrongArgument(code);

    error_page.path_page = popToken(tokens);
    if (error_page.path_page == ";" || error_page.path_page == "{" || error_page.path_page == "}")
        throw ExecptionWrongArgument("error_page");
    error_page.path_page = unquote(error_page.path_page);
    if (error_page.path_page.empty())
        throw ExecptionWrongArgument("error_page");

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    is_init = true;
    return error_page;
}

HttpReturn parseReturnDirective(std::vector<std::string> &tokens, bool &is_init)
{
    HttpReturn ret = HttpReturn();

    if (frontToken(tokens) != "return")
        return ret;

    popToken(tokens);
    std::string code = popToken(tokens);

    ret.code = parseStatusCode(code);

    std::string value = popToken(tokens);
    if (value == ";" || value == "{" || value == "}")
        throw ExecptionWrongArgument("return");
    ret.value = unquote(value);
    if (ret.value.empty())
        throw ExecptionWrongArgument("return");

    if (popToken(tokens) != ";")
        throw ExecptionMissSemiColon();

    is_init = true;
    return ret;
}

std::string intToString(int value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// A body can be larger than an int: Content-Length must never be truncated
std::string sizeToString(size_t value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string toLowerString(const std::string &str)
{
    std::string result = str;
    for (std::string::size_type i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    return result;
}

std::string trimSpaces(const std::string &value)
{
    std::string::size_type begin = value.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return ("");
    std::string::size_type end = value.find_last_not_of(" \t");
    return (value.substr(begin, end - begin + 1));
}

uint64_t getCurrentTime(void)
{
    return static_cast<uint64_t>(time(NULL)) * 1000;
}

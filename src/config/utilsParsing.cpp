/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsParsing.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 17:21:25 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:48:17 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utilsParsing.hpp"
#include "execption.hpp"

int parseConfigFile(const char *filename, std::string &content_file)
{
    // Reading
    std::ifstream f(filename);
    if (!f.is_open())
    {
        std::cout << "Error: can't open " << filename << std::endl;
        return (1);
    }

    std::string line;
    while (std::getline(f, line))
    {
        line.append("\n");
        content_file.append(line);
    }

    f.close();
    return (0);
}

int readRawFile(const char *filename, std::string &content_file)
{
    std::ifstream f(filename, std::ios::binary);
    if (!f.is_open())
    {
        std::cout << "Error: can't open " << filename << std::endl;
        return (1);
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    content_file = ss.str();

    f.close();
    return (0);
}

std::vector<std::string> tokenizeString(std::string &content_file)
{
    std::vector<std::string> tokens;
    std::string token;
    bool inQuote = false;
    bool inComment = false;

    for (std::string::size_type i = 0; i < content_file.size(); ++i)
    {
        char c = content_file[i];

        if (inComment)
        {
            if (c == '\n')
                inComment = false;
            continue;
        }

        if (inQuote)
        {
            token += c;
            if (c == '"')
                inQuote = false;
            continue;
        }

        if (c == '#')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            inComment = true;
        }
        else if (c == '"')
        {
            token += c;
            inQuote = true;
        }
        else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else if (c == '{' || c == '}' || c == ';')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, c));
        }
        else
            token += c;
    }

    if (inQuote)
        throw ExecptionWrongArgument("unclosed quote");
    if (!token.empty())
        tokens.push_back(token);

    return tokens;
}

unsigned int countOccurrences(const std::string &string, const char occ)
{
    unsigned int nb_occ = 0;

    for (std::string::size_type i = 0; i < string.size(); ++i)
        if (string[i] == occ)
            ++nb_occ;
    return nb_occ;
}

std::string joinPath(const std::string &string1, const std::string &string2)
{
    if (string1.empty())
        return string2;
    if (string2.empty())
        return string1;

    std::string new_path = string1;
    bool endsWithSlash = (new_path[new_path.size() - 1] == '/');
    bool startsWithSlash = (string2[0] == '/');

    if (!endsWithSlash && !startsWithSlash)
        new_path += '/';
    else if (endsWithSlash && startsWithSlash)
        new_path.erase(new_path.size() - 1);

    new_path += string2;
    return new_path;
}

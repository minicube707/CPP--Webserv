/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/03 15:24:58 by fmotte            #+#    #+#             */
/*   Updated: 2026/04/23 13:32:58 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "colors.hpp"
#include "struct.hpp"
#include "utilsParsing.hpp"

volatile sig_atomic_t stop_webserv = 0;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Error: Wrong number of argument\n";
        return (1);
    }

    std::string content_file;
    if (parseConfigFile(argv[1], content_file))
        return (1);

    std::vector<std::string> tokens;
    try
    {
        tokens = tokenizeString(content_file);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return (1);
    }

    Webserv webser;
    if (webser.initializeWebserv(tokens))
        return (1);

    if (webser.initializeConnection())
        return (2);

    return (0);
}

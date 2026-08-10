/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsDuplicate.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/08 12:58:22 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:45:03 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"

const std::string &frontToken(const std::vector<std::string> &tokens);
std::string popToken(std::vector<std::string> &tokens);

std::string parseRootDirective(std::vector<std::string> &tokens);
std::string parseUploadStoreDirective(std::vector<std::string> &tokens);
bool parseCgiPassDirective(std::vector<std::string> &tokens, std::string &extension, std::string &interpreter);
int parseAutoIndexDirective(std::vector<std::string> &tokens);
unsigned int parseClientMaxBodySizeDirective(std::vector<std::string> &tokens);
HttpErrorPage parseErrorPageDirective(std::vector<std::string> &tokens, bool &is_init);
HttpReturn parseReturnDirective(std::vector<std::string> &tokens, bool &is_init);
std::string intToString(int value);
std::string sizeToString(size_t value);
std::string trimSpaces(const std::string &value);
std::string toLowerString(const std::string &str);
uint64_t getCurrentTime(void);
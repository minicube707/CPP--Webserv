/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utilsRequest.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fmotte <fmotte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/21 13:53:22 by fmotte            #+#    #+#             */
/*   Updated: 2026/07/05 23:25:39 by fmotte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "struct.hpp"
#include <sys/stat.h>

int longestPrefixMatch(std::string uri, std::string location);
size_t parseChunkSize(const std::string &line);
std::string returnLastElementPath(std::string path);

std::string percentDecode(const std::string &value);
std::string normalizeUriPath(const std::string &path);
std::string stripTrailingSlashes(const std::string &path);

void checkPermisionReadFile(std::string path);
bool isFinishByFile(std::string path);
bool isFinishByFolder(std::string path);

bool isCompleteRequest(const std::string &request);
size_t completeRequestLength(const std::string &request);
bool isDeclaredBodySizeExceeding(const std::string &request, size_t maxBodySize);
bool isCompleteChunkedBody(const std::string &request, std::string::size_type bodyStart);
bool isFinalChunkComplete(const std::string &request, std::string::size_type current);
std::string initSizeToken(const std::string &request, const std::string::size_type &current,
                          const std::string::size_type &lineEnd);
std::string getHeaderValue(const std::string &request, const std::string &headerName);
bool parseDecimalLength(const std::string &value, size_t &contentLength);
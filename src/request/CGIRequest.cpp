/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIRequest.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/16 16:35:36 by fmotte            #+#    #+#             */
/*   Updated: 2026/08/10 03:36:08 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIRequest.hpp"

#include "ARequest.hpp"
#include "Body.hpp"
#include "HandlePath.hpp"
#include "Header.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "RequestContext.hpp"
#include "ResponseContext.hpp"
#include "Server.hpp"

#include "utilsConnection.hpp"
#include "utilsRequest.hpp"

#include <cctype>
#include <cstdlib>
#include <sys/wait.h>

// =====================
// ==       OCF       ==
// =====================

CGIRequest::CGIRequest(ARequest arequest)
    : ARequest(arequest), _pid(-1), _cgiBuffer(""), _bodyBytesSent(0), _eventDataWriteChild(NULL),
      _eventDataReadChild(NULL)
{
    _pipeIn[0] = -1;
    _pipeIn[1] = -1;
    _pipeOut[0] = -1;
    _pipeOut[1] = -1;
}

CGIRequest::~CGIRequest()
{
    // closing the fds also removes them from epoll; never use a fd already
    // closed elsewhere (it could have been reused by another client)
    closeStdinPipe();
    closeStdoutPipe();

    delete _eventDataWriteChild;
    delete _eventDataReadChild;
}

// =====================
// ==     Getters     ==
// =====================

int *CGIRequest::getPipeIn()
{
    return _pipeIn;
}

void CGIRequest::setPipeIn(int pipeIn[2])
{
    _pipeIn[0] = pipeIn[0];
    _pipeIn[1] = pipeIn[1];
}

int *CGIRequest::getPipeOut()
{
    return _pipeOut;
}

void CGIRequest::setPipeOut(int pipeOut[2])
{
    _pipeOut[0] = pipeOut[0];
    _pipeOut[1] = pipeOut[1];
}

pid_t CGIRequest::getPid() const
{
    return _pid;
}

void CGIRequest::setPid(pid_t pid)
{
    _pid = pid;
}

EventData *CGIRequest::geteventDataWrite() const
{
    return _eventDataWriteChild;
}

void CGIRequest::seteventDataWrite(EventData *eventDataWriteChild)
{
    if (eventDataWriteChild == NULL)
        throw ExecptionErrorUninitializedVariable("*eventDataWriteChild", "CGIRequest");

    _eventDataWriteChild = eventDataWriteChild;
}

EventData *CGIRequest::geteventDataRead() const
{
    return _eventDataReadChild;
}

void CGIRequest::seteventDataRead(EventData *eventDataReadChild)
{
    if (eventDataReadChild == NULL)
        throw ExecptionErrorUninitializedVariable("*eventDataReadChild", "CGIRequest");

    _eventDataReadChild = eventDataReadChild;
}

// =====================
// == 	  Member	  ==
// =====================

// LEs doubles Pipes et le in/out :
//  pipeIn[1]  ->  stdin   (body POST envoyé par le parent)
//  pipeIn[0]  <-  lecture par l'enfant
//  pipeOut[1] ->  stdout  (réponse HTML générée par l'enfant)
//  pipeOut[0] <-  lecture par le parent
// CGIRequest pipeIn[1] à fermer sinon le serveur attendra indéfiniment (deadlock)

void CGIRequest::createPipe(int pipeIn[2], int pipeOut[2])
{
    if (pipe(pipeOut) == -1)
    {
        std::cerr << "Error pipe\n";
        throw std::runtime_error("500");
    }
    if (pipe(pipeIn) == -1)
    {
        close(pipeOut[0]);
        close(pipeOut[1]);
        std::cerr << "Error pipe\n";
        throw std::runtime_error("500");
    }
    // The child ends (pipeIn[0]/pipeOut[1]) stay blocking: they become the
    // CGI stdin/stdout. The parent ends are made non-blocking by addFdToEvent.
}

void CGIRequest::checkForkCreate(pid_t pid)
{
    if (pid == -1)
    {
        std::cerr << "Error fork\n";
        close(getPipeOut()[0]);
        close(getPipeOut()[1]);
        close(getPipeIn()[0]);
        close(getPipeIn()[1]);
        _pipeOut[0] = -1;
        _pipeOut[1] = -1;
        _pipeIn[0] = -1;
        _pipeIn[1] = -1;
        throw std::runtime_error("500");
    }
}

void CGIRequest::initializationCGIRequest(const std::string &interpreter)
{
    int pipeIn[2];
    int pipeOut[2];


    HandlePath handlePath(getRequestContext()->getHttpRequest());
    checkPermisionReadFile(handlePath.createPathCgi(getRequestContext()->getLocation()));

    createPipe(pipeIn, pipeOut);
    setPipeIn(pipeIn);
    setPipeOut(pipeOut);

    std::cout.flush();
    std::cerr.flush();

    pid_t pid = fork();
    setPid(pid);
    checkForkCreate(pid);

    if (pid == 0)
    {
        // child must never return to the parent's event loop: any exception here ends the child process
        try
        {
            manage_pipe(interpreter);
        }
        catch (const std::exception &e)
        {
            std::cerr << "CGI child error: " << e.what() << "\n";
        }
        std::cout.flush();
        std::cerr.flush();
        std::exit(EXIT_FAILURE);
    }

    close(getPipeOut()[1]);
    _pipeOut[1] = -1;
    close(getPipeIn()[0]);
    _pipeIn[0] = -1;

    connectToEpoll();
}

void CGIRequest::connectToEpoll()
{
    int epoll_fd = getRequestContext()->getClient()->getWebserv()->getEpollFd();
    EventData *eventData1 = NULL;

    try
    {
        eventData1 = addFdToEvent(epoll_fd, getPipeIn()[1], EPOLLOUT, WRITECHILD, this);
        EventData *eventData2 = addFdToEvent(epoll_fd, getPipeOut()[0], EPOLLIN, READCHILD, this);

        seteventDataWrite(eventData1);
        seteventDataRead(eventData2);
    }
    catch (...)
    {
        if (eventData1 != NULL)
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, getPipeIn()[1], NULL);
            delete eventData1;
        }
        closeStdinPipe();
        closeStdoutPipe();
        if (getPid() > 0)
        {
            kill(getPid(), SIGKILL);
            waitpid(getPid(), NULL, 0);
        }
        throw;
    }

    std::cout << "Add to Epoll\n";
}

bool CGIRequest::sendDataToChild()
{
    Body *body = getRequestContext()->getHttpRequest()->getBody();

    if (body != NULL && _bodyBytesSent < body->getBodyContent().size())
    {
        const BodyContent &content = body->getBodyContent();
        size_t remaining = content.size() - _bodyBytesSent;
        ssize_t nb_written = write(getPipeIn()[1], &content[_bodyBytesSent], remaining);

        if (nb_written > 0)
        {
            _bodyBytesSent += nb_written;
            if (_bodyBytesSent < content.size())
                return false;
        }

    }

    closeStdinPipe();
    return true;
}

bool CGIRequest::receivedDataFromChild()
{
    char buffer[65536];
    ssize_t nb_read = read(getPipeOut()[0], buffer, sizeof(buffer));

    if (nb_read > 0)
    {
        _cgiBuffer.append(buffer, nb_read);
        return false;
    }

    if (nb_read == 0)
    {
        getResponseContext()->setPayload(_cgiBuffer);
        return true;
    }
    return false;
}

void CGIRequest::closeStdinPipe()
{
    if (_pipeIn[1] >= 0)
    {
        epoll_ctl(getRequestContext()->getClient()->getWebserv()->getEpollFd(), EPOLL_CTL_DEL, _pipeIn[1], NULL);
        close(_pipeIn[1]);
        _pipeIn[1] = -1;
    }
}

void CGIRequest::closeStdoutPipe()
{
    if (_pipeOut[0] >= 0)
    {
        epoll_ctl(getRequestContext()->getClient()->getWebserv()->getEpollFd(), EPOLL_CTL_DEL, _pipeOut[0], NULL);
        close(_pipeOut[0]);
        _pipeOut[0] = -1;
    }
}

static std::string methodToString(HttpMethod method)
{
    if (method == POST)
        return "POST";
    if (method == DELETE)
        return "DELETE";
    if (method == HEAD)
        return "HEAD";
    return "GET";
}

void CGIRequest::processDataFromChild()
{
    const std::string &payload = getResponseContext()->getPayload();

    if (payload.empty())
        throw std::runtime_error("502");

    std::string::size_type sep = payload.find("\r\n\r\n");
    std::string::size_type sepLen = 4;
    if (sep == std::string::npos)
    {
        sep = payload.find("\n\n");
        sepLen = 2;
    }
    if (sep != std::string::npos)
    {
        forwardCgiHeaders(payload.substr(0, sep));
        return getResponseContext()->setPayload(payload.substr(sep + sepLen));
    }

    getResponseContext()->setPayload(payload);
}

void CGIRequest::forwardCgiHeaders(const std::string &headerBlock)
{
    std::stringstream stream(headerBlock);
    std::string line;
    bool statusSet = false;

    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = toLowerString(trimSpaces(line.substr(0, colon)));
        std::string value = trimSpaces(line.substr(colon + 1));

        if (key == "set-cookie")
            getResponseContext()->addCgiSetCookie(value);
        else if (key == "status")
        {
            std::stringstream ss(value);
            int code = 0;
            if (ss >> code && code >= 100 && code <= 599)
            {
                getResponseContext()->setStatusCode(code);
                statusSet = true;
            }
        }
        else if (key != "content-length")
            getResponseContext()->addCgiHeader(key, value);
    }

    if (!statusSet)
        applyDefaultCgiStatus();
}

void CGIRequest::applyDefaultCgiStatus()
{
    HttpMethod method = getRequestContext()->getHttpRequest()->getHeader()->getMethod();

    if (method == POST)
        getResponseContext()->setStatusCode(201);
    else if (method == DELETE)
        getResponseContext()->setStatusCode(204);
}

void CGIRequest::manage_pipe(const std::string &interpreter)
{
    int *pipeIn = getPipeIn();
    int *pipeOut = getPipeOut();

    close(pipeOut[0]);

    if (dup2(pipeOut[1], STDOUT_FILENO) == -1)
    {
        std::cerr << "CGI: dup2 stdout failed\n";
        std::exit(EXIT_FAILURE);
    }

    close(pipeOut[1]);
    close(pipeIn[1]);

    if (dup2(pipeIn[0], STDIN_FILENO) == -1)
    {
        std::cerr << "CGI: dup2 stdin failed\n";
        std::exit(EXIT_FAILURE);
    }
    close(pipeIn[0]);

    // URI / query string / script name
    std::string query = getRequestContext()->getHttpRequest()->getHeader()->getQuery();
    std::string scriptName = getRequestContext()->getHttpRequest()->getHeader()->getScriptName();

    std::string method = methodToString(getRequestContext()->getHttpRequest()->getHeader()->getMethod());
    std::string protocol = getRequestContext()->getHttpRequest()->getHeader()->getProtocol();

    HeaderContent hc = getRequestContext()->getHttpRequest()->getHeader()->getHeaderContent();
    HeaderContent::const_iterator contentTypeIt = hc.find("content-type");
    std::string contentType = (contentTypeIt != hc.end()) ? contentTypeIt->second : "";
    std::string contentLength = sizeToString(getRequestContext()->getHttpRequest()->getBody()->getBodyContent().size());

    Listen *listen = getRequestContext()->getServer()->getListen(0);
    std::string serverName = (listen && !listen->ip.empty()) ? listen->ip : "localhost";
    std::string serverPort = "80";
    if (listen)
    {
        std::ostringstream oss;
        oss << listen->port;
        serverPort = oss.str();
    }

    HandlePath handlePath(getRequestContext()->getHttpRequest());
    std::string path = handlePath.createPathCgi(getRequestContext()->getLocation());

    checkPermisionReadFile(path);

    std::string::size_type pslash = path.rfind('/');
    std::string scriptFile = (pslash != std::string::npos) ? path.substr(pslash + 1) : path;

    std::vector<std::string> envStrings;
    envStrings.push_back("REQUEST_METHOD=" + method);
    envStrings.push_back("QUERY_STRING=" + query);
    envStrings.push_back("SERVER_PROTOCOL=" + protocol);
    envStrings.push_back("CONTENT_TYPE=" + contentType);
    envStrings.push_back("CONTENT_LENGTH=" + contentLength);
    envStrings.push_back("SCRIPT_NAME=" + scriptName);
    envStrings.push_back("SCRIPT_FILENAME=" + scriptFile);
    envStrings.push_back("REDIRECT_STATUS=200");
    envStrings.push_back("PATH_INFO=");
    envStrings.push_back("SERVER_NAME=" + serverName);
    envStrings.push_back("SERVER_PORT=" + serverPort);
    envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envStrings.push_back("SERVER_SOFTWARE=webserv/1.0");

    for (HeaderContent::const_iterator it = hc.begin(); it != hc.end(); ++it)
    {
        if (it->first == "content-type" || it->first == "content-length")
            continue;

        std::string name = it->first;
        for (std::string::size_type i = 0; i < name.size(); ++i)
            name[i] = (name[i] == '-') ? '_' : std::toupper(static_cast<unsigned char>(name[i]));

        envStrings.push_back("HTTP_" + name + "=" + it->second);
    }

    std::vector<char *> envp;
    for (std::vector<std::string>::iterator it = envStrings.begin(); it != envStrings.end(); ++it)
        envp.push_back(const_cast<char *>(it->c_str()));
    envp.push_back(NULL);

    std::string::size_type slash = path.rfind('/');
    if (slash != std::string::npos && chdir(path.substr(0, slash).c_str()) == -1)
    {
        std::cerr << "CGI: chdir failed on " << path.substr(0, slash) << "\n";
        std::exit(EXIT_FAILURE);
    }

    std::string localScript = "./" + scriptFile;

    char *args[3];
    if (interpreter.empty())
    {
        args[0] = const_cast<char *>(localScript.c_str());
        args[1] = NULL;
        args[2] = NULL;
    }
    else
    {
        args[0] = const_cast<char *>(interpreter.c_str());
        args[1] = const_cast<char *>(localScript.c_str());
        args[2] = NULL;
    }

    execve(args[0], args, &envp[0]);

    // execve only returns on failure: 502 Bad Gateway
    std::cerr << "CGI: cannot execute " << args[0] << "\n";
    std::exit(EXIT_FAILURE);
}

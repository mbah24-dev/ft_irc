/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerUserCommands.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:19:05 by zcherif           #+#    #+#             */
/*   Updated: 2026/10/06 12:21:06 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void Server::handleCapCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();

    if (params.empty())
        return;

    const std::string& subCommand = params[0];

    //CAP LS : Liste des capacités (aucune supportée)
    if (subCommand == "LS")
    {
        sendMessage(std::string(SERVER_NAME) + " CAP * LS :", clientSocket);
    }
    //CAP END : Fin de négociation
    else if (subCommand == "END")
    {
        sendMessage(std::string(SERVER_NAME) + " CAP * ACK", clientSocket);
    }
    //Autres sous-commandes : on accuse réception
    else
    {
        sendMessage(std::string(SERVER_NAME) + " CAP * ACK", clientSocket);
    }
}

void Server::handlePassCommand(const Request& request)
{
    User* client = request.getUser();
    const Request::ParamList& params = request.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();

    std::string response = std::string(SERVER_NAME);

    if (client->isRegistered())
    {
        response += " 462 " + nickname + " :Unauthorized command (already registered)";
        sendMessage(response, clientSocket);
        return;
    }
    if (params.size() != 1)
    {
        response += " 461 " + nickname + " PASS :Not enough parameters";
        sendMessage(response, clientSocket);
        return;
    }
    if (params[0] != _connectionPassword)
    {
        response += " 464 " + nickname + " :Password incorrect";
        sendMessage(response, clientSocket);
        return;
    }
    client->setPasswordProvided(true);
    sendMessage(":" + std::string(SERVER_NAME) + " NOTICE * :Password accepted", clientSocket);
}

void Server::handleNickCommand(const Request& request)
{
    User* client = request.getUser();
    int clientSocket = client->getSocketFd();
    const std::string currentNickname = client->getNickName();
    if (validateNickCommand(request))
        return;
    const std::string& newNickname = request.getParams()[0];
    const std::string nickChangeMsg = client->getPrefix() + " NICK " + newNickname;
    client->setNickName(newNickname);
    const std::map<std::string, Channel*>& channels = client->getChannels();
    for (std::map<std::string, Channel*>::const_iterator it = channels.begin();
         it != channels.end(); ++it)
        if (it->second != NULL)
            broadcast(nickChangeMsg, NULL, *(it->second));
    checkRegistrationComplete(client);
    if (!currentNickname.empty())
        sendMessage(nickChangeMsg, clientSocket);
}

bool Server::validateNickCommand(const Request& request)
{
    User* client = request.getUser();
    const Request::ParamList& params = request.getParams();
    const int clientSocket = client->getSocketFd();
    const std::string& currentNickname = client->getNickName();
    std::string response;
    if (!client->hasPasswordProvided())
    {
        response = std::string(SERVER_NAME) + " 464 " + currentNickname +
                   " :Password required (PASS first)";
        sendMessage(response, clientSocket);
        return (true);
    }
    if (params.empty())
    {
        response = std::string(SERVER_NAME) + " 431 " + currentNickname +
                   " :No nickname given";
        sendMessage(response, clientSocket);
        return (true);
    }
    const std::string& newNickname = params[0];
    if (containsForbiddenChars(newNickname))
    {
        response = std::string(SERVER_NAME) + " 432 " + currentNickname +
                   " " + newNickname + " :Invalid nickname";
        sendMessage(response, clientSocket);
        return (true);
    }
    ClientRegistry::iterator existingUser = findUserByNickname(newNickname);
    if (existingUser != _connectedClients.end() &&
        existingUser->second.getSocketFd() != clientSocket)
    {
        response = std::string(SERVER_NAME) + " 433 " + currentNickname +
                   " " + newNickname + " :Nickname already in use";
        sendMessage(response, clientSocket);
        return (true);
    }
    if (newNickname == currentNickname)
        return (true);
    return (false);
}

void Server::handleUserCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();

    std::string response;

    if (!client->hasPasswordProvided())
    {
        response = std::string(SERVER_NAME) + " 464 " + nickname +
                   " :Password required (PASS first)";
        sendMessage(response, clientSocket);
        return;
    }
    if (client->isRegistered())
    {
        response = std::string(SERVER_NAME) + " 462 " + nickname +
                   " :Already registered";
        sendMessage(response, clientSocket);
        return;
    }
    if (params.size() < 3)
    {
        response = std::string(SERVER_NAME) + " 461 " + nickname +
                   " USER :Not enough parameters";
        sendMessage(response, clientSocket);
        return;
    }
    client->setName(params[0]);
    client->setFullName(req.getInfo());
    // client->setFullName(params[2]);

    checkRegistrationComplete(client);
}

void Server::handlePingCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string token;

    if (!params.empty())
        token = params[0];
    else
        token = req.getInfo();

    if (token.empty())
    {
        sendMessage(std::string(SERVER_NAME) + " 461 " + nickname +
                    " PING :Not enough parameters", clientSocket);
        return;
    }

    sendMessage(":" + std::string(SERVER_NAME) + " PONG " + nickname +
                " :" + token, clientSocket);
}

void Server::handlePongCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string token;

    if (!params.empty())
        token = params[0];
    else
        token = req.getInfo();

    if (token.empty())
    {
        sendMessage(std::string(SERVER_NAME) + " 461 " + nickname +
                    " PONG :Not enough parameters", clientSocket);
        return;
    }

    sendMessage(":" + std::string(SERVER_NAME) + " PONG " + nickname +
                " :" + token, clientSocket);
}

void Server::handlePrivmsgCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        response += " 451 " + nickname + " :You have not registered";
        sendMessage(response, clientSocket);
        return;
    }
    if (params.size() != 1)
    {
        response += " 411 " + nickname + " :No recipient given (PRIVMSG)";
        sendMessage(response, clientSocket);
        return;
    }
    if (req.getInfo().empty())
    {
        response += " 412 " + nickname + " :No text to send";
        sendMessage(response, clientSocket);
        return;
    }

    deliverMessage(client, params[0], req.getInfo(), "PRIVMSG", true);
}

void Server::handleNoticeCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();

    if (!client->isRegistered() || params.size() != 1 || req.getInfo().empty())
        return;

    deliverMessage(client, params[0], req.getInfo(), "NOTICE", false);
}

void Server::deliverMessage(User* sender, const std::string& target,
                            const std::string& text, const std::string& command,
                            bool reportErrors)
{
    const int senderSocket = sender->getSocketFd();
    const std::string& nickname = sender->getNickName();
    const std::string message = sender->getPrefix() + " " + command + " " +
                                target + " :" + text;
    Channel* channel = findChannel(target);
    if (channel != NULL)
    {
        if (!channel->isMember(sender))
        {
            if (reportErrors)
                sendMessage(std::string(SERVER_NAME) + " 404 " + nickname + " " +
                            target + " :Cannot send to channel", senderSocket);
            return;
        }
        broadcast(message, sender, *channel);
        return;
    }
    ClientRegistry::iterator recipient = findUserByNickname(target);
    if (recipient == _connectedClients.end())
    {
        if (reportErrors)
            sendMessage(std::string(SERVER_NAME) + " 401 " + nickname + " " +
                        target + " :No such nick", senderSocket);
        return;
    }
    sendMessage(message, recipient->second.getSocketFd());
}

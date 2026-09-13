/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 16:35:23 by mbah              #+#    #+#             */
/*   Updated: 2026/09/13 11:40:14 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

//COMMANDES D'AUTHENTIFICATION

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
    //On envoie pas de réponse, le client continue avec NICK et USER
}

void Server::handleNickCommand(const Request& request)
{
    User* client = request.getUser();
    const Request::ParamList& params = request.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& currentNickname = client->getNickName();

    std::string response;

    if (!client->hasPasswordProvided())
    {
        response = std::string(SERVER_NAME) + " 464 " + currentNickname +
                   " :Password required (PASS first)";
        sendMessage(response, clientSocket);
        return;
    }

    if (params.empty())
    {
        response = std::string(SERVER_NAME) + " 431 " + currentNickname +
                   " :No nickname given";
        sendMessage(response, clientSocket);
        return;
    }

    const std::string& newNickname = params[0];

    if (containsForbiddenChars(newNickname))
    {
        response = std::string(SERVER_NAME) + " 432 " + currentNickname +
                   " " + newNickname + " :Invalid nickname";
        sendMessage(response, clientSocket);
        return;
    }

    ClientRegistry::iterator existingUser = findUserByNickname(newNickname);
    if (existingUser != _connectedClients.end())
    {
        response = std::string(SERVER_NAME) + " 433 " + currentNickname +
                   " " + newNickname + " :Nickname already in use";
        sendMessage(response, clientSocket);
        return;
    }

    std::string nickChangeMsg = client->getPrefix() + " NICK " + newNickname;
    client->setNickName(newNickname);

	const std::map<std::string, Channel*>& channels = client->getChannels();
    std::map<std::string, Channel*>::const_iterator channelIt = channels.begin();

    while (channelIt != channels.end())
    {
        if (channelIt->second != NULL)
            broadcast(nickChangeMsg, NULL, *(channelIt->second));
        ++channelIt;
    }

    checkRegistrationComplete(client);

    if (!currentNickname.empty())
        sendMessage(nickChangeMsg, clientSocket);
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

// ============================================================================
//                       COMMANDES DE COMMUNICATION
// ============================================================================

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

    const std::string& target = params[0];
    std::string message = client->getPrefix() + " PRIVMSG " + target +
                          " :" + req.getInfo();
    Channel* channel = findChannel(target);

    if (channel != NULL)
    {
        if (!channel->isMember(client))
        {
            response += " 404 " + nickname + " " + target +
                        " :Cannot send to channel";
            sendMessage(response, clientSocket);
            return;
        }
        broadcast(message, client, *channel);
        return;
    }

    ClientRegistry::iterator recipient = findUserByNickname(target);
    if (recipient == _connectedClients.end())
    {
        response += " 401 " + nickname + " " + target + " :No such nick";
        sendMessage(response, clientSocket);
        return;
    }
    sendMessage(message, recipient->second.getSocketFd());
}

void Server::handleNoticeCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();

    if (!client->isRegistered() || params.size() != 1 || req.getInfo().empty())
        return;

    const std::string& target = params[0];
    std::string message = client->getPrefix() + " NOTICE " + target +
                          " :" + req.getInfo();
    Channel* channel = findChannel(target);

    if (channel != NULL)
    {
        if (channel->isMember(client))
            broadcast(message, client, *channel);
        return;
    }

    ClientRegistry::iterator recipient = findUserByNickname(target);
    if (recipient != _connectedClients.end())
        sendMessage(message, recipient->second.getSocketFd());
}

// ============================================================================
//                       COMMANDES DE CANAUX
// ============================================================================

void Server::handleJoinCommand(const Request& req)
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
    if (params.empty())
    {
        response += " 461 " + nickname + " JOIN :Not enough parameters";
        sendMessage(response, clientSocket);
        return;
    }

    const std::string& channelName = params[0];
    if (!Channel::isValidChannelName(channelName))
    {
        response += " 479 " + nickname + " " + channelName +
                    " :Bad channel name";
        sendMessage(response, clientSocket);
        return;
    }

    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        _activeChannels.insert(std::make_pair(channelName,
                                               Channel(channelName, "", this)));
        channel = findChannel(channelName);
    }
    if (channel->isMember(client))
    {
        response += " 443 " + nickname + " " + channelName +
                    " :is already on channel";
        sendMessage(response, clientSocket);
        return;
    }
    if (channel->hasMode('k') &&
        (params.size() < 2 || params[1] != channel->getPassword()))
    {
        response += " 475 " + nickname + " " + channelName +
                    " :Cannot join channel (+k)";
        sendMessage(response, clientSocket);
        return;
    }
    if (channel->hasMode('l') &&
        static_cast<int>(channel->getMembers().size()) >= channel->getLimit())
    {
        response += " 471 " + nickname + " " + channelName +
                    " :Cannot join channel (+l)";
        sendMessage(response, clientSocket);
        return;
    }

    channel->addMember(client);
    client->addChannel(channelName, channel);
    std::string joinMessage = client->getPrefix() + " JOIN " + channelName;
    const std::list<User*>& members = channel->getMembers(0);
    for (std::list<User*>::const_iterator it = members.begin();
         it != members.end(); ++it)
        sendMessage(joinMessage, (*it)->getSocketFd());
}

void Server::handlePartCommand(const Request& req)
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
        response += " 461 " + nickname + " PART :Not enough parameters";
        sendMessage(response, clientSocket);
        return;
    }

    const std::string& channelName = params[0];
    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        response += " 403 " + nickname + " " + channelName +
                    " :No such channel";
        sendMessage(response, clientSocket);
        return;
    }
    if (!channel->isMember(client))
    {
        response += " 442 " + nickname + " " + channelName +
                    " :You're not on that channel";
        sendMessage(response, clientSocket);
        return;
    }

    std::string partMessage = client->getPrefix() + " PART " + channelName;
    if (!req.getInfo().empty())
        partMessage += " :" + req.getInfo();
    const std::list<User*>& members = channel->getMembers(0);
    for (std::list<User*>::const_iterator it = members.begin();
         it != members.end(); ++it)
        sendMessage(partMessage, (*it)->getSocketFd());

    channel->removeMember(client);
    client->removeChannel(channelName);
    if (channel->getMembers().empty())
        _activeChannels.erase(channelName);
}

void Server::handleTopicCommand(const Request& req)
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
        response += " 461 " + nickname + " TOPIC :Not enough parameters";
        sendMessage(response, clientSocket);
        return;
    }

    const std::string& channelName = params[0];
    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        response += " 403 " + nickname + " " + channelName +
                    " :No such channel";
        sendMessage(response, clientSocket);
        return;
    }
    if (!channel->isMember(client))
    {
        response += " 442 " + nickname + " " + channelName +
                    " :You're not on that channel";
        sendMessage(response, clientSocket);
        return;
    }
    if (req.getInfo().empty())
    {
        if (channel->getTopic().empty())
            response += " 331 " + nickname + " " + channelName +
                        " :No topic is set";
        else
            response += " 332 " + nickname + " " + channelName +
                        " :" + channel->getTopic();
        sendMessage(response, clientSocket);
        return;
    }
    if (channel->hasMode('t') && !channel->isOperator(client))
    {
        response += " 482 " + nickname + " " + channelName +
                    " :You're not a channel operator";
        sendMessage(response, clientSocket);
        return;
    }

    channel->setTopic(req.getInfo());
    std::string topicMessage = client->getPrefix() + " TOPIC " + channelName +
                               " :" + channel->getTopic();
    const std::list<User*>& members = channel->getMembers(0);
    for (std::list<User*>::const_iterator it = members.begin();
         it != members.end(); ++it)
        sendMessage(topicMessage, (*it)->getSocketFd());
}

void Server::handleListCommand(const Request& req)
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
    if (params.size() > 1)
    {
        response += " 461 " + nickname + " LIST :Not enough parameters";
        sendMessage(response, clientSocket);
        return;
    }

    sendMessage(response + " 321 " + nickname + " Channel :Users  Name",
                clientSocket);

    if (params.empty())
    {
        for (ChannelRegistry::const_iterator it = _activeChannels.begin();
             it != _activeChannels.end(); ++it)
        {
            const Channel& channel = it->second;
            sendMessage(std::string(SERVER_NAME) + " 322 " + nickname + " " +
                        channel.getName() + " " +
                        intToString(static_cast<int>(channel.getMembers().size())) +
                        " :" + channel.getTopic(), clientSocket);
        }
    }
    else
    {
        Channel* channel = findChannel(params[0]);
        if (channel != NULL)
        {
            sendMessage(std::string(SERVER_NAME) + " 322 " + nickname + " " +
                        channel->getName() + " " +
                        intToString(static_cast<int>(channel->getMembers().size())) +
                        " :" + channel->getTopic(), clientSocket);
        }
    }

    sendMessage(std::string(SERVER_NAME) + " 323 " + nickname +
                " :End of /LIST", clientSocket);
}

void Server::handleNamesCommand(const Request& req)
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
    if (params.size() > 1)
    {
        response += " 461 " + nickname + " NAMES :Too many parameters";
        sendMessage(response, clientSocket);
        return;
    }

    if (params.empty())
    {
        for (ChannelRegistry::const_iterator it = _activeChannels.begin();
             it != _activeChannels.end(); ++it)
        {
            const Channel& channel = it->second;
            std::string names;
            const std::list<User*>& members = channel.getMembers();
            for (std::list<User*>::const_iterator memberIt = members.begin();
                 memberIt != members.end(); ++memberIt)
            {
                if (!names.empty())
                    names += " ";
                if (channel.isOperator(*memberIt))
                    names += "@";
                names += (*memberIt)->getNickName();
            }
            sendMessage(std::string(SERVER_NAME) + " 353 " + nickname +
                        " = " + channel.getName() + " :" + names,
                        clientSocket);
            sendMessage(std::string(SERVER_NAME) + " 366 " + nickname + " " +
                        channel.getName() + " :End of /NAMES list", clientSocket);
        }
        return;
    }

    Channel* channel = findChannel(params[0]);
    if (channel != NULL)
    {
        std::string names;
        const std::list<User*>& members = channel->getMembers();
        for (std::list<User*>::const_iterator memberIt = members.begin();
             memberIt != members.end(); ++memberIt)
        {
            if (!names.empty())
                names += " ";
            if (channel->isOperator(*memberIt))
                names += "@";
            names += (*memberIt)->getNickName();
        }
        sendMessage(std::string(SERVER_NAME) + " 353 " + nickname + " = " +
                    channel->getName() + " :" + names, clientSocket);
    }
    sendMessage(std::string(SERVER_NAME) + " 366 " + nickname + " " +
                params[0] + " :End of /NAMES list", clientSocket);
}

void Server::handleWhoCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter WHO
}

void Server::handleInviteCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter INVITE
}

void Server::handleKickCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter KICK
}

void Server::handleModeCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter MODE
}

// ============================================================================
//                       COMMANDES D'OPÉRATEUR
// ============================================================================

void Server::handleOperCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter OPER
}

void Server::handleKillCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter KILL
}

// ============================================================================
//                       COMMANDES SPÉCIALES
// ============================================================================

void Server::handleGlobopsCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter GLOBOPS
}

void Server::handleShowtimeCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter SHOWTIME
}

// ============================================================================
//                       COMMANDE DE DÉCONNEXION
// ============================================================================

void Server::handleQuitCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter QUIT
}
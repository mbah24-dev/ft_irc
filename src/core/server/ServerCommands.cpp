/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 16:35:23 by mbah              #+#    #+#             */
/*   Updated: 2026/09/29 10:37:51 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <climits>

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
    if (channel->hasMode('i') &&
        !channel->isInvited(client->getNickName()))
    {
        response += " 473 " + nickname + " " + channelName +
                    " :Cannot join channel (+i)";
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
    channel->removeInvitation(client->getNickName());
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
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (params.size() > 2)
    {
        sendMessage(response + " 461 " + nickname +
                    " WHO :Too many parameters", clientSocket);
        return;
    }

    const std::string target = params.empty() ? "*" : params[0];
    const bool operatorsOnly = params.size() == 2 && params[1] == "o";
    Channel* channel = findChannel(target);
    if (channel != NULL)
    {
        const std::list<User*>& members = channel->getMembers();
        for (std::list<User*>::const_iterator it = members.begin();
             it != members.end(); ++it)
        {
            User* subject = *it;
            if (operatorsOnly && !channel->isOperator(subject))
                continue;
            std::string flags = "H";
            if (channel->isOperator(subject))
                flags += "@";
            sendMessage(response + " 352 " + nickname + " " + target + " " +
                        subject->getName() + " " + subject->getHostMask() +
                        " " + SERVER_NAME + " " + subject->getNickName() +
                        " " + flags + " :0 " + subject->getFullName(),
                        clientSocket);
        }
    }
    else if (target == "*")
    {
        for (ClientRegistry::iterator it = _connectedClients.begin();
             it != _connectedClients.end(); ++it)
        {
            User& subject = it->second;
            if (!subject.isRegistered() ||
                (operatorsOnly && !subject.isOperator()))
                continue;
            std::string channelName = "*";
            std::string flags = "H";
            const std::map<std::string, Channel*>& channels = subject.getChannels();
            if (!channels.empty())
            {
                channelName = channels.begin()->first;
                if (channels.begin()->second != NULL &&
                    channels.begin()->second->isOperator(&subject))
                    flags += "@";
            }
            sendMessage(response + " 352 " + nickname + " " + channelName +
                        " " + subject.getName() + " " + subject.getHostMask() +
                        " " + SERVER_NAME + " " + subject.getNickName() +
                        " " + flags + " :0 " + subject.getFullName(),
                        clientSocket);
        }
    }
    else
    {
        ClientRegistry::iterator it = findUserByNickname(target);
        if (it != _connectedClients.end() && it->second.isRegistered() &&
            (!operatorsOnly || it->second.isOperator()))
        {
            User& subject = it->second;
            std::string channelName = "*";
            std::string flags = "H";
            const std::map<std::string, Channel*>& channels = subject.getChannels();
            if (!channels.empty())
            {
                channelName = channels.begin()->first;
                if (channels.begin()->second != NULL &&
                    channels.begin()->second->isOperator(&subject))
                    flags += "@";
            }
            sendMessage(response + " 352 " + nickname + " " + channelName +
                        " " + subject.getName() + " " + subject.getHostMask() +
                        " " + SERVER_NAME + " " + subject.getNickName() +
                        " " + flags + " :0 " + subject.getFullName(),
                        clientSocket);
        }
    }

    sendMessage(response + " 315 " + nickname + " " + target +
                " :End of /WHO list", clientSocket);
}

void Server::handleInviteCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (params.size() != 2)
    {
        sendMessage(response + " 461 " + nickname +
                    " INVITE :Not enough parameters", clientSocket);
        return;
    }

    ClientRegistry::iterator invitee = findUserByNickname(params[0]);
    if (invitee == _connectedClients.end() || !invitee->second.isRegistered())
    {
        sendMessage(response + " 401 " + nickname + " " + params[0] +
                    " :No such nick", clientSocket);
        return;
    }

    Channel* channel = findChannel(params[1]);
    if (channel == NULL)
    {
        sendMessage(response + " 403 " + nickname + " " + params[1] +
                    " :No such channel", clientSocket);
        return;
    }
    if (!channel->isMember(client))
    {
        sendMessage(response + " 442 " + nickname + " " + params[1] +
                    " :You're not on that channel", clientSocket);
        return;
    }
    if (channel->isMember(&invitee->second))
    {
        sendMessage(response + " 443 " + nickname + " " + params[0] + " " +
                    params[1] + " :is already on channel", clientSocket);
        return;
    }
    if (channel->hasMode('i') && !channel->isOperator(client))
    {
        sendMessage(response + " 482 " + nickname + " " + params[1] +
                    " :You're not a channel operator", clientSocket);
        return;
    }

    channel->addInvitation(params[0]);
    sendMessage(response + " 341 " + nickname + " " + params[0] + " " +
                params[1], clientSocket);
    sendMessage(client->getPrefix() + " INVITE " + params[0] + " :" +
                params[1], invitee->second.getSocketFd());
}

void Server::handleKickCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (params.size() < 2)
    {
        sendMessage(response + " 461 " + nickname +
                    " KICK :Not enough parameters", clientSocket);
        return;
    }

    Channel* channel = findChannel(params[0]);
    if (channel == NULL)
    {
        sendMessage(response + " 403 " + nickname + " " + params[0] +
                    " :No such channel", clientSocket);
        return;
    }
    if (!channel->isMember(client))
    {
        sendMessage(response + " 442 " + nickname + " " + params[0] +
                    " :You're not on that channel", clientSocket);
        return;
    }
    if (!channel->isOperator(client))
    {
        sendMessage(response + " 482 " + nickname + " " + params[0] +
                    " :You're not a channel operator", clientSocket);
        return;
    }

    User* target = channel->findMember(params[1]);
    if (target == NULL)
    {
        sendMessage(response + " 441 " + nickname + " " + params[1] + " " +
                    params[0] + " :They aren't on that channel", clientSocket);
        return;
    }

    std::string kickMessage = client->getPrefix() + " KICK " + params[0] +
                              " " + params[1] + " :";
    if (!req.getInfo().empty())
        kickMessage += req.getInfo();
    else
        kickMessage += nickname;
    const std::list<User*>& members = channel->getMembers();
    for (std::list<User*>::const_iterator it = members.begin();
         it != members.end(); ++it)
        sendMessage(kickMessage, (*it)->getSocketFd());

    channel->removeMember(target);
    target->removeChannel(params[0]);
    if (channel->getMembers().empty())
        _activeChannels.erase(params[0]);
}

void Server::handleModeCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (params.empty())
    {
        sendMessage(response + " 461 " + nickname +
                    " MODE :Not enough parameters", clientSocket);
        return;
    }

    const std::string& channelName = params[0];
    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        sendMessage(response + " 403 " + nickname + " " + channelName +
                    " :No such channel", clientSocket);
        return;
    }
    if (params.size() == 1 && req.getInfo().empty())
    {
        std::string modeReply = response + " 324 " + nickname + " " +
                                channelName + " " + channel->getModeString();
        if (channel->hasMode('l'))
            modeReply += " " + intToString(channel->getLimit());
        sendMessage(modeReply, clientSocket);
        return;
    }
    if (params.size() < 2)
    {
        sendMessage(response + " 461 " + nickname +
                    " MODE :Not enough parameters", clientSocket);
        return;
    }
    if (!channel->isOperator(client))
    {
        sendMessage(response + " 482 " + nickname + " " + channelName +
                    " :You're not a channel operator", clientSocket);
        return;
    }

    std::vector<std::string> modeParams;
    for (Request::ParamList::const_iterator it = params.begin() + 2;
         it != params.end(); ++it)
        modeParams.push_back(*it);
    if (!req.getInfo().empty())
        modeParams.push_back(req.getInfo());

    const std::string& modeString = params[1];
    char sign = '\0';
    std::size_t paramIndex = 0;
    for (std::string::size_type i = 0; i < modeString.size(); ++i)
    {
        if (modeString[i] == '+' || modeString[i] == '-')
        {
            sign = modeString[i];
            continue;
        }
        if (sign == '\0' || std::string(CHANNEL_MODES).find(modeString[i]) ==
            std::string::npos)
        {
            sendMessage(response + " 472 " + nickname + " " + modeString[i] +
                        " :is unknown mode char to me for " + channelName,
                        clientSocket);
            return;
        }

        const char mode = modeString[i];
        const bool needsParam = mode == 'o' ||
                                (mode == 'k' && sign == '+') ||
                                (mode == 'l' && sign == '+');
        if (needsParam && paramIndex >= modeParams.size())
        {
            sendMessage(response + " 461 " + nickname +
                        " MODE :Not enough parameters", clientSocket);
            return;
        }
        if (needsParam)
        {
            const std::string& value = modeParams[paramIndex];
            if (mode == 'o' && channel->findMember(value) == NULL)
            {
                sendMessage(response + " 441 " + nickname + " " + value +
                            " " + channelName +
                            " :They aren't on that channel", clientSocket);
                return;
            }
            if (mode == 'l')
            {
                char* endPointer;
                long limit = std::strtol(value.c_str(), &endPointer, 10);
                if (value.empty() || *endPointer != '\0' || limit <= 0 ||
                    limit > INT_MAX)
                {
                    sendMessage(response + " 461 " + nickname +
                                " MODE :Invalid channel limit", clientSocket);
                    return;
                }
            }
            ++paramIndex;
        }
    }
    if (sign == '\0')
    {
        sendMessage(response + " 461 " + nickname +
                    " MODE :Not enough parameters", clientSocket);
        return;
    }

    sign = '\0';
    paramIndex = 0;
    std::string changedModes;
    std::string changedParams;
    for (std::string::size_type i = 0; i < modeString.size(); ++i)
    {
        if (modeString[i] == '+' || modeString[i] == '-')
        {
            sign = modeString[i];
            continue;
        }

        const char mode = modeString[i];
        const bool needsParam = mode == 'o' ||
                                (mode == 'k' && sign == '+') ||
                                (mode == 'l' && sign == '+');
        std::string value;
        if (needsParam)
            value = modeParams[paramIndex++];

        bool changed = false;
        if (mode == 'o')
        {
            User* target = channel->findMember(value);
            if (sign == '+' && !channel->isOperator(target))
            {
                channel->addOperator(target);
                changed = true;
            }
            else if (sign == '-' && channel->isOperator(target))
            {
                channel->removeOperator(target);
                changed = true;
            }
        }
        else if (mode == 'k')
        {
            if (sign == '+' &&
                (!channel->hasMode('k') || channel->getPassword() != value))
            {
                channel->setPassword(value);
                channel->editMode('k', '+');
                changed = true;
            }
            else if (sign == '-' && channel->hasMode('k'))
            {
                channel->setPassword("");
                channel->editMode('k', '-');
                changed = true;
            }
        }
        else if (mode == 'l')
        {
            if (sign == '+')
            {
                char* endPointer;
                int limit = static_cast<int>(std::strtol(value.c_str(),
                                                          &endPointer, 10));
                if (!channel->hasMode('l') || channel->getLimit() != limit)
                {
                    channel->setLimit(limit);
                    channel->editMode('l', '+');
                    changed = true;
                }
            }
            else if (channel->hasMode('l'))
            {
                channel->setLimit(-1);
                channel->editMode('l', '-');
                changed = true;
            }
        }
        else if (sign == '+' && !channel->hasMode(mode))
        {
            channel->editMode(mode, '+');
            changed = true;
        }
        else if (sign == '-' && channel->hasMode(mode))
        {
            channel->editMode(mode, '-');
            changed = true;
        }

        if (!changed)
            continue;
        if (changedModes.empty() || changedModes[changedModes.size() - 1] != sign)
            changedModes += sign;
        changedModes += mode;
        if (mode == 'o' || (mode == 'k' && sign == '+') ||
            (mode == 'l' && sign == '+'))
        {
            if (!changedParams.empty())
                changedParams += " ";
            changedParams += value;
        }
    }

    if (!changedModes.empty())
    {
        std::string modeMessage = client->getPrefix() + " MODE " + channelName +
                                  " " + changedModes;
        if (!changedParams.empty())
            modeMessage += " " + changedParams;
        const std::list<User*>& members = channel->getMembers(0);
        for (std::list<User*>::const_iterator it = members.begin();
             it != members.end(); ++it)
            sendMessage(modeMessage, (*it)->getSocketFd());
    }
}

// ============================================================================
//                       COMMANDES D'OPÉRATEUR
// ============================================================================

void Server::handleOperCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (params.size() != 2)
    {
        sendMessage(response + " 461 " + nickname +
                    " OPER :Not enough parameters", clientSocket);
        return;
    }
    // This server has no separate operator account configuration: use the
    // registered username as the OPER name and the server password as secret.
    if (params[0] != client->getName() || params[1] != _connectionPassword)
    {
        sendMessage(response + " 464 " + nickname +
                    " :Password incorrect", clientSocket);
        return;
    }

    client->setOperator(true);
    sendMessage(response + " 381 " + nickname +
                " :You are now an IRC operator", clientSocket);
}

void Server::handleKillCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (params.empty())
    {
        sendMessage(response + " 461 " + nickname +
                    " KILL :Not enough parameters", clientSocket);
        return;
    }
    if (!client->isOperator())
    {
        sendMessage(response + " 481 " + nickname +
                    " :Permission Denied- You're not an IRC operator",
                    clientSocket);
        return;
    }

    ClientRegistry::iterator targetIt = findUserByNickname(params[0]);
    if (targetIt == _connectedClients.end())
    {
        sendMessage(response + " 401 " + nickname + " " + params[0] +
                    " :No such nick", clientSocket);
        return;
    }

    User* target = &targetIt->second;
    std::string reason = req.getInfo();
    if (reason.empty() && params.size() > 1)
        reason = params[1];
    if (reason.empty())
        reason = "Killed";

    sendMessage(client->getPrefix() + " KILL " + target->getNickName() +
                " :" + reason, target->getSocketFd());
    disconnectClient(target, "Killed by " + nickname + " (" + reason + ")");
}

// ============================================================================
//                       COMMANDES SPÉCIALES
// ============================================================================

void Server::handleGlobopsCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (!client->isOperator())
    {
        sendMessage(response + " 481 " + nickname +
                    " :Permission Denied- You're not an IRC operator",
                    clientSocket);
        return;
    }

    std::string message = req.getInfo();
    if (message.empty())
    {
        for (Request::ParamList::const_iterator it = params.begin();
             it != params.end(); ++it)
        {
            if (!message.empty())
                message += " ";
            message += *it;
        }
    }
    if (message.empty())
    {
        sendMessage(response + " 461 " + nickname +
                    " GLOBOPS :Not enough parameters", clientSocket);
        return;
    }

    const std::string globopsMessage = client->getPrefix() +
                                       " GLOBOPS :" + message;
    for (ClientRegistry::iterator it = _connectedClients.begin();
         it != _connectedClients.end(); ++it)
    {
        if (it->second.isOperator())
            sendMessage(globopsMessage, it->second.getSocketFd());
    }
}

void Server::handleShowtimeCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    std::string response = std::string(SERVER_NAME);

    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname +
                    " :You have not registered", clientSocket);
        return;
    }
    if (!params.empty() || !req.getInfo().empty())
    {
        sendMessage(response + " 461 " + nickname +
                    " SHOWTIME :Too many parameters", clientSocket);
        return;
    }

    std::time_t currentTime = std::time(NULL);
    const char* timeText = std::ctime(&currentTime);
    std::string timeMessage = timeText != NULL ? timeText : "Unknown time";
    if (!timeMessage.empty() && timeMessage[timeMessage.size() - 1] == '\n')
        timeMessage.erase(timeMessage.size() - 1);
    sendMessage(response + " 391 " + nickname + " " + SERVER_NAME +
                " :" + timeMessage, clientSocket);
}

// ============================================================================
//                       COMMANDE DE DÉCONNEXION
// ============================================================================

void Server::handleQuitCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    std::string reason = req.getInfo();

    if (reason.empty())
    {
        for (Request::ParamList::const_iterator it = params.begin();
             it != params.end(); ++it)
        {
            if (!reason.empty())
                reason += " ";
            reason += *it;
        }
    }
    if (reason.empty())
        reason = "Client quit";

    disconnectClient(client, reason);
}
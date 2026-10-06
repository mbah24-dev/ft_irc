/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerChannelCommands.cpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:19:20 by zcherif           #+#    #+#             */
/*   Updated: 2026/10/06 12:19:24 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

static std::vector<std::string> splitCommaList(const std::string& value)
{
    std::vector<std::string> items;
    std::string::size_type start = 0;
    while (start <= value.length())
    {
        std::string::size_type comma = value.find(',', start);
        if (comma == std::string::npos)
            comma = value.length();
        if (comma > start)
            items.push_back(value.substr(start, comma - start));
        if (comma == value.length())
            break;
        start = comma + 1;
    }
    return (items);
}

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

    const std::vector<std::string> channelNames = splitCommaList(params[0]);
    const std::vector<std::string> keys = params.size() > 1
        ? splitCommaList(params[1]) : std::vector<std::string>();
    for (std::size_t channelIndex = 0; channelIndex < channelNames.size(); ++channelIndex)
        joinChannel(client, channelNames[channelIndex],
                    channelIndex < keys.size() ? keys[channelIndex] : "");
}

void Server::joinChannel(User* client, const std::string& channelName,
                         const std::string& key)
{
    const int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    if (!Channel::isValidChannelName(channelName))
    {
        sendMessage(std::string(SERVER_NAME) + " 479 " + nickname + " " +
                    channelName + " :Bad channel name", clientSocket);
        return;
    }
    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        _activeChannels.insert(std::make_pair(channelName, Channel(channelName, "", this)));
        channel = findChannel(channelName);
    }
    if (!validateChannelJoin(client, channel, key))
        return;

    channel->addMember(client);
    channel->removeInvitation(nickname);
    client->addChannel(channel->getName(), channel);
    const std::string message = client->getPrefix() + " JOIN " + channel->getName();
    const std::list<User*>& members = channel->getMembers();
    for (std::list<User*>::const_iterator it = members.begin(); it != members.end(); ++it)
        sendMessage(message, (*it)->getSocketFd());
    if (channel->getTopic().empty())
        sendMessage(std::string(SERVER_NAME) + " 331 " + nickname + " " +
                    channel->getName() + " :No topic is set", clientSocket);
    else
        sendMessage(std::string(SERVER_NAME) + " 332 " + nickname + " " +
                    channel->getName() + " :" + channel->getTopic(), clientSocket);
    sendChannelNames(client, channel);
}

bool Server::validateChannelJoin(User* client, Channel* channel,
                                 const std::string& key)
{
    const int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    const std::string& name = channel->getName();
    if (channel->isMember(client))
        sendMessage(std::string(SERVER_NAME) + " 443 " + nickname + " " + name +
                    " :is already on channel", clientSocket);
    else if (channel->hasMode('i') && !channel->isInvited(nickname))
        sendMessage(std::string(SERVER_NAME) + " 473 " + nickname + " " + name +
                    " :Cannot join channel (+i)", clientSocket);
    else if (channel->hasMode('k') && key != channel->getPassword())
        sendMessage(std::string(SERVER_NAME) + " 475 " + nickname + " " + name +
                    " :Cannot join channel (+k)", clientSocket);
    else if (channel->hasMode('l') &&
             static_cast<int>(channel->getMembers().size()) >= channel->getLimit())
        sendMessage(std::string(SERVER_NAME) + " 471 " + nickname + " " + name +
                    " :Cannot join channel (+l)", clientSocket);
    else
        return (true);
    return (false);
}

void Server::sendChannelNames(User* client, const Channel* channel)
{
    std::string names;
    const std::list<User*>& members = channel->getMembers();
    for (std::list<User*>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        if (!names.empty())
            names += " ";
        if (channel->isOperator(*it))
            names += "@";
        names += (*it)->getNickName();
    }
    const std::string& nickname = client->getNickName();
    const std::string& name = channel->getName();
    sendMessage(std::string(SERVER_NAME) + " 353 " + nickname + " = " + name +
                " :" + names, client->getSocketFd());
    sendMessage(std::string(SERVER_NAME) + " 366 " + nickname + " " + name +
                " :End of /NAMES list", client->getSocketFd());
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
        sendChannelError(client, 403, channelName, "No such channel");
        return;
    }
    if (!channel->isMember(client))
    {
        sendChannelError(client, 442, channelName, "You're not on that channel");
        return;
    }

    std::string partMessage = client->getPrefix() + " PART " + channelName;
    if (!req.getInfo().empty())
        partMessage += " :" + req.getInfo();
    broadcast(partMessage, NULL, *channel);
    removeUserFromChannel(channel, client);
}

void Server::removeUserFromChannel(Channel* channel, User* user)
{
    const std::string channelName = channel->getName();
    channel->removeMember(user);
    user->removeChannel(channelName);
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
        sendChannelError(client, 403, channelName, "No such channel");
        return;
    }
    if (!channel->isMember(client))
    {
        sendChannelError(client, 442, channelName, "You're not on that channel");
        return;
    }
    if (req.getInfo().empty())
    {
        sendChannelTopic(client, channel);
        return;
    }
    if (channel->hasMode('t') && !channel->isOperator(client))
    {
        sendChannelError(client, 482, channelName,
                         "You're not a channel operator");
        return;
    }
    updateChannelTopic(client, channel, req.getInfo());
}

void Server::sendChannelTopic(User* client, const Channel* channel)
{
    const std::string code = channel->getTopic().empty() ? "331" : "332";
    const std::string topic = channel->getTopic().empty()
        ? " :No topic is set" : " :" + channel->getTopic();
    sendMessage(std::string(SERVER_NAME) + " " + code + " " +
                client->getNickName() + " " + channel->getName() + topic,
                client->getSocketFd());
}

void Server::updateChannelTopic(User* client, Channel* channel,
                                const std::string& topic)
{
    channel->setTopic(topic);
    const std::string message = client->getPrefix() + " TOPIC " +
                                channel->getName() + " :" + topic;
    const std::list<User*>& members = channel->getMembers();
    for (std::list<User*>::const_iterator it = members.begin(); it != members.end(); ++it)
        sendMessage(message, (*it)->getSocketFd());
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
            sendChannelListEntry(client, it->second);
    }
    else
    {
        Channel* channel = findChannel(params[0]);
        if (channel != NULL)
            sendChannelListEntry(client, *channel);
    }

    sendMessage(std::string(SERVER_NAME) + " 323 " + nickname +
                " :End of /LIST", clientSocket);
}

void Server::sendChannelListEntry(User* client, const Channel& channel)
{
    sendMessage(std::string(SERVER_NAME) + " 322 " + client->getNickName() + " " +
                channel.getName() + " " +
                intToString(static_cast<int>(channel.getMembers().size())) +
                " :" + channel.getTopic(), client->getSocketFd());
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
            sendChannelNames(client, &it->second);
        return;
    }

    Channel* channel = findChannel(params[0]);
    if (channel != NULL)
        sendChannelNames(client, channel);
    else
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
            if (!operatorsOnly || channel->isOperator(*it))
                sendWhoReply(client, target, *it, channel);
        }
    }
    else if (target == "*")
        sendWhoForAll(client, operatorsOnly);
    else
    {
        ClientRegistry::iterator it = findUserByNickname(target);
        if (it != _connectedClients.end() && it->second.isRegistered() &&
            (!operatorsOnly || it->second.isOperator()))
        {
            User& subject = it->second;
            sendWhoReply(client, "*", &subject, NULL);
        }
    }

    sendMessage(response + " 315 " + nickname + " " + target +
                " :End of /WHO list", clientSocket);
}

void Server::sendWhoForAll(User* requester, bool operatorsOnly)
{
    for (ClientRegistry::iterator it = _connectedClients.begin();
         it != _connectedClients.end(); ++it)
    {
        User& subject = it->second;
        if (!subject.isRegistered() || (operatorsOnly && !subject.isOperator()))
            continue;
        sendWhoReply(requester, "*", &subject, NULL);
    }
}

void Server::sendWhoReply(User* requester, const std::string& requestedChannel,
                          User* subject, const Channel* channel)
{
    std::string channelName = requestedChannel;
    bool isOperator = channel != NULL && channel->isOperator(subject);
    if (channel == NULL && !subject->getChannels().empty())
    {
        const std::map<std::string, Channel*>& channels = subject->getChannels();
        channelName = channels.begin()->first;
        isOperator = channels.begin()->second != NULL &&
                     channels.begin()->second->isOperator(subject);
    }
    const std::string flags = isOperator ? "H@" : "H";
    sendMessage(std::string(SERVER_NAME) + " 352 " + requester->getNickName() +
                " " + channelName + " " + subject->getName() + " " +
                subject->getHostMask() + " " + SERVER_NAME + " " +
                subject->getNickName() + " " + flags + " :0 " +
                subject->getFullName(), requester->getSocketFd());
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

    processInvite(client, params[0], params[1]);
}

void Server::processInvite(User* inviter, const std::string& invitedNickname,
                           const std::string& channelName)
{
    const int clientSocket = inviter->getSocketFd();
    const std::string& nickname = inviter->getNickName();
    const std::string response = SERVER_NAME;
    ClientRegistry::iterator invitee = findUserByNickname(invitedNickname);
    if (invitee == _connectedClients.end() || !invitee->second.isRegistered())
    {
        sendMessage(response + " 401 " + nickname + " " + invitedNickname +
                    " :No such nick", clientSocket);
        return;
    }
    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        sendMessage(response + " 403 " + nickname + " " + channelName +
                    " :No such channel", clientSocket);
        return;
    }
    if (!channel->isMember(inviter))
    {
        sendMessage(response + " 442 " + nickname + " " + channelName +
                    " :You're not on that channel", clientSocket);
        return;
    }
    if (channel->isMember(&invitee->second))
    {
        sendMessage(response + " 443 " + nickname + " " + invitedNickname + " " +
                    channelName + " :is already on channel", clientSocket);
        return;
    }
    if (channel->hasMode('i') && !channel->isOperator(inviter))
    {
        sendMessage(response + " 482 " + nickname + " " + channelName +
                    " :You're not a channel operator", clientSocket);
        return;
    }
    channel->addInvitation(invitedNickname);
    sendMessage(response + " 341 " + nickname + " " + invitedNickname + " " +
                channelName, clientSocket);
    sendMessage(inviter->getPrefix() + " INVITE " + invitedNickname + " :" +
                channelName, invitee->second.getSocketFd());
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

    processKick(client, params[0], params[1], req.getInfo());
}

void Server::processKick(User* kicker, const std::string& channelName,
                         const std::string& nickname, const std::string& reason)
{
    const int clientSocket = kicker->getSocketFd();
    const std::string& kickerNickname = kicker->getNickName();
    const std::string response = SERVER_NAME;
    Channel* channel = findChannel(channelName);
    if (channel == NULL)
    {
        sendMessage(response + " 403 " + kickerNickname + " " + channelName +
                    " :No such channel", clientSocket);
        return;
    }
    if (!channel->isMember(kicker))
    {
        sendMessage(response + " 442 " + kickerNickname + " " + channelName +
                    " :You're not on that channel", clientSocket);
        return;
    }
    if (!channel->isOperator(kicker))
    {
        sendMessage(response + " 482 " + kickerNickname + " " + channelName +
                    " :You're not a channel operator", clientSocket);
        return;
    }

    User* target = channel->findMember(nickname);
    if (target == NULL)
    {
        sendMessage(response + " 441 " + kickerNickname + " " + nickname + " " +
                    channelName + " :They aren't on that channel", clientSocket);
        return;
    }

    const std::string kickReason = reason.empty() ? kickerNickname : reason;
    const std::string kickMessage = kicker->getPrefix() + " KICK " + channelName +
                                    " " + nickname + " :" + kickReason;
    broadcast(kickMessage, NULL, *channel);

    removeUserFromChannel(channel, target);
}


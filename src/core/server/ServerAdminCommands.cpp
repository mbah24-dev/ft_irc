/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerAdminCommands.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:23:21 by zcherif           #+#    #+#             */
/*   Updated: 2026/10/06 14:31:19 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void Server::handleModeCommand(const Request& req)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    Channel* channel = NULL;
    if (!prepareModeCommand(req, channel))
        return;

    std::vector<std::string> modeParams;
    for (Request::ParamList::const_iterator it = params.begin() + 2;
         it != params.end(); ++it)
        modeParams.push_back(*it);
    if (!req.getInfo().empty())
        modeParams.push_back(req.getInfo());

    const std::string& modeString = params[1];
    const std::string& channelName = params[0];
    if (!validateModeArguments(client, channel, channelName, modeString, modeParams))
        return;
    std::string changedModes;
    std::string changedParams;
    applyModeChanges(channel, modeString, modeParams,
                     changedModes, changedParams);
    if (changedModes.empty())
        return;
    std::string message = client->getPrefix() + " MODE " + channelName +
                          " " + changedModes;
    if (!changedParams.empty())
        message += " " + changedParams;
    broadcast(message, NULL, *channel);
}

bool Server::prepareModeCommand(const Request& req, Channel*& channel)
{
    User* client = req.getUser();
    const Request::ParamList& params = req.getParams();
    const int clientSocket = client->getSocketFd();
    const std::string& nickname = client->getNickName();
    const std::string response = SERVER_NAME;
    if (!client->isRegistered())
    {
        sendMessage(response + " 451 " + nickname + " :You have not registered", clientSocket);
        return (false);
    }
    if (params.empty())
    {
        sendMessage(response + " 461 " + nickname + " MODE :Not enough parameters", clientSocket);
        return (false);
    }
    channel = findChannel(params[0]);
    if (channel == NULL)
    {
        sendChannelError(client, 403, params[0], "No such channel");
        return (false);
    }
    if (params.size() == 1 && req.getInfo().empty())
    {
        std::string reply = response + " 324 " + nickname + " " + params[0] +
                            " " + channel->getModeString();
        if (channel->hasMode('l'))
            reply += " " + intToString(channel->getLimit());
        sendMessage(reply, clientSocket);
        return (false);
    }
    if (params.size() < 2)
    {
        sendMessage(response + " 461 " + nickname + " MODE :Not enough parameters", clientSocket);
        return (false);
    }
    if (!channel->isOperator(client))
    {
        sendChannelError(client, 482, params[0], "You're not a channel operator");
        return (false);
    }
    return (true);
}

bool Server::validateModeArguments(User* client, Channel* channel,
                                   const std::string& channelName,
                                   const std::string& modes,
                                   const std::vector<std::string>& params)
{
    char sign = '\0';
    std::size_t paramIndex = 0;
    for (std::string::size_type i = 0; i < modes.size(); ++i)
    {
        if (modes[i] == '+' || modes[i] == '-')
        {
            sign = modes[i];
            continue;
        }
        if (sign == '\0' || std::string(CHANNEL_MODES).find(modes[i]) == std::string::npos)
        {
            sendMessage(std::string(SERVER_NAME) + " 472 " + client->getNickName() +
                        " " + modes[i] + " :is unknown mode char to me for " + channelName,
                        client->getSocketFd());
            return (false);
        }
        const char mode = modes[i];
        const bool needsParam = mode == 'o' || (mode == 'k' && sign == '+') ||
                                (mode == 'l' && sign == '+');
        if (needsParam && paramIndex >= params.size())
        {
            sendMessage(std::string(SERVER_NAME) + " 461 " + client->getNickName() +
                        " MODE :Not enough parameters", client->getSocketFd());
            return (false);
        }
        if (needsParam)
        {
            const std::string& value = params[paramIndex];
            if (!validateModeParameter(client, channel, channelName, mode, value))
                return (false);
            ++paramIndex;
        }
    }
    if (sign != '\0')
        return (true);
    sendMessage(std::string(SERVER_NAME) + " 461 " + client->getNickName() +
                " MODE :Not enough parameters", client->getSocketFd());
    return (false);
}

bool Server::validateModeParameter(User* client, Channel* channel,
                                   const std::string& channelName, char mode,
                                   const std::string& value)
{
    if (mode == 'o' && channel->findMember(value) == NULL)
    {
        sendMessage(std::string(SERVER_NAME) + " 441 " + client->getNickName() +
                    " " + value + " " + channelName +
                    " :They aren't on that channel", client->getSocketFd());
        return (false);
    }
    if (mode == 'l')
    {
        char* endPointer;
        const long limit = std::strtol(value.c_str(), &endPointer, 10);
        if (value.empty() || *endPointer != '\0' || limit <= 0 || limit > INT_MAX)
        {
            sendMessage(std::string(SERVER_NAME) + " 461 " + client->getNickName() +
                        " MODE :Invalid channel limit", client->getSocketFd());
            return (false);
        }
    }
    return (true);
}

bool Server::applyChannelMode(Channel* channel, char mode, char sign,
                              const std::string& value)
{
    if (mode == 'o')
        return (applyOperatorMode(channel, sign, value));
    if (mode == 'k')
        return (applyKeyMode(channel, sign, value));
    if (mode == 'l')
        return (applyLimitMode(channel, sign, value));
    if (sign == '+' && !channel->hasMode(mode))
    {
        channel->editMode(mode, '+');
        return (true);
    }
    if (sign == '-' && channel->hasMode(mode))
    {
        channel->editMode(mode, '-');
        return (true);
    }
    return (false);
}

bool Server::applyOperatorMode(Channel* channel, char sign,
                               const std::string& nickname)
{
    User* target = channel->findMember(nickname);
    if (sign == '+' && !channel->isOperator(target))
    {
        channel->addOperator(target);
        return (true);
    }
    if (sign == '-' && channel->isOperator(target))
    {
        channel->removeOperator(target);
        return (true);
    }
    return (false);
}

bool Server::applyKeyMode(Channel* channel, char sign, const std::string& key)
{
    if (sign == '+' && (!channel->hasMode('k') || channel->getPassword() != key))
    {
        channel->setPassword(key);
        channel->editMode('k', '+');
        return (true);
    }
    if (sign == '-' && channel->hasMode('k'))
    {
        channel->setPassword("");
        channel->editMode('k', '-');
        return (true);
    }
    return (false);
}

bool Server::applyLimitMode(Channel* channel, char sign, const std::string& value)
{
    if (sign == '+' && (!channel->hasMode('l') ||
        channel->getLimit() != std::atoi(value.c_str())))
    {
        channel->setLimit(std::atoi(value.c_str()));
        channel->editMode('l', '+');
        return (true);
    }
    if (sign == '-' && channel->hasMode('l'))
    {
        channel->setLimit(-1);
        channel->editMode('l', '-');
        return (true);
    }
    return (false);
}

void Server::applyModeChanges(Channel* channel, const std::string& modes,
                              const std::vector<std::string>& params,
                              std::string& changedModes,
                              std::string& changedParams)
{
    char sign = '\0';
    std::size_t paramIndex = 0;
    for (std::string::size_type i = 0; i < modes.size(); ++i)
    {
        if (modes[i] == '+' || modes[i] == '-')
        {
            sign = modes[i];
            continue;
        }
        const char mode = modes[i];
        const bool needsParam = mode == 'o' || (mode == 'k' && sign == '+') ||
                                (mode == 'l' && sign == '+');
        const std::string value = needsParam ? params[paramIndex++] : "";
        if (!applyChannelMode(channel, mode, sign, value))
            continue;
        if (changedModes.empty() || changedModes[changedModes.size() - 1] != sign)
            changedModes += sign;
        changedModes += mode;
        if (needsParam)
        {
            if (!changedParams.empty())
                changedParams += " ";
            changedParams += value;
        }
    }
}

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

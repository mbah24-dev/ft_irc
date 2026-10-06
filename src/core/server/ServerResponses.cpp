/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerResponses.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 15:23:53 by mbah              #+#    #+#             */
/*   Updated: 2026/10/01 11:38:42 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <sstream> 

void Server::sendFormattedResponse(const Request& clientRequest, int responseCode)
{
    User* client = clientRequest.getUser();
    int clientSocket = client->getSocketFd();

    std::string responseMessage = buildResponseMessage(clientRequest, responseCode);

    if (!responseMessage.empty())
        sendMessage(responseMessage, clientSocket);
}

/**
 * @brief Construit le message de réponse en fonction du code
 * 
 * @param clientRequest La requête du client
 * @param responseCode Le code de réponse
 * @return std::string Le message formaté
 */
std::string Server::buildResponseMessage(const Request& clientRequest, int responseCode)
{
    User* client = clientRequest.getUser();
    const std::string& nickname = client->getNickName();
    const std::string& channelName = clientRequest.getChannelName();
    const std::string& info = clientRequest.getInfo();
    const std::string& command = clientRequest.getCommand();
    std::string prefix = ":" + std::string(SERVER_NAME);
    std::string codeStr = intToString(responseCode);
    std::string response = buildModeResponse(clientRequest, responseCode);
    if (!response.empty()) return (response);
    response = buildChannelResponse(clientRequest, responseCode);
    if (!response.empty()) return (response);
    response = buildUserResponse(clientRequest, responseCode);
    if (!response.empty()) return (response);
    switch (responseCode)
    {
        case ERR_NEED_MORE_PARAMS:
            return (prefix + " " + codeStr + " " + nickname + " " + command + " :Not enough parameters");
        case RPL_MODE_CHANGED:
            return (prefix + " MODE " + channelName + " " + info);
        case RPL_CHANNEL_LEFT:
            return (":" + client->getNickName() + "!" + client->getName() + "@" + SERVER_NAME + " PART " + channelName + " :" + info);
        case RPL_CHANNEL_JOINED:
            return (":" + client->getNickName() + "!" + client->getName() + "@" + SERVER_NAME + " JOIN " + channelName);
        default:
            return (prefix + " " + nickname + " :An error occurred");
    }
}

std::string Server::buildModeResponse(const Request& req, int code) const
{
    const std::string prefix = ":" + std::string(SERVER_NAME) + " " + intToString(code) +
                               " " + req.getUser()->getNickName() + " ";
    if (code == ERR_UNKNOWN_MODE)
        return (prefix + req.getChannelName() + " :Unknown mode");
    if (code == ERR_BAD_CHAN_NAME)
        return (prefix + req.getChannelName() + " :Bad channel name");
    return ("");
}

std::string Server::buildChannelResponse(const Request& req, int code) const
{
    const std::string prefix = ":" + std::string(SERVER_NAME) + " " + intToString(code) +
                               " " + req.getUser()->getNickName() + " ";
    const std::string& channel = req.getChannelName();
    const std::string& info = req.getInfo();
    switch (code)
    {
        case RPL_LIST_START: return (prefix + "Channel :Users  Name");
        case RPL_LIST_ENTRY: return (prefix + info);
        case RPL_LIST_END: return (prefix + "End of /LIST");
        case RPL_TOPIC: return (prefix + channel + " :" + info);
        case RPL_NO_TOPIC: return (prefix + channel + " :No topic is set");
        case RPL_WHO_REPLY: return (prefix + channel + " " + info);
        case RPL_END_OF_WHO: return (prefix + channel + " :End of /WHO list");
        case ERR_NOT_ON_CHANNEL: return (prefix + channel + " :You're not on that channel");
        case ERR_USER_NOT_IN_CHANNEL: return (prefix + info + " " + channel + " :They aren't on the channel");
        case ERR_NO_SUCH_CHANNEL: return (prefix + channel + " :No such channel");
        case ERR_CHANNEL_IS_FULL: return (prefix + channel + " :Cannot join channel (+l)");
        case ERR_BAD_CHANNEL_KEY: return (prefix + channel + " :Password is incorrect");
        case ERR_BANNED_FROM_CHAN: return (prefix + channel + " :Permission denied. Have you provided the password?");
        case ERR_INVITE_ONLY_CHAN: return (prefix + channel + " :Cannot join channel (+i)");
        case ERR_CHAN_OP_PRIVS_NEEDED: return (prefix + channel + " :You're not a channel operator");
        case ERR_NOT_AN_OPERATOR: return (prefix + channel + " :" + info + " is not an operator");
        case ERR_ALREADY_AN_OPERATOR: return (prefix + channel + " :" + info + " is already an operator");
        case ERR_CHANNEL_ALREADY_JOINED: return (prefix + channel + " :" + info + " :Already on channel");
        default: return ("");
    }
}

std::string Server::buildUserResponse(const Request& req, int code) const
{
    const std::string prefix = ":" + std::string(SERVER_NAME) + " " + intToString(code) +
                               " " + req.getUser()->getNickName() + " ";
    if (code == ERR_NO_SUCH_NICK)
        return (prefix + req.getInfo() + " :No such nick");
    if (code == ERR_NOT_REGISTERED)
        return (prefix + ":You have not registered");
    return ("");
}

/**
 * @brief Convertit un entier en chaîne de caractères
 */
std::string Server::intToString(int number) const
{
    std::ostringstream converter;
    converter << number;
    return (converter.str());
}
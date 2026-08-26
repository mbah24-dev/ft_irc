/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerResponses.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 15:23:53 by mbah              #+#    #+#             */
/*   Updated: 2026/08/26 16:42:45 by mbah             ###   ########.fr       */
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

    switch (responseCode)
    {
        //ERREURS DE MODE 
        case ERR_UNKNOWN_MODE:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :Unknown mode");

        case ERR_BAD_CHAN_NAME:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :Bad channel name");

        //LISTE DES CANAUX
        case RPL_LIST_START:
            return (prefix + " " + codeStr + " " + nickname + " Channel :Users  Name");

        case RPL_LIST_ENTRY:
            return (prefix + " " + codeStr + " " + nickname + " " + info);

        case RPL_LIST_END:
            return (prefix + " " + codeStr + " " + nickname + " End of /LIST");

        //TOPIC 
        case RPL_TOPIC:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :" + info);

        case RPL_NO_TOPIC:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :No topic is set");

        //WHO
        case RPL_WHO_REPLY:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " " + info);

        case RPL_END_OF_WHO:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :End of /WHO list");

        //ERREURS UTILISATEUR
        case ERR_NO_SUCH_NICK:
            return (prefix + " " + codeStr + " " + nickname + " " + info + " :No such nick");

        case ERR_NOT_ON_CHANNEL:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :You're not on that channel");

        case ERR_USER_NOT_IN_CHANNEL:
            return (prefix + " " + codeStr + " " + nickname + " " + info + " " + channelName + " :They aren't on the channel");

        case ERR_NOT_REGISTERED:
            return (prefix + " " + codeStr + " " + nickname + " :You have not registered");

        //ERREURS DE CANAL
        case ERR_NO_SUCH_CHANNEL:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :No such channel");

        case ERR_CHANNEL_IS_FULL:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :Cannot join channel (+l)");

        case ERR_BAD_CHANNEL_KEY:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :Password is incorrect");

        case ERR_BANNED_FROM_CHAN:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :Permission denied. Have you provided the password?");

        case ERR_INVITE_ONLY_CHAN:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :Cannot join channel (+i)");

        //ERREURS D'OPÉRATEUR
        case ERR_CHAN_OP_PRIVS_NEEDED:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :You're not a channel operator");

        case ERR_NOT_AN_OPERATOR:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :" + info + " is not an operator");

        case ERR_ALREADY_AN_OPERATOR:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :" + info + " is already an operator");

        //ERREURS DE PARAMÈTRES 
        case ERR_NEED_MORE_PARAMS:
            return (prefix + " " + codeStr + " " + nickname + " " + command + " :Not enough parameters");

        //MESSAGES PERSONNALISÉS
        case RPL_MODE_CHANGED:
            return (prefix + " MODE " + channelName + " " + info);

        case RPL_CHANNEL_LEFT:
            return (":" + client->getNickName() + "!" + client->getName() + "@" + SERVER_NAME + " PART " + channelName + " :" + info);

        case RPL_CHANNEL_JOINED:
            return (":" + client->getNickName() + "!" + client->getName() + "@" + SERVER_NAME + " JOIN " + channelName);

        //ERREUR CANAL DÉJÀ REJOINT
        case ERR_CHANNEL_ALREADY_JOINED:
            return (prefix + " " + codeStr + " " + nickname + " " + channelName + " :" + info + " :Already on channel");

        //DÉFAUT 
        default:
            return (prefix + " " + nickname + " :An error occurred");
    }
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
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 16:35:23 by mbah              #+#    #+#             */
/*   Updated: 2026/08/27 17:32:20 by mbah             ###   ########.fr       */
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
    client->setFullName(params[3]);

    checkRegistrationComplete(client);
}

// ============================================================================
//                       COMMANDES DE COMMUNICATION
// ============================================================================

void Server::handlePingCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter PING
}

void Server::handlePongCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter PONG
}

void Server::handlePrivmsgCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter PRIVMSG
}

void Server::handleNoticeCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter NOTICE
}

// ============================================================================
//                       COMMANDES DE CANAUX
// ============================================================================

void Server::handleJoinCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter JOIN
}

void Server::handlePartCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter PART
}

void Server::handleTopicCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter TOPIC
}

void Server::handleListCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter LIST
}

void Server::handleNamesCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter NAMES
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
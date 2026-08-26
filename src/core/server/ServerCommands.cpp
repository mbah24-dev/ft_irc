/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 16:35:23 by mbah              #+#    #+#             */
/*   Updated: 2026/08/26 16:35:34 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

// ============================================================================
//                       COMMANDES D'AUTHENTIFICATION
// ============================================================================

void Server::handleCapCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter CAP
}

void Server::handlePassCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter PASS
}

void Server::handleNickCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter NICK
}

void Server::handleUserCommand(const Request& req)
{
    (void)req;
    // TODO: Implémenter USER
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
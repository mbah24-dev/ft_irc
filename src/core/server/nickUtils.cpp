/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   nickUtils.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 12:09:52 by mbah              #+#    #+#             */
/*   Updated: 2026/10/01 10:54:35 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

/**
 * @brief Vérifie si un pseudo contient des caractères interdits
 */
bool Server::containsForbiddenChars(const std::string& nickname) const
{
    if (nickname.empty() || nickname.length() > 9)
        return (true);
    const std::string firstChars =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]\\`_^{}|";
    const std::string otherChars = firstChars + "0123456789-";
    if (firstChars.find(nickname[0]) == std::string::npos)
        return (true);
    for (std::string::size_type i = 1; i < nickname.length(); ++i)
        if (otherChars.find(nickname[i]) == std::string::npos)
            return (true);
    return (false);
}

/**
 * @brief Vérifie si l'utilisateur est complètement enregistré
 * 
 * Un utilisateur est enregistré quand :
 * - Le mot de passe a été fourni
 * - Le pseudo n'est pas vide
 * - Le nom d'utilisateur n'est pas vide
 * - Il n'est pas déjà enregistré
*/
void Server::checkRegistrationComplete(User* user)
{
    if (user == NULL)
        return;

    int clientSocket = user->getSocketFd();
    const std::string& nickname = user->getNickName();

    if (user->hasPasswordProvided() &&
        !nickname.empty() &&
        !user->getName().empty() &&
        !user->isRegistered())
    {
        sendMessage(":" + std::string(SERVER_NAME) + " 001 " + nickname +
                    " :Welcome to the " + std::string(SERVER_NAME) + " IRC server",
                    clientSocket);

        sendMessage(":" + std::string(SERVER_NAME) + " 002 " + nickname +
                    " :Your host is " + std::string(SERVER_NAME) +
                    ", running version " + std::string(SERVER_VERSION),
                    clientSocket);

        sendMessage(":" + std::string(SERVER_NAME) + " 003 " + nickname +
                    " :Server created on " + _startupTimestamp,
                    clientSocket);

        sendMessage(":" + std::string(SERVER_NAME) + " 004 " + nickname +
                    " :" + std::string(SERVER_NAME) + " " +
                    std::string(SERVER_VERSION),
                    clientSocket);

        user->setRegistered(true);
        std::cout << "User " << nickname << " fully registered." << std::endl;
    }
}

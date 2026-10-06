/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelModes.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/26 14:05:03 by mbah              #+#    #+#             */
/*   Updated: 2026/10/06 14:31:19 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"
#include "../server/Server.hpp"


bool Channel::handleModeChange(char mode, char sign, std::string param, Request request)
{
    if (mode == 'i' || mode == 't')
        return (true);

    if (mode == 'o')
        return (handleOperatorMode(sign, param, request));

    if (mode == 'k')
        return (handleKeyMode(sign, param));

    if (mode == 'l')
        return (handleLimitMode(sign, param, request));

    return (false);
}

//GESTION DU MODE OPERATEUR (o)

/**
 * @brief Gère le mode operator (+o / -o)
 */
bool Channel::handleOperatorMode(char sign, const std::string& param, Request request)
{
    if (param.empty())
    {
        _server->sendFormattedResponse(request, ERR_NEED_MORE_PARAMS);
        return (false);
    }

    User* targetUser = findMember(param);
    User* requester = request.getUser();

    if (requester == targetUser && sign == '-')
        return (false);

    if (targetUser == NULL)
    {
        _server->sendFormattedResponse(request, ERR_USER_NOT_IN_CHANNEL);
        return (false);
    }

    if (sign == '+')
    {
        if (isOperator(targetUser))
        {
            _server->sendFormattedResponse(request, ERR_ALREADY_AN_OPERATOR);
            return (false);
        }
        addOperator(targetUser);
        return (true);
    }

    if (sign == '-')
    {
        if (!isOperator(targetUser))
        {
            _server->sendFormattedResponse(request, ERR_NOT_AN_OPERATOR);
            return (false);
        }
        removeOperator(targetUser);
        return (true);
    }

    return (false);
}

//GESTION DU MODE KEY (k)

/**
 * @brief Gère le mode key / password (+k / -k)
 */
bool Channel::handleKeyMode(char sign, const std::string& param)
{
    if (sign == '-')
    {
        setPassword("");
        return (true);
    }

    setPassword(param);
    return (true);
}

//GESTION DU MODE LIMIT (l)

/**
 * @brief Gère le mode user limit (+l / -l)
 */
bool Channel::handleLimitMode(char sign, const std::string& param, Request request)
{
    if (sign == '-')
    {
        setLimit(-1);
        return (true);
    }

    char* endPointer;
    long newLimit = strtol(param.c_str(), &endPointer, 10);

    if (*endPointer != '\0' || newLimit < 0)
    {
        _server->sendFormattedResponse(request, ERR_NEED_MORE_PARAMS);
        return (false);
    }

    if (newLimit >= static_cast<long>(_members.size()))
    {
        _limit = static_cast<int>(newLimit);
        return (true);
    }

    return (false);
}

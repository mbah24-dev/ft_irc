/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 14:29:55 by mbah              #+#    #+#             */
/*   Updated: 2026/10/01 10:54:36 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"
#include "../server/Server.hpp"
#include <cctype>

static bool channelNamesEqual(const std::string& left, const std::string& right)
{
    if (left.length() != right.length())
        return (false);
    for (std::string::size_type i = 0; i < left.length(); ++i)
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i])))
            return (false);
    return (true);
}

Channel::Channel(void)
    : _name(""),
      _topic(""),
      _modes(0),
      _password(""),
      _members(),
      _operators(),
    _invitedNicknames(),
      _limit(-1)
{
}

Channel::Channel(const std::string& name, const std::string& password, Server* server)
    : _name(name),
      _topic(""),
      _modes(0),
      _password(password),
      _members(),
      _operators(),
    _invitedNicknames(),
      _limit(-1),
	  _server(server)
{
}

Channel::Channel(const Channel& other)
    : _name(other._name),
      _topic(other._topic),
      _modes(other._modes),
      _password(other._password),
      _members(other._members),
      _operators(other._operators),
            _invitedNicknames(other._invitedNicknames),
    _limit(other._limit),
    _server(other._server)
{
}

Channel::~Channel(void)
{
}

Channel& Channel::operator=(const Channel& other)
{
    if (this != &other)
    {
        _name = other._name;
        _topic = other._topic;
        _modes = other._modes;
        _password = other._password;
        _members = other._members;
        _operators = other._operators;
        _invitedNicknames = other._invitedNicknames;
        _limit = other._limit;
        _server = other._server;
    }
    return (*this);
}

const std::string& Channel::getName(void) const
{
    return (_name);
}

const std::string& Channel::getTopic(void) const
{
    return (_topic);
}

const std::string& Channel::getPassword(void) const
{
    return (_password);
}

int Channel::getLimit(void) const
{
    return (_limit);
}

short Channel::getModes(void) const
{
    return (_modes);
}

//Surcharge Version const - lecture seule
const std::list<User*>& Channel::getMembers(void) const
{
    return (_members);
}

//Version non-const - modification possible
std::list<User*>& Channel::getMembers(int)
{
    return (_members);
}

const std::list<User*>& Channel::getOperators(void) const
{
    return (_operators);
}

std::list<User*>& Channel::getOperators(int)
{
    return (_operators);
}

bool Channel::isInvited(const std::string& nickname) const
{
    for (std::vector<std::string>::const_iterator it = _invitedNicknames.begin();
         it != _invitedNicknames.end(); ++it)
        if (channelNamesEqual(*it, nickname))
            return (true);
    return (false);
}

void Channel::addInvitation(const std::string& nickname)
{
    if (!isInvited(nickname))
        _invitedNicknames.push_back(nickname);
}

void Channel::removeInvitation(const std::string& nickname)
{
    for (std::vector<std::string>::iterator it = _invitedNicknames.begin();
         it != _invitedNicknames.end(); )
    {
        if (channelNamesEqual(*it, nickname))
            it = _invitedNicknames.erase(it);
        else
            ++it;
    }
}

void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

void Channel::setPassword(const std::string& password)
{
    _password = password;
}

void Channel::setLimit(int limit)
{
    _limit = limit;
}

void Channel::addMember(User* user)
{
    if (!isMember(user))
    {
        _members.push_back(user);
        // Le premier membre devient automatiquement opérateur
        if (_members.size() == 1)
            addOperator(user);
    }
}

int Channel::removeMember(User* user)
{
    _members.remove(user);
    removeOperator(user);
    return (_members.empty());
}

bool Channel::isMember(User* user) const
{
    return (std::find(_members.begin(), _members.end(), user) != _members.end());
}

User* Channel::findMember(const std::string& nickname)
{
    std::list<User*>::iterator it;
    
    for (it = _members.begin(); it != _members.end(); ++it)
    {
        if (channelNamesEqual((*it)->getNickName(), nickname))
            return (*it);
    }
    return (NULL);
}

void Channel::addOperator(User* user)
{
    if (!isOperator(user))
        _operators.push_back(user);
}

int Channel::removeOperator(User* operatorUser)
{
    //RETIRE L'OPÉRATEUR
    _operators.remove(operatorUser);

    if (_operators.empty() && !_members.empty())
    {
        //Le premier membre devient opérateur
        User* newOperator = *_members.begin();
        addOperator(newOperator);

        //DIFFUSE LE CHANGEMENT
        std::string modeMessage = ":" + std::string(SERVER_NAME)
                                 + " MODE " + _name
                                 + " +o " + newOperator->getNickName();

        //Envoie à tous les membres du canal via le serveur
        if (_server != NULL)
        {
            const std::list<User*>& members = getMembers(0);
            for (std::list<User*>::const_iterator memberIt = members.begin();
                 memberIt != members.end(); ++memberIt)
            {
                _server->sendMessage(modeMessage, (*memberIt)->getSocketFd());
            }
        }
    }

    return (_operators.empty() ? 1 : 0);
}

bool Channel::isOperator(User* user) const
{
    return (std::find(_operators.begin(), _operators.end(), user) != _operators.end());
}

static short modeToBitmask(char mode)
{
    if (mode == 'i')
        return (MODE_INVITE_ONLY);
    if (mode == 't')
        return (MODE_TOPIC_RESTRICTED);
    if (mode == 'k')
        return (MODE_PASSWORD);
    if (mode == 'o')
        return (MODE_OPERATOR);
    if (mode == 'l')
        return (MODE_USER_LIMIT);
    return (0);
}

void Channel::enableMode(char mode)
{
    _modes |= modeToBitmask(mode);
}

void Channel::disableMode(char mode)
{
    _modes &= ~modeToBitmask(mode);
}
//&= ~ (AND avec NOT)
//Met le bit correspondant à 0 pour désactiver un mode sans toucher aux autres
//|= (OR)
//Met le bit correspondant à 1 pour activer un mode sans toucher aux autres
void Channel::editMode(char mode, char sign)
{
    short bitmask = modeToBitmask(mode);
    
    if (sign == '+')
        _modes |= bitmask;
    else if (sign == '-')
        _modes &= ~bitmask;
}

bool Channel::hasMode(char mode) const
{
    return (_modes & modeToBitmask(mode));
}

bool Channel::isModeValid(char mode) const
{
    return (std::string(CHANNEL_MODES).find_first_of(mode) != std::string::npos);
}

bool Channel::isModeWithParam(char mode) const
{
    return (mode == 'o' || mode == 'k' || mode == 'l');
}

std::string Channel::getModeString(void) const
{
    std::string modes = "+";
    
    if (_modes & MODE_INVITE_ONLY)
        modes += "i";
    if (_modes & MODE_TOPIC_RESTRICTED)
        modes += "t";
    if (_modes & MODE_PASSWORD)
        modes += "k";
    if (_modes & MODE_USER_LIMIT)
        modes += "l";
    
    return (modes);
}

bool Channel::isValidChannelName(const std::string& name)
{
    size_t i;
    
    if (name.empty() || name[0] != '#')
        return (false);
    
    for (i = 1; i < name.length(); ++i)
    {
        if (name[i] == ' ' || name[i] == ',' || name[i] == 7)
            return (false);
    }
    
    return (true);
}

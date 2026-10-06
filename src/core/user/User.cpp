/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:52 by mbah              #+#    #+#             */
/*   Updated: 2026/10/01 10:56:04 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "User.hpp"

User::User(void)
    : _receiveBuffer(""),
    _sendBuffer(""),
      _socketFd(-1),
      _registered(false),
      _password_provided(false),
      _operator_status(false),
      _name(""),
      _hostmask(""),
      _fullname(""),
      _nickname("")
{
}

User::User(const User & source)
    : _receiveBuffer(source._receiveBuffer),
    _sendBuffer(source._sendBuffer),
      _socketFd(source._socketFd),
      _registered(source._registered),
      _password_provided(source._password_provided),
      _operator_status(source._operator_status),
      _name(source._name),
      _hostmask(source._hostmask),
      _fullname(source._fullname),
      _nickname(source._nickname)
{
    //Copie de la map des canaux
    //On ne copie pas les pointeurs vers les canaux
    //car les canaux sont gérés par le serveur
    //On copie juste la structure de la map
    for (ChannelMap::const_iterator it = source._channelMap.begin();
         it != source._channelMap.end(); ++it)
    {
        //On copie les pointeurs (pas les objets eux-mêmes)
        this->_channelMap[it->first] = it->second;
    }
}

User::User(int socketFd, const std::string& hostMask)
    : _receiveBuffer(""),
    _sendBuffer(""),
      _socketFd(socketFd),
      _registered(false),
      _password_provided(false),
      _operator_status(false),
      _name(""),
      _hostmask(hostMask),
      _fullname(""),
      _nickname("")
{
}

User::~User(void)
{
    //Les canaux ne sont pas supprimés ici
    //Le serveur est responsable de la gestion des canaux
    //L'utilisateur doit juste se retirer des canaux
}

User& User::operator=(const User& source)
{
    if (this != &source)
    {
        //Copie des attributs simples
        this->_receiveBuffer = source._receiveBuffer;
        this->_sendBuffer = source._sendBuffer;
        this->_socketFd = source._socketFd;
        this->_registered = source._registered;
        this->_password_provided = source._password_provided;
        this->_operator_status = source._operator_status;
        this->_name = source._name;
        this->_hostmask = source._hostmask;
        this->_fullname = source._fullname;
        this->_nickname = source._nickname;
        
        //Copie de la map des canaux
        this->_channelMap.clear();
        for (ChannelMap::const_iterator it = source._channelMap.begin();
             it != source._channelMap.end(); ++it)
        {
            this->_channelMap[it->first] = it->second;
        }
    }
    return (*this);
}

const std::string& User::getName(void) const
{
    return (this->_name);
}

const std::string& User::getHostMask(void) const
{
    return (this->_hostmask);
}

const std::string& User::getFullName(void) const
{
    return (this->_fullname);
}

const std::string& User::getNickName(void) const
{
    return (this->_nickname);
}

/**
 * @brief Retourne le préfixe IRC complet
 * 
 * Format : ":nickname!username@hostmask"
*/
std::string User::getPrefix(void) const
{
    std::string prefix;
    
    //Réservation de mémoire pour éviter les réallocations
    prefix.reserve(_nickname.length() + _name.length() + _hostmask.length() + 3);
    
    prefix = ":";
    prefix += _nickname;
    prefix += "!";
    prefix += _name;
    prefix += "@";
    prefix += _hostmask;
    
    return (prefix);
}

int User::getSocketFd(void) const
{
    return (this->_socketFd);
}

bool User::isRegistered(void) const
{
    return (this->_registered);
}

bool User::hasPasswordProvided(void) const
{
    return (this->_password_provided);
}

bool User::isOperator(void) const
{
    return (this->_operator_status);
}

const User::ChannelMap& User::getChannels(void) const
{
    return (this->_channelMap);
}

void User::addChannel(const std::string& name, Channel* channel)
{
    if (channel != NULL)
        _channelMap[name] = channel;
}

void User::removeChannel(const std::string& name)
{
    _channelMap.erase(name);
}

void User::setName(const std::string& name)
{
    this->_name = name;
}

void User::setNickName(const std::string& nickname)
{
    this->_nickname = nickname;
}

void User::setFullName(const std::string& fullname)
{
    this->_fullname = fullname;
}

void User::setOperator(bool val)
{
    this->_operator_status = val;
}

void User::setRegistered(bool val)
{
    this->_registered = val;
}

void User::setPasswordProvided(bool val)
{
    this->_password_provided = val;
}

/**
 * @brief Ajoute des données au buffer de réception
*/
void User::appendToBuffer(const std::string& data)
{
    this->_receiveBuffer += data;
}

void User::appendToBuffer(const char* data)
{
    if (data)
        this->_receiveBuffer += data;
}

void User::appendToSendBuffer(const std::string& data)
{
    this->_sendBuffer += data;
}

bool operator==(const User & first, const User & second)
{
	if (first.getNickName() != second.getNickName())
		return (false);
	return (true);
}

/**
 * @brief Opérateur de flux pour afficher un utilisateur
 * 
 * Format : "<nickname> (<username>) [fd: X] on X channels"
*/
std::ostream& operator<<(std::ostream& output, const User& user)
{
    output << user.getNickName() << " (" << user.getName() << ") "
           << "[fd: " << user.getSocketFd() << "] "
           << "on " << user.getChannels().size() << " channels";
    return (output);
}

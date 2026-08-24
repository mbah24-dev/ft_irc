/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 17:55:06 by mbah              #+#    #+#             */
/*   Updated: 2026/08/24 22:13:18 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

Request::Request(void)
    : _command(""),
      _params(),
      _user(NULL),
      _channel_name(""),
      _info("")
{
}

Request::Request(const Request& source)
    : _command(source._command),
      _params(source._params),
      _user(source._user),
      _channel_name(source._channel_name),
      _info(source._info)
{
}

Request::Request(const std::string& line, User* user)
    : _command(""),
      _params(),
      _user(user),
      _channel_name(""),
      _info("")
{
    this->cmdLineparser(line);
}

Request::~Request(void)
{
}

Request& Request::operator=(const Request& source)
{
    if (this != &source)
    {
        this->_command = source._command;
        this->_params = source._params;
        this->_user = source._user;
        this->_channel_name = source._channel_name;
        this->_info = source._info;
    }
    return (*this);
}

const std::string& Request::getCommand(void) const
{
    return (this->_command);
}

const Request::ParamList& Request::getParams(void) const
{
    return (this->_params);
}

User* Request::getUser(void) const
{
    return (this->_user);
}

const std::string& Request::getChannelName(void) const
{
    return (this->_channel_name);
}

const std::string& Request::getInfo(void) const
{
    return (this->_info);
}

void Request::setUser(User* user)
{
    this->_user = user;
}

void Request::setChannelName(const std::string& channelName)
{
    this->_channel_name = channelName;
}

void Request::setInfo(const std::string& info)
{
    this->_info = info;
}

/*============================================================================*/
/*                              PARSER                                        */
/*============================================================================*/

/**
 * @brief Parse une ligne de commande IRC
 * 
 * Décompose la ligne en commande, paramètres et trailing.
 * 
 * @param input La ligne de commande brute
 */
void Request::cmdLineparser(const std::string& input)
{
    std::string line = _cleanLine(input);
    
    if (line.empty())
        return;
    
    _extractCommand(line);
    
    if (_hasParameters(line))
        _extractParameters(line);
}


/**
 * @brief Nettoie la ligne en supprimant les caractères de fin de ligne
 * 
 * @param input La ligne brute
 * @return std::string La ligne nettoyée
 */
std::string Request::_cleanLine(const std::string& input) const
{
    std::string line = input;
    
    //Supprime les retours à la ligne
    while (!line.empty() && (line[line.length() - 1] == '\r' || line[line.length() - 1] == '\n'))
        line.erase(line.length() - 1);
    
    //Supprime les espaces en début de ligne
    while (!line.empty() && line[0] == ' ')
        line.erase(0, 1);
    
    // Supprime les espaces en fin de ligne
    while (!line.empty() && line[line.length() - 1] == ' ')
        line.erase(line.length() - 1);
    
    return (line);
}

/**
 * @brief Vérifie si la ligne contient des paramètres
 * 
 * @param line La ligne à vérifier
 * @return true Si la ligne contient des paramètres
 */
bool Request::_hasParameters(const std::string& line) const
{
    return (line.find(' ') != std::string::npos);
}

/**
 * @brief Extrait la commande de la ligne
 * 
 * @param line La ligne contenant la commande
 */
void Request::_extractCommand(const std::string& line)
{
    std::string::size_type pos = line.find(' ');
    
    if (pos == std::string::npos)
        this->_command = line;
    else
        this->_command = line.substr(0, pos);
}

/**
 * @brief Extrait les paramètres de la ligne
 * 
 * @param line La ligne contenant les paramètres
 */
void Request::_extractParameters(const std::string& line)
{
    std::string::size_type pos = line.find(' ');
    std::string rest = line.substr(pos + 1);
    
    if (_hasTrailing(rest))
        _parseWithTrailing(rest);
    else
        _parseWithoutTrailing(rest);
}

/**
 * @brief Vérifie si la chaîne contient un trailing parameter
 * 
 * @param rest La partie de la ligne après la commande
 * @return true Si un ':' est présent
 */
bool Request::_hasTrailing(const std::string& rest) const
{
    return (rest.find(':') != std::string::npos);
}

/**
 * @brief Parse les paramètres et le trailing
 * 
 * @param rest La partie de la ligne après la commande
 */
void Request::_parseWithTrailing(const std::string& rest)
{
    std::string::size_type pos = rest.find(':');
    
    //Extraire les paramètres avant le ':'
    std::string beforeTrailing = rest.substr(0, pos);
    _parseParams(beforeTrailing);
    
    //Extraire le trailing (info)
    _extractTrailing(rest, pos);
}

/**
 * @brief Parse sans trailing
 * 
 * @param rest La partie de la ligne après la commande
*/
void Request::_parseWithoutTrailing(const std::string& rest)
{
    _parseParams(rest);
}

/**
 * @brief Parse une liste de paramètres séparés par des espaces
 * 
 * @param paramsStr La chaîne contenant les paramètres
 */
void Request::_parseParams(const std::string& paramsStr)
{
    if (paramsStr.empty())
        return;
    
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    
    while (start < paramsStr.length())
    {
        end = paramsStr.find(' ', start);
        if (end == std::string::npos)
            end = paramsStr.length();
        
        if (start != end)
            this->_params.push_back(paramsStr.substr(start, end - start));
        
        start = end + 1;
    }
}

/**
 * @brief Extrait le trailing parameter
 * 
 * @param rest La partie de la ligne après la commande
 * @param pos La position du ':'
 */
void Request::_extractTrailing(const std::string& rest, std::string::size_type pos)
{
    this->_info = rest.substr(pos + 1);
    
    //Supprime l'espace initial si présent
    if (!this->_info.empty() && this->_info[0] == ' ')
        this->_info.erase(0, 1);
}

void Request::debug(void) const
{
    std::cout << "===== REQUEST DEBUG =====" << std::endl;
    std::cout << "Command:   " << this->_command << std::endl;
    std::cout << "Params:    ";
    for (ParamList::const_iterator it = this->_params.begin();
         it != this->_params.end(); ++it)
    {
        std::cout << "[" << *it << "] ";
    }
    std::cout << std::endl;
    std::cout << "Channel:   " << this->_channel_name << std::endl;
    std::cout << "Info:      " << this->_info << std::endl;
    std::cout << "User:      " << (this->_user ? this->_user->getNickName() : "NULL") << std::endl;
    std::cout << "========================" << std::endl;
}

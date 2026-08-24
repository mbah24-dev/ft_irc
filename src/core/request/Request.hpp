/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 17:55:09 by mbah              #+#    #+#             */
/*   Updated: 2026/08/24 22:13:16 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <vector>
# include <string>
# include <iostream>
# include "../user/User.hpp"

/**
 * @brief Représente une commande IRC reçue d'un client
 * e.g : PRIVMSG #general :Hello world! How r u
*/
class Request
{
    public:
        typedef std::vector<std::string> ParamList;

    public:
        Request(void);
        Request(const Request& source);
        Request(const std::string& input, User* user);
        ~Request(void);
        Request& operator=(const Request& source);

    public:
        const std::string&  getCommand(void) const;
        const ParamList&    getParams(void) const;
        User*               getUser(void) const;
        const std::string&  getChannelName(void) const;
        const std::string&  getInfo(void) const;

    public:
        void setUser(User* user);
        void setChannelName(const std::string& channelName);
        void setInfo(const std::string& info);

    public:
        void debug(void) const;

    public:
        void cmdLineparser(const std::string& input);

    private:
        std::string     _command;
        ParamList       _params;
        User*           _user;
        std::string     _channel_name;
        std::string     _info;

    private:
        std::string _cleanLine(const std::string& input) const;
        
        bool _hasParameters(const std::string& line) const;
        bool _hasTrailing(const std::string& rest) const;
        
        void _extractCommand(const std::string& line);
        void _extractParameters(const std::string& line);
        void _extractTrailing(const std::string& rest, std::string::size_type pos);
        
        void _parseWithTrailing(const std::string& rest);
        void _parseWithoutTrailing(const std::string& rest);
        void _parseParams(const std::string& paramsStr);
};

#endif

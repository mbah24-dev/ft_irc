/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 14:29:58 by mbah              #+#    #+#             */
/*   Updated: 2026/09/29 10:17:54 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <iostream>

#include "../user/User.hpp"
#include "../request/Request.hpp"
#include "../responseCodes/ResponseCodes.hpp"
#include "../utils/config.hpp"

enum ChannelMode
{
    MODE_INVITE_ONLY       = 1 << 0,  // i
    MODE_TOPIC_RESTRICTED  = 1 << 1,  // t
    MODE_PASSWORD          = 1 << 2,  // k
    MODE_OPERATOR          = 1 << 3,  // o
    MODE_USER_LIMIT        = 1 << 4   // l
};

class Server;
class Request;
class User;

class Channel
{
    public:
        // ======================== CONSTRUCTEURS ========================
        Channel(void);
        Channel(const std::string& name, const std::string& password, Server* server);
        Channel(const Channel& other);
        ~Channel(void);
        
        Channel& operator=(const Channel& other);

        // ======================== GETTERS ========================
        const std::string&      getName(void) const;
        const std::string&      getTopic(void) const;
        const std::string&      getPassword(void) const;
        int                     getLimit(void) const;
        short                   getModes(void) const;
        
        const std::list<User*>& getMembers(void) const;
        std::list<User*>&       getMembers(int);
        const std::list<User*>& getOperators(void) const;
        std::list<User*>&       getOperators(int);
        bool                    isInvited(const std::string& nickname) const;
        void                    addInvitation(const std::string& nickname);
        void                    removeInvitation(const std::string& nickname);

        // ======================== SETTERS ========================
        void setTopic(const std::string& topic);
        void setPassword(const std::string& password);
        void setLimit(int limit);

        // ======================== GESTION DES MEMBRES ========================
        void    addMember(User* user);
        int     removeMember(User* user);
        bool    isMember(User* user) const;
        User*   findMember(const std::string& nickname);

        void    addOperator(User* user);
        int     removeOperator(User* user);
        bool    isOperator(User* user) const;

        // ======================== GESTION DES MODES ========================
        void    enableMode(char mode);
        void    disableMode(char mode);
        void    editMode(char mode, char sign);
        bool    hasMode(char mode) const;
        bool    isModeValid(char mode) const;
        bool    isModeWithParam(char mode) const;
        std::string getModeString(void) const;
        
		bool		handleModeChange(char mode, char sign, std::string param, Request request);

        static bool isValidChannelName(const std::string& name);

    private:
        std::string         _name;
        std::string         _topic;
        short               _modes;
        std::string         _password;
        std::list<User*>    _members;
        std::list<User*>    _operators;
        std::vector<std::string> _invitedNicknames;
        int                 _limit;
		Server*				_server;
	
	private:
		bool				handleOperatorMode(char sign, const std::string& param, Request request);
        bool				handleKeyMode(char sign, const std::string& param);
        bool				handleLimitMode(char sign, const std::string& param, Request request);
};

#endif

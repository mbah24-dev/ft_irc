/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 14:29:58 by mbah              #+#    #+#             */
/*   Updated: 2026/08/25 12:49:17 by mbah             ###   ########.fr       */
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

#define CHANNEL_MODES "itkol"
#define ARG_CHAN_MODES "kol"

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
        Channel(const std::string& name, const std::string& password);
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
        
        bool    handleModeChange(char mode, char sign, const std::string& param,
                                 const Request& request, const Server& server);

        static bool isValidChannelName(const std::string& name);

    private:
        std::string         _name;
        std::string         _topic;
        short               _modes;
        std::string         _password;
        std::list<User*>    _members;
        std::list<User*>    _operators;
        int                 _limit;
};

#endif

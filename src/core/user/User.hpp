/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:47 by mbah              #+#    #+#             */
/*   Updated: 2026/10/01 10:56:04 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef USER_HPP
# define USER_HPP

# include <iostream>
# include <map>

class Channel;

/**
 * @class User
 * @brief Represente a client connected to the server
 */
class User
{
	public:
		std::string	_receiveBuffer;
		std::string	_sendBuffer;

	private:
		int			_socketFd;
		
		bool		_registered;
		bool		_password_provided;
		bool		_operator_status;
		
		typedef std::map<std::string, Channel *> ChannelMap;
		
		std::string _name;
		std::string _hostmask;
		std::string _fullname;
		std::string _nickname;

		ChannelMap      _channelMap; /**< Map des canaux rejoints */
		
	public:
		User();
		User(const User & source);
		User(int socketFd, const std::string& hostMask);
		~User(void);
		
		User& operator=(const User& source);

		const std::string& getName() const;
		const std::string& getHostMask() const;
		const std::string& getFullName() const;
		const std::string& getNickName() const;
		std::string getPrefix() const; /** Retourne le préfixe IRC : "nick!user@host" */
		int			getSocketFd() const;

		bool isRegistered() const;
		bool hasPasswordProvided() const;
		bool isOperator() const;
		
		const ChannelMap& getChannels(void) const;
		void addChannel(const std::string& name, Channel* channel);
		void removeChannel(const std::string& name);
		
		void setName(const std::string& name);
		void setNickName(const std::string& nickname);
		void setFullName(const std::string& fullname);
		void setOperator(bool val);
		void setRegistered(bool val);
		void setPasswordProvided(bool val);

		void appendToBuffer(const std::string& data);
		void appendToBuffer(const char* data);
		void appendToSendBuffer(const std::string& data);
};

bool operator==(const User & first, const User & second);

std::ostream& operator<<(std::ostream& output, const User& user);

#endif


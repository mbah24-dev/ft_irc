/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:52 by mbah              #+#    #+#             */
/*   Updated: 2026/04/19 16:13:17 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "User.hpp"

User::User() : _fd(-1), _host(), _inputBuffer()
{
}

User::User(int fd, const std::string &host) : _fd(fd), _host(host), _inputBuffer()
{
}

User::~User()
{
}

int User::getFd() const
{
	return _fd;
}

const std::string &User::getHost() const
{
	return _host;
}

const std::string &User::getInputBuffer() const
{
	return _inputBuffer;
}

std::string &User::getInputBuffer()
{
	return _inputBuffer;
}

void User::setFd(int fd)
{
	_fd = fd;
}

void User::setHost(const std::string &host)
{
	_host = host;
}

void User::appendInput(const char *data, std::size_t size)
{
	if (data == NULL || size == 0)
		return;
	_inputBuffer.append(data, size);
}

void User::clearInputBuffer()
{
	_inputBuffer.clear();
}


/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:55 by mbah              #+#    #+#             */
/*   Updated: 2026/08/26 15:44:47 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/core/server/Server.hpp"
#include "src/core/user/User.hpp"

int main(int ac, char **av)
{
	try
	{
		if (ac != 3)
			throw std::invalid_argument("Usage: ./ircserv <port> <password>");
		Server	ircServer(av);
		std::cout << "Server listening on port: " << ircServer.getListeningPort()
                  << "\nPassword: " << ircServer.getPassword() << std::endl;
		ircServer.startEventLoop();
	}
	catch(const std::exception& error)
	{
		std::cerr << error.what() << '\n';
        return (1);
	}
	return (0);
}
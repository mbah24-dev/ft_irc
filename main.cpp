/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:55 by mbah              #+#    #+#             */
/*   Updated: 2026/04/19 16:13:33 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "Server.hpp"

namespace
{
	/**
	 * @brief Convertit et valide une valeur de port.
	 */
	int parsePort(const std::string &value)
	{
		char *end = NULL;
		long port = std::strtol(value.c_str(), &end, 10);

		if (value.empty() || end == NULL || *end != '\0' || port < 1 || port > 65535)
			throw std::invalid_argument("port invalide: utiliser une valeur entre 1 et 65535");
		return static_cast<int>(port);
	}

	/**
	 * @brief Verifie qu'un mot de passe n'est pas vide.
	 */
	void validatePassword(const std::string &value)
	{
		if (value.empty())
			throw std::invalid_argument("mot de passe invalide: il ne peut pas etre vide");
	}
}

/**
 * @brief Point d'entree du serveur IRC.
 */
int main(int argc, char **argv)
{
	try
	{
		if (argc != 3)
			throw std::invalid_argument("utilisation: ./ircserv <port> <password>");

		const int port = parsePort(argv[1]);
		const std::string password(argv[2]);
		
		validatePassword(password);

		Server server(port, password);
		server.run();
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << std::endl;
		return (1);
	}
	return (0);
}


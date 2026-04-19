/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:44 by mbah              #+#    #+#             */
/*   Updated: 2026/04/19 16:13:55 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <arpa/inet.h>
# include <cerrno>
# include <cstddef>
# include <csignal>
# include <cstdlib>
# include <cstring>
# include <fcntl.h>
# include <iostream>
# include <map>
# include <netinet/in.h>
# include <poll.h>
# include <stdexcept>
# include <string>
# include <sys/socket.h>
# include <unistd.h>
# include <vector>

# include "User.hpp"

/**
 * @class Server
 * @brief Gere le socle reseau du serveur IRC.
 */
class Server
{
	public:
		/**
		 * @brief Construit le serveur avec le port et le mot de passe fournis.
		 */
		Server(int port, const std::string &password);

		/**
		 * @brief Detruit le serveur et ferme toutes les sockets ouvertes.
		 */
		~Server();

		/**
		 * @brief Retourne le port d'ecoute courant.
		 */
		int getPort() const;

		/**
		 * @brief Retourne le mot de passe du serveur.
		 */
		const std::string &getPassword() const;

		/**
		 * @brief Lance la boucle principale du serveur.
		 */
		void run();

	private:
		typedef std::map<int, User> ClientMap;

		int                 _port;
		std::string         _password;
		int                 _listenFd;
		std::vector<pollfd> _pollFds;
		ClientMap           _clients;
		bool                _isRunning;

		/**
		 * @brief Prepare la socket d'ecoute avec les attributs minimaux.
		 */
		void setupSocket();

		/**
		 * @brief Configure un descripteur en mode non-bloquant.
		 */
		void setNonBlocking(int fd) const;

		/**
		 * @brief Accepte toutes les connexions en attente.
		 */
		void acceptClients();

		/**
		 * @brief Lit les donnees d'un client deja connecte.
		 * @return Vrai si le client a ete supprime pendant la lecture.
		 */
		bool handleClient(std::size_t index);

		/**
		 * @brief Supprime un client du suivi reseau et ferme sa socket.
		 */
		void removeClient(std::size_t index);

		/**
		 * @brief Ferme la socket d'ecoute si elle est encore ouverte.
		 */
		void closeListenSocket();

		/**
		 * @brief Valide le port d'ecoute fourni.
		 */
		static void validatePort(int port);

		/**
		 * @brief Recoit le signal de terminaison et arrete la boucle.
		 */
		static void signalHandler(int signum);

		Server();
		Server(const Server &other);
		Server &operator=(const Server &other);
};

#endif


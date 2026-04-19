/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:50 by mbah              #+#    #+#             */
/*   Updated: 2026/04/19 16:13:21 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

namespace
{
	volatile sig_atomic_t g_stopRequested = 0;
	const std::size_t     kBufferSize = 4096;
}

void Server::signalHandler(int signum)
{
	(void)signum;
	g_stopRequested = 1;
}

void Server::validatePort(int port)
{
	if (port < 1 || port > 65535)
		throw std::invalid_argument("port invalide: utiliser une valeur entre 1 et 65535");
}

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1), _pollFds(), _clients(), _isRunning(false)
{
	validatePort(_port);
}

Server::~Server()
{
	closeListenSocket();

	for (ClientMap::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);
	_clients.clear();
	_pollFds.clear();
}

int Server::getPort() const
{
	return _port;
}

const std::string &Server::getPassword() const
{
	return _password;
}

void Server::closeListenSocket()
{
	if (_listenFd != -1)
	{
		close(_listenFd);
		_listenFd = -1;
	}
}

void Server::setNonBlocking(int fd) const
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error(std::string("fcntl(F_GETFL) failed: ") + std::strerror(errno));
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error(std::string("fcntl(F_SETFL) failed: ") + std::strerror(errno));
}

/**
	 * @brief Initialise le socket d'ecoute TCP non bloquant du serveur IRC.
	 *
	 * Elements utilises:
	 * - AF_INET: famille d'adresses IPv4.
	 * - SOCK_STREAM: socket TCP (flux fiable, ordonne, oriente connexion).
	 * - SOL_SOCKET/SO_REUSEADDR: autorise la reutilisation rapide du port
	 *   apres fermeture du processus (utile apres un redemarrage).
	 * - INADDR_ANY + htonl(): ecoute sur toutes les interfaces reseau locales.
	 * - htons(_port): conversion du port en ordre reseau (big endian).
	 * - sockaddr_in: structure d'adresse IPv4 passée a bind().
	 * - SOMAXCONN: taille maximale conseillee de la file des connexions en attente.
	 * - pollfd/POLLIN: enregistre le fd d'ecoute pour detecter les nouvelles connexions.
*/
void Server::setupSocket()
{
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd == -1)
		throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));

	int reuse = 1;

	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
	{
		closeListenSocket();
		throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR) failed: ") + std::strerror(errno));
	}

	sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(static_cast<unsigned short>(_port));

	// associe socket au port
	if (bind(_listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1)
	{
		closeListenSocket();
		throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
	}

	if (listen(_listenFd, SOMAXCONN) == -1)
	{
		closeListenSocket();
		throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
	}

	// rend serveur non bloquant
	setNonBlocking(_listenFd);

	pollfd listenPollFd;
	std::memset(&listenPollFd, 0, sizeof(listenPollFd));
	listenPollFd.fd = _listenFd;
	listenPollFd.events = POLLIN; // POLLIN: surveille entrée (connexion entrante)
	_pollFds.push_back(listenPollFd); // ajoute socket serveur au poll
}

/**
	* @brief Accepte toutes les connexions en attente sur le socket d'ecoute.
	*
	* Elements utilisées:
	* - boucle while(true): vide la file des connexions pretes tant que accept() reussit.
	* - sockaddr_storage: structure generique assez grande pour IPv4 et IPv6.
	* - socklen_t addressLen: taille de la structure passée a accept() (in/out).
	* - accept(listenFd, ...): cree un nouveau fd client pour chaque connexion.
	* - EAGAIN/EWOULDBLOCK: plus de connexion disponible en mode non bloquant,
	*   on quitte proprement la fonction.
	* - EINTR: appel interrompu par un signal, on retente sans fermer le serveur.
	* - setNonBlocking(clientFd): evite de bloquer la boucle d'evenements.
	* - INET6_ADDRSTRLEN: taille max du buffer texte pour une adresse IP.
	* - ss_family + AF_INET/AF_INET6: detection de la famille d'adresse recue.
	* - inet_ntop(): conversion binaire -> chaine lisible pour journaliser l'hote.
	* - pollfd/POLLIN: inscription du client dans poll() pour surveiller les lectures.
*/
void Server::acceptClients()
{
	while (true)
	{
		sockaddr_storage clientAddress;
		socklen_t addressLen = sizeof(clientAddress);
		std::memset(&clientAddress, 0, sizeof(clientAddress));

		int clientFd = accept(_listenFd, reinterpret_cast<sockaddr *>(&clientAddress), &addressLen);
		if (clientFd == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			if (errno == EINTR)
				continue;
			throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
		}

		setNonBlocking(clientFd);

		char hostBuffer[INET6_ADDRSTRLEN];
		std::memset(hostBuffer, 0, sizeof(hostBuffer));
		if (clientAddress.ss_family == AF_INET)
		{
			const sockaddr_in *ipv4 = reinterpret_cast<const sockaddr_in *>(&clientAddress);
			if (inet_ntop(AF_INET, &ipv4->sin_addr, hostBuffer, sizeof(hostBuffer)) == NULL)
				std::strncpy(hostBuffer, "unknown", sizeof(hostBuffer) - 1);
		}
		else if (clientAddress.ss_family == AF_INET6)
		{
			const sockaddr_in6 *ipv6 = reinterpret_cast<const sockaddr_in6 *>(&clientAddress);
			if (inet_ntop(AF_INET6, &ipv6->sin6_addr, hostBuffer, sizeof(hostBuffer)) == NULL)
				std::strncpy(hostBuffer, "unknown", sizeof(hostBuffer) - 1);
		}
		else
		{
			std::strncpy(hostBuffer, "unknown", sizeof(hostBuffer) - 1);
		}

		_clients.insert(std::make_pair(clientFd, User(clientFd, hostBuffer)));

		pollfd clientPollFd;
		std::memset(&clientPollFd, 0, sizeof(clientPollFd));
		clientPollFd.fd = clientFd;
		clientPollFd.events = POLLIN;
		_pollFds.push_back(clientPollFd);

		std::cout << "client connected: fd=" << clientFd << " host=" << hostBuffer << std::endl;
	}
}

/**
 * @brief Lit et traite les donnees disponibles pour un client.
 *
 * Elements utilises:
 * - index: position du descripteur client dans _pollFds.
 * - recv(clientFd, ..., kBufferSize, 0): lecture socket en mode non bloquant.
 * - kBufferSize: taille max lue par iteration (4096 octets).
 * - readSize == 0: fermeture distante, le client est retiré.
 * - EAGAIN/EWOULDBLOCK: plus de donnees immediates a lire.
 * - EINTR: interruption signal, lecture retentée.
 * - appendInput()/clearInputBuffer(): transfert des donnees dans l'etat User.
 *
 * @return true si le client a ete supprime.
 * @return false si le client reste actif.
 */
bool Server::handleClient(std::size_t index)
{
	if (index >= _pollFds.size())
		return false;

	const int clientFd = _pollFds[index].fd;
	ClientMap::iterator clientIt = _clients.find(clientFd);
	if (clientIt == _clients.end())
		return false;

	char buffer[kBufferSize + 1];
	while (true)
	{
		std::memset(buffer, 0, sizeof(buffer));
		const ssize_t readSize = recv(clientFd, buffer, kBufferSize, 0);
		if (readSize == 0)
		{
			removeClient(index);
			return true;
		}
		if (readSize < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return false;
			if (errno == EINTR)
				continue;
			removeClient(index);
			return true;
		}

		clientIt->second.appendInput(buffer, static_cast<std::size_t>(readSize));
		clientIt->second.clearInputBuffer();

		if (static_cast<std::size_t>(readSize) < kBufferSize)
			return false;
	}
}

/**
 * @brief Supprime un client des structures internes du serveur.
 *
 * Elements utilises:
 * - close(clientFd): fermeture du socket client.
 * - _clients.erase(clientFd): suppression de l'objet User associe.
 * - _pollFds.erase(...): retrait du fd de la boucle poll.
 * - difference_type: type requis par l'iterateur de vector::erase.
 */
void Server::removeClient(std::size_t index)
{
	if (index >= _pollFds.size())
		return;

	const int clientFd = _pollFds[index].fd;

	std::cout << "client disconnected: fd=" << clientFd << std::endl;
	close(clientFd);
	_clients.erase(clientFd);
	_pollFds.erase(_pollFds.begin() + static_cast<std::vector<pollfd>::difference_type>(index));
}

/**
 * @brief Execute la boucle principale d'evenements du serveur IRC.
 *
 * Elements utilises:
 * - setupSocket(): initialisation du socket d'ecoute.
 * - signal(SIGINT/SIGTERM, signalHandler): arret gracieux via g_stopRequested.
 * - poll(_pollFds, timeout=500): multiplexage des sockets.
 * - POLLERR/POLLHUP/POLLNVAL: etats d'erreur/deconnexion d'un fd.
 * - POLLIN: donnees entrantes ou connexion en attente.
 * - _isRunning: protection contre un double lancement du serveur.
 * :: = namespace global
 */
void Server::run()
{
	if (_isRunning)
		throw std::runtime_error("server already running");

	setupSocket();
	::signal(SIGINT, Server::signalHandler);
	::signal(SIGTERM, Server::signalHandler);
	_isRunning = true;

	while (!g_stopRequested)
	{
		// surveille tous les sockets avec un timeout de 500ms
		int pollResult = poll(&_pollFds[0], _pollFds.size(), 500);
		// cas d'erreur
		if (pollResult < 0)
		{
			if (errno == EINTR) 
				continue;
			throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
		}
		//Timeout : rien ne s’est passé -> boucle suivante
		if (pollResult == 0)
			continue;

		// parcours des sockets
		for (std::size_t index = 0; index < _pollFds.size();)
		{
			// Aucun événement: ce socket n'a rien fait, on passe au suivant
			if (_pollFds[index].revents == 0)
			{
				++index;
				continue;
			}

			// Erreurs socket (POLLERR = erreur) (POLLHUP = deconnexion) et (POLLNVAL = fd invalide)
			if (_pollFds[index].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				// cas serveur cassé, on stop tout
				if (_pollFds[index].fd == _listenFd)
					throw std::runtime_error("listen socket became invalid");
				
				// supprime client, pas de ++index, car vector a changer
				removeClient(index);
				continue;
			}

			// le serveur reçoit une connexion
			if (_pollFds[index].fd == _listenFd && (_pollFds[index].revents & POLLIN))
			{
				acceptClients(); // accepte TOUS les clients en attente et on passe au suivant
				++index;
				continue;
			}

			// un client a envoyé des données (Message client)
			if (_pollFds[index].revents & POLLIN)
			{
				// si true, client supprimer, pas de ++index, on continue
				if (handleClient(index))
					continue;
			}

			++index; // sinon: on passe au socket suivant
		}
	}

	_isRunning = false;
}


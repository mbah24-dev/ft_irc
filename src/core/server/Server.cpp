/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:50 by mbah              #+#    #+#             */
/*   Updated: 2026/08/26 16:34:52 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(char** arguments)
{
    char*   endPointer;
    long    portNumber = std::strtol(arguments[1], &endPointer, 0);
    
    if (portNumber < 1 || portNumber > 65535 || *endPointer != '\0')
        throw IncorrectPortValue();
    
    _listeningPort = static_cast<int>(portNumber);

    _connectionPassword = arguments[2];
    
    if (_connectionPassword.empty())
        throw InvalidPassword();

    time_t      currentTime = std::time(NULL);
    std::string creationDate = std::ctime(&currentTime);
    
    // Suppression du '\n' en fin de chaîne
    if (!creationDate.empty() && creationDate[creationDate.size() - 1] == '\n')
        creationDate.erase(creationDate.size() - 1);
    
    _startupTimestamp = creationDate;
}

Server::~Server(void)
{
    // TODO: Nettoie les ressources
}

void Server::initializeServerSocket(void)
{
    struct sockaddr_in  serverAddress;

    //CRÉATION DU SOCKET 
    _serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1)
        throw CreateSocketError();

    std::cout << "Server listening on port: " << _listeningPort << std::endl;

    //CONFIGURATION DE L'ADRESSE 
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(_listeningPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;  //0.0.0.0

    //OPTION REUSEADDR 
	//Permet de relancer le serveur immédiatement sans attendre 
	//que le port soit libéré (évite l'erreur "Address already in use")
    int reuseOption = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuseOption), sizeof(int)) == -1)
        throw SetSocketOptionError();

    //MODE NON-BLOQUANT imposer par 42
    if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) == -1)
        throw SetSocketOptionError();

    //BIND 
	//Attache le socket à un port et une IP spécifiques
    if (bind(_serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress),
             sizeof(serverAddress)) == -1)
        throw BindSocketError();

    //LISTEN 
    if (listen(_serverSocket, SOMAXCONN) == -1)
        throw ListenSocketError();

    //INITIALISATION DE POLL
    std::memset(_eventPolling, 0, sizeof(_eventPolling));
    _eventPolling[0].fd = _serverSocket;
    _eventPolling[0].events = POLLIN;
    _activeDescriptors = 1;
}

void Server::disconnectClient(User* user, const std::string& message)
{
    if (user == NULL)
        return;

    int clientSocket = user->getSocketFd();
    std::list<std::string> emptyChannels;

    //RETIRE L'UTILISATEUR DE TOUS LES CANAUX
    ChannelRegistry::iterator channelIt = _activeChannels.begin();
    while (channelIt != _activeChannels.end())
    {
        Channel& currentChannel = channelIt->second;
        
        if (currentChannel.isMember(user))
        {
            //Retire l'utilisateur du canal
            if (currentChannel.removeMember(user))
                emptyChannels.push_back(currentChannel.getName());

            //Envoie le message QUIT à tous les membres du canal
            const std::list<User*>& channelMembers = currentChannel.getMembers(0);
            for (std::list<User*>::const_iterator memberIt = channelMembers.begin();
                 memberIt != channelMembers.end(); ++memberIt)
            {
                sendMessage(user->getPrefix() + " QUIT :" + message, (*memberIt)->getSocketFd());
            }
        }
        ++channelIt;
    }

    //SUPPRIMER LES CANAUX VIDES
    for (std::list<std::string>::const_iterator emptyIt = emptyChannels.begin();
         emptyIt != emptyChannels.end(); ++emptyIt)
    {
        _activeChannels.erase(*emptyIt);
    }

    //RETIRE DU TABLEAU POLL()
    for (unsigned int index = 0; index < _activeDescriptors; ++index)
    {
        if (_eventPolling[index].fd == clientSocket)
        {
            removeFromPolling(index);
            break;
        }
    }

    //SUPPRIME DE LA MAP DES CLIENTS
    _connectedClients.erase(clientSocket);

    std::cout << "Client (fd [" << clientSocket << "]) disconnected: " << message << std::endl;
}

void Server::startEventLoop(void)
{
    initializeServerSocket();

    while (1)
    {
        //Attente d'événements (temps infini = -1)
        int pollStatus = poll(_eventPolling, _activeDescriptors, -1);
        if (pollStatus == -1)
            throw PollFailedError();

        //Parcours des descripteurs actifs (taille fixe au début de la boucle)
        unsigned int currentDescriptorCount = _activeDescriptors;
        unsigned int index = 0;
        while (index < currentDescriptorCount)
        {
            try
            {
                //ÉVÉNEMENT POLLIN: DONNÉES ENTRANTES
                if (_eventPolling[index].revents & POLLIN)
                {
                    if (_eventPolling[index].fd == _serverSocket)
                        acceptNewClient();          //Nouvelle connexion
                    else
                        processClientRequest(index); //Message d'un client
                }
                
                //ÉVÉNEMENT POLLHUP: DÉCONNEXION
                else if (_eventPolling[index].revents & (POLLHUP | POLLERR))
                {
                    //Récupére l'utilisateur associé à ce FD
                    ClientRegistry::iterator clientIt = _connectedClients.find(_eventPolling[index].fd);
                    if (clientIt != _connectedClients.end())
                        disconnectClient(&clientIt->second, "connection lost");
                }
            }
            catch (const std::exception& error)
            {
                std::cerr << error.what() << '\n';
            }
            ++index;
        }
    }    
    close(_serverSocket);
}

void Server::acceptNewClient(void)
{
    //ACCEPTATION DE LA CONNEXION
    struct sockaddr_in  clientAddress;
    socklen_t           addressLength = sizeof(clientAddress);
    
    int clientSocket = accept(_serverSocket, 
                              reinterpret_cast<struct sockaddr*>(&clientAddress),
                              &addressLength);
    
    if (clientSocket == -1)
        throw AcceptSocketError();

    //RÉCUPÉRATION DE L'ADRESSE IP
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ipAddress, INET_ADDRSTRLEN);

    //AJOUT À LA MAP DES CLIENTS
    _connectedClients.insert(std::make_pair(clientSocket, User(clientSocket, ipAddress)));

    //AJOUT AU TABLEAU POLL() ========================
    addToPolling(clientSocket);

    std::cout << "New client connected on fd " << clientSocket 
              << " from " << ipAddress << std::endl;
}

void Server::addToPolling(int clientSocket)
{
    if (_activeDescriptors >= SOMAXCONN)
        throw FdPollFullError();

    _eventPolling[_activeDescriptors].fd = clientSocket;
    _eventPolling[_activeDescriptors].events = POLLIN;
    _eventPolling[_activeDescriptors].revents = 0;  //Réinitialise

    ++_activeDescriptors;
}

void Server::removeFromPolling(int index)
{
    if (index < 0 || static_cast<unsigned int>(index) >= _activeDescriptors)
        return;

    //FERME LE SOCKET
    close(_eventPolling[index].fd);

    //REMPLACE PAR LE DERNIER ÉLÉMENT 
    int lastIndex = _activeDescriptors - 1;
    
    if (index != lastIndex)
    {
        _eventPolling[index].fd = _eventPolling[lastIndex].fd;
        _eventPolling[index].events = _eventPolling[lastIndex].events;
        _eventPolling[index].revents = _eventPolling[lastIndex].revents;
    }

    //NETTOIE LE DERNIER ÉLÉMENT
    _eventPolling[lastIndex].fd = -1;
    _eventPolling[lastIndex].events = 0;
    _eventPolling[lastIndex].revents = 0;

    --_activeDescriptors;
}

void Server::executeCommand(const Request& req)
{
    const std::string& command = req.getCommand();
    User* client = req.getUser();
    int clientSocket = client->getSocketFd();

    //COMMANDES D'AUTH
    if (command == "CAP")
        handleCapCommand(req);
    else if (command == "PASS")
        handlePassCommand(req);
    else if (command == "NICK")
        handleNickCommand(req);
    else if (command == "USER")
        handleUserCommand(req);

    //COMMANDES DE COMMUNICATION
    else if (command == "PING")
        handlePingCommand(req);
    else if (command == "PONG")
        handlePongCommand(req);
    else if (command == "PRIVMSG")
        handlePrivmsgCommand(req);
    else if (command == "NOTICE")
        handleNoticeCommand(req);

    //COMMANDES DE CANAUX 
    else if (command == "JOIN")
        handleJoinCommand(req);
    else if (command == "PART")
        handlePartCommand(req);
    else if (command == "TOPIC")
        handleTopicCommand(req);
    else if (command == "LIST")
        handleListCommand(req);
    else if (command == "NAMES")
        handleNamesCommand(req);
    else if (command == "WHO")
        handleWhoCommand(req);
    else if (command == "INVITE")
        handleInviteCommand(req);
    else if (command == "KICK")
        handleKickCommand(req);
    else if (command == "MODE")
        handleModeCommand(req);

    //COMMANDES D'OPÉRATEUR
    else if (command == "OPER")
        handleOperCommand(req);
    else if (command == "KILL")
        handleKillCommand(req);

    //COMMANDES SPÉCIALES / BOT
    else if (command == "GLOBOPS")
        handleGlobopsCommand(req);
    else if (command == "SHOWTIME")
        handleShowtimeCommand(req);

    //COMMANDE DE DÉCONNEXION
    else if (command == "QUIT")
        handleQuitCommand(req);

    //COMMANDE INCONNUE
    else
    {
        std::string errorMessage = std::string(SERVER_NAME) + " 421 " + client->getNickName() + " " + command + " :Unknown command";
        sendMessage(errorMessage, clientSocket);
    }
}

void Server::processClientRequest(int pollIndex)
{
    //RÉCUPÉRATION DU SOCKET
    int clientSocket = _eventPolling[pollIndex].fd;

    //LECTURE DES DONNÉES
    char receiveBuffer[BUFFER_SIZE];
    std::memset(receiveBuffer, 0, BUFFER_SIZE);
    
    int bytesReceived = recv(clientSocket, receiveBuffer, BUFFER_SIZE, 0);

    //GESTION DE LA DÉCONNEXION
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
            std::cout << "Client (fd [" << clientSocket << "]) disconnected" << std::endl;
        else
            throw ReceiveMessageFailed();
        
        //Retire le client
        ClientRegistry::iterator clientIt = _connectedClients.find(clientSocket);
        if (clientIt != _connectedClients.end())
            disconnectClient(&clientIt->second, "disconnected");
        
        return;
    }

    //TRAITEMENT DE LA COMMANDE
    handleCommand(receiveBuffer, clientSocket);
}

void Server::handleCommand(char* rawData, int clientSocket)
{
    //RÉCUPÉRATION DE L'UTILISATEUR
    ClientRegistry::iterator clientIt = _connectedClients.find(clientSocket);
    if (clientIt == _connectedClients.end())
        return;  //Client introuvable
    
    User* currentUser = &clientIt->second; //(key = first, value = second)

    //AJOUT DES DONNÉES AU BUFFER
    currentUser->appendToBuffer(rawData);

    //EXTRACTION DES COMMANDES 
    const std::string commandSeparator = "\r\n";
    size_t separatorPosition = currentUser->_receiveBuffer.find(commandSeparator);
    
    while (separatorPosition != std::string::npos &&
           separatorPosition < BUFFER_SIZE)
    {
        //Extraire une commande complète
        std::string commandLine = currentUser->_receiveBuffer.substr(0, separatorPosition);
        currentUser->_receiveBuffer.erase(0, separatorPosition + 2);  // +2 pour \r\n

        //Crée et exécute la requête
        Request clientRequest(commandLine, currentUser);
        clientRequest.debug();
        
        executeCommand(clientRequest);

        //Vérifie si le client a été supprimé (ex: commande QUIT)
        if (_connectedClients.find(clientSocket) == _connectedClients.end())
            break;

        //Cherche la prochaine commande
        separatorPosition = currentUser->_receiveBuffer.find(commandSeparator);
    }
}


std::string Server::getPassword() const
{
    return (_connectionPassword);
}

int Server::getListeningPort() const
{
    return (_listeningPort);
}

const Server::ChannelRegistry& Server::getActiveChannels() const
{
    return (_activeChannels);
}

Server::ClientRegistry::iterator Server::findUserByNickname(const std::string& nickname)
{
    ClientRegistry::iterator clientIt = _connectedClients.begin();
    
    while (clientIt != _connectedClients.end())
    {
        if (clientIt->second.getNickName() == nickname)
            return (clientIt);
        
        ++clientIt;
    }
    
    return (_connectedClients.end());
}

void Server::sendMessage(const std::string& message, int clientSocket)
{
    std::cout << "SENDING: <" << message << ">" << std::endl;

    std::string formattedMessage = message;
    formattedMessage += IRC_END_SEQUENCE;

    ssize_t bytesSent = send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
    
    if (bytesSent == -1)
        std::cerr << "Failed to send message to fd " << clientSocket << std::endl;
}

Channel* Server::findChannel(const std::string& channelName)
{
    ChannelRegistry::iterator channelIt = _activeChannels.begin();
    
    while (channelIt != _activeChannels.end())
    {
        if (channelIt->second.getName() == channelName)
            return (&channelIt->second);
        
        ++channelIt;
    }
    
    return (NULL);
}

void Server::broadcast(const std::string& message, User* sender, Channel& channel)
{
    const std::list<User*>& members = channel.getMembers(0);
    
    for (std::list<User*>::const_iterator it = members.begin();
         it != members.end(); ++it)
    {
        if (sender != NULL && *it == sender)
            continue;
        
        sendMessage(message, (*it)->getSocketFd());
    }
}


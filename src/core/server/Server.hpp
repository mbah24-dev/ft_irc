/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:44 by mbah              #+#    #+#             */
/*   Updated: 2026/08/27 12:48:31 by mbah             ###   ########.fr       */
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
# include <ctime>

# include "User.hpp"
# include "Channel.hpp"
# include "Request.hpp"
# include "../utils/config.hpp"

/**
 * @class Server
 * @brief Gere le socle reseau du serveur IRC.
 */
class Server
{
    public:
        /** Map associant un descripteur de fichier à un utilisateur */
        typedef std::map<int, User>     ClientRegistry;
        
        /** Map associant un nom de canal à un canal */
        typedef std::map<std::string, Channel> ChannelRegistry;

    public:
        Server(char** arguments);
        ~Server(void);

    // ======================== GETTERS ========================
    public:
        std::string                     getPassword(void) const;
        int                             getListeningPort(void) const;
        const ChannelRegistry&          getActiveChannels(void) const;

    // ======================== MÉTHODES PUBLIQUES ========================
    public:
        void                            startEventLoop(void);
        void                            sendMessage(const std::string& message, int clientSocket);
        
        ClientRegistry::iterator        findUserByNickname(const std::string& nickname);
        Channel*                        findChannel(const std::string& channelName);

        void                            disconnectClient(User* user, const std::string& message);
		void							broadcast(const std::string& message, User* sender, Channel& channel);
		void							sendFormattedResponse(const Request& clientRequest, int responseCode);


    // ======================== MÉTHODES PRIVÉES ========================
    private:
        void                            initializeServerSocket(void);
        
        void                            acceptNewClient(void);
        void                            processClientRequest(int pollIndex);
        void                            handleCommand(char* rawData, int clientSocket);
        
        void                            addToPolling(int clientSocket);
        void                            removeFromPolling(int index);

        void                            executeCommand(const Request& req);
		std::string						buildResponseMessage(const Request& clientRequest, int responseCode);
		std::string						intToString(int number) const;

    // ======================== COMMANDES IRC ========================
    private:
        // --- Authentification ---
        void                            handleCapCommand(const Request& req);
        void                            handlePassCommand(const Request& req);
		
        void                            handleNickCommand(const Request& request);
		bool							containsForbiddenChars(const std::string& nickname) const;
		void							checkRegistrationComplete(User* user);
		
        void                            handleUserCommand(const Request& req);

        // --- Communication ---
        void                            handlePingCommand(const Request& req);
        void                            handlePongCommand(const Request& req);
        void                            handlePrivmsgCommand(const Request& req);
        void                            handleNoticeCommand(const Request& req);

        // --- Canaux ---
        void                            handleJoinCommand(const Request& req);
        void                            handlePartCommand(const Request& req);
        void                            handleTopicCommand(const Request& req);
        void                            handleListCommand(const Request& req);
        void                            handleNamesCommand(const Request& req);
        void                            handleWhoCommand(const Request& req);
        void                            handleInviteCommand(const Request& req);
        void                            handleKickCommand(const Request& req);
        void                            handleModeCommand(const Request& req);

        // --- Opérateur ---
        void                            handleOperCommand(const Request& req);
        void                            handleKillCommand(const Request& req);

        // --- Spéciales ---
        void                            handleGlobopsCommand(const Request& req);
        void                            handleShowtimeCommand(const Request& req);

        // --- Déconnexion ---
        void                            handleQuitCommand(const Request& req);

    // ======================== EXCEPTIONS ========================
    public:
        class IncorrectPortValue : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Port must be between 1 and 65535.");
                }
        };

        class InvalidPassword : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Password cannot be empty.");
                }
        };

        class CreateSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to create socket.");
                }
        };

        class SetSocketOptionError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to set socket option.");
                }
        };

        class BindSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to bind socket to port.");
                }
        };

        class ListenSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to listen on socket.");
                }
        };

        class AcceptSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to accept connection.");
                }
        };

        class PollFailedError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Poll failed.");
                }
        };

        class ReceiveMessageFailed : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to receive message.");
                }
        };

        class FdPollFullError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Poll array is full.");
                }
        };

    private:
        // --- Authentification ---
        std::string         _connectionPassword;    /**< Mot de passe requis pour se connecter */
        
        // --- Réseau ---
        int                 _listeningPort;         /**< Port sur lequel le serveur écoute */
        int                 _serverSocket;          /**< Socket du serveur pour les connexions entrantes */
        
        // --- Gestion des événements ---
        struct pollfd       _eventPolling[SOMAXCONN];  /**< Tableau des descripteurs surveillés par poll() */
        nfds_t              _activeDescriptors;     /**< Nombre de descripteurs actifs dans le tableau */
        
        // --- Données ---
        ClientRegistry      _connectedClients;      /**< Map des clients connectés (FD -> User) */
        ChannelRegistry     _activeChannels;        /**< Map des canaux actifs (nom -> Channel) */
        
        // --- Métadonnées ---
        std::string         _startupTimestamp;      /**< Date et heure de démarrage du serveur */
};

#endif
